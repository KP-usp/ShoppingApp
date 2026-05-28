#include "OrderManager.h"
#include "CartManager.h"
#include "Database.h"
#include <string>

using std::nullopt;
using std::optional;
using std::string;

void OrderManager::load_full_orders(const int user_id,
                                    ProductManager &product_manager) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载订单到内存。");
    }

    check_and_update_arrived_orders(user_id);

    orders_map.clear();
    is_loaded = false;

    try {
        auto res = Database::get_session()
                       .sql("SELECT user_id, product_id, order_id, count, "
                            "UNIX_TIMESTAMP(order_time) AS order_time_unix, "
                            "delivery_selection, address, status FROM orders "
                            "WHERE user_id = ? AND status = ?")
                       .bind(user_id,
                             static_cast<int>(FullOrderStatus::NOT_COMPLETED))
                       .execute();
        while (auto row = res.fetchOne()) {
            OrderItem temp;

            temp.id = row[0].get<int>();
            if (temp.id == user_id) {
                temp.product_id = row[1].get<int>();
                temp.order_id = row[2].get<int64_t>();
                temp.count = row[3].get<int>();
                temp.order_time = static_cast<time_t>(row[4].get<int64_t>());
                temp.delivery_selection = row[5].get<int>();
                temp.address = row[6].get<std::string>();
                temp.status = static_cast<FullOrderStatus>(row[7].get<int>());

                FullOrder &order = orders_map[temp.order_id];

                double item_price =
                    product_manager.get_price_by_id(temp.product_id);
                order.total_price += temp.count * item_price;

                if (order.items.empty()) {
                    order.total_price +=
                        DELIVERY_PRICES[temp.delivery_selection];
                    order.order_id = temp.order_id;
                    order.order_time = temp.order_time;

                    order.address = temp.address;
                    order.status = temp.status;
                }

                order.items.push_back(temp);
            }
        }

    } catch (const mysqlx::Error &e) {
        LOG_ERROR("加载订单到内存失败");
    }

    is_loaded = true;
}

void OrderManager::add_order(const int user_id,
                             std::vector<CartItem> cart_lists,
                             const std::string address) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法添加订单到数据库。");
    }

    auto time = get_current_time();

    try {
        for (auto &cart_item : cart_lists) {
            OrderItem new_order(cart_item.id, cart_item.product_id,
                                cart_item.count, time,
                                cart_item.delivery_selection, address,
                                FullOrderStatus::NOT_COMPLETED);

            Database::get_session()
                .sql("INSERT INTO orders (user_id, product_id, order_id, "
                     "count, order_time, delivery_selection, address, status) "
                     "values(?, ?, ?, ?, CURRENT_TIMESTAMP, ?, ?, ?)")
                .bind(cart_item.id, cart_item.product_id,
                      cart_item.id + static_cast<int64_t>(time),
                      cart_item.count, cart_item.delivery_selection, address,
                      static_cast<int>(FullOrderStatus::NOT_COMPLETED))
                .execute();
        }
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("添加新订单失败: " + std::string(e.what()));
    }
}

void OrderManager::update_order(const long long order_id,
                                std::optional<FullOrderStatus> new_status,
                                std::optional<std::string> new_address,
                                std::optional<int> new_delivery) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新数据库中的订单。");
    }

    if (!new_status.has_value() && !new_address.has_value() &&
        !new_delivery.has_value()) {
        return;
    }

    try {
        string sql = "UPDATE orders SET ";
        bool need_comma = false;

        if (new_status.has_value()) {
            sql += "status = ? ";
            need_comma = true;
        }
        if (new_address.has_value()) {
            if (need_comma)
                sql += ", ";
            sql += "address = ? ";
            need_comma = true;
        }
        if (new_delivery.has_value()) {
            if (need_comma)
                sql += ", ";
            sql += "delivery_selection = ? ";
        }

        sql += "WHERE order_id = ? ";

        auto stmt = Database::get_session().sql(sql);
        if (new_status.has_value())
            stmt.bind(static_cast<int>(new_status.value()));
        if (new_address.has_value())
            stmt.bind(new_address.value());
        if (new_delivery.has_value())
            stmt.bind(new_delivery.value());
        stmt.bind(order_id);
        stmt.execute();

    } catch (const mysqlx::Error &e) {
        LOG_ERROR("更新数据库中的订单失败: " + std::string(e.what()));
    }
}

void OrderManager::update_stock_by_order_id(const long long order_id,
                                            ProductManager product_manager) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新数据库中的库存。");
    }

    try {
        auto res = Database::get_session()
                       .sql("SELECT product_id, count FROM orders "
                            "WHERE order_id = ?")
                       .bind(static_cast<int64_t>(order_id))
                       .execute();
        while (auto row = res.fetchOne()) {
            int product_id = row[0].get<int>();
            int buy_count = row[1].get<int>();

            auto p_opt = product_manager.get_product(product_id);
            if (p_opt.has_value()) {
                auto &p = p_opt.value();
                product_manager.update_product(p.product_name, product_id,
                                               p.price, p.stock + buy_count);
            }
        }
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("无法更新数据库商品库存");
    }
}

void OrderManager::cancel_order(const long long order_id,
                                ProductManager &product_manager) {
    update_stock_by_order_id(order_id, product_manager);
    return update_order(order_id, FullOrderStatus::CANCEL, std::nullopt,
                        std::nullopt);
}

void OrderManager::update_order_info(const long long order_id,
                                     const std::string &new_address,
                                     const int new_delivery_selection) {
    if (new_address.empty()) {
        return update_order(order_id, std::nullopt, std::nullopt,
                            new_delivery_selection);
    }
    if (new_delivery_selection == -1) {
        return update_order(order_id, std::nullopt, new_address, nullopt);
    }

    return update_order(order_id, std::nullopt, new_address,
                        new_delivery_selection);
}

void OrderManager::check_and_update_arrived_orders(int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新到达订单状态。");
    }

    time_t now = get_current_time();

    try {

        string sql = "UPDATE orders SET status = ? WHERE user_id = ? AND "
                     "status = ? AND (UNIX_TIMESTAMP(order_time) + CASE "
                     "delivery_selection ";

        for (int i = 0; i < DELIVERY_DAYS.size(); i++) {
            long long seconds = (long long)DELIVERY_DAYS[i] * 86400;
            sql += "WHEN " + std::to_string(i) + " THEN " +
                   std::to_string(seconds) + " ";
        }

        sql += "ELSE 3153600000 END) <= ?";

        Database::get_session()
            .sql(sql)
            .bind(static_cast<int>(FullOrderStatus::COMPLETED), user_id,
                  static_cast<int>(FullOrderStatus::NOT_COMPLETED),
                  static_cast<int64_t>(now))
            .execute();

    } catch (const mysqlx::Error &e) {
        LOG_ERROR("更新到达订单状态失败。");
    }
}

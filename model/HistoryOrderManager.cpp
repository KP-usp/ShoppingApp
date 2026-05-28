#include "HistoryOrderManager.h"
#include "Database.h"

using std::string;

void HistoryOrderManager::load_history_orders(const int user_id,
                                              ProductManager &product_manager) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载历史订单信息到内存。");
    }

    check_and_update_arrived_orders(user_id);

    history_orders_map.clear();
    is_loaded = false;

    try {
        auto res = Database::get_session()
                       .sql("SELECT user_id, product_name, order_id, price, "
                            "count, UNIX_TIMESTAMP(order_time) AS "
                            "order_time_unix, delivery_selection, address, "
                            "status FROM history_orders "
                            "WHERE user_id = ? AND status IN (1, -1)")
                       .bind(user_id)
                       .execute();
        while (auto row = res.fetchOne()) {
            HistoryOrderItem temp;

            temp.id = row[0].get<int>();
            if (temp.id == user_id) {
                temp.product_name = row[1].get<std::string>();
                temp.order_id = row[2].get<int64_t>();
                temp.price = row[3].get<double>();
                temp.count = row[4].get<int>();
                temp.order_time = static_cast<time_t>(row[5].get<int64_t>());
                temp.delivery_selection = row[6].get<int>();
                temp.address = row[7].get<std::string>();
                temp.status = static_cast<FullOrderStatus>(row[8].get<int>());

                HistoryFullOrder &order =
                    history_orders_map[temp.order_id];

                auto product_opt =
                    product_manager.get_product(temp.product_name);
                if (!product_opt.has_value())
                    LOG_CRITICAL("数据库原商品信息被物理删除！");

                auto pro_info = product_opt.value();

                double item_price = pro_info.price;
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
        LOG_ERROR("加载历史订单到内存失败");
    }

    is_loaded = true;
}

void HistoryOrderManager::add_history_order(const int user_id,
                                            ProductManager &product_manager,
                                            std::vector<CartItem> cart_lists,
                                            const std::string address) {

    std::vector<HistoryOrderItem> history_order_list;

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法添加历史订单到数据库。");
    }

    time_t time = get_current_time();

    try {
        for (auto &cart_item : cart_lists) {
            auto product_opt =
                product_manager.get_product(cart_item.product_id);
            if (!product_opt.has_value())
                LOG_CRITICAL("数据库原商品信息被物理删除！");

            auto pro_info = product_opt.value();

            Database::get_session()
                .sql("INSERT INTO history_orders (user_id, product_name, price, "
                     "order_id, count, order_time, delivery_selection, address, "
                     "status) VALUES(?, ?, ?, ?, ?, CURRENT_TIMESTAMP, ?, ?, ?)")
                .bind(cart_item.id, pro_info.product_name, pro_info.price,
                      cart_item.id + static_cast<int64_t>(time),
                      cart_item.count, cart_item.delivery_selection, address,
                      static_cast<int>(FullOrderStatus::NOT_COMPLETED))
                .execute();
        }
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("添加历史订单失败: " + std::string(e.what()));
    }
}

void HistoryOrderManager::update_history_order(
    const long long order_id, std::optional<FullOrderStatus> new_status,
    std::optional<std::string> new_address, std::optional<int> new_delivery) {

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新数据库中的历史订单。");
    }

    if (!new_status.has_value() && !new_address.has_value() &&
        !new_delivery.has_value()) {
        return;
    }

    try {
        string sql = "UPDATE history_orders SET ";
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
        LOG_ERROR("更新数据库中的历史订单失败: " + std::string(e.what()));
    }
}

void HistoryOrderManager::cancel_history_order(
    const long long order_id, ProductManager &product_manager) {
    return update_history_order(order_id, FullOrderStatus::CANCEL, std::nullopt,
                                std::nullopt);
}

void HistoryOrderManager::update_history_order_info(
    const long long order_id, const std::string &new_address,
    const int new_delivery_selection) {
    if (new_address.empty()) {
        return update_history_order(order_id, std::nullopt, std::nullopt,
                                    new_delivery_selection);
    }
    if (new_delivery_selection == -1) {
        return update_history_order(order_id, std::nullopt, new_address,
                                    std::nullopt);
    }

    return update_history_order(order_id, std::nullopt, new_address,
                                new_delivery_selection);
}

void HistoryOrderManager::check_and_update_arrived_orders(int user_id) {

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新到达的历史订单状态。");
    }

    time_t now = get_current_time();

    try {

        string sql =
            "UPDATE history_orders SET status = ? WHERE user_id = ? AND "
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
        LOG_ERROR("更新到达的历史订单状态失败。");
    }
}

void HistoryOrderManager::delete_all_history_orders(const int user_id) {

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除数据库中的历史订单。");
    }

    try {
        Database::get_session()
            .sql("UPDATE history_orders SET status = ? WHERE user_id = ?")
            .bind(static_cast<int>(FullOrderStatus::DELETED), user_id)
            .execute();

    } catch (const mysqlx::Error &e) {
        LOG_ERROR("更新数据库中的历史订单失败: " + std::string(e.what()));
    }
}

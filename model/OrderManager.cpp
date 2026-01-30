/**
 * @file      OrderManager.cpp
 * @brief     订单管理模块实现文件
 * @details   实现了 OrderManager 类，核心功能包括：
 *            1. 将购物车条目转化为订单项并在 MySQL 数据库中持久化。
 *            2. 将分散的数据库订单项(OrderItem)聚合为完整订单(FullOrder)。
 *            3. 实现订单状态流转（下单 -> 配送 -> 完成/取消）。
 *            4. 模拟物流时间的自动收货逻辑。
 * @author    KP-usp
 * @date      2025-01-23
 * @version   2.0
 * @copyright Copyright (c) 2025
 */

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

    // 先执行自动收货逻辑，确保数据最新
    check_and_update_arrived_orders(user_id);

    orders_map.clear();
    is_loaded = false;

    try {
        Database::prepare(
            "SELECT user_id, product_id, order_id, "
            "count, UNIX_TIMESTAMP(order_time) AS order_time_unix, "
            "delivery_selection, address, status  FROM orders "
            "WHERE user_id = ? AND status = ?",
            [this, &product_manager, &user_id](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, user_id);
                pstmt->setInt(2,
                              static_cast<int>(FullOrderStatus::NOT_COMPLETED));
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                while (res->next()) {
                    OrderItem temp;

                    temp.id = res->getInt("user_id");
                    if (temp.id == user_id) {
                        temp.product_id = res->getInt("product_id");
                        temp.order_id = res->getInt64("order_id");
                        temp.count = res->getInt("count");
                        temp.order_time = static_cast<time_t>(
                            res->getInt64("order_time_unix"));
                        temp.delivery_selection =
                            res->getInt("delivery_selection");
                        temp.address = res->getString("address");
                        temp.status =
                            static_cast<FullOrderStatus>(res->getInt("status"));

                        FullOrder &order = orders_map[temp.order_id];

                        // 计算商品价格
                        double item_price =
                            product_manager.get_price_by_id(temp.product_id);
                        order.total_price += temp.count * item_price;

                        // 加载商品的信息，汇总到 map 中
                        if (order.items.empty()) {
                            // 第一次加载该 order_id 订单，加上运费
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
            });

    } catch (sql::SQLException &e) {
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
            std::string sql =
                "INSERT INTO orders (user_id, product_id, order_id, "
                "count, order_time,"
                "delivery_selection, address, status) "
                "values(?, ?, ?, ?, CURRENT_TIMESTAMP, ?, ?, ?) ";

            Database::prepare(sql, [&cart_item, &time,
                                    &address](sql::PreparedStatement *pstmt) {
                OrderItem new_order(cart_item.id, cart_item.product_id,
                                    cart_item.count, time,
                                    cart_item.delivery_selection, address,
                                    FullOrderStatus::NOT_COMPLETED);

                pstmt->setInt(1, cart_item.id);
                pstmt->setInt(2, cart_item.product_id);
                pstmt->setInt64(3, cart_item.id + static_cast<int64_t>(time));
                pstmt->setInt(4, cart_item.count);
                pstmt->setInt(5, cart_item.delivery_selection);
                pstmt->setString(6, address);
                pstmt->setInt(7,
                              static_cast<int>(FullOrderStatus::NOT_COMPLETED));
                pstmt->executeUpdate();
            });
        }
    } catch (sql::SQLException &e) {
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
        bool need_comma = false; // 判断是否要在字段前加逗号

        // 修改需要更新的字段
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

        Database::prepare(sql, [&](sql::PreparedStatement *pstmt) {
            int idx = 1;

            if (new_status.has_value()) {
                pstmt->setInt(idx++, static_cast<int>(new_status.value()));
            }
            if (new_address.has_value()) {
                pstmt->setString(idx++, new_address.value());
            }
            if (new_delivery.has_value()) {
                pstmt->setInt(idx++, new_delivery.value());
            }

            if (idx != 1)
                pstmt->setInt(idx, order_id);
            pstmt->executeUpdate();
        });

    } catch (sql::SQLException &e) {
        LOG_ERROR("更新数据库中的订单失败: " + std::string(e.what()));
    }
}

void OrderManager::update_stock_by_order_id(const long long order_id,
                                            ProductManager product_manager) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新数据库中的库存。");
    }

    try {
        Database::prepare(
            "SELECT product_id, count FROM orders WHERE order_id = ? ",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setInt64(1, static_cast<int64_t>(order_id));

                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                while (res->next()) {
                    int product_id = res->getInt("product_id");
                    int buy_count = res->getInt("count");

                    auto p_opt = product_manager.get_product(product_id);
                    if (p_opt.has_value()) {
                        auto &p = p_opt.value();
                        // 更新库存
                        product_manager.update_product(p.product_name,
                                                       product_id, p.price,
                                                       p.stock + buy_count);
                    }
                }
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("无法更新数据库商品库存");
    }
}

void OrderManager::cancel_order(const long long order_id,
                                ProductManager &product_manager) {
    // 更新商品库存
    update_stock_by_order_id(order_id, product_manager);
    // 仅更新状态
    return update_order(order_id, FullOrderStatus::CANCEL, std::nullopt,
                        std::nullopt);
}

void OrderManager::update_order_info(const long long order_id,
                                     const std::string &new_address,
                                     const int new_delivery_selection) {
    // "" 表示不修改地址
    if (new_address.empty()) {
        return update_order(order_id, std::nullopt, std::nullopt,
                            new_delivery_selection);
    }
    // -1 表示不修改送达方式
    if (new_delivery_selection == -1) {
        return update_order(order_id, std::nullopt, new_address, nullopt);
    }

    // 更新地址和配送方式，保持状态不变
    return update_order(order_id, std::nullopt, new_address,
                        new_delivery_selection);
}

void OrderManager::check_and_update_arrived_orders(int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新到达订单状态。");
    }

    OrderItem temp;
    time_t now = get_current_time();

    // 待更新的订单的 order_id 列表
    std::vector<long long> orders_to_complete;

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

        Database::prepare(sql, [&user_id, &now](sql::PreparedStatement *pstmt) {
            pstmt->setInt(1, static_cast<int>(FullOrderStatus::COMPLETED));
            pstmt->setInt(2, user_id);
            pstmt->setInt(3, static_cast<int>(FullOrderStatus::NOT_COMPLETED));
            pstmt->setInt64(4, static_cast<int64_t>(now));
            pstmt->executeUpdate();
        });

    } catch (sql::SQLException &e) {
        LOG_ERROR("更新到达订单状态失败。");
    }
}

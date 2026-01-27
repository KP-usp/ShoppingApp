/**
 * @file      HistoryOrderManager.cpp
 * @brief     历史订单管理模块实现文件
 * @details   实现了 HistoryOrderManager 类，用于管理已归档的历史订单快照。
 *            提供了历史记录的写入、聚合查询、状态更新（如取消/删除）
 *            以及清空历史记录（逻辑删除）的具体实现。
 * @author    KP-usp
 * @date      2025-01-24
 * @version   2.0
 * @copyright Copyright (c) 2025
 */

#include "HistoryOrderManager.h"
#include "Database.h"

using std::string;

void HistoryOrderManager::load_history_orders(const int user_id,
                                              ProductManager &product_manager) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载历史订单信息到内存。");
    }

    // 先执行自动收货逻辑，确保数据最新
    check_and_update_arrived_orders(user_id);

    history_orders_map.clear();
    is_loaded = false;

    try {
        Database::prepare(
            "SELECT user_id, product_name, order_id, price, count, "
            "UNIX_TIMESTAMP(order_time) AS order_time_unix, "
            "delivery_selection, address, status FROM history_orders "
            "WHERE user_id = ? AND status IN (1, -1) ",
            [this, &product_manager, &user_id](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, user_id);

                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                while (res->next()) {
                    HistoryOrderItem temp;

                    temp.id = res->getInt("user_id");
                    if (temp.id == user_id) {
                        temp.product_name = res->getString("product_name");
                        temp.order_id = res->getInt64("order_id");
                        temp.price = res->getDouble("price");
                        temp.count = res->getInt("count");
                        temp.order_time = static_cast<time_t>(
                            res->getInt64("order_time_unix"));
                        temp.delivery_selection =
                            res->getInt("delivery_selection");
                        temp.address = res->getString("address");
                        temp.status =
                            static_cast<FullOrderStatus>(res->getInt("status"));

                        HistoryFullOrder &order =
                            history_orders_map[temp.order_id];

                        // 计算商品价格
                        auto product_opt =
                            product_manager.get_product(temp.product_name);
                        if (!product_opt.has_value())
                            LOG_CRITICAL("数据库原商品信息被物理删除！");

                        auto pro_info = product_opt.value();

                        double item_price = pro_info.price;
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
            std::string sql =
                "insert into history_orders (user_id, "
                "product_name, price, order_id, "
                "count, order_time,"
                "delivery_selection, address, status) "
                "values(?, ?, ?, ?, ?, CURRENT_TIMESTAMP, ?, ?, ?) ";

            Database::prepare(sql, [&cart_item, &time, &address,
                                    &product_manager](
                                       sql::PreparedStatement *pstmt) {
                auto product_opt =
                    product_manager.get_product(cart_item.product_id);
                if (!product_opt.has_value())
                    LOG_CRITICAL("数据库原商品信息被物理删除！");

                auto pro_info = product_opt.value();

                HistoryOrderItem new_order(
                    cart_item.id, pro_info.product_name, pro_info.price,
                    cart_item.count, time, cart_item.delivery_selection,
                    address, FullOrderStatus::NOT_COMPLETED);

                pstmt->setInt(1, cart_item.id);
                pstmt->setString(2, pro_info.product_name);
                pstmt->setDouble(3, pro_info.price);
                pstmt->setInt64(4, cart_item.id + static_cast<int64_t>(time));
                pstmt->setInt(5, cart_item.count);
                pstmt->setInt(6, cart_item.delivery_selection);
                pstmt->setString(7, address);
                pstmt->setInt(8,
                              static_cast<int>(FullOrderStatus::NOT_COMPLETED));
                pstmt->executeUpdate();
            });
        }
    } catch (sql::SQLException &e) {
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
        LOG_ERROR("更新数据库中的历史订单失败: " + std::string(e.what()));
    }
}

void HistoryOrderManager::cancel_history_order(
    const long long order_id, ProductManager &product_manager) {
    // 仅更新状态
    return update_history_order(order_id, FullOrderStatus::CANCEL, std::nullopt,
                                std::nullopt);
}

void HistoryOrderManager::update_history_order_info(
    const long long order_id, const std::string &new_address,
    const int new_delivery_selection) {
    // "" 表示不修改地址
    if (new_address.empty()) {
        return update_history_order(order_id, std::nullopt, std::nullopt,
                                    new_delivery_selection);
    }
    // -1 表示不修改送达方式
    if (new_delivery_selection == -1) {
        return update_history_order(order_id, std::nullopt, new_address,
                                    std::nullopt);
    }

    // 更新地址和配送方式，保持状态不变
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

        Database::prepare(sql, [&user_id, &now](sql::PreparedStatement *pstmt) {
            pstmt->setInt(1, static_cast<int>(FullOrderStatus::COMPLETED));
            pstmt->setInt(2, user_id);
            pstmt->setInt(3, static_cast<int>(FullOrderStatus::NOT_COMPLETED));
            pstmt->setInt64(4, static_cast<int64_t>(now));
            pstmt->executeUpdate();
        });

    } catch (sql::SQLException &e) {
        LOG_ERROR("更新到达的历史订单状态失败。");
    }
}

void HistoryOrderManager::delete_all_history_orders(const int user_id) {

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除数据库中的历史订单。");
    }

    try {
        string sql = "UPDATE history_orders SET status = ? WHERE user_id = ? ";

        Database::prepare(sql, [&](sql::PreparedStatement *pstmt) {
            pstmt->setInt(1, static_cast<int>(FullOrderStatus::DELETED));
            pstmt->setInt(2, user_id);
            pstmt->executeUpdate();
        });

    } catch (sql::SQLException &e) {
        LOG_ERROR("更新数据库中的历史订单失败: " + std::string(e.what()));
    }
}

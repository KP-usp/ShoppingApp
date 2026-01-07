/**
 * @file      HistoryOrderManager.cpp
 * @brief     历史订单管理模块实现文件
 * @details   实现了 HistoryOrderManager 类，用于管理已归档的历史订单快照。
 *            提供了历史记录的写入、聚合查询、状态更新（如取消/删除）
 *            以及清空历史记录（逻辑删除）的具体实现。
 * @author    KP-usp
 * @date      2025-01-7
 * @version   1.0
 * @copyright Copyright (c) 2025
 */

#include "HistoryOrderManager.h"
#include <fstream>
#include <iostream>
#include <vector>

using std::fstream;
using std::ifstream;
using std::ios_base;
using std::ofstream;
using std::string;

// 辅助函数：更新底层文件状态
FileErrorCode
HistoryOrderManager::update_status_in_file(long long order_id,
                                           FullOrderStatus new_status) {
    string path = Utils::get_database_path(m_db_filename);
    fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open())
        return FileErrorCode::OpenFailure;

    HistoryOrderItem temp;
    bool found = false;

    while (iofile.read(reinterpret_cast<char *>(&temp),
                       sizeof(HistoryOrderItem))) {
        // 找到对应订单ID，且状态不一致时才写入
        if (temp.order_id == order_id && temp.status != new_status) {
            temp.status = new_status;

            // 回退写指针
            long write_pos = (long)iofile.tellg() - sizeof(HistoryOrderItem);
            iofile.seekp(write_pos);
            iofile.write(reinterpret_cast<const char *>(&temp),
                         sizeof(HistoryOrderItem));

            iofile.seekg(iofile.tellp());
            found = true;
        }
    }
    iofile.close();
    return found ? FileErrorCode::OK : FileErrorCode::NotFound;
}

FileErrorCode
HistoryOrderManager::cancel_history_order(const long long order_id,
                                          ProductManager &product_manager) {
    // 仅更新状态
    return update_history_order_in_file(order_id, FullOrderStatus::CANCEL,
                                        std::nullopt, std::nullopt);
}

FileErrorCode HistoryOrderManager::update_history_order_info(
    const long long order_id, const std::string &new_address,
    const int new_delivery_selection) {
    // "" 表示不修改地址
    if (new_address.empty()) {
        return update_history_order_in_file(
            order_id, std::nullopt, std::nullopt, new_delivery_selection);
    }
    // -1 表示不修改送达方式
    if (new_delivery_selection == -1) {
        return update_history_order_in_file(order_id, std::nullopt, new_address,
                                            std::nullopt);
    }

    // 更新地址和配送方式，保持状态不变
    return update_history_order_in_file(order_id, std::nullopt, new_address,
                                        new_delivery_selection);
}

FileErrorCode HistoryOrderManager::update_history_order_in_file(
    const long long order_id, std::optional<FullOrderStatus> new_status,
    std::optional<std::string> new_address, std::optional<int> new_delivery) {

    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    HistoryOrderItem temp;
    bool found = false;

    // 遍历整个文件，查找所有匹配 order_id 的记录
    while (iofile.read(reinterpret_cast<char *>(&temp),
                       sizeof(HistoryOrderItem))) {
        if (temp.order_id == order_id) {
            found = true;

            // 修改需要更新的字段
            if (new_status.has_value()) {
                temp.status = new_status.value();
            }
            if (new_address.has_value()) {
                temp.address = new_address.value();
            }
            if (new_delivery.has_value()) {
                temp.delivery_selection = new_delivery.value();
            }

            // 回退写指针
            long write_pos = (long)iofile.tellg() - sizeof(HistoryOrderItem);
            iofile.seekp(write_pos);

            iofile.write(reinterpret_cast<const char *>(&temp),
                         sizeof(HistoryOrderItem));

            iofile.seekg(iofile.tellp());
        }
    }

    iofile.close();
    return found ? FileErrorCode::OK : FileErrorCode::NotFound;
}

void HistoryOrderManager::check_and_update_arrived_orders(int user_id) {
    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open())
        return;

    HistoryOrderItem temp;
    time_t now = get_current_time();
    std::vector<long long> orders_to_complete;

    // 遍历寻找需要自动收货的订单ID
    while (infile.read(reinterpret_cast<char *>(&temp),
                       sizeof(HistoryOrderItem))) {
        if (temp.id == user_id &&
            temp.status == FullOrderStatus::NOT_COMPLETED) {

            // 获取配送所需天数
            int days_needed = 0;
            if (temp.delivery_selection >= 0 &&
                temp.delivery_selection < DELIVERY_DAYS.size()) {
                days_needed = DELIVERY_DAYS[temp.delivery_selection];
            }

            // 计算预计到达时间
            time_t arrival_time = temp.order_time + (days_needed * 86400);

            // 如果当前时间超过了预计到达时间
            if (now >= arrival_time) {
                // 简单的去重检查
                bool already_added = false;
                for (long long id : orders_to_complete) {
                    if (id == temp.order_id) {
                        already_added = true;
                        break;
                    }
                }
                if (!already_added) {
                    orders_to_complete.push_back(temp.order_id);
                }
            }
        }
    }
    infile.close();

    //  批量更新状态
    for (long long order_id : orders_to_complete) {
        update_status_in_file(order_id, FullOrderStatus::COMPLETED);
    }
}

FileErrorCode
HistoryOrderManager::load_history_orders(const int user_id,
                                         ProductManager &product_manager) {
    // 先执行自动收货逻辑，确保数据最新
    check_and_update_arrived_orders(user_id);

    history_orders_map.clear();
    is_loaded = false;

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    HistoryOrderItem temp;
    while (infile.read(reinterpret_cast<char *>(&temp),
                       sizeof(HistoryOrderItem))) {
        if (temp.id == user_id) {

            // 只处理已完成或已取消的订单
            if (temp.status == FullOrderStatus::COMPLETED ||
                temp.status == FullOrderStatus::CANCEL) {
                HistoryFullOrder &order = history_orders_map[temp.order_id];

                // 计算商品总价
                order.total_price += temp.count * temp.price;

                // 初始化头部信息
                if (order.items.empty()) {
                    order.order_id = temp.order_id;
                    order.order_time = temp.order_time;
                    order.address = temp.address;
                    order.status = temp.status;

                    // 加上运费
                    if (temp.delivery_selection >= 0 &&
                        temp.delivery_selection < 3)
                        order.total_price +=
                            DELIVERY_PRICES[temp.delivery_selection];
                }
                order.items.push_back(temp);
            }
        }
    }

    infile.close();
    is_loaded = true;
    return FileErrorCode::OK;
}

FileErrorCode HistoryOrderManager::add_history_order(
    const int user_id, ProductManager &product_manager,
    std::vector<CartItem> cart_lists, const std::string address) {

    time_t time = get_current_time();
    std::vector<HistoryOrderItem> history_order_list;

    // 添加订单的默认初始状态是 NOT_COMPLETED
    for (const auto &item : cart_lists) {
        if (!product_manager.get_product(item.product_id).has_value())
            return FileErrorCode::NotFound;
        auto p = product_manager.get_product(item.product_id).value();
        auto name = string(p.product_name);
        int price = p.price;

        HistoryOrderItem new_order(item.id, name, price, item.count, time,
                                   item.delivery_selection, address,
                                   FullOrderStatus::NOT_COMPLETED);
        history_order_list.push_back(new_order);
    }

    string path = Utils::get_database_path(m_db_filename);

    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        return FileErrorCode::OpenFailure;
    }
    for (const auto &history_order : history_order_list) {
        outfile.write(reinterpret_cast<const char *>(&history_order),
                      sizeof(HistoryOrderItem));
    }
    outfile.close();

    return FileErrorCode::OK;
}

FileErrorCode
HistoryOrderManager::delete_all_history_orders(const int user_id) {
    string path = Utils::get_database_path(m_db_filename);

    fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    HistoryOrderItem temp;
    bool found_any = false;

    // 遍历文件
    while (iofile.read(reinterpret_cast<char *>(&temp),
                       sizeof(HistoryOrderItem))) {

        // 匹配用户ID，且当前状态不是 DELETED (避免重复写入)
        if (temp.id == user_id && temp.status != FullOrderStatus::DELETED) {

            temp.status = FullOrderStatus::DELETED;

            long write_pos = (long)iofile.tellg() - sizeof(HistoryOrderItem);

            iofile.seekp(write_pos);

            iofile.write(reinterpret_cast<const char *>(&temp),
                         sizeof(HistoryOrderItem));

            iofile.seekg(iofile.tellp());

            found_any = true;
        }
    }

    iofile.close();

    // 如果内存中已加载该用户的数据，清空内存缓存以保持同步
    if (is_loaded) {
        history_orders_map.clear();
        // 此时 is_loaded 仍为 true，但 map 为空，表现为无历史记录
    }

    // 只要文件操作没问题，就算没找到订单也返回 OK
    return FileErrorCode::OK;
}

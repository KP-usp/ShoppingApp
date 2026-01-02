#include "HistoryOrderManager.h"
#include <fstream>
#include <iostream>
#include <vector>

using std::cerr;
using std::endl;
using std::fstream;
using std::ifstream;
using std::ios_base;
using std::ofstream;
using std::string;

// 辅助函数：更新底层文件状态
FileErrorCode
HistoryOrderManager::update_status_in_file(long long order_id,
                                           FullOrderStatus new_status) {
    string path = Utils::get_database_path(m_order_db_filename);
    fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open())
        return FileErrorCode::OpenFailure;

    OrderItem temp;
    bool found = false;

    while (iofile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {
        // 找到对应订单ID，且状态不一致时才写入
        if (temp.order_id == order_id && temp.status != new_status) {
            temp.status = new_status;

            // 回退写指针
            long write_pos = (long)iofile.tellg() - sizeof(OrderItem);
            iofile.seekp(write_pos);
            iofile.write(reinterpret_cast<const char *>(&temp),
                         sizeof(OrderItem));

            iofile.seekg(iofile.tellp());
            found = true;
        }
    }
    iofile.close();
    return found ? FileErrorCode::OK : FileErrorCode::NotFound;
}

void HistoryOrderManager::check_and_update_arrived_orders(int user_id) {
    string path = Utils::get_database_path(m_order_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open())
        return;

    OrderItem temp;
    time_t now = get_current_time();
    std::vector<long long> orders_to_complete;

    // 遍历寻找需要自动收货的订单ID
    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {
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

    string path = Utils::get_database_path(m_order_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    OrderItem temp;
    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {
        if (temp.id == user_id) {

            // 只处理已完成或已取消的订单
            if (temp.status == FullOrderStatus::COMPLETED ||
                temp.status == FullOrderStatus::CANCEL) {
                FullOrder &order = history_orders_map[temp.order_id];

                // 计算商品总价
                double item_price =
                    product_manager.get_price_by_id(temp.product_id);
                order.total_price += temp.count * item_price;

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

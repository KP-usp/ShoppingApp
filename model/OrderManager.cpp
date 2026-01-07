#include "OrderManager.h"
#include "CartManager.h"
#include <fstream>
#include <iostream>
#include <string>

using std::cerr;
using std::endl;
using std::ifstream;
using std::ios_base;
using std::nullopt;
using std::ofstream;
using std::optional;
using std::string;

FileErrorCode OrderManager::load_full_orders(const int user_id,
                                             ProductManager &product_manager) {
    // 先执行自动收货逻辑，确保数据最新
    check_and_update_arrived_orders(user_id);

    orders_map.clear();
    is_loaded = false;

    string path = Utils::get_database_path(m_order_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open()) {
        return FileErrorCode::OK; // 可能还没有下过单
    }

    OrderItem temp;

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {

        if (temp.id == user_id) {

            std::string path1 = "data/debug.log";

            std::ofstream outfile(path1, std::ios_base::app);
            if (outfile.is_open()) {
                outfile << "找到了订单" << std::endl;
                outfile.close();
            }

            FullOrder &order = orders_map[temp.order_id];

            // 计算商品价格
            double item_price =
                product_manager.get_price_by_id(temp.product_id);
            order.total_price += temp.count * item_price;

            // 加载商品的信息，汇总到 map 中
            if (order.items.empty()) {
                order.total_price += DELIVERY_PRICES[temp.delivery_selection];
                order.order_id = temp.order_id;
                order.order_time = temp.order_time;

                order.address = temp.address;
                order.status = temp.status;
            }

            order.items.push_back(temp);
        }
    }

    is_loaded = true;

    infile.close();

    return FileErrorCode::OK;
}

FileErrorCode OrderManager::add_order(const int user_id,
                                      std::vector<CartItem> cart_lists,
                                      const std::string address) {

    time_t time = get_current_time();
    std::vector<OrderItem> order_list;

    // 添加订单的默认初始状态是 NOT_COMPLETED
    for (auto cart_item : cart_lists) {

        OrderItem new_order(cart_item.id, cart_item.product_id, cart_item.count,
                            time, cart_item.delivery_selection, address,
                            FullOrderStatus::NOT_COMPLETED);
        order_list.push_back(new_order);
    }

    string path = Utils::get_database_path(m_order_db_filename);

    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }
    for (const auto &order : order_list) {
        outfile.write(reinterpret_cast<const char *>(&order),
                      sizeof(OrderItem));
    }
    outfile.close();

    return FileErrorCode::OK;
}

FileErrorCode OrderManager::update_order_in_file(
    const long long order_id, std::optional<FullOrderStatus> new_status,
    std::optional<std::string> new_address, std::optional<int> new_delivery) {

    string path = Utils::get_database_path(m_order_db_filename);
    std::fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    OrderItem temp;
    bool found = false;

    // 遍历整个文件，查找所有匹配 order_id 的记录
    while (iofile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {
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
            long write_pos = (long)iofile.tellg() - sizeof(OrderItem);
            iofile.seekp(write_pos);

            iofile.write(reinterpret_cast<const char *>(&temp),
                         sizeof(OrderItem));

            iofile.seekg(iofile.tellp());
        }
    }

    iofile.close();
    return found ? FileErrorCode::OK : FileErrorCode::NotFound;
}

FileErrorCode
OrderManager::update_stock_by_order_id(const long long order_id,
                                       ProductManager product_manager) {
    string path = Utils::get_database_path(m_order_db_filename);
    std::fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    OrderItem temp;
    bool found = false;

    // 遍历整个文件，查找所有匹配 order_id 的记录
    while (iofile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {
        if (temp.order_id == order_id) {
            int product_id = temp.product_id;
            int buy_count = temp.count;
            auto p_opt = product_manager.get_product(product_id);
            if (p_opt.has_value()) {
                auto &p = p_opt.value();
                product_manager.update_product(p.product_name, product_id,
                                               p.price, p.stock + buy_count);
            }
        }
    }
    return FileErrorCode::OK;
}

FileErrorCode OrderManager::cancel_order(const long long order_id,
                                         ProductManager &product_manager) {
    // 更新商品库存
    update_stock_by_order_id(order_id, product_manager);
    // 仅更新状态
    return update_order_in_file(order_id, FullOrderStatus::CANCEL, std::nullopt,
                                std::nullopt);
}

FileErrorCode
OrderManager::update_order_info(const long long order_id,
                                const std::string &new_address,
                                const int new_delivery_selection) {
    // "" 表示不修改地址
    if (new_address.empty()) {
        return update_order_in_file(order_id, std::nullopt, std::nullopt,
                                    new_delivery_selection);
    }
    // -1 表示不修改送达方式
    if (new_delivery_selection == -1) {
        return update_order_in_file(order_id, std::nullopt, new_address,
                                    nullopt);
    }

    // 更新地址和配送方式，保持状态不变
    return update_order_in_file(order_id, std::nullopt, new_address,
                                new_delivery_selection);
}

void OrderManager::check_and_update_arrived_orders(int user_id) {
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

// 辅助函数：更新底层文件状态
FileErrorCode OrderManager::update_status_in_file(long long order_id,
                                                  FullOrderStatus new_status) {
    string path = Utils::get_database_path(m_order_db_filename);
    std::fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

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

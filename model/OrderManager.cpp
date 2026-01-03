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
    long long order_id, std::optional<FullOrderStatus> new_status,
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

FileErrorCode OrderManager::cancel_order(long long order_id) {
    // 仅更新状态
    return update_order_in_file(order_id, FullOrderStatus::CANCEL, std::nullopt,
                                std::nullopt);
}

FileErrorCode OrderManager::update_order_info(long long order_id,
                                              const std::string &new_address,
                                              int new_delivery_selection) {
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

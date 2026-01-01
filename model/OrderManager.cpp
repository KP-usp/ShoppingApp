
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
        cerr << "open" << path << "is failed" << endl;
        return FileErrorCode::OpenFailure;
    }

    std::ofstream log("debug_load.log");

    OrderItem temp;
    int count = 0;

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {
        count++;
        // 每读10条打印一次，或者每条都打印
        log << "Reading Item " << count << ", OrderID: " << temp.order_id
            << std::flush;

        if (temp.id == user_id) {
            log << " -> Matching User..." << std::flush;

            FullOrder &order = orders_map[temp.order_id]; // 这里的 map 操作

            log << " [Map Accessed] " << std::flush;

            double item_price =
                product_manager.get_price_by_id(temp.product_id);
            order.total_price += temp.count * item_price;

            if (order.items.empty()) {
                log << " [Init Order] " << std::flush;
                order.total_price += DELIVERY_PRICES[temp.delivery_selection];
                order.order_id = temp.order_id;
                order.order_time = temp.order_time;

                // 【重点】这里会触发 FixedString 的赋值
                order.address = temp.address;
                log << " [Address Copied]: " + order.address << std::flush;
            }

            order.items.push_back(temp);
            log << " -> Done." << std::endl;
        } else {
            log << " -> Skipped." << std::endl;
        }
    }

    log << "Loop Finished." << std::endl;

    is_loaded = true;

    infile.close();

    return FileErrorCode::OK;
}

FileErrorCode OrderManager::add_order(const int user_id,
                                      std::vector<CartItem> cart_lists,
                                      const std::string address) {

    time_t time = get_current_time();

    std::vector<OrderItem> order_list;

    for (auto cart_item : cart_lists) {

        OrderItem new_order(cart_item.id, cart_item.product_id, cart_item.count,
                            time, cart_item.delivery_selection, address);
        order_list.push_back(new_order);
    }

    string path = Utils::get_database_path(m_order_db_filename);

    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    for (auto order : order_list) {
        outfile.write(reinterpret_cast<char *>(&order), sizeof(OrderItem));
    }

    outfile.close();
    return FileErrorCode::OK;
}

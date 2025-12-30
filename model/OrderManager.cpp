
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

    OrderItem temp;

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(OrderItem))) {

        if (temp.id == user_id) {
            orders_map[temp.order_id].total_price +=
                temp.count *
                    (product_manager.get_price_by_id(temp.product_id)) +
                DELIVERY_PRICES[temp.delivery_selection];

            std::string path = "data/debug.log";

            std::ofstream outfile(path, std::ios_base::app);
            if (outfile.is_open()) {
                outfile << "递送选项："
                        << DELIVERY_PRICES[temp.delivery_selection]
                        << std::endl;
                outfile.close();
            }
            orders_map[temp.order_id].order_id = temp.order_id;
            orders_map[temp.order_id].order_time = temp.order_time;
            orders_map[temp.order_id].items.push_back(temp);
        }
    }

    is_loaded = true;

    infile.close();

    return FileErrorCode::OK;
}

FileErrorCode OrderManager::add_order(const int user_id,
                                      std::vector<CartItem> cart_lists) {

    string path1 = Utils::get_database_path(m_cart_db_filename);
    ifstream infile(path1, ios_base::binary);
    if (!infile.is_open()) {
        cerr << "open " << path1 << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    time_t time = get_current_time();

    std::vector<OrderItem> order_list;

    for (auto cart_item : cart_lists) {

        OrderItem new_order(cart_item.id, cart_item.product_id, cart_item.count,
                            time, cart_item.delivery_selection);
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

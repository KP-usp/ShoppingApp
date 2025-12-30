#include "CartManager.h"
#include <fstream>
#include <iostream>

using std::cerr;
using std::endl;
using std::ifstream;
using std::ios_base;
using std::ofstream;
using std::string;
using std::string_view;

FileErrorCode CartManager::load_cart(const int user_id) {
    // 状态清空
    is_loaded = false;
    cart_list.clear();

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open()) {
        cerr << "open" << path << "is failed" << endl;
        return FileErrorCode::OpenFailure;
    }

    CartItem temp;

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(CartItem))) {
        if (temp.status == CartItemStatus::NOT_ORDERED) {

            std::string path = Utils::get_database_path("debug.log");

            std::ofstream outfile(path, std::ios_base::app);
            if (outfile.is_open()) {
                outfile << "加载商品: " << temp.product_id << " load_cart数量："
                        << temp.count << std::endl;
                outfile.close();
            }
            cart_list.emplace_back(temp);
        }
    }

    infile.close();

    is_loaded = true;

    return FileErrorCode::OK;
}

std::pair<std::streampos, CartItem>
CartManager::find_item(const int user_id, const int product_id,
                       std::vector<CartItemStatus> target_status) {

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return {-1, {}}; // 表示没找到(不管原因)
    }

    CartItem temp;

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(CartItem))) {

        if (temp.id == user_id && temp.product_id == product_id) {
            for (auto s : target_status) {
                if (temp.status == s) {
                    auto file_pos =
                        infile.tellg() - (std::streamoff)sizeof(CartItem);

                    return {file_pos, temp};
                }
            }
        }
    }

    infile.close();

    return {-1, {}};
}

FileErrorCode CartManager::add_item(const int user_id, const int product_id,
                                    const int count) {

    std::string path1 = Utils::get_database_path("debug.log");
    std::ofstream outfile1(path1, std::ios_base::app);
    if (outfile1.is_open()) {
        outfile1 << "商品号 " << product_id << " add_item 商品的数量: " << count
                 << std::endl;
        outfile1.close();
    }

    auto [pos_not_ordered, item_not_ordered] =
        find_item(user_id, product_id, {CartItemStatus::NOT_ORDERED});

    if (pos_not_ordered != -1) {
        // 有相同类型的已下单商品, 合并数量
        int new_count = item_not_ordered.count + count;

        CartItem new_item = item_not_ordered;
        new_item.count = new_count;
        update_item_at_pos(pos_not_ordered, new_item);
        return FileErrorCode::OK;
    }

    auto [pos_deleted, item_deleted] =
        find_item(user_id, product_id, {CartItemStatus::DELETED});

    if (pos_deleted != -1) {
        // 有相同类型的已删除商品，覆盖原来的数量并更新状态
        CartItem new_item = item_deleted;
        new_item.status = CartItemStatus::NOT_ORDERED;
        new_item.count = count;
        update_item_at_pos(pos_deleted, new_item);
        return FileErrorCode::OK;
    }

    // 若没有同类型的商品，直接写在数据库末尾
    string path = Utils::get_database_path(m_db_filename);
    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    CartItem temp(user_id, product_id, count); // 默认状态是未下单和未选择

    outfile.write(reinterpret_cast<const char *>(&temp), sizeof(CartItem));

    if (outfile.fail())
        return FileErrorCode::WriteFailure;

    outfile.flush();
    outfile.close();
    return FileErrorCode::OK;
}

FileErrorCode CartManager::update_item_at_pos(std::streampos pos,
                                              const CartItem &item) {
    string path = Utils::get_database_path(m_db_filename);

    std::fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);

    if (!iofile.is_open()) {
        return FileErrorCode::OpenFailure;
    }

    iofile.seekp(pos);
    iofile.write(reinterpret_cast<const char *>(&item), sizeof(CartItem));

    if (iofile.fail())
        return FileErrorCode::WriteFailure;

    iofile.close();
    return FileErrorCode::OK;
}

FileErrorCode CartManager::update_item(const int user_id, const int product_id,
                                       const int count,
                                       const CartItemStatus status,
                                       const int delivery_selection) {

    auto [file_pos, item] = find_item(user_id, product_id, {status});

    if (file_pos == -1)
        return FileErrorCode::NotFound;

    string path = Utils::get_database_path(m_db_filename);
    ofstream iofile(path, ios_base::binary | ios_base::out | ios_base::in);
    if (!iofile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }
    iofile.seekp(file_pos);

    if (iofile.fail()) {
        cerr << "Write failed." << endl;
        iofile.close();
        return FileErrorCode::SeekFailure;
    }

    CartItem temp(user_id, product_id, count, status, delivery_selection);

    iofile.write(reinterpret_cast<const char *>(&temp), sizeof(CartItem));

    if (iofile.fail()) {
        cerr << "Write failed." << endl;
        iofile.close();
        return FileErrorCode::WriteFailure;
    }

    iofile.flush();
    iofile.close();

    return FileErrorCode::OK;
}

FileErrorCode CartManager::delete_item(const int user_id,
                                       const int product_id) {

    auto [file_pos, item] =
        find_item(user_id, product_id, {CartItemStatus::NOT_ORDERED});

    if (file_pos == -1)
        return FileErrorCode::NotFound;

    string path = Utils::get_database_path(m_db_filename);
    std::ofstream outfile(path,
                          ios_base::out | ios_base::binary | ios_base::in);
    if (!outfile.is_open()) {
        cerr << "open " << path << "is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    CartItem temp(user_id, product_id, item.count, CartItemStatus::DELETED);

    outfile.seekp(file_pos);

    outfile.write(reinterpret_cast<const char *>(&temp), sizeof(CartItem));

    outfile.close();

    return FileErrorCode::OK;
}

std::vector<CartItem> CartManager::checkout(int user_id) {

    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::binary | ios_base::out | ios_base::in);
    if (!iofile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return {}; // 表示不能找到(暂且不管原因)
    }

    CartItem item;
    std::vector<CartItem> items;

    while (iofile.read(reinterpret_cast<char *>(&item), sizeof(CartItem))) {
        // 订单需要添加未下单但是已经勾选的商品
        if (item.id == user_id && item.status == CartItemStatus::NOT_ORDERED &&
            item.delivery_selection != -1) {
            auto current_pos = iofile.tellg();
            auto item_head_pos = current_pos - (std::streamoff)sizeof(CartItem);
            item.status = CartItemStatus::DELETED;

            iofile.seekp(item_head_pos);
            iofile.write(reinterpret_cast<const char *>(&item),
                         sizeof(CartItem));
            items.push_back(item);
        }
    }

    return items;
}

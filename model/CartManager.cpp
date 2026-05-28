#include "CartManager.h"
#include "Database.h"

using std::string;
using std::string_view;

void CartManager::load_cart(const int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载商品信息到内存。");
    }

    is_loaded = false;
    cart_list.clear();

    try {
        auto res = Database::get_session()
                       .sql("SELECT user_id, product_id, count, status, "
                            "delivery_selection FROM carts")
                       .execute();
        while (auto row = res.fetchOne()) {
            if (static_cast<int>(CartItemStatus::NOT_ORDERED) !=
                row[3].get<int>()) {
                continue;
            }

            CartItem temp;

            temp.id = row[0].get<int>();
            temp.product_id = row[1].get<int>();
            temp.count = row[2].get<int>();
            temp.delivery_selection = row[4].get<int>();
            temp.status = static_cast<CartItemStatus>(row[3].get<int>());
            cart_list.push_back(temp);
        }
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("加载购物车列表到内存失败，" + string(e.what()));
    };

    is_loaded = true;
}

void CartManager::add_item(const int user_id, const int product_id,
                           const int count) {

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法添加新商品。");
    }

    try {
        Database::get_session()
            .sql("INSERT INTO carts (user_id, product_id, count, status) "
                 "VALUES (?, ?, ?, ?) "
                 "ON DUPLICATE KEY UPDATE count = count + VALUES(count)")
            .bind(user_id, product_id, count,
                  static_cast<int>(CartItemStatus::NOT_ORDERED))
            .execute();
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("添加新商品失败: " + std::string(e.what()));
    }
}

void CartManager::update_item(const int user_id, const int product_id,
                              const int count, const int delivery_selection) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新购物车商品。");
    }

    try {
        Database::get_session()
            .sql("UPDATE carts SET count = ?, delivery_selection = ? WHERE "
                 "user_id = ? AND product_id = ? AND status = ?")
            .bind(count, delivery_selection, user_id, product_id,
                  static_cast<int>(CartItemStatus::NOT_ORDERED))
            .execute();
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("更新购物车商品失败: " + std::string(e.what()));
    }
}

void CartManager::delete_item(const int user_id, const int product_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除购物车商品。");
    }

    try {
        Database::get_session()
            .sql("DELETE FROM carts WHERE user_id = ? AND product_id = ? "
                 "AND status = ?")
            .bind(user_id, product_id,
                  static_cast<int>(CartItemStatus::DELETED))
            .execute();

        Database::get_session()
            .sql("UPDATE carts SET status = ? WHERE user_id = ? AND "
                 "product_id = ?")
            .bind(static_cast<int>(CartItemStatus::DELETED), user_id, product_id)
            .execute();
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("删除购物车商品失败: " + std::string(e.what()));
    }
}

std::vector<CartItem> CartManager::checkout(int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载结账商品。");
    }

    std::vector<CartItem> items;

    std::vector<int> product_ids_to_delete;

    try {
        auto res = Database::get_session()
                       .sql("SELECT user_id, product_id, count, "
                            "delivery_selection FROM carts "
                            "WHERE user_id = ? AND status = ? "
                            "AND delivery_selection != ?")
                       .bind(user_id,
                             static_cast<int>(CartItemStatus::NOT_ORDERED), -1)
                       .execute();
        while (auto row = res.fetchOne()) {
            CartItem temp;

            temp.id = row[0].get<int>();
            temp.product_id = row[1].get<int>();
            temp.count = row[2].get<int>();
            temp.delivery_selection = row[3].get<int>();

            product_ids_to_delete.push_back(temp.product_id);
            items.push_back(temp);
        }

        for (auto &pid : product_ids_to_delete)
            delete_item(user_id, pid);

    } catch (const mysqlx::Error &e) {
        LOG_ERROR("结账商品失败" + string(e.what()));
    };

    return items;
}

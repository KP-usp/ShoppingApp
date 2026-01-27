/**
 * @file      CartManager.cpp
 * @brief     购物车管理模块实现文件
 * @details   实现了 CartManager 类的功能，包括购物车数据的加载、
 *            商品添加/删除/更新、库存检查预备逻辑以及结算数据打包。
 *            负责处理储存在 MySQL 表单的购物车数据的CRUD 操作。
 * @author    KP-usp
 * @date      2025-01-23
 * @version   1.0
 * @copyright Copyright (c) 2025
 */

#include "CartManager.h"
#include "Database.h"

using std::string;
using std::string_view;

void CartManager::load_cart(const int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载商品信息到内存。");
    }

    // 状态清空
    is_loaded = false;
    cart_list.clear();

    try {
        Database::prepare(
            "SELECT user_id, product_id, count, status, "
            "delivery_selection FROM carts ",
            [this](sql::PreparedStatement *pstmt) {
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

                while (res->next()) {
                    if (static_cast<int>(CartItemStatus::NOT_ORDERED) !=
                        res->getInt("status")) {
                        continue;
                    }

                    CartItem temp;

                    temp.id = res->getInt("user_id");
                    temp.product_id = res->getInt("product_id");
                    temp.count = res->getInt("count");
                    temp.delivery_selection = res->getInt("delivery_selection");
                    temp.status =
                        static_cast<CartItemStatus>(res->getInt("status"));
                    cart_list.push_back(temp);
                }
            });
    } catch (sql::SQLException &e) {
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
        // 添加商品，若存在则更新数量
        std::string sql =
            "INSERT INTO carts (user_id, product_id, count, status) VALUES (?, "
            "?, ?, ?) "
            "ON DUPLICATE KEY UPDATE count = count + VALUES(count) ";
        Database::prepare(sql, [&user_id, &product_id,
                                &count](sql::PreparedStatement *pstmt) {
            pstmt->setInt(1, user_id);
            pstmt->setInt(2, product_id);
            pstmt->setInt(3, count);
            pstmt->setInt(4, static_cast<int>(CartItemStatus::NOT_ORDERED));
            pstmt->executeUpdate();
        });
    } catch (sql::SQLException &e) {
        LOG_ERROR("添加新商品失败: " + std::string(e.what()));
    }
}

void CartManager::update_item(const int user_id, const int product_id,
                              const int count, const int delivery_selection) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新购物车商品。");
    }

    try {
        Database::prepare(
            "UPDATE carts SET count = ?, delivery_selection = ? WHERE "
            " user_id = ? AND product_id = ? AND status = ?",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, count);
                pstmt->setInt(2, delivery_selection);
                pstmt->setInt(3, user_id);
                pstmt->setInt(4, product_id);
                pstmt->setInt(5, static_cast<int>(CartItemStatus::NOT_ORDERED));

                pstmt->executeUpdate();
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("更新购物车商品失败: " + std::string(e.what()));
    }
}

void CartManager::delete_item(const int user_id, const int product_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除购物车商品。");
    }

    try {
        // 软删除结账后的购物车商品
        Database::prepare(
            "DELETE FROM carts WHERE user_id = ? AND product_id = ? "
            "AND status = ? ",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, user_id);
                pstmt->setInt(2, product_id);
                pstmt->setInt(3, static_cast<int>(CartItemStatus::DELETED));
                pstmt->executeUpdate();
            });

        Database::prepare(
            "UPDATE carts SET status = ? WHERE user_id = ? AND product_id = ? ",
            [&user_id, &product_id](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, static_cast<int>(CartItemStatus::DELETED));
                pstmt->setInt(2, user_id);
                pstmt->setInt(3, product_id);
                pstmt->executeUpdate();
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("删除购物车商品失败: " + std::string(e.what()));
    }
}

std::vector<CartItem> CartManager::checkout(int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载结账商品。");
    }

    // 返回给订单类生成订单和历史订单
    std::vector<CartItem> items;

    // 要删除的商品 id
    std::vector<int> product_ids_to_delete;

    try {
        // 结账已勾选、未软删除的商品
        Database::prepare(
            "SELECT user_id, product_id, count, delivery_selection FROM carts "
            " WHERE user_id = ? AND status = ? AND delivery_selection != ?",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, user_id);
                pstmt->setInt(2, static_cast<int>(CartItemStatus::NOT_ORDERED));
                pstmt->setInt(3, -1); // 筛选出勾选的商品

                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                while (res->next()) {
                    CartItem temp;

                    temp.id = res->getInt("user_id");
                    temp.product_id = res->getInt("product_id");
                    temp.count = res->getInt("count");
                    temp.delivery_selection = res->getInt("delivery_selection");

                    product_ids_to_delete.push_back(temp.product_id);
                    items.push_back(temp);
                }
            });

        for (auto &product_id : product_ids_to_delete)
            delete_item(user_id, product_id);

    } catch (sql::SQLException &e) {
        LOG_ERROR("结账商品失败" + string(e.what()));
    };

    return items;
}

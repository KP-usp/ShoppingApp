/**
 * @file      ProductManager.cpp
 * @brief     商品管理模块实现文件
 * @details   实现了 ProductManager 类，负责商品信息的全生命周期管理。
 *            包括基于名称的模糊搜索和基于 id 精确搜索实现和负责处理储存购物车
 *            MySQL 表单数据的 CRUD 操作。
 * @author    KP-usp
 * @date      2025-01-23
 * @version   2.0
 * @copyright Copyright (c) 2025
 */

#include "ProductManager.h"
#include "Database.h"
#include "Logger.h"
#include <fstream>

using std::ifstream;
using std::nullopt;
using std::ofstream;
using std::optional;
using std::string;
using std::string_view;

void ProductManager::load_all_product() {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法加载商品信息到内存。");
    }

    is_loaded = false;

    product_list.clear();

    try {
        Database::prepare(
            "SELECT product_name, product_id, price, stock, status FROM "
            "products",
            [this](sql::PreparedStatement *pstmt) {
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                while (res->next()) {
                    Product temp;

                    temp.product_name = res->getString("product_name");
                    temp.product_id = res->getInt("product_id");
                    temp.price = res->getDouble("price");
                    temp.stock = res->getInt("stock");
                    temp.status =
                        static_cast<ProductStatus>(res->getInt("status"));
                    product_list.push_back(temp);
                }
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("加载商品列表到内存失败，" + string(e.what()));
    };

    is_loaded = true;
}

void ProductManager::add_product(const string &product_name, const double price,
                                 const int stock) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法添加新商品。");
    }

    try {
        std::string sql = "INSERT INTO products (product_name, price, stock) "
                          "VALUES(?, ?, ?)";
        Database::prepare(sql, [&product_name, &price,
                                &stock](sql::PreparedStatement *pstmt) {
            pstmt->setString(1, product_name);
            pstmt->setDouble(2, price);
            pstmt->setInt(3, stock);
            pstmt->executeUpdate();
        });
    } catch (sql::SQLException &e) {
        LOG_ERROR("添加新商品失败: " + std::string(e.what()));
    }
}

void ProductManager::delete_product(const int product_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除商品。");
    }

    try {

        Database::prepare("UPDATE products SET status = ? WHERE product_id = ?",
                          [&product_id](sql::PreparedStatement *pstmt) {
                              pstmt->setInt(
                                  1, static_cast<int>(ProductStatus::DELETED));
                              pstmt->setInt(2, product_id);
                              pstmt->executeUpdate();
                          });
    } catch (sql::SQLException &e) {
        LOG_ERROR("删除商品失败: " + std::string(e.what()));
    }
}

void ProductManager::restore_product(const int product_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法恢复商品。");
    }

    try {
        Database::prepare("UPDATE products SET status = ? WHERE product_id = ?",
                          [&product_id](sql::PreparedStatement *pstmt) {
                              pstmt->setInt(
                                  1, static_cast<int>(ProductStatus::NORMAL));
                              pstmt->setInt(2, product_id);
                              pstmt->executeUpdate();
                          });
    } catch (sql::SQLException &e) {
        LOG_ERROR("恢复商品失败: " + std::string(e.what()));
    }
}

void ProductManager::update_product(const string &product_name,
                                    const int product_id, const double price,
                                    const int stock) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新商品。");
    }

    try {
        Database::prepare(
            "UPDATE products SET product_name = ?, price = ?, stock = ? WHERE "
            "product_id = ?",
            [&product_name, &price, &stock,
             &product_id](sql::PreparedStatement *pstmt) {
                pstmt->setString(1, product_name);
                pstmt->setDouble(2, price);
                pstmt->setInt(3, stock);
                pstmt->setInt(4, product_id);
                pstmt->executeUpdate();
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("更新商品失败: " + std::string(e.what()));
    }
}

std::vector<Product>
ProductManager::search_all_product(const std::string &query) {
    std::vector<Product> result;

    // 确保数据已加载
    if (!is_loaded) {
        load_all_product();
    }

    // 如果查询为空，返回所有商品
    if (query.empty()) {
        result = product_list;
        return result;
    }

    // 转换为小写以进行不区分大小写的搜索
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   ::tolower);

    for (const auto &p : product_list) {

        // 检查 ID (将 ID 转字符串匹配)
        std::string id_str = std::to_string(p.product_id);

        // 检查名称 (转小写匹配)
        std::string name_str = std::string(p.product_name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                       ::tolower);

        // 如果 ID 等于查询串或名称包含查询串
        if (id_str == lower_query) {
            result.push_back(p);
            break;
        }

        if (name_str.find(lower_query) != std::string::npos) {
            result.push_back(p);
        }
    }
    return result;
}

std::vector<Product>
ProductManager::search_product(const std::string &query_name) {
    std::vector<Product> result;

    // 由于可能库存更新所以这里需要重新从数据库加载
    product_list.clear();
    load_all_product();

    // 如果查询为空，返回所有未删除的商品
    if (query_name.empty()) {
        for (const auto &p : product_list) {
            if (p.status != ProductStatus::DELETED)
                result.emplace_back(p);
        }
        return result;
    }

    // 转换为小写以进行不区分大小写的搜索
    std::string lower_query = query_name;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   ::tolower);

    for (const auto &p : product_list) {
        if (p.status == ProductStatus::DELETED)
            continue;

        // 检查 ID (将 ID 转字符串匹配)
        std::string id_str = std::to_string(p.product_id);

        // 检查名称 (转小写匹配)
        std::string name_str = std::string(p.product_name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                       ::tolower);

        // 如果 ID 等于查询串或名称包含查询串
        if (id_str == lower_query) {
            result.push_back(p);
            break;
        }

        if (name_str.find(lower_query) != std::string::npos) {
            result.push_back(p);
        }
    }
    return result;
}

std::optional<Product> ProductManager::get_product(const int product_id) {
    auto con = Database::get_connection();
    if (!con)
        LOG_ERROR("数据库未连接，无法获取商品信息。");

    // 查询结果
    optional<Product> result = nullopt;

    try {
        Database::prepare(
            "SELECT product_name, price, stock, status FROM products "
            "WHERE product_id = ?",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setInt(1, product_id);
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

                if (res->next() &&
                    res->getInt("status") ==
                        static_cast<int>(ProductStatus::NORMAL)) {
                    Product temp;
                    temp.product_id = product_id;
                    temp.product_name = res->getString("product_name");
                    temp.price = res->getDouble("price");
                    temp.stock = res->getInt("stock");

                    result = temp;
                }
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("获取商品信息失败，" + string(e.what()));
    }

    return result;
}

std::optional<Product>
ProductManager::get_product(const std::string &product_name) {
    auto con = Database::get_connection();
    if (!con)
        LOG_ERROR("数据库未连接，无法获取商品信息。");

    // 查询结果
    Product result;

    try {
        Database::prepare(
            "SELECT product_name, product_id, price, stock, status FROM "
            "products "
            "WHERE product_name = ?",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setString(1, product_name);

                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

                if (res->next() &&
                    res->getInt("status") ==
                        static_cast<int>(ProductStatus::NORMAL)) {
                    Product temp;
                    temp.product_id = res->getInt("product_id");
                    temp.product_name = res->getString("product_name");
                    temp.price = res->getDouble("price");
                    temp.stock = res->getInt("stock");

                    result = temp;
                }
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("获取商品信息失败，" + string(e.what()));
    }

    return result;
}

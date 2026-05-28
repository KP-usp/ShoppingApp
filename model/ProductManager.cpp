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
        auto res = Database::get_session()
                       .sql("SELECT product_name, product_id, price, stock, "
                            "status FROM products")
                       .execute();
        while (auto row = res.fetchOne()) {
            Product temp;

            temp.product_name = row[0].get<std::string>();
            temp.product_id = row[1].get<int>();
            temp.price = row[2].get<double>();
            temp.stock = row[3].get<int>();
            temp.status = static_cast<ProductStatus>(row[4].get<int>());
            product_list.push_back(temp);
        }
    } catch (const mysqlx::Error &e) {
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
        Database::get_session()
            .sql("INSERT INTO products (product_name, price, stock) "
                 "VALUES(?, ?, ?)")
            .bind(product_name, price, stock)
            .execute();
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("添加新商品失败: " + std::string(e.what()));
    }
}

void ProductManager::delete_product(const int product_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除商品。");
    }

    try {
        Database::get_session()
            .sql("UPDATE products SET status = ? WHERE product_id = ?")
            .bind(static_cast<int>(ProductStatus::DELETED), product_id)
            .execute();
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("删除商品失败: " + std::string(e.what()));
    }
}

void ProductManager::restore_product(const int product_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法恢复商品。");
    }

    try {
        Database::get_session()
            .sql("UPDATE products SET status = ? WHERE product_id = ?")
            .bind(static_cast<int>(ProductStatus::NORMAL), product_id)
            .execute();
    } catch (const mysqlx::Error &e) {
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
        Database::get_session()
            .sql("UPDATE products SET product_name = ?, price = ?, stock = ? "
                 "WHERE product_id = ?")
            .bind(product_name, price, stock, product_id)
            .execute();
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("更新商品失败: " + std::string(e.what()));
    }
}

std::vector<Product>
ProductManager::search_all_product(const std::string &query) {
    std::vector<Product> result;

    if (!is_loaded) {
        load_all_product();
    }

    if (query.empty()) {
        result = product_list;
        return result;
    }

    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   ::tolower);

    for (const auto &p : product_list) {

        std::string id_str = std::to_string(p.product_id);

        std::string name_str = std::string(p.product_name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                       ::tolower);

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

    product_list.clear();
    load_all_product();

    if (query_name.empty()) {
        for (const auto &p : product_list) {
            if (p.status != ProductStatus::DELETED)
                result.emplace_back(p);
        }
        return result;
    }

    std::string lower_query = query_name;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   ::tolower);

    for (const auto &p : product_list) {
        if (p.status == ProductStatus::DELETED)
            continue;

        std::string id_str = std::to_string(p.product_id);

        std::string name_str = std::string(p.product_name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                       ::tolower);

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
    if (!Database::is_connected())
        LOG_ERROR("数据库未连接，无法获取商品信息。");

    optional<Product> result = nullopt;

    try {
        auto res = Database::get_session()
                       .sql("SELECT product_name, price, stock, status "
                            "FROM products WHERE product_id = ?")
                       .bind(product_id)
                       .execute();
        auto row = res.fetchOne();
        if (row && row[3].get<int>() == static_cast<int>(ProductStatus::NORMAL)) {
            Product temp;
            temp.product_id = product_id;
            temp.product_name = row[0].get<std::string>();
            temp.price = row[1].get<double>();
            temp.stock = row[2].get<int>();

            result = temp;
        }
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("获取商品信息失败，" + string(e.what()));
    }

    return result;
}

std::optional<Product>
ProductManager::get_product(const std::string &product_name) {
    if (!Database::is_connected())
        LOG_ERROR("数据库未连接，无法获取商品信息。");

    Product result;

    try {
        auto res = Database::get_session()
                       .sql("SELECT product_name, product_id, price, stock, "
                            "status FROM products WHERE product_name = ?")
                       .bind(product_name)
                       .execute();
        auto row = res.fetchOne();
        if (row && row[4].get<int>() == static_cast<int>(ProductStatus::NORMAL)) {
            Product temp;
            temp.product_id = row[1].get<int>();
            temp.product_name = row[0].get<std::string>();
            temp.price = row[2].get<double>();
            temp.stock = row[3].get<int>();

            result = temp;
        }
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("获取商品信息失败，" + string(e.what()));
    }

    return result;
}

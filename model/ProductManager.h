#pragma once
#include "FileError.h"

#include "FixedString.h"
#include <Utils.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// 商品状态：是否删除
enum class ProductStatus {
    DELETED = -1,
    NORMAL = 0,
};

// 存储在数据库文件中的商品信息结构体
struct Product {
    static constexpr int MAX_PRODUCT_NAME_SIZE = 100;
    FixedString<MAX_PRODUCT_NAME_SIZE> product_name;
    int product_id;
    double price;
    ProductStatus status;

    Product() = default;

    Product(const std::string_view &name, const double p, const int id = -1,
            ProductStatus s = ProductStatus::NORMAL)
        : product_id(id), product_name(name), price(p), status(s) {}
};

// 商品管理类
class ProductManager {
  private:
    using string_view = std::string_view;

    // 数据库文件
    std::string m_db_filename;

    // 从数据库中加载的商品列表（供 UI 绘制）
    std::vector<Product> product_list;

    // product_list 是否加载
    bool is_loaded = false;

    // 初始化数据库，向数据库写入文件头（方便为商品生成 ID）
    void init_db_file();

    // 生成 ID 并更新文件
    int generate_and_update_id();

    // 辅助函数：获取商品结构在文件中的位置
    std::optional<std::streampos> get_product_pos(const int product_id);

    // 添加删除标志
    void mark_deleted(Product &product) {
        product.status = ProductStatus::DELETED;
    }

  public:
    ProductManager(const string_view &db_filename)
        : m_db_filename(db_filename) {
        init_db_file();
    }

    // 从数据库中导入商品信息
    FileErrorCode load_product();

    // 获取从数据库中导入的商品列表
    std::optional<std::vector<Product> *> get_product_list_ptr() {
        if (is_loaded)
            return &product_list;
        else
            return std::nullopt;
    };

    // 向数据库添加商品
    FileErrorCode add_product(const string_view &product_name, const int price);

    // 删除数据库中的商品（添加删除标志）
    FileErrorCode delete_product(const int product_id);

    // 更新数据库中的商品
    FileErrorCode update_product(const string_view &new_product_name,
                                 const int product_id, const int new_price);

    // 根据商品 id 获取商品信息
    std::optional<Product> get_product(const int product_id);

    // 根据商品名获取商品 id
    const std::optional<int> get_id_by_name(const string_view &product_name);

    // 根据 id 获取商品价格（用于价格计算）
    const double get_price_by_id(int product_id) {

        auto product_opt = get_product(product_id);
        if (product_opt.has_value()) {
            auto product = product_opt.value();
            return product.price;
        } else
            return -1;
    }
};

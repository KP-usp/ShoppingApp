/**
 * @file      ProductManager.h
 * @brief     商品管理模块头文件
 * @details   定义了商品结构体(Product)和商品管理类(ProductManager)，
 *            负责商品的增删改查、库存管理及数据持久化。
 */

#pragma once
#include "FileError.h"
#include "FixedString.h"
#include <Utils.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief 商品状态枚举
 *
 * 标识商品是否已被逻辑删除
 */
enum class ProductStatus {
    DELETED = -1, ///< 已删除
    NORMAL = 0,   ///< 正常
};

/**
 * @brief 商品信息结构体 (数据库存储格式)
 *
 * 存储商品的 ID、名称、价格、库存及当前状态。
 * 使用 FixedString 以确保文件读写时的定长对齐。
 */
struct Product {
    static constexpr int MAX_PRODUCT_NAME_SIZE = 100;

    FixedString<MAX_PRODUCT_NAME_SIZE> product_name; ///< 商品名称
    int product_id;                                  ///< 商品唯一 ID
    double price;                                    ///< 商品单价
    int stock;                                       ///< 当前库存
    ProductStatus status;                            ///< 商品状态

    Product() = default;

    Product(const std::string_view &name, const double p, const int stock,
            const int id = -1, ProductStatus s = ProductStatus::NORMAL)
        : product_id(id), product_name(name), price(p), stock(stock),
          status(s) {}
};

/**
 * @brief 商品管理类
 *
 * 提供对商品数据库的 CRUD 操作，包括加载列表、模糊搜索、库存调整及价格查询等。
 */
class ProductManager {
  private:
    using string_view = std::string_view;

    // 商品数据库文件名
    std::string m_db_filename;

    // 内存缓存：所有商品列表（供 UI 绘制及快速查询）
    std::vector<Product> product_list;

    // 标志位：product_list 是否已加载
    bool is_loaded = false;

    // 辅助函数：初始化数据库，写入文件头（用于 ID 生成管理）
    void init_db_file();

    // 辅助函数：生成新 ID 并更新文件头计数器
    int generate_and_update_id();

    // 辅助函数：获取指定 ID 商品在文件流中的位置
    std::optional<std::streampos> get_product_pos(const int product_id);

  public:
    /**
     * @brief 构造函数
     *
     * @param db_filename 商品数据库文件名
     */
    ProductManager(const string_view &db_filename)
        : m_db_filename(db_filename) {
        init_db_file();
    }

    /**
     * @brief 加载所有商品
     *
     * 从数据库中读取所有商品信息（包含已删除的商品）到内存缓存中。
     *
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS
     */
    FileErrorCode load_all_product();

    /**
     * @brief 添加新商品
     *
     * @param product_name 商品名称
     * @param price 商品单价
     * @param stock 初始库存
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS
     */
    FileErrorCode add_product(const string_view &product_name,
                              const double price, const int stock);

    /**
     * @brief 删除商品
     *
     * 对商品进行软删除（状态置为 DELETED）。
     *
     * @param product_id 商品 ID
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS
     */
    FileErrorCode delete_product(const int product_id);

    /**
     * @brief 更新商品信息
     *
     * @param new_product_name 新商品名
     * @param product_id 目标商品 ID
     * @param new_price 新价格
     * @param stock 新库存
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS
     */
    FileErrorCode update_product(const string_view &new_product_name,
                                 const int product_id, const double new_price,
                                 const int stock);

    /**
     * @brief 搜索所有商品 (包括已删除)
     *
     * 支持 ID 精确匹配或商品名称模糊查找。
     *
     * @param query 查询关键词（ID字符串或商品名）
     * @return std::vector<Product> 匹配的商品列表
     */
    std::vector<Product> search_all_product(const std::string &query);

    /**
     * @brief 搜索有效商品 (仅限未删除)
     *
     * 仅支持商品名称模糊查找，且过滤掉状态为 DELETED
     * 的商品。通常用于用户端展示。
     *
     * @param name 商品名关键词
     * @return std::vector<Product> 匹配的商品列表
     */
    std::vector<Product> search_product(const std::string &name);

    /**
     * @brief 恢复商品
     *
     * 将已删除的商品状态恢复为 NORMAL。
     *
     * @param product_id 商品 ID
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS
     */
    FileErrorCode restore_product(const int product_id);

    /**
     * @brief 获取单个商品信息
     *
     * @param product_id 商品 ID
     * @return std::optional<Product> 成功返回商品对象，失败返回 nullopt
     */
    std::optional<Product> get_product(const int product_id);

    /**
     * @brief 根据名称获取商品 ID
     *
     * @param product_name 商品名称（需完全匹配）
     * @return const std::optional<int> 成功返回 ID，失败返回 nullopt
     */
    const std::optional<int> get_id_by_name(const string_view &product_name);

    /**
     * @brief 获取商品价格
     *
     * 辅助函数，用于订单计算。
     *
     * @param product_id 商品 ID
     * @return double 商品价格，若未找到返回 -1
     */
    const double get_price_by_id(int product_id) {
        auto product_opt = get_product(product_id);
        if (product_opt.has_value()) {
            auto product = product_opt.value();
            return product.price;
        } else
            return -1;
    }
};

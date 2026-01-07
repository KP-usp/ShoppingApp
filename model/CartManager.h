/**
 * @file      CartManager.h
 * @brief     购物车管理模块头文件
 * @details   定义了购物车商品结构体(CartItem)和购物车管理类(CartManager)，
 *            主要负责商品的添加、删除、修改以及结账和购物车数据持久化。
 */

#pragma once
#include "FileError.h"
#include <Utils.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief 购物车商品状态枚举
 *
 * 标识商品是在购物车中等待下单，还是已被逻辑删除
 */
enum class CartItemStatus {
    NOT_ORDERED = 0, ///< 未下单（在购物车中）
    DELETED = -1,    ///< 已删除
};

/**
 * @brief 购物车单项商品结构体
 *
 * 存储购物车中单个商品条目的信息，包括所属用户、商品ID、数量及状态
 */
struct CartItem {
    int id; ///< 所属用户的 ID (User ID)

    int product_id; ///< 商品 ID

    int count; ///< 购买数量

    CartItemStatus status; ///< 商品条目状态

    int delivery_selection; ///< 配送方式选择 (-1 表示还未勾选)

    CartItem() = default;

    CartItem(const int user_id, const int p_i, const int count,
             const CartItemStatus s = CartItemStatus::NOT_ORDERED,
             const int selection = -1)
        : id(user_id), product_id(p_i), count(count), status(s),
          delivery_selection(selection) {}
};

/**
 * @brief 购物车管理类
 *
 * 管理购物车数据，提供商品添加、状态更新、持久化存储以及加载未下单商品到内存等功能
 */
class CartManager {
  private:
    // 存储购物车数据的数据库文件名
    std::string m_db_filename;

    // 内存缓存：从数据库中导出的该用户未下单的购物车商品列表
    std::vector<CartItem> cart_list;

    // 标志位：当前用户的购物车数据是否已加载到内存
    bool is_loaded = false;

    // 辅助函数：将购物车条目状态标记为已删除
    void mark_deleted(CartItem &cart_item) {
        cart_item.status = CartItemStatus::DELETED;
    }

    // 辅助函数：根据文件流位置更新记录，不负责查找逻辑
    FileErrorCode update_item_at_pos(std::streampos pos, const CartItem &item);

  public:
    /**
     * @brief 构造函数
     *
     * @param db_filename 购物车数据库文件名
     */
    CartManager(const std::string_view &db_filename)
        : m_db_filename(db_filename) {}

    /**
     * @brief 加载购物车数据
     *
     * 从数据库文件中筛选出指定用户且状态为未下单(NOT_ORDERED)的商品，加载到内存中
     *
     * @param user_id 用户 ID
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode load_cart(const int user_id);

    /**
     * @brief 获取购物车商品列表指针
     *
     * 获取内存中缓存的购物车列表。
     *
     * @return std::optional<std::vector<CartItem> *>
     * 若已加载则返回列表指针，否则返回 nullopt
     * @note 调用此函数前必须先调用 load_cart 确保数据已加载
     */
    std::optional<std::vector<CartItem> *> get_cart_list_ptr() {
        if (is_loaded)
            return &cart_list;
        else
            return std::nullopt;
    }

    /**
     * @brief 添加商品到购物车
     *
     * 向数据库写入一条新的购物车记录。如果该用户已存在相同的未下单商品，逻辑上可能进行数量叠加（视具体实现而定）。
     *
     * @param user_id 用户 ID
     * @param product_id 商品 ID
     * @param count 购买数量
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode add_item(const int user_id, const int product_id,
                           const int count);

    /**
     * @brief 查找购物车商品
     *
     * 在数据库文件中查找符合条件的商品条目位置和内容。
     *
     * @param user_id 用户 ID
     * @param product_id 商品 ID
     * @param status_list 待匹配的状态列表（如查找未下单或已删除的记录）
     * @return std::pair<std::streampos, CartItem>
     * 返回文件流位置和对应的商品对象；若未找到，位置通常为 -1
     */
    std::pair<std::streampos, CartItem>
    find_item(const int user_id, const int product_id,
              std::vector<CartItemStatus> status_list);

    /**
     * @brief 更新购物车商品信息
     *
     * 修改数据库中已存在的商品条目信息（数量、状态、配送选项）。
     *
     * @param user_id 用户 ID
     * @param product_id 商品 ID
     * @param count 新的数量
     * @param status 新的状态
     * @param delivery_selection 配送选项（默认为 -1 不变）
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode update_item(const int user_id, const int product_id,
                              const int count, const CartItemStatus status,
                              const int delivery_selection = -1);

    /**
     * @brief 删除购物车商品
     *
     * 对商品进行软删除（修改状态为 DELETED），而非物理删除。
     *
     * @param user_id 用户 ID
     * @param product_id 商品 ID
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode delete_item(const int user_id, const int product_id);

    /**
     * @brief 购物车结算准备
     *
     * 获取用户当前所有有效的购物车商品，准备生成订单。
     *
     * @param user_id 用户 ID
     * @return std::vector<CartItem> 返回待结算的商品列表
     * @note 此函数通常会配合 OrderManager 使用
     */
    std::vector<CartItem> checkout(int user_id);

    // 析构器
    ~CartManager() {}
};

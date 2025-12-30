#pragma once
#include "FileError.h"
#include <Utils.h>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class CartItemStatus {
    NOT_ORDERED = 0,
    DELETED = -1,
}; // 添加状态

struct CartItem { // 结构发生改变
    int id;

    int product_id;

    int count;

    CartItemStatus status;

    int delivery_selection; // -1 表示还未勾选

    CartItem() = default;

    CartItem(const int user_id, const int p_i, const int count,
             const CartItemStatus s = CartItemStatus::NOT_ORDERED,
             const int selection = -1)
        : id(user_id), product_id(p_i), count(count), status(s),
          delivery_selection(selection) {}
};

class CartManager {
  private:
    // 数据库文件
    std::string m_db_filename;

    // 这是引用，指向AppContext 的文件锁 ·
    // std::mutex &db_mutex;

    // 从数据库中导出的未下单的购物车商品
    std::vector<CartItem> cart_list;

    // 是否加载到内存
    bool is_loaded = false;

    // 辅助函数：添加删除标志
    void mark_deleted(CartItem &cart_item) {
        cart_item.status = CartItemStatus::DELETED;
    }

    // 通用的底层更新函数，直接根据位置更新，不负责查找
    FileErrorCode update_item_at_pos(std::streampos pos, const CartItem &item);

  public:
    CartManager(const std::string_view &db_filename)
        : m_db_filename(db_filename) {}

    // 从数据库中加载未下单的购物车商品
    FileErrorCode load_cart(const int user_id);

    // 获取购物车未下单商品列表的指针
    std::optional<std::vector<CartItem> *> get_cart_list_ptr() {
        if (is_loaded)
            return &cart_list;
        else
            return std::nullopt;
    }

    // 向数据库文件添加购物车商品(支持同类同用户商品的数量叠加)
    FileErrorCode add_item(const int user_id, const int product_id,
                           const int count);

    // 通用的查找位置函数（方便更新和查找是否存在）
    std::pair<std::streampos, CartItem> find_item(const int user_id,
                                                  const int product_id,
                                                  std::vector<CartItemStatus>);

    // 更新数据库中购物车商品(这里指定购物车商品默认状态为未下单)
    FileErrorCode update_item(const int user_id, const int product_id,
                              const int count, const CartItemStatus status,
                              const int delivery_selection = -1);

    // 删除数据库中购物车商品
    FileErrorCode delete_item(const int user_id, const int product_id);

    // 将该用户的商品打包到内存，传给 OrderManager 处理
    std::vector<CartItem> checkout(int user_id);

    ~CartManager() {}
};

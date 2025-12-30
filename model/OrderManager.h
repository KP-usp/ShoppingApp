#pragma once
#include "CartManager.h"
#include "FileError.h"
#include "ProductManager.h"
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct OrderItem {
    int id;
    int product_id;
    long long order_id;
    int count;
    time_t order_time;
    int delivery_selection;

    OrderItem() = default;
    OrderItem(const int user_id, const int product_id, const int count,
              const time_t time, const int delivery_selection)
        : id(user_id), product_id(product_id), count(count), order_time(time),
          delivery_selection(delivery_selection), order_id(time + user_id) {}
};

enum class FullOrderStatus {
    NOT_COMPLETED = 0, // 未抵达
    COMPLETED = 1,     // 已抵达
    CANCEL = -1,       // 取消
};

struct FullOrder {
    long long order_id;
    double total_price = 0.0;
    time_t order_time;
    std::vector<OrderItem> items;
};

constexpr int DELIVERY_PRICES[] = {
    0, // StandardDelivery (索引0)
    3, // FastDelivery (索引1)
    6, // ExpressDelivery (索引2)
};

// 这个订单类负责从购物车数据库的同个用户不同时间点的订单分别打包到内存中
// 总体来说还是为了方便 UI 绘制
class OrderManager {

  private:
    using string_view = std::string_view;

    // 数据库文件1: 存储 OrderItem 的文件
    std::string m_order_db_filename;

    // 数据库文件2： Cart 数据库文件
    std::string m_cart_db_filename;

    // 这是引用，指向AppContext 的文件锁, 当操作 Cart
    // 数据库加载下单商品时使用·
    // std::mutex &db_mutex;

    // 从数据库中加载的单一用户的订单列表（供 UI 绘制）
    std::map<long long, FullOrder> orders_map;

    // order_list 是否加载
    bool is_loaded = false;

    // 获取当前时间(无格式化)
    time_t get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        return in_time_t;
    }

  public:
    OrderManager(const string_view &order_db_filename)
        : m_order_db_filename(order_db_filename) {}

    // 从 Order
    // 数据库打包不同时间批次订单到内存（orders_map）
    FileErrorCode load_full_orders(const int user_id,
                                   ProductManager &product_manager);

    // 获取从数据库中导入的商品列表
    std::optional<std::map<long long, FullOrder> *> get_orders_map_ptr() {
        if (is_loaded)
            return &orders_map;
        else
            return std::nullopt;
    }

    // 数据库文件添加已下单商品（从 Cart 数据库 load 再重新组装成 OrderItem）
    FileErrorCode add_order(const int user_id,
                            std::vector<CartItem> cart_lists);

    // 根据用户 id 和 商品 id
    // 从数据库文件获取购物车商品信息
    std::optional<OrderItem> get_order(const int user_id, const int product_id);

    // 更新数据库中购物车商品(这里指定购物车商品默认状态为未下单)
    FileErrorCode update_order(const int user_id, const int product_id,
                               const int count);
    // // 删除数据库中订单，这里不完善
    // FileErrorCode delete_order(const int user_id, const int product_id);

    ~OrderManager() {}
};

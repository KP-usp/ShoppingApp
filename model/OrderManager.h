#pragma once
#include "CartManager.h"
#include "FileError.h"
#include "ProductManager.h"
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class FullOrderStatus {
    NOT_COMPLETED = 0, // 未抵达
    COMPLETED = 1,     // 已抵达
    CANCEL = -1,       // 取消
};

struct OrderItem {
    static constexpr int MAX_ADDRESS_LENGTH = 50;
    int id;
    int product_id;
    long long order_id;
    int count;
    time_t order_time;
    int delivery_selection;
    FixedString<MAX_ADDRESS_LENGTH> address;
    FullOrderStatus status; // 与聚合物一致的状态

    OrderItem() = default;
    OrderItem(const int user_id, const int product_id, const int count,
              const time_t time, const int delivery_selection,
              const std::string addr, const FullOrderStatus s)
        : id(user_id), product_id(product_id), count(count), order_time(time),
          delivery_selection(delivery_selection), order_id(time + user_id),
          address(addr), status(s) {}
};

struct FullOrder {
    static constexpr int MAX_ADDRESS_LENGTH = 50;
    long long order_id;
    double total_price = 0.0;
    time_t order_time;
    std::vector<OrderItem> items;
    FixedString<MAX_ADDRESS_LENGTH> address;
    FullOrderStatus status = FullOrderStatus::NOT_COMPLETED; // 聚合状态
};

constexpr int DELIVERY_PRICES[] = {
    0, // 普通递送 (索引0)
    3, // 快速递送 (索引1)
    6, // 特快递送 (索引2)
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

    // 辅助：通用更新函数，根据 order_id 更新特定字段
    FileErrorCode
    update_order_in_file(const long long order_id,
                         std::optional<FullOrderStatus> new_status,
                         std::optional<std::string> new_address,
                         std::optional<int> new_delivery);
    // 辅助：根据 order_id 获取相应订单所有商品的 product_id 和 购买数量,
    // 据此更新商品库存
    FileErrorCode update_stock_by_order_id(const long long order_id,
                                           ProductManager product_manager);

  public:
    OrderManager(const string_view &order_db_filename)
        : m_order_db_filename(order_db_filename) {}

    // 加载并聚合订单
    // UI层可以通过检查 FullOrder.status
    // 来过滤“正在进行”或“历史”订单(HistoryOrderPage)
    FileErrorCode load_full_orders(const int user_id,
                                   ProductManager &product_manager);

    std::optional<std::map<long long, FullOrder> *> get_orders_map_ptr() {
        if (is_loaded)
            return &orders_map;
        else
            return std::nullopt;
    }

    // 下单
    FileErrorCode add_order(const int user_id, std::vector<CartItem> cart_lists,
                            const std::string address);

    // 取消订单, 并更新商品库存
    // 将该 order_id 下的所有 Item 状态置为 CANCEL
    FileErrorCode cancel_order(const long long order_id,
                               ProductManager &product_manager);

    // 更新订单信息 (包含地址和配送服务)
    FileErrorCode update_order_info(const long long order_id,
                                    const std::string &new_address,
                                    const int new_delivery_selection);

    ~OrderManager() {}
};

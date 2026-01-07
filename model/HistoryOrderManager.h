#pragma once
#include "CartManager.h"
#include "FileError.h"
#include "OrderManager.h"
#include "ProductManager.h"
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct HistoryOrderItem {
    static constexpr int MAX_ADDRESS_LENGTH = 50;
    static constexpr int MAX_PRODUCT_NAME_SIZE = 100;
    int id;
    FixedString<MAX_PRODUCT_NAME_SIZE> product_name;
    double price;
    long long order_id;
    int count;
    time_t order_time;
    int delivery_selection;
    FixedString<MAX_ADDRESS_LENGTH> address;
    FullOrderStatus status; // 与聚合物一致的状态

    HistoryOrderItem() = default;
    HistoryOrderItem(const int user_id, const std::string &name, const double p,
                     const int count, const time_t time,
                     const int delivery_selection, const std::string addr,
                     const FullOrderStatus s)
        : id(user_id), product_name(name), price(p), count(count),
          order_time(time), delivery_selection(delivery_selection),
          order_id(time + user_id), address(addr), status(s) {}
};

struct HistoryFullOrder {
    static constexpr int MAX_ADDRESS_LENGTH = 50;
    long long order_id;
    double total_price = 0.0;
    time_t order_time;
    std::vector<HistoryOrderItem> items;
    FixedString<MAX_ADDRESS_LENGTH> address;
    FullOrderStatus status = FullOrderStatus::NOT_COMPLETED; // 聚合状态
};

class HistoryOrderManager {
  private:
    using string_view = std::string_view;

    // 数据库文件名
    std::string m_db_filename;

    // 历史订单缓存 (key: order_id)
    std::map<long long, HistoryFullOrder> history_orders_map;

    bool is_loaded = false;

    // 配送时间映射 (索引对应配送方式 0, 1, 2)
    // 0: 普通(5天), 1: 普快(3天), 2: 特快(0天)
    const std::vector<int> DELIVERY_DAYS = {5, 3, 1};

    // 获取当前系统时间
    time_t get_current_time() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::system_clock::to_time_t(now);
    }

    // 内部辅助：更新文件中的订单状态
    FileErrorCode update_status_in_file(long long order_id,
                                        FullOrderStatus new_status);

    // 辅助：通用更新函数，根据 order_id 更新特定字段
    FileErrorCode
    update_history_order_in_file(const long long order_id,
                                 std::optional<FullOrderStatus> new_status,
                                 std::optional<std::string> new_address,
                                 std::optional<int> new_delivery);

  public:
    HistoryOrderManager(const string_view &order_db_filename)
        : m_db_filename(order_db_filename) {}

    // 检查是否有订单已到期，并自动更新状态为 COMPLETED
    void check_and_update_arrived_orders(int user_id);

    // 加载属于该用户的历史订单（已完成 + 已取消）
    FileErrorCode load_history_orders(const int user_id,
                                      ProductManager &product_manager);
    // 添加历史订单（订单快照）
    FileErrorCode add_history_order(const int user_id,
                                    ProductManager &product_manager,
                                    std::vector<CartItem> cart_lists,
                                    const std::string address);

    // 将对应历史订单置删除状态
    FileErrorCode cancel_history_order(const long long order_id,
                                       ProductManager &product_manager);

    // 更新历史订单信息 (包含地址和配送服务)
    FileErrorCode update_history_order_info(const long long order_id,
                                            const std::string &new_address,
                                            const int new_delivery_selection);

    // 获取历史订单 Map 指针
    std::optional<std::map<long long, HistoryFullOrder> *>
    get_history_map_ptr() {
        if (is_loaded)
            return &history_orders_map;
        else
            return std::nullopt;
    }

    // 清空历史记录（逻辑删除或物理删除，此处暂且保留接口，视需求实现）
    // FileErrorCode clear_history(int user_id);

    ~HistoryOrderManager() {}
};

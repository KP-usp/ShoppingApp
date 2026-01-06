#pragma once
#include "FileError.h"
#include "OrderManager.h"
#include "ProductManager.h"
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class HistoryOrderManager {
  private:
    using string_view = std::string_view;

    // 数据库文件名(与 OrderManager 的一致)
    std::string m_order_db_filename;

    // 历史订单缓存 (key: order_id)
    std::map<long long, FullOrder> history_orders_map;

    bool is_loaded = false;

    // 配送时间映射 (索引对应配送方式 0, 1, 2)
    // 0: 普通(5天), 1: 普快(3天), 2: 特快(0天)
    const std::vector<int> DELIVERY_DAYS = {5, 3, 0};

    // 获取当前系统时间
    time_t get_current_time() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::system_clock::to_time_t(now);
    }

    // 内部辅助：更新文件中的订单状态
    FileErrorCode update_status_in_file(long long order_id,
                                        FullOrderStatus new_status);

  public:
    HistoryOrderManager(const string_view &order_db_filename)
        : m_order_db_filename(order_db_filename) {}

    // 检查是否有订单已到期，并自动更新状态为 COMPLETED
    void check_and_update_arrived_orders(int user_id);

    // 加载属于该用户的历史订单（已完成 + 已取消）
    FileErrorCode load_history_orders(const int user_id,
                                      ProductManager &product_manager);
    // 添加历史订单（订单快照）
    FileErrorCode add_history_order(const int user_id,
                                    std::vector<CartItem> cart_lists,
                                    const std::string address);

    // 获取历史订单 Map 指针
    std::optional<std::map<long long, FullOrder> *> get_history_map_ptr() {
        if (is_loaded)
            return &history_orders_map;
        else
            return std::nullopt;
    }

    // 清空历史记录（逻辑删除或物理删除，此处暂且保留接口，视需求实现）
    // FileErrorCode clear_history(int user_id);

    ~HistoryOrderManager() {}
};

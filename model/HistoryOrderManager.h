/**
 * @file      HistoryOrderManager.h
 * @brief     历史订单管理模块头文件
 * @details 定义了历史订单项(HistoryOrderItem)、历史订单聚合体(HistoryFullOrder)
 *            及历史订单管理类(HistoryOrderManager)。
 *            负责历史订单的归档、查询、状态自动更新以及记录清除。
 */

#pragma once
#include "CartManager.h"
#include "OrderManager.h"
#include "ProductManager.h"
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief 历史订单项结构体 (数据库存储格式)
 *
 * 存储已经归档（下单后）的商品记录快照。
 * 包含当时购买的价格、名称等信息，不再随商品库变化而变化。
 */
struct HistoryOrderItem {
    static constexpr int MAX_ADDRESS_LENGTH = 50;
    static constexpr int MAX_PRODUCT_NAME_SIZE = 100;

    int id;                   ///< 用户 ID
    std::string product_name; ///< 商品名称快照
    double price;             ///< 购买时的单价快照
    long long order_id;       ///< 订单号
    int count;                ///< 购买数量
    time_t order_time;        ///< 下单时间
    int delivery_selection;   ///< 配送方式索引
    std::string address;      ///< 配送地址
    FullOrderStatus status;   ///< 订单状态

    HistoryOrderItem() = default;

    HistoryOrderItem(const int user_id, const std::string &name, const double p,
                     const int count, const time_t time,
                     const int delivery_selection, const std::string addr,
                     const FullOrderStatus s)
        : id(user_id), product_name(name), price(p), count(count),
          order_time(time), delivery_selection(delivery_selection),
          order_id(time + user_id), address(addr), status(s) {}
};

/**
 * @brief 历史订单聚合体 (内存聚合格式)
 *
 * 用于 UI 展示。将同一订单号的分散商品项聚合在一起，
 * 并计算总金额（含运费）。
 */
struct HistoryFullOrder {
    static constexpr int MAX_ADDRESS_LENGTH = 50;

    long long order_id;                  ///< 订单号
    double total_price = 0.0;            ///< 订单总价
    time_t order_time;                   ///< 下单时间
    std::vector<HistoryOrderItem> items; ///< 包含的商品项列表
    std::string address;                 ///< 配送地址
    FullOrderStatus status = FullOrderStatus::NOT_COMPLETED; ///< 聚合状态
};

/**
 * @brief 历史订单管理类
 *
 * 管理历史订单数据库，提供归档记录的增加、读取、状态更新及清空功能。
 * 用于“我的订单”或“历史记录”页面的数据支持。
 */
class HistoryOrderManager {
  private:
    // 内存缓存：历史订单映射表 (key: order_id)
    std::map<long long, HistoryFullOrder> history_orders_map;

    // 标志位：当前用户的历史数据是否已加载
    bool is_loaded = false;

    // 配送时间映射 (索引对应配送方式 0, 1, 2)
    // 0: 普通(5天), 1: 普快(3天), 2: 特快(1天)
    const std::vector<int> DELIVERY_DAYS = {5, 3, 1};

    // 辅助函数：获取当前系统时间戳
    time_t get_current_time() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::system_clock::to_time_t(now);
    }

    // 辅助函数：通用更新函数，根据 order_id 更新特定字段(状态/地址/配送)
    void update_history_order(const long long order_id,
                              std::optional<FullOrderStatus> new_status,
                              std::optional<std::string> new_address,
                              std::optional<int> new_delivery);

  public:
    /**
     * @brief 构造函数
     *
     */
    HistoryOrderManager() = default;

    /**
     * @brief 检查并自动收货
     *
     * 遍历数据库，检查未完成的订单是否已超过预计送达时间。
     * 若已超时，自动将状态更新为 COMPLETED。
     *
     * @param user_id 用户 ID
     */
    void check_and_update_arrived_orders(int user_id);

    /**
     * @brief 加载历史订单
     *
     * 读取属于该用户的历史记录，过滤出 COMPLETED 或 CANCEL 状态的订单，
     * 并聚合到内存 map 中供 UI 调用。
     *
     * @param user_id 用户 ID
     * @param product_manager 商品管理器引用（预留接口，目前主要使用快照数据）
     * @return 无返回值
     */
    void load_history_orders(const int user_id,
                             ProductManager &product_manager);

    /**
     * @brief 添加历史订单
     *
     * 在用户下单时调用，将购物车内容转换为历史快照存入数据库。
     *
     * @param user_id 用户 ID
     * @param product_manager 商品管理器（用于获取商品名称和当前价格）
     * @param cart_lists 购物车商品列表
     * @param address 配送地址
     * @return 无返回值
     */
    void add_history_order(const int user_id, ProductManager &product_manager,
                           std::vector<CartItem> cart_lists,
                           const std::string address);

    /**
     * @brief 取消历史订单
     *
     * 将对应的历史订单状态修改为 CANCEL。
     *
     * @param order_id 订单号
     * @param product_manager 商品管理器引用
     * @return 无返回值
     */
    void cancel_history_order(const long long order_id,
                              ProductManager &product_manager);

    /**
     * @brief 更新历史订单信息
     *
     * 修改订单的配送地址或配送方式。
     *
     * @param order_id 订单号
     * @param new_address 新地址（传空字符串表示不修改）
     * @param new_delivery_selection 新配送方式（传-1表示不修改）
     * @return 无返回值
     */
    void update_history_order_info(const long long order_id,
                                   const std::string &new_address,
                                   const int new_delivery_selection);

    /**
     * @brief 删除指定用户的所有历史订单（逻辑删除）
     *
     * 将该用户所有的历史订单项状态更新为 DELETED。
     * 这些记录仍在数据库文件中，但不再会被 load_history_orders 加载。
     *
     * @param user_id 用户 ID
     * @return 无返回值
     */
    void delete_all_history_orders(const int user_id);

    /**
     * @brief 获取历史订单映射指针
     *
     * @return std::optional<std::map<long long, HistoryFullOrder> *>
     * 若已加载返回指针，否则返回 nullopt
     */
    std::optional<std::map<long long, HistoryFullOrder> *>
    get_history_map_ptr() {
        if (is_loaded)
            return &history_orders_map;
        else {
            LOG_CRITICAL("历史订单列表未加载到内存");
            return std::nullopt;
        }
    }

    // 析构器
    ~HistoryOrderManager() {}
};

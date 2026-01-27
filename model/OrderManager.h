/**
 * @file      OrderManager.h
 * @brief     订单管理模块头文件
 * @details
 * 定义了订单项(OrderItem)、完整订单(FullOrder)结构及订单管理类(OrderManager)，
 *            负责订单的创建、聚合加载、状态更新、取消及自动收货检测。
 */

#pragma once
#include "CartManager.h"
#include "Logger.h"
#include "ProductManager.h"
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief 订单状态枚举
 *
 * 标识订单当前的物流或逻辑状态
 */
enum class FullOrderStatus {
    NOT_COMPLETED = 0, ///< 未抵达（运输中）
    COMPLETED = 1,     ///< 已抵达（已完成）
    CANCEL = -1,       ///< 已取消
    DELETED = -2,      ///< 已删除
};

/**
 * @brief 单个订单项结构体 (数据库存储格式)
 *
 * 代表订单中的一行记录（即一种商品）。一个逻辑上的“大订单”可能由多个 OrderItem
 * 组成。
 */
struct OrderItem {
    static constexpr int MAX_ADDRESS_LENGTH = 50;

    int id;                 ///< 用户 ID
    int product_id;         ///< 商品 ID
    long long order_id;     ///< 订单号 (通常由 时间戳 + 用户ID 生成)
    int count;              ///< 购买数量
    time_t order_time;      ///< 下单时间
    int delivery_selection; ///< 配送方式索引
    std::string address;    ///< 配送地址
    FullOrderStatus status; ///< 订单项状态

    OrderItem() = default;
    OrderItem(const int user_id, const int product_id, const int count,
              const time_t time, const int delivery_selection,
              const std::string addr, const FullOrderStatus s)
        : id(user_id), product_id(product_id), count(count), order_time(time),
          delivery_selection(delivery_selection), order_id(time + user_id),
          address(addr), status(s) {}
};

/**
 * @brief 完整订单结构体 (内存聚合格式)
 *
 * 用于 UI 展示。将数据库中具有相同 order_id 的多个 OrderItem 聚合在一起，
 * 形成一个完整的“购物车结算单”。
 */
struct FullOrder {
    static constexpr int MAX_ADDRESS_LENGTH = 50;

    long long order_id;           ///< 订单号
    double total_price = 0.0;     ///< 订单总价
    time_t order_time;            ///< 下单时间
    std::vector<OrderItem> items; ///< 包含的所有商品项
    std::string address;          ///< 地址
    FullOrderStatus status = FullOrderStatus::NOT_COMPLETED; ///< 聚合状态
};

/**
 * @brief 配送费用常量数组
 * 索引对应配送方式：0-普通, 1-快速, 2-特快
 */
constexpr int DELIVERY_PRICES[] = {
    0, // 普通递送
    3, // 快速递送
    6, // 特快递送
};

/**
 * @brief 订单管理类
 *
 * 负责订单数据的持久化、从购物车生成订单、取消订单以及模拟物流时间自动收货。
 * 该类会将分散存储的 OrderItem 聚合成 FullOrder 以供前端 UI 使用。
 */
class OrderManager {

  private:
    using string_view = std::string_view;

    // 内存缓存：聚合后的订单列表 map (Key: order_id, Value: FullOrder)
    std::map<long long, FullOrder> orders_map;

    // 标志位：orders_map 是否已加载
    bool is_loaded = false;

    // 辅助函数：获取当前系统时间戳(time_t)
    time_t get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        return in_time_t;
    }

    // 辅助函数：通用更新函数，根据 order_id 更新特定字段(状态/地址/配送方式)
    void update_order(const long long order_id,
                      std::optional<FullOrderStatus> new_status,
                      std::optional<std::string> new_address,
                      std::optional<int> new_delivery);

    // 辅助函数：根据 order_id 获取所有商品并恢复库存(用于取消订单)
    void update_stock_by_order_id(const long long order_id,
                                  ProductManager product_manager);

    // 配送时间映射 (索引对应配送方式 0, 1, 2)
    // 0: 普通(5天), 1: 普快(3天), 2: 特快(0天)
    const std::vector<int> DELIVERY_DAYS = {5, 3, 1};

  public:
    /**
     * @brief 构造函数
     *
     */
    OrderManager() = default;
    /**
     * @brief 加载并聚合订单
     *
     * 读取数据库文件，将属于该用户的分散 OrderItem 按 order_id 聚合成 FullOrder
     * 对象， 并计算总价。
     *
     * @param user_id 用户 ID
     * @param product_manager 商品管理器（用于查询商品价格以计算总价）
     * @return 无返回值
     */
    void load_full_orders(const int user_id, ProductManager &product_manager);

    /**
     * @brief 获取聚合后的订单映射指针
     *
     * @return std::optional<std::map<long long, FullOrder> *> 若已加载返回 map
     * 指针，否则返回 nullopt
     */
    std::optional<std::map<long long, FullOrder> *> get_orders_map_ptr() {
        if (is_loaded)
            return &orders_map;
        else {
            LOG_CRITICAL("订单列表未加载到内存");
            return std::nullopt;
        }
    }

    /**
     * @brief 创建新订单 (下单)
     *
     * 将购物车中的商品列表转换为订单项写入数据库，并生成唯一的 order_id。
     *
     * @param user_id 用户 ID
     * @param cart_lists 待结算的购物车商品列表
     * @param address 配送地址
     * @return 无返回值
     */
    void add_order(const int user_id, std::vector<CartItem> cart_lists,
                   const std::string address);

    /**
     * @brief 取消订单
     *
     * 将指定 order_id 下的所有 Item 状态置为 CANCEL，并自动调用 ProductManager
     * 恢复商品库存。
     *
     * @param order_id 订单号
     * @param product_manager 商品管理器（用于恢复库存）
     * @return 无返回值
     */
    void cancel_order(const long long order_id,
                      ProductManager &product_manager);

    /**
     * @brief 更新订单信息
     *
     * 修改订单的配送地址或配送方式（仅限未完成的订单）。
     *
     * @param order_id 订单号
     * @param new_address 新地址
     * @param new_delivery_selection 新的配送方式索引
     * @return 无返回值
     */
    void update_order_info(const long long order_id,
                           const std::string &new_address,
                           const int new_delivery_selection);

    /**
     * @brief 检查并更新已送达订单
     *
     * 根据下单时间、配送方式和当前系统时间，判断订单是否应当“已送达”。
     * 若已送达，自动更新数据库状态为 COMPLETED。
     *
     * @param user_id 用户 ID
     * @return 无返回值
     */
    void check_and_update_arrived_orders(int user_id);

    /**
     * @brief 析构函数
     *
     */
    ~OrderManager() {}
};

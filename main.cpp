#ifdef _WIN32
system("chcp 65001");
#endif

#include "ShopAppUI.h"
#include <string>
#include <string_view>
#include <vector>

using std::string;
using std::string_view;
using std::vector;
int main() {

    string user_db_filename = "UserInfoDatabase.dat";
    string product_db_filename = "ProductInfoDatabase.dat";
    string cartItem_db_filename = "CartInfoDatabase.dat";
    string orderItem_db_filename = "OrderInfoDatabase.dat";
    string historyOrderItem_db_filename = "HistoryOrderInfoDatabase.dat";

    // 管理员
    string username = "lei";
    string password = "1234";
    bool is_admin = true;

    // 用户
    string username1 = "zhang";
    string password1 = "1234";
    bool is_admin1 = false;

    // 模拟的商品信息
    // 商品名称
    vector<string> product_name = {
        // --- 数码电子 (1-10) ---
        "iPhone 15 Pro", "华为 Mate 60 Pro", "MacBook Air M2", "iPad Air 5",
        "索尼 WH-1000XM5 耳机", "任天堂 Switch OLED", "罗技 MX Master 3 鼠标",
        "Keychron 机械键盘", "小米充电宝 20000mAh", "4K 显示器 27寸",

        // --- 家用电器 (11-18) ---
        "戴森高速吹风机", "飞利浦电动牙刷", "美的智能电饭煲", "小米空气净化器",
        "石头扫地机器人", "苏泊尔电热水壶", "松下护眼台灯", "格兰仕微波炉",

        // --- 日用百货 (19-26) ---
        "维达卷纸 (24卷)", "蓝月亮洗衣液 (3kg)", "海飞丝洗发水", "高露洁牙膏",
        "多芬沐浴露", "纯棉洗脸巾", "N95 口罩 (50只)", "垃圾袋 (100只)",

        // --- 食品饮料 (27-36) ---
        "可口可乐 (24罐整箱)", "农夫山泉 (24瓶整箱)", "康师傅红烧牛肉面 (袋)",
        "乐事薯片 (原味)", "费列罗巧克力 (24粒)", "星巴克咖啡豆 (500g)",
        "五常大米 (5kg)", "金龙鱼调和油 (5L)", "蒙牛纯牛奶 (箱)",
        "三只松鼠坚果礼包",

        // --- 服饰鞋包 (37-43) ---
        "优衣库纯棉T恤", "耐克 Air Force 1", "李维斯牛仔裤", "阿迪达斯连帽卫衣",
        "纯棉运动袜 (5双)", "新秀丽双肩背包", "防紫外线遮阳伞",

        // --- 文具书籍 (44-46) ---
        "C++ Primer Plus (第6版)", "Moleskine 笔记本", "百乐中性笔 (10支盒装)",

        // --- 运动与其他 (47-50) ---
        "乐高布加迪跑车", "瑜伽垫 (加厚)", "威尔胜篮球", "渴望猫粮 (5.4kg)"};

    // 商品价格
    vector<double> price = {
        // 数码
        7999.0, 6999.0, 8999.0, 4799.0, 2299.0, 2099.0, 799.0, 468.0, 129.0,
        1999.0,

        // 家电
        2599.0, 399.0, 299.0, 899.0, 3299.0, 89.0, 169.0, 499.0,

        // 日用
        59.9, 49.9, 45.0, 15.5, 39.9, 25.0, 35.0, 12.9,

        // 食品
        48.0, 36.0, 3.5, 7.5, 98.0, 85.0, 69.9, 75.0, 55.0, 128.0,

        // 服饰
        79.0, 799.0, 499.0, 399.0, 29.9, 399.0, 49.0,

        // 书籍
        99.0, 180.0, 45.0,

        // 运动
        2699.0, 69.0, 220.0, 580.0};

    // 商品库存
    vector<int> stock = {// 数码 (库存较少)
                         50, 30, 15, 40, 60, 80, 120, 100, 300, 35,

                         // 家电 (中等库存)
                         25, 150, 200, 50, 20, 400, 180, 80,

                         // 日用 (大量库存)
                         600, 500, 350, 800, 400, 1000, 2000, 1500,

                         // 食品 (大量库存)
                         150, 200, 1200, 800, 100, 90, 120, 80, 250, 60,

                         // 服饰
                         300, 45, 70, 60, 500, 90, 250,

                         // 书籍
                         30, 50, 200,

                         // 运动
                         10, 100, 60, 40};

    AppContext ctx(user_db_filename, product_db_filename, cartItem_db_filename,
                   orderItem_db_filename, historyOrderItem_db_filename);

    ShopAppUI my_app(ctx);

    // // 注释块，方便调试
    // auto hash_password1 = SecurityUtils::hash_password(password1);
    // User user(username1, hash_password1, is_admin1);
    // auto hash_password = SecurityUtils::hash_password(password);
    // User admin(username, hash_password, is_admin);

    // FileErrorCode opt1 = ctx.user_manager.append_user(admin);
    // FileErrorCode opt = ctx.user_manager.append_user(user);
    // if (opt != FileErrorCode::OK || opt1 != FileErrorCode::OK)
    //     return 0;

    // // 循环添加产品
    // for (int i = 0; i < price.size(); i++)
    //     ctx.product_manager.add_product(product_name[i], price[i], stock[i]);

    my_app.run();

    return 0;
}

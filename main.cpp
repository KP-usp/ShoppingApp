#ifdef _WIN32
system("chcp 65001");
#endif

#include "Database.h"
#include "Logger.h"
#include "ShopAppUI.h"
#include <string>
#include <string_view>
#include <vector>

using std::string;
using std::string_view;
using std::vector;
int main() {
    // 配置日志系统
    auto &logger = Logger::get_instance();
    logger.set_level(LogLevel::DEBUG);

    logger.set_log_file("data/shopping_app.log");
    logger.set_console_output(true);
    logger.set_file_output(true);

    LOG_INFO("Shopping App 启动中...");

    if (!Database::connect("localhost", "root", "password", "ShoppingApp")) {
        LOG_ERROR("无法连接到数据库，程序终止。");
        return -1;
    }

    // 插入初始数据
    // 加载用户信息
    Database::prepare(
        "LOAD DATA  LOCAL INFILE 'data/UsersInfo.txt' INTO TABLE users",
        [](sql::PreparedStatement *pstmt) { pstmt->execute(); });

    // 加载商品信息
    Database::prepare(
        "LOAD DATA  LOCAL INFILE 'data/ProductsInfo.txt' INTO TABLE products",
        [](sql::PreparedStatement *pstmt) { pstmt->execute(); });

    string user_db_filename = "UserInfo  Database.dat";
    string product_db_filename = "ProductInfoDatabase.dat";
    string cartItem_db_filename = "CartInfoDatabase.dat";
    string orderItem_db_filename = "OrderInfoDatabase.dat";
    string historyOrderItem_db_filename = "HistoryOrderInfoDatabase.dat";

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

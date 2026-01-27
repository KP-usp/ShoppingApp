#ifdef _WIN32
system("chcp 65001");
#endif

#include "Database.h"
#include "Logger.h"
#include "ShopAppUI.h"
#include <string>
#include <string_view>

using std::string;
using std::string_view;

int main() {
    // 配置日志系统
    auto &logger = Logger::get_instance();
    logger.set_level(LogLevel::DEBUG);

    logger.set_log_file("/home/lei/ShoppingApp/data/shopping_app.log");
    logger.set_console_output(true);
    logger.set_file_output(true);

    LOG_INFO("Shopping App 启动中...");

    DbConfig config;

    const char *db_pass = std::getenv("DB_PASSWORD");
    if (!db_pass) {
        LOG_ERROR("未找到环境变量 DB_PASSWORD");
        return -1;
    }

    config.password = db_pass;
    config.database = "ShoppingApp";

    if (!Database::connect(config)) {
        LOG_ERROR("无法连接到数据库，程序终止。");
        return -1;
    }

    // 初始化商品信息

    // try {

    //     // 获取普通 statement
    //     std::unique_ptr<sql::Statement> stmt(
    //         Database::get_connection()->createStatement());

    //     std::string path2 = "/home/lei/ShoppingApp/data/ProductsInfo.txt";
    //     std::string sql2 =
    //         "LOAD DATA LOCAL INFILE '" + path2 + "' INTO TABLE products";

    //     // 执行加载商品信息
    //     stmt->execute(sql2);
    //     LOG_INFO("商品信息加载成功");

    // } catch (sql::SQLException &e) {
    //     LOG_ERROR("加载数据失败: " + std::string(e.what()));
    // }

    AppContext ctx;

    // // 添加管理员和普通用户
    // std::string hash_password1 = SecurityUtils::hash_password("1234");
    // User temp1("lei", hash_password1, true);
    // std::string hash_password2 = SecurityUtils::hash_password("1234");
    // User temp2("zhang", hash_password2, false);

    // ctx.user_manager.append_user(temp1);
    // ctx.user_manager.append_user(temp2);

    ShopAppUI my_app(ctx);

    my_app.run();

    return 0;
}

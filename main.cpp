#include "SecurityUtils.h"
#include "ShopAppUI.h"
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::string_view;
using std::vector;
int main() {

    string user_db_filename = "UserInfoDatabase.dat";
    string product_db_filename = "ProductInfoDatabase.dat";
    string cartItem_db_filename = "CartInfoDatabase.dat";
    string orderItem_db_filename = "OrderInfoDatabase.dat";

    // 管理员
    string username = "lei";
    string password = "1234";
    bool is_admin = true;

    // 用户
    string username1 = "zhang";
    string password1 = "1234";
    bool is_admin1 = false;

    vector<string> product_name = {"phone", "watch", "computer", "toilet"};
    vector<double> price = {3999, 1599, 5999, 500};
    vector<int> stock = {100, 100, 100, 3};

    AppContext ctx(user_db_filename, product_db_filename, cartItem_db_filename,
                   orderItem_db_filename);

    ShopAppUI my_app(ctx);

    // 注释块，方便调试
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

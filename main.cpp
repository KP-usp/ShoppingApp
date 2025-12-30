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
    string product_db_filename = "ProductInforDatabase.dat";
    string cartItem_db_filename = "CartInforDatabase.dat";
    string orderItem_db_filename = "OrderInfoDatabase.dat";

    // 用户
    string username = "lei";
    string password = "1234";
    string rolename = "user"; // ID 自动生成，为 1

    vector<string> product_name = {"phone", "watch", "computer"};
    vector<double> price = {3999, 1599, 5999};

    AppContext ctx(user_db_filename, product_db_filename, cartItem_db_filename,
                   orderItem_db_filename);

    ShopAppUI my_app(ctx);

    // 注释块，方便调试

    // User current_user(username, password, rolename);
    // FileErrorCode opt = ctx.user_manager.append_user(current_user); //
    // 添加用户 if (opt != FileErrorCode::OK)
    //     return 0;

    // 循环添加产品
    // for (int i = 0; i < 3; i++)
    //     ctx.product_manager.add_product(product_name[i], price[i]);

    my_app.run();

    return 0;
}

#include "AppContext.h"
#include "DashboardPage.h"
#include "InventoryPage.h"
#include "SharedComponent.h"
#include "UserManagePage.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <ftxui/dom/elements.hpp>

class AdminPortal {
  private:
    AppContext &ctx;

    int admin_tab_index = 0; // 管理管理员页面 0-仪表盘 1-用户管理 2-商品管理

    // 管理页面智能指针
    std::shared_ptr<InventoryLayOut> inventory_layout;
    std::shared_ptr<UserManageLayOut> user_manage_layout;

    // 外壳
    Component inventory_container_slot = Container::Vertical({});
    Component user_manage_container_slot = Container::Vertical({});

    Component tab_container; // 页面切换逻辑

    // 定义全部回调函数
    std::function<void()> back_dashboard;           // 回到仪表盘
    std::function<void()> on_inventory_page;        // 前往商品管理
    std::function<void()> on_user_manage_page;      // 前往用户管理
    std::function<void()> refresh_inventory_page;   // 刷新商品管理页
    std::function<void()> refresh_user_manage_page; // 刷新用户管理页
  public:
    explicit AdminPortal(AppContext &context);

    // 获取当前管理员页面
    Component get_component() { return tab_container; }
};

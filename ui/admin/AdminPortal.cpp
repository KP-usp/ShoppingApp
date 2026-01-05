#include "AdminPortal.h"

AdminPortal::AdminPortal(AppContext &context) : ctx(context) {
    // 回调函数定义
    back_dashboard = [&, this] { admin_tab_index = 0; };

    on_user_manage_page = [&, this] { admin_tab_index = 1; };

    on_inventory_page = [&, this] { admin_tab_index = 2; };

    // 初始化子页面(其中仪表盘是不涉及数据修改，这里不套壳)
    auto dashboard_page =
        DashboradLayOut::Create(on_inventory_page, on_user_manage_page);
    inventory_layout = std::make_shared<InventoryLayOut>(ctx, back_dashboard);
    user_manage_layout =
        std::make_shared<UserManageLayOut>(ctx, back_dashboard);

    // 套壳
    inventory_container_slot->Add(inventory_layout->get_component());
    user_manage_container_slot->Add(user_manage_layout->get_component());

    tab_container = ftxui::Container::Tab(
        {dashboard_page, inventory_container_slot, user_manage_container_slot},
        &admin_tab_index);
}

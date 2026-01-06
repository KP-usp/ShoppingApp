#include "AdminPortal.h"

AdminPortal::AdminPortal(AppContext &context) : ctx(context) {
    // 回调函数定义
    back_dashboard = [this] { admin_tab_index = 0; };

    on_user_manage_page = [this] { admin_tab_index = 1; };

    on_inventory_page = [this] { admin_tab_index = 2; };

    refresh_inventory_page = [this] {
        inventory_layout->refresh(ctx, back_dashboard, refresh_inventory_page);
        inventory_container_slot->DetachAllChildren();
        inventory_container_slot->Add(inventory_layout->get_component());
    };

    refresh_user_manage_page = [this] {
        user_manage_layout->refresh(ctx, back_dashboard,
                                    refresh_user_manage_page);
        user_manage_container_slot->DetachAllChildren();
        user_manage_container_slot->Add(user_manage_layout->get_component());
    };
    std::string path = "data/debug.log";

    // 初始化子页面(其中仪表盘是不涉及数据修改，这里不套壳)
    auto dashboard_page =
        DashboradLayOut::Create(on_inventory_page, on_user_manage_page);

    std::ofstream outfile2(path, std::ios_base::app);
    if (outfile2.is_open()) {
        outfile2 << "dashboard成功" << std::endl;
        outfile2.close();
    }

    inventory_layout = std::make_shared<InventoryLayOut>(
        ctx, back_dashboard, refresh_inventory_page);

    std::ofstream outfile(path, std::ios_base::app);
    if (outfile.is_open()) {
        outfile << "inventory 成功" << std::endl;
        outfile.close();
    }

    user_manage_layout = std::make_shared<UserManageLayOut>(
        ctx, back_dashboard, refresh_user_manage_page);
    std::ofstream outfile1(path, std::ios_base::app);
    if (outfile1.is_open()) {
        outfile1 << "usermanage 成功" << std::endl;
        outfile1.close();
    }

    // 套壳
    inventory_container_slot->Add(inventory_layout->get_component());
    user_manage_container_slot->Add(user_manage_layout->get_component());

    tab_container = ftxui::Container::Tab(
        {
            dashboard_page,
            user_manage_container_slot,
            inventory_container_slot,
        },
        &admin_tab_index);
}

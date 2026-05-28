# ShoppingApp — 终端购物商城

C++17 编写的终端购物商城（TUI），支持普通用户和管理员双角色。使用 CMake + vcpkg 构建，MySQL 持久化。

## 技术栈

| 依赖 | 用途 |
|---|---|
| **FTXUI** | 终端 UI 框架 |
| **MySQL Connector C++** (X DevAPI) | 数据库连接与 SQL 执行 |
| **OpenSSL** | PBKDF2-SHA256 密码哈希 |
| **nlohmann-json** | IP 定位 API 响应解析 |
| **cpp-httplib** | HTTP 客户端（IP 地理定位） |

## 项目结构

```
ShoppingApp/
├── CMakeLists.txt          # 顶层构建配置
├── vcpkg.json              # 第三方依赖声明
├── main.cpp                # 程序入口
├── model_utils/            # 密码哈希、工具函数、Result 枚举
├── logger/                 # 单例日志（文件+控制台，线程安全）
├── database/               # MySQL 封装（自动建表、SQL 执行）
├── model/                  # 业务逻辑层
│   ├── UserManager         # 用户注册、登录、CRUD
│   ├── ProductManager      # 商品增删改查、搜索、库存管理
│   ├── CartManager         # 购物车管理、结算
│   ├── OrderManager        # 订单创建、取消、自动收货
│   └── HistoryOrderManager # 历史订单归档、查询
├── ui_utils/               # 全局上下文、IP 定位、时间工具
└── ui/                     # FTXUI 终端页面
    ├── pages/              # 登录/注册/商城/购物车/订单/历史订单
    └── admin/              # 管理员后台（仪表盘/商品/用户管理）
```

## 功能特性

- **用户系统**：注册（格式校验）、登录、密码 PBKDF2 哈希存储
- **商品浏览**：列表查看、名称模糊搜索、按 ID 精确搜索
- **购物车**：添加商品、修改数量、删除、配送方式选择
- **下单结算**：从购物车下单、支付弹窗模拟、自动收货（基于配送时间）
- **订单管理**：查看当前订单、修改地址/配送方式、取消订单（自动恢复库存）
- **历史订单**：已完成/已取消订单归档（商品名和价格为快照）
- **管理员后台**：仪表盘 → 商品管理（CRUD）/ 用户管理（封禁/恢复）
- **IP 定位**：启动时自动获取地理位置填入收货地址
- **导航栏**：实时时钟、角色标识、页面快捷切换

### 页面路由

| 页面 | 索引 | 说明 |
|---|---|---|
| LoginPage | 0 | 用户登录 |
| RegisterPage | 1 | 用户注册 |
| ShopPage | 2 | 商城浏览、搜索、加购 |
| CartPage | 3 | 购物车、结算 |
| OrderPage | 4 | 当前订单 |
| HistoryOrderPage | 5 | 历史订单 |
| AdminPortal | 6 | 管理后台 |

## 数据库

程序启动自动建表（5 张），无需手动初始化：

- `users` — 用户名、密码哈希、角色、状态
- `products` — 商品名、价格、库存、状态
- `carts` — 用户 ID、商品 ID、数量、配送选择
- `orders` — 订单项（支持同一订单多商品）
- `history_orders` — 归档快照（商品名和价格为下单时记录）

## 部署方式

### 前置要求

- C++17 编译器（GCC 9+ / Clang 12+）
- [vcpkg](https://github.com/microsoft/vcpkg) 包管理器
- MySQL 8.0+（需启用 X Plugin，默认端口 33060）

### 安装依赖

```bash
# 确保 vcpkg 已安装并设置环境变量
export VCPKG_ROOT=/path/to/vcpkg
```

### 构建（CMakePresets）

```bash
git clone <repo-url>
cd ShoppingApp

# 确保 X Plugin 已启用
mysql -u root -p -e "INSTALL PLUGIN mysqlx SONAME 'mysqlx.so';"

# 设置数据库密码
export DB_PASSWORD="你的数据库密码"

# 构建
cmake --preset debug
cmake --build build

# 运行
./build/shopping_app
```

### 构建（命令行）

```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)
./shopping_app
```

### 运行

```bash
export DB_PASSWORD="你的数据库密码"
./build/shopping_app
```

按 `q` 退出程序。

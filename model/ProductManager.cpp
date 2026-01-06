#include "ProductManager.h"
#include "FileHeader.h"
#include <fstream>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios_base;
using std::nullopt;
using std::ofstream;
using std::optional;
using std::string;
using std::string_view;

void ProductManager::init_db_file() {
    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);

    if (!infile.is_open() || infile.peek() == ifstream::traits_type::eof()) {
        infile.close();

        // 数据库为空，初始化 id 自增文件头, 这里截断文件没问题
        ofstream outfile(path, ios_base::binary | ios_base::out);
        FileHeader header;
        header.next_id = 1;
        outfile.write(reinterpret_cast<const char *>(&header),
                      sizeof(FileHeader));
        outfile.close();
    }
}

int ProductManager::generate_and_update_id() {

    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::binary | ios_base::in | ios_base::out);
    if (!iofile.is_open()) {
        return -1;
    }

    FileHeader header;

    iofile.seekg(0, ios_base::beg);
    iofile.read(reinterpret_cast<char *>(&header), sizeof(FileHeader));

    int new_id = header.next_id;

    header.next_id++;

    iofile.seekp(0, ios_base::beg);
    iofile.write(reinterpret_cast<const char *>(&header), sizeof(FileHeader));

    iofile.close();
    return new_id;
}

FileErrorCode ProductManager::load_all_product() {
    is_loaded = false;

    product_list.clear();

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, std::ios_base::binary);
    if (!infile.is_open()) {
        cerr << "open" << path << "is failed" << endl;
        return FileErrorCode::OpenFailure;
    }

    Product temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(Product))) {
        product_list.emplace_back(temp);
    }

    infile.close();

    is_loaded = true;

    return FileErrorCode::OK;
}

FileErrorCode ProductManager::add_product(const string_view &product_name,
                                          const double price, const int stock) {
    int product_id = generate_and_update_id();

    if (product_id == -1)
        return FileErrorCode::WriteFailure;

    string path = Utils::get_database_path(m_db_filename);
    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    Product temp(product_name, price, stock, product_id);

    // 写入逻辑
    outfile.write(reinterpret_cast<const char *>(&temp), sizeof(Product));
    if (outfile.fail())
        return FileErrorCode::WriteFailure;

    outfile.flush();
    outfile.close();
    return FileErrorCode::OK;

    return FileErrorCode::WriteFailure;
}

FileErrorCode ProductManager::delete_product(const int product_id) {
    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::in | ios_base::out | ios_base::binary);
    if (!iofile.is_open()) {
        cerr << "open " << path << "is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    optional<std::streampos> target_position = get_product_pos(product_id);
    if (!target_position.has_value()) {
        iofile.close();
        return FileErrorCode::NotFound;
    }

    Product temp;

    iofile.seekg(target_position.value());
    iofile.read(reinterpret_cast<char *>(&temp), sizeof(Product));

    if (temp.status == ProductStatus::NORMAL) {
        temp.status = ProductStatus::DELETED;
    } else {
        iofile.close();
        return FileErrorCode::NotFound;
    }
    iofile.seekp(target_position.value());

    iofile.write(reinterpret_cast<const char *>(&temp), sizeof(Product));

    iofile.close();

    return FileErrorCode::OK;
}

FileErrorCode
ProductManager::update_product(const string_view &new_product_name,
                               const int product_id, const double new_price,
                               const int stock) {
    string path = Utils::get_database_path(m_db_filename);
    ofstream iofile(path, ios_base::binary | ios_base::out | ios_base::in);
    if (!iofile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    optional<std::streampos> target_position = get_product_pos(product_id);

    if (!target_position.has_value()) {
        iofile.close();
        cout << "NotFound" << endl;
        return FileErrorCode::NotFound;
    }

    iofile.seekp(target_position.value());

    if (iofile.fail()) {
        cerr << "Write failed." << endl;
        iofile.close();
        return FileErrorCode::SeekFailure;
    }

    Product temp(new_product_name, new_price, stock, product_id);

    iofile.write(reinterpret_cast<const char *>(&temp), sizeof(Product));

    if (iofile.fail()) {
        cerr << "Write failed." << endl;
        iofile.close();
        return FileErrorCode::WriteFailure;
    }

    iofile.flush();
    iofile.close();

    return FileErrorCode::OK;
}

std::vector<Product>
ProductManager::search_all_product(const std::string &query) {
    std::vector<Product> result;

    // 确保数据已加载
    if (!is_loaded) {
        load_all_product();
    }

    // 如果查询为空，返回所有商品
    if (query.empty()) {
        result = product_list;
        return result;
    }

    // 转换为小写以进行不区分大小写的搜索
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   ::tolower);

    for (const auto &p : product_list) {

        // 检查 ID (将 ID 转字符串匹配)
        std::string id_str = std::to_string(p.product_id);

        // 检查名称 (转小写匹配)
        std::string name_str = std::string(p.product_name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                       ::tolower);

        // 如果 ID 等于查询串或名称包含查询串
        if (id_str == query ||
            name_str.find(lower_query) != std::string::npos) {
            result.push_back(p);
        }
    }
    return result;
}

std::vector<Product>
ProductManager::search_product(const std::string &query_name) {
    std::vector<Product> result;

    // 确保数据已加载
    if (!is_loaded) {
        load_all_product();
    }

    // 如果查询为空，返回所有未删除的商品
    if (query_name.empty()) {
        for (const auto &p : product_list) {
            if (p.status != ProductStatus::DELETED)
                result.emplace_back(p);
        }
        return result;
    }

    // 转换为小写以进行不区分大小写的搜索
    std::string lower_query = query_name;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   ::tolower);

    for (const auto &p : product_list) {
        if (p.status == ProductStatus::DELETED)
            continue;

        // 检查 ID (将 ID 转字符串匹配)
        std::string id_str = std::to_string(p.product_id);

        // 检查名称 (转小写匹配)
        std::string name_str = std::string(p.product_name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                       ::tolower);

        // 如果 ID 等于查询串或名称包含查询串
        if (id_str == query_name ||
            name_str.find(lower_query) != std::string::npos) {
            result.push_back(p);
        }
    }
    return result;
}

FileErrorCode ProductManager::restore_product(const int product_id) {
    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::in | ios_base::out | ios_base::binary);
    if (!iofile.is_open()) {
        cerr << "open " << path << "is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    optional<std::streampos> target_position = get_product_pos(product_id);
    if (!target_position.has_value()) {
        iofile.close();
        return FileErrorCode::NotFound;
    }

    Product temp;

    iofile.seekg(target_position.value());
    iofile.read(reinterpret_cast<char *>(&temp), sizeof(Product));

    if (temp.status == ProductStatus::DELETED) {
        temp.status = ProductStatus::NORMAL;
    } else {
        iofile.close();
        return FileErrorCode::NotFound;
    }
    iofile.seekp(target_position.value());

    iofile.write(reinterpret_cast<const char *>(&temp), sizeof(Product));

    iofile.close();

    return FileErrorCode::OK;
}

std::optional<Product> ProductManager::get_product(const int product_id) {
    string path = Utils::get_database_path(m_db_filename);

    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;

        return nullopt;
    }

    Product temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(Product))) {

        if (temp.product_id == product_id &&
            temp.status != ProductStatus::DELETED) {
            return temp;
        }
    }

    infile.close();

    return nullopt;
}

const std::optional<int>
ProductManager::get_id_by_name(const string_view &product_name) {
    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);

    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return nullopt;
    }

    Product temp;

    while (true) {
        infile.read(reinterpret_cast<char *>(&temp), sizeof(Product));

        if (!infile)
            break;

        if (temp.product_name == product_name) {
            return temp.product_id;
        }
    }

    infile.close();

    return nullopt;
}

optional<std::streampos> ProductManager::get_product_pos(const int product_id) {
    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return nullopt;
    }

    Product temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (true) {
        std::streampos current_pos = infile.tellg();

        infile.read(reinterpret_cast<char *>(&temp), sizeof(Product));

        if (!infile)
            break;

        if (temp.product_id == product_id) {
            return current_pos;
        }
    }

    infile.close();

    return nullopt;
}

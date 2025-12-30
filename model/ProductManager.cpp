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

FileErrorCode ProductManager::load_product() {

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
                                          const int price) {
    int product_id = generate_and_update_id();

    if (product_id == -1)
        return FileErrorCode::WriteFailure;

    string path = Utils::get_database_path(m_db_filename);
    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    Product temp(product_name, price, product_id);

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
        mark_deleted(temp);
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
                               const int product_id, const int new_price) {
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

    Product temp(new_product_name, product_id, new_price);

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

std::optional<Product> ProductManager::get_product(const int product_id) {
    string path = Utils::get_database_path(m_db_filename);

    std::string path1 = "data/debug.log";

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
        return nullopt; // 表示
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

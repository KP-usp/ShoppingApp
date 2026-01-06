#include "UserManager.h"
#include "FileHeader.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios_base;
using std::nullopt;
using std::ofstream;
using std::optional;
using std::string;

void UserManager::init_db_file() {
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

int UserManager::generate_and_update_id() {

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

Result UserManager::check_login(const string &username,
                                const string &input_password) {
    // 这里 user  的状态已经是正常的
    optional<User> user_opt = get_user_by_name(username);

    if (user_opt.has_value()) {
        User user = user_opt.value();
        if (check_password(input_password, string(user.password)) ==
            Result::SUCCESS) {
            active_user =
                std::make_shared<User>(user.username, user.password,
                                       user.is_admin, user.id, user.status);
            return Result::SUCCESS;
        }
    }

    return Result::FAILURE;
}

Result UserManager::is_valid_username_format(const string &username,
                                             string &error_message) {
    if (username.empty()) {
        error_message = "用户名不能为空";
        return Result::FAILURE;
    } else if (username.size() > User::MAX_USERNAME_SIZE) {
        error_message = "用户名过长";
        return Result::FAILURE;
    } else if (username.length() < User::MIN_USERNAME_SIZE) {
        error_message = "用户名过短";
        return Result::FAILURE;
    } else {
        error_message = "";
        return Result::SUCCESS;
    }
}

Result UserManager::is_valid_password_format(const string &password,
                                             string &error_message) {

    if (password.empty()) {
        error_message = "密码不能为空";
        return Result::FAILURE;
    } else if (password.size() > User::MAX_PASSWORD_SIZE) {
        error_message = "密码过长";
        return Result::FAILURE;
    } else if (password.length() < User::MIN_PASSWORD_SIZE) {
        error_message = "密码过短";
        return Result::FAILURE;
    } else
        return Result::SUCCESS;
}

Result UserManager::check_register(const string &username,
                                   const string &password,
                                   const string &again_password,
                                   string &error_message) {
    if (is_valid_username_format(username, error_message) == Result::FAILURE ||
        is_valid_password_format(password, error_message) == Result::FAILURE)
        return Result::FAILURE;
    if (get_user_by_name(username).has_value()) {
        error_message = "用户名已存在";
        return Result::FAILURE;
    }

    if (password != again_password) {
        error_message = "两次输入的密码不一致";
        return Result::FAILURE;
    }

    string hash_password = SecurityUtils::hash_password(password);

    // 添加加密密码的用户
    User temp(username, hash_password, false);
    append_user(temp);

    return Result::SUCCESS;
}

FileErrorCode UserManager::append_user(const User &new_user) {
    // 生成新的 ID
    int new_id = generate_and_update_id();
    if (new_id == -1)
        return FileErrorCode::OpenFailure;

    User user_to_save = new_user;
    user_to_save.id = new_id;

    string path = Utils::get_database_path(m_db_filename);

    std::string path1 = "data/debug.log";

    std::ofstream outfile1(path1, std::ios_base::app);
    if (outfile1.is_open()) {
        outfile1 << "用户 ID：" << user_to_save.id << std::endl;
        outfile1.close();
    }

    //  将用户写入文件末尾
    ofstream outfile(path, ios_base::binary | ios_base::app);
    if (!outfile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    outfile.write(reinterpret_cast<const char *>(&user_to_save), sizeof(User));

    if (outfile.fail())
        return FileErrorCode::WriteFailure;

    outfile.flush();
    outfile.close();
    return FileErrorCode::OK;
}

FileErrorCode UserManager::update_user(const User &new_user) {

    string path = Utils::get_database_path(m_db_filename);
    ofstream iofile(path, ios_base::binary | ios_base::out | ios_base::in);
    if (!iofile.is_open()) {
        cerr << "open " << path << " is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    optional<streampos> target_position = get_user_pos(new_user.id);

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

    iofile.write(reinterpret_cast<const char *>(&new_user), sizeof(User));

    if (iofile.fail()) {
        cerr << "Write failed." << endl;
        iofile.close();
        return FileErrorCode::WriteFailure;
    }

    iofile.flush();
    iofile.close();

    return FileErrorCode::OK;
}

optional<User> UserManager::get_user_by_name(const string_view &username) {

    string path = Utils::get_database_path(m_db_filename);

    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return nullopt;
    }

    User temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(User))) {
        string_view found_username = temp.username;
        if (username == found_username && temp.status == UserStatus::NORMAL) {
            return temp;
        }
    }

    infile.close();

    return nullopt;
}

std::optional<User> UserManager::get_user_by_id(const int user_id) {
    string path = Utils::get_database_path(m_db_filename);

    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return nullopt;
    }

    User temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(User))) {

        if (user_id == temp.id && temp.status == UserStatus::NORMAL) {
            return temp;
        }
    }

    infile.close();

    return nullopt;
}

int UserManager::search_user(const string_view &keyword) {
    // 支持模糊查找

    // 匹配个数
    int count = 0;

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << " is failed." << endl;
        return count;
    }

    User temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(User))) {

        string_view current_username = temp.username;
        if (current_username.size() >= keyword.size() &&
            current_username.compare(0, keyword.size(), keyword) == 0 &&
            temp.status == UserStatus::NORMAL) {
            count++;
        }
    }

    infile.close();

    return count;
}

std::vector<User> UserManager::search_users_list(const string &query) {
    std::vector<User> result;

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {
        return result;
    }

    User temp;

    string low_query = query;

    std::transform(low_query.begin(), low_query.end(), low_query.begin(),
                   ::tolower);
    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(User))) {

        if (query.empty()) {
            result.push_back(temp);
            continue;
        }

        string user_id_str = std::to_string(temp.id);
        string username_str = string(temp.username);

        if (user_id_str == query || username_str.find(query))
            result.push_back(temp);
    }

    infile.close();

    return result;
}

FileErrorCode UserManager::delete_user(const int id) {

    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::in | ios_base::out | ios_base::binary);
    if (!iofile.is_open()) {
        cerr << "open " << path << "is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    User temp;

    optional<streampos> target_position = get_user_pos(id);
    if (!target_position.has_value()) {
        iofile.close();
        return FileErrorCode::NotFound;
    }

    iofile.seekg(target_position.value());
    iofile.read(reinterpret_cast<char *>(&temp), sizeof(User));

    if (temp.status == UserStatus::NORMAL) {
        temp.status = UserStatus::DELETED;
    } else {
        iofile.close();
        return FileErrorCode::NotFound;
    }
    iofile.seekp(target_position.value());

    iofile.write(reinterpret_cast<const char *>(&temp), sizeof(User));

    iofile.close();

    return FileErrorCode::OK;
}

FileErrorCode UserManager::restore_user(const int id) {
    string path = Utils::get_database_path(m_db_filename);
    std::fstream iofile(path, ios_base::in | ios_base::out | ios_base::binary);
    if (!iofile.is_open()) {
        cerr << "open " << path << "is failed." << endl;
        return FileErrorCode::OpenFailure;
    }

    optional<streampos> target_position = get_user_pos(id);
    if (!target_position.has_value()) {
        iofile.close();
        return FileErrorCode::NotFound;
    }

    User temp;
    iofile.seekg(target_position.value());
    iofile.read(reinterpret_cast<char *>(&temp), sizeof(User));

    // 只有状态为 DELETED 才恢复
    if (temp.status == UserStatus::DELETED) {
        temp.status = UserStatus::NORMAL;
    } else {
        iofile.close();
        return FileErrorCode::NotFound;
    }

    iofile.seekp(target_position.value());
    iofile.write(reinterpret_cast<const char *>(&temp), sizeof(User));
    iofile.close();

    return FileErrorCode::OK;
}

optional<std::streampos> UserManager::get_user_pos(const int id) {

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);
    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return nullopt;
    }

    User temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    streampos record_start_pos = infile.tellg();

    while (infile.read(reinterpret_cast<char *>(&temp), sizeof(User))) {

        if (temp.id == id) {
            return record_start_pos;
        }

        record_start_pos = infile.tellg();
    }

    infile.close();

    return nullopt;
}

int UserManager::get_id_by_username(const string_view &username) {

    int error = -1;

    string path = Utils::get_database_path(m_db_filename);
    ifstream infile(path, ios_base::binary);

    if (!infile.is_open()) {

        cerr << "open " << path << "is failed." << endl;
        return error;
    }

    User temp;

    infile.seekg(sizeof(FileHeader), ios_base::beg);

    while (true) {
        streampos current_pos = infile.tellg();

        infile.read(reinterpret_cast<char *>(&temp), sizeof(User));

        if (!infile)
            break;

        string_view found_username = temp.username;
        if (username == found_username) {
            return temp.id;
        }
    }

    infile.close();

    return error;
}

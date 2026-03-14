#pragma once

#include <memory>
#include <string>
#include <optional>
#include <vector>

#include <pqxx/pqxx>

#include "../database/postgre_pool.hpp"

namespace bre {

struct User {
    int id;
    std::string username;
    std::string email;
    std::string created_at;
};

class UserService {
public:
    static UserService& Instance();

    bool Register(const std::string& username, const std::string& password, const std::string& email = "");
    std::optional<User> Login(const std::string& username, const std::string& password);
    std::optional<User> GetUserById(int id);
    std::optional<User> GetUserByUsername(const std::string& username);
    std::vector<User> GetAllUsers(int limit = 100, int offset = 0);
    bool DeleteUser(int id);

private:
    UserService() = default;
    
    std::string _hashPassword(const std::string& password);
    bool _verifyPassword(const std::string& password, const std::string& hash);
};

} 

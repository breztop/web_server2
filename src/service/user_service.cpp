#include "user_service.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

namespace bre {

UserService& UserService::Instance() {
    static UserService instance;
    return instance;
}

std::string UserService::_hashPassword(const std::string& password) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    if (EVP_DigestUpdate(ctx, password.c_str(), password.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return oss.str();
}

bool UserService::_verifyPassword(const std::string& password, const std::string& hash) {
    std::string hashed = _hashPassword(password);
    return hashed == hash;
}

bool UserService::Register(const std::string& username, const std::string& password, const std::string& email) {
    if (username.empty() || password.empty()) {
        return false;
    }

    auto pool = PostgrePool::Instance();
    if (!pool || !pool->IsInitialized()) {
        std::cerr << "Database pool not initialized" << std::endl;
        return false;
    }

    try {
        auto conn = pool->Acquire();
        pqxx::work w(*conn);

        std::string hashedPassword = _hashPassword(password);

        w.exec("INSERT INTO users (username, password, email) VALUES (" +
               w.quote(username) + ", " + w.quote(hashedPassword) + ", " + w.quote(email) + ")");
        
        w.commit();
        return true;
    } catch (const pqxx::unique_violation& e) {
        std::cerr << "Username already exists: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Registration error: " << e.what() << std::endl;
        return false;
    }
}

std::optional<User> UserService::Login(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        return std::nullopt;
    }

    auto pool = PostgrePool::Instance();
    if (!pool || !pool->IsInitialized()) {
        std::cerr << "Database pool not initialized" << std::endl;
        return std::nullopt;
    }

    try {
        auto conn = pool->Acquire();
        pqxx::work w(*conn);

        pqxx::result r = w.exec("SELECT id, username, password, email, created_at FROM users WHERE username = " + w.quote(username));

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        std::string storedHash = row["password"].as<std::string>();

        if (!_verifyPassword(password, storedHash)) {
            return std::nullopt;
        }

        User user;
        user.id = row["id"].as<int>();
        user.username = row["username"].as<std::string>();
        user.email = row["email"].as<std::string>();
        user.created_at = row["created_at"].as<std::string>();

        return user;
    } catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> UserService::GetUserById(int id) {
    auto pool = PostgrePool::Instance();
    if (!pool || !pool->IsInitialized()) {
        return std::nullopt;
    }

    try {
        auto conn = pool->Acquire();
        pqxx::work w(*conn);

        pqxx::result r = w.exec("SELECT id, username, email, created_at FROM users WHERE id = " + std::to_string(id));

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        User user;
        user.id = row["id"].as<int>();
        user.username = row["username"].as<std::string>();
        user.email = row["email"].as<std::string>();
        user.created_at = row["created_at"].as<std::string>();

        return user;
    } catch (const std::exception& e) {
        std::cerr << "GetUserById error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> UserService::GetUserByUsername(const std::string& username) {
    auto pool = PostgrePool::Instance();
    if (!pool || !pool->IsInitialized()) {
        return std::nullopt;
    }

    try {
        auto conn = pool->Acquire();
        pqxx::work w(*conn);

        pqxx::result r = w.exec("SELECT id, username, email, created_at FROM users WHERE username = " + w.quote(username));

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        User user;
        user.id = row["id"].as<int>();
        user.username = row["username"].as<std::string>();
        user.email = row["email"].as<std::string>();
        user.created_at = row["created_at"].as<std::string>();

        return user;
    } catch (const std::exception& e) {
        std::cerr << "GetUserByUsername error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<User> UserService::GetAllUsers(int limit, int offset) {
    std::vector<User> users;

    auto pool = PostgrePool::Instance();
    if (!pool || !pool->IsInitialized()) {
        return users;
    }

    try {
        auto conn = pool->Acquire();
        pqxx::work w(*conn);

        pqxx::result r = w.exec("SELECT id, username, email, created_at FROM users ORDER BY id LIMIT " + 
                               std::to_string(limit) + " OFFSET " + std::to_string(offset));

        for (const auto& row : r) {
            User user;
            user.id = row["id"].as<int>();
            user.username = row["username"].as<std::string>();
            user.email = row["email"].as<std::string>();
            user.created_at = row["created_at"].as<std::string>();
            users.push_back(user);
        }
    } catch (const std::exception& e) {
        std::cerr << "GetAllUsers error: " << e.what() << std::endl;
    }

    return users;
}

bool UserService::DeleteUser(int id) {
    auto pool = PostgrePool::Instance();
    if (!pool || !pool->IsInitialized()) {
        return false;
    }

    try {
        auto conn = pool->Acquire();
        pqxx::work w(*conn);

        w.exec("DELETE FROM users WHERE id = " + std::to_string(id));
        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "DeleteUser error: " << e.what() << std::endl;
        return false;
    }
}

} 

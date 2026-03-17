#include "Session.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>

#include "breutil/json.hpp"

#ifdef HAVE_POSTGRESQL
#include "service/user_service.hpp"
#endif  // HAVE_POSTGRESQL

namespace bre {

std::atomic<uint64_t> Session::_sessionCounter{0};

Session::Session(boost::asio::ip::tcp::socket socket, const std::string& resourceDir)
    : _socket(std::move(socket))
    , _timer(_socket.get_executor())
    , _readBuffer(4096)
    , _resourceDir(resourceDir)
    , _sessionId(++_sessionCounter)
    , _timeoutDuration(30) // 30秒超时
    , _keepAlive(false)
    , _closed(false) {
    std::cout << "[Session " << _sessionId << "] Created from " << GetRemoteAddress() << std::endl;
}

Session::~Session() { std::cout << "[Session " << _sessionId << "] Destroyed" << std::endl; }

void Session::Start() {
    StartTimer();
    DoRead();
}

void Session::Stop() {
    if (_closed) {
        return;
    }

    _closed = true;
    CancelTimer();

    boost::system::error_code ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    _socket.close(ec);

    std::cout << "[Session " << _sessionId << "] Stopped" << std::endl;
}

std::string Session::GetRemoteAddress() const {
    try {
        auto endpoint = _socket.remote_endpoint();
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    } catch (...) {
        return "unknown";
    }
}

void Session::DoRead() {
    auto self = shared_from_this();

    // 确保有足够的写入空间
    _readBuffer.EnsureWritableBytes(4096);

    _socket.async_read_some(
        boost::asio::buffer(_readBuffer.BeginWrite(), _readBuffer.WritableBytes()),
        [this, self](boost::system::error_code ec, std::size_t bytesTransferred) {
            if (ec) {
                if (ec != boost::asio::error::operation_aborted) {
                    std::cout << "[Session " << _sessionId << "] Read error: " << ec.message()
                              << std::endl;
                }
                Stop();
                return;
            }

            // 更新读索引
            _readBuffer.HasWritten(bytesTransferred);
            ResetTimer();

            // 解析HTTP请求
            if (_request.Parse(_readBuffer)) {
                // 解析完成，处理请求
                std::cout << "[Session " << _sessionId
                          << "] Request: " << _request.GetMethodString() << " "
                          << _request.GetPath() << std::endl;

                HandleRequest();
            } else {
                // 需要更多数据，继续读取
                DoRead();
            }
        });
}

void Session::DoWrite() {
    auto self = shared_from_this();

    boost::asio::async_write(_socket,
                             boost::asio::buffer(_writeBuffer.Peek(), _writeBuffer.ReadableBytes()),
                             [this, self](boost::system::error_code ec, std::size_t) {
                                 if (ec) {
                                     std::cout << "[Session " << _sessionId
                                               << "] Write error: " << ec.message() << std::endl;
                                     Stop();
                                     return;
                                 }

                                 _writeBuffer.RetrieveAll();

                                 // 检查是否Keep-Alive
                                 if (_keepAlive && !_closed) {
                                     // 重置请求对象，继续接收下一个请求
                                     _request.Reset();
                                     DoRead();
                                 } else {
                                      // 关闭连接
                                      Stop();
                                  }
                              });
}

std::string Session::_extractParam(const std::string& key) {
    std::string body = _request.GetBody();
    std::string searchKey = key + "=";
    size_t pos = body.find(searchKey);
    if (pos == std::string::npos) {
        return "";
    }
    pos += searchKey.size();
    size_t end = body.find("&", pos);
    if (end == std::string::npos) {
        end = body.size();
    }
    std::string value = body.substr(pos, end - pos);
    std::replace(value.begin(), value.end(), '+', ' ');
    return value;
}

void Session::HandleApiLogin(HttpResponse& response) {
#ifdef HAVE_POSTGRESQL
    std::string username = _extractParam("username");
    std::string password = _extractParam("password");

    if (username.empty() || password.empty()) {
        bre::json::Value json;
        json["success"] = false;
        json["error"] = "Missing username or password";

        response = HttpResponse::MakeJsonResponse(json.ToString());
        response.SetStatus(HttpStatus::BAD_REQUEST);
        return;
    }

    auto user = bre::UserService::Instance().Login(username, password);
    if (user) {
        bre::json::Value json;
        json["success"] = true;
        json["user"]["id"] = user->id;
        json["user"]["username"] = user->username;
        json["user"]["email"] = user->email;

        response = HttpResponse::MakeJsonResponse(json.ToString());
    } else {
        bre::json::Value json;
        json["success"] = false;
        json["error"] = "Invalid username or password";
        response = HttpResponse::MakeJsonResponse(json.ToString());
        response.SetStatus(HttpStatus::UNAUTHORIZED);
    }
#else
    bre::json::Value json;
    json["success"] = false;
    json["error"] = "PostgreSQL support is not enabled";
    response = HttpResponse::MakeJsonResponse(json.ToString());
    response.SetStatus(HttpStatus::SERVICE_UNAVAILABLE);
#endif
}

void Session::HandleApiRegister(HttpResponse& response) {
#ifdef HAVE_POSTGRESQL
    std::string username = _extractParam("username");
    std::string password = _extractParam("password");
    std::string email = _extractParam("email");

    if (username.empty() || password.empty()) {
        bre::json::Value json;
        json["success"] = false;
        json["error"] = "Missing username or password";

        response = HttpResponse::MakeJsonResponse(json.ToString());
        response.SetStatus(HttpStatus::BAD_REQUEST);
        return;
    }

    bool success = bre::UserService::Instance().Register(username, password, email);
    if (success) {
        bre::json::Value json;
        json["success"] = true;
        json["message"] = "User registered successfully";
        response = HttpResponse::MakeJsonResponse(json.ToString());
    } else {
        bre::json::Value json;
        json["success"] = false;
        json["error"] = "Username already exists";
        response = HttpResponse::MakeJsonResponse(json.ToString());
        response.SetStatus(HttpStatus::CONFLICT);
    }
#else
    bre::json::Value json;
    json["success"] = false;
    json["error"] = "PostgreSQL support is not enabled";
    response = HttpResponse::MakeJsonResponse(json.ToString());
    response.SetStatus(HttpStatus::SERVICE_UNAVAILABLE);
#endif
}

void Session::HandleApiGetUsers(HttpResponse& response) {
#ifdef HAVE_POSTGRESQL
    auto users = bre::UserService::Instance().GetAllUsers(100, 0);
    
    bre::json::Value json;
    json["success"] = true;
    for (const auto& user : users) {
        bre::json::Value userJson;
        userJson["id"] = user.id;
        userJson["username"] = user.username;
        userJson["email"] = user.email;
        json["users"].Append(userJson);
    }

    response = HttpResponse::MakeJsonResponse(json.ToString());
#else
    bre::json::Value json;
    json["success"] = false;
    json["error"] = "PostgreSQL support is not enabled";
    response = HttpResponse::MakeJsonResponse(json.ToString());
    response.SetStatus(HttpStatus::SERVICE_UNAVAILABLE);
#endif  // HAVE_POSTGRESQL
}


void Session::HandleApiGetUser(HttpResponse& response) {
#ifdef HAVE_POSTGRESQL
    std::string idStr = _extractParam("id");
    if (idStr.empty()) {
        bre::json::Value json;
        json["success"] = false;
        json["error"] = "Missing user id";

        response = HttpResponse::MakeJsonResponse(json.ToString());
        response.SetStatus(HttpStatus::BAD_REQUEST);
        return;
    }
    
    int id = std::stoi(idStr);
    auto user = bre::UserService::Instance().GetUserById(id);
    
    if (user) {
        bre::json::Value json;
        json["success"] = true;
        json["user"]["id"] = user->id;
        json["user"]["username"] = user->username;
        json["user"]["email"] = user->email;
        json["user"]["created_at"] = user->created_at;
        response = HttpResponse::MakeJsonResponse(json.ToString());
    } else {
        bre::json::Value json;
        json["success"] = false;
        json["error"] = "User not found";
        response = HttpResponse::MakeJsonResponse(json.ToString());
        response.SetStatus(HttpStatus::NOT_FOUND);
    }
#else
    bre::json::Value json;
    json["success"] = false;
    json["error"] = "PostgreSQL support is not enabled";
    response = HttpResponse::MakeJsonResponse(json.ToString());
    response.SetStatus(HttpStatus::SERVICE_UNAVAILABLE);
#endif  // HAVE_POSTGRESQL
}

void Session::HandleRequest() {
    HttpResponse response;

    // 检查是否Keep-Alive
    std::string connection = _request.GetHeader("Connection");
    _keepAlive = (connection == "keep-alive" || connection == "Keep-Alive");
    response.SetKeepAlive(_keepAlive);

    // 获取请求路径
    std::string path = _request.GetPath();
    if (path == "/") {
        path = "/index.html";
    }

    // 简单的路由处理
    if (path.find("/api/") == 0) {
        if (path == "/api/health") {
            response = HttpResponse::MakeJsonResponse(R"({"status":"ok","version":"2.0"})");
        } else if (path == "/api/info") {
            std::string json = R"({"server":"Bre WebServer","version":"2.0","session":")" +
                               std::to_string(_sessionId) + R"("})";
            response = HttpResponse::MakeJsonResponse(json);
        } else if (path == "/api/login" && _request.GetMethod() == HttpMethod::POST) {
            HandleApiLogin(response);
        } else if (path == "/api/register" && _request.GetMethod() == HttpMethod::POST) {
            HandleApiRegister(response);
        } else if (path == "/api/users" && _request.GetMethod() == HttpMethod::GET) {
            HandleApiGetUsers(response);
        } else if (path == "/api/user" && _request.GetMethod() == HttpMethod::GET) {
            HandleApiGetUser(response);
        } else {
            response =
                HttpResponse::MakeErrorResponse(HttpStatus::NOT_FOUND, "API endpoint not found");
        }
    } else {
        // 静态文件服务
        std::filesystem::path filePath = std::filesystem::path(_resourceDir) / path.substr(1);

        // 安全检查：防止目录遍历攻击
        std::filesystem::path canonicalResourceDir = std::filesystem::canonical(_resourceDir);
        std::filesystem::path canonicalFilePath;

        try {
            canonicalFilePath = std::filesystem::canonical(filePath);
        } catch (...) {
            response = HttpResponse::MakeErrorResponse(HttpStatus::NOT_FOUND);
            response.Build(_writeBuffer);
            DoWrite();
            return;
        }

        // 检查文件路径是否在资源目录内
        auto mismatch_pair = std::mismatch(canonicalResourceDir.begin(), canonicalResourceDir.end(),
                                           canonicalFilePath.begin(), canonicalFilePath.end());

        if (mismatch_pair.first != canonicalResourceDir.end()) {
            // 路径不在资源目录内，拒绝访问
            response = HttpResponse::MakeErrorResponse(HttpStatus::FORBIDDEN);
        } else if (std::filesystem::exists(canonicalFilePath) &&
                   std::filesystem::is_regular_file(canonicalFilePath)) {
            // 文件存在，加载并发送
            if (response.LoadFile(canonicalFilePath)) {
                response.SetStatus(HttpStatus::OK);
            } else {
                response = HttpResponse::MakeErrorResponse(HttpStatus::INTERNAL_SERVER_ERROR);
            }
        } else {
            // 文件不存在
            response = HttpResponse::MakeErrorResponse(HttpStatus::NOT_FOUND);
        }
    }

    // 构建响应并发送
    response.SetKeepAlive(_keepAlive);
    response.Build(_writeBuffer);
    DoWrite();
}

void Session::HandleTimeout() {
    if (_closed) {
        return;
    }

    std::cout << "[Session " << _sessionId << "] Timeout" << std::endl;
    Stop();
}

void Session::StartTimer() {
    _timer.expires_after(_timeoutDuration);

    auto self = shared_from_this();
    _timer.async_wait([this, self](boost::system::error_code ec) {
        if (!ec) {
            HandleTimeout();
        }
    });
}

void Session::ResetTimer() { _timer.expires_after(_timeoutDuration); }

void Session::CancelTimer() {
    boost::system::error_code ec;
    _timer.cancel(ec);
}

}  // namespace bre

# 第3章：HTTP 协议解析与响应

本章将讨论如何将原始的 TCP 字节流转化为结构化的 HTTP 对象，并根据业务逻辑生成符合标准的响应报文。

## 1. 有限状态机（FSM）解析 HTTP 请求

HTTP 是基于文本的协议，请求通常由“请求行”、“头部（Headers）”和“主体（Body）”组成。为了高效解析，本项目采用了 **ParseState** 枚举来维护当前解析的状态。

```cpp
enum class ParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH
};
```

### 解析逻辑：
* **REQUEST_LINE**: 寻找第一个 `\r\n`，提取方法（GET/POST）、路径和版本。
* **HEADERS**: 循环读取每一行直到遇到空行，提取键值对并存储在 `std::unordered_map` 中。
* **BODY**: 根据 `Content-Length` 头部信息读取指定长度的字节。

这种解析方式能够处理分片到达的数据（即一次 `read` 可能只收到了请求的一半），解析器会记录当前状态并在下次数据到达时继续。

## 2. 灵活的 HTTP 响应生成器

`HttpResponse` 类负责构建发往客户端的报文。它封装了：
* **状态码**: 200 (OK), 404 (Not Found), 500 (Internal Error) 等。
* **头信息**: 自动填充 `Content-Type`, `Content-Length`, `Server`, `Date` 等。
* **Body**: 无论是内存中的字符串还是从磁盘读取的文件流。

## 3. 静态资源与简单的 API 路由

服务器收到请求后，会进入 `HandleRequest` 逻辑。

### 静态资源映射：
如果请求路径不以 `/api/` 开头，服务器会尝试在磁盘上查找文件：
1. **安全检查**: 使用 `std::filesystem::canonical` 防止目录遍历攻击（Path Traversal）。
2. **文件类型判断**: 根据后缀名设置 `Content-Type`。
3. **高效加载**: 调用 `response.LoadFile(filePath)` 将文件内容读入内存并构建 HTTP 报文。

```cpp
// Session.cpp 中的逻辑片段
std::filesystem::path filePath = std::filesystem::path(_resourceDir) / path.substr(1);
if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
    response.LoadFile(filePath);
} else {
    response = HttpResponse::MakeErrorResponse(HttpStatus::NOT_FOUND);
}
```

## 4. 连接策略 (Keep-Alive)

在 HTTP/1.1 中，默认开启 `Keep-Alive` 以复用 TCP 连接。
* 如果请求中带有 `Connection: keep-alive`，服务器在发送完响应后不会关闭 Socket，而是重新调用 `DoRead()`。
* 这极大地减少了频繁建立/销毁 TCP 连接带来的开销，显著提升了页面加载速度。

# winsock2-cpp

一个使用RAII对Winsock2进行简易封装的C++单头文件库，让Windows下的网络编程更便捷安全。

## 特性

- **头文件库（Header-Only）**：只需包含一个头文件即可使用
- **RAII支持**：自动管理Winsock初始化/清理和Socket资源
- **异常安全**：所有Winsock错误都转换为C++异常
- **简洁API**：提供面向对象的现代C++接口
- **支持TCP客户端和服务器**：支持连接、绑定、监听和接受连接

## 快速开始

### 1. 包含库文件

在你的项目中创建一个源文件（例如`main.cpp`），并按以下方式包含头文件：

```cpp
// 在包含头文件之前定义此宏，用于插入实现
#define WINSOCK2_CPP_IMPLEMENTATION
#include "winsock2-cpp.hpp"

// 或者如果你的项目中只需要一处实现：
#include "winsock2-cpp.hpp"  // 普通头文件
// 在某个.cpp文件中：
#define WINSOCK2_CPP_IMPLEMENTATION
#include "winsock2-cpp.hpp"
```

### 2. 客户端示例

```cpp
#include <iostream>

#define WINSOCK2_CPP_IMPLEMENTATION
#include "winsock2-cpp.hpp"

int main() {
    try {
        // 初始化Winsock2（必须在所有网络操作前调用）
        winsock2::WSAData::Initialize();
      
        // 创建TCP客户端Socket
        winsock2::Socket client(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      
        // 连接到服务器
        client.Connect("127.0.0.1", 8080);
      
        // 发送数据
        client.Send("Hello Server!");
      
        // 接收数据（阻塞模式）
        std::string response;
        if (client.Recv(true, response)) {
            std::cout << "Received: " << response << std::endl;
        }
      
        // 关闭发送通道
        client.Shutdown();
      
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
  
    return 0;
}
```

### 3. 服务器示例

```cpp
#include <iostream>
#include <thread>

#define WINSOCK2_CPP_IMPLEMENTATION
#include "winsock2-cpp.hpp"

void handle_client(winsock2::Socket client) {
    std::string request;
    if (client.Recv(true, request)) {
        std::cout << "Client says: " << request << std::endl;
        client.Send("Hello from server!");
    }
    // client析构时会自动关闭socket
}

int main() {
    try {
        // 初始化Winsock2
        winsock2::WSAData::Initialize();
      
        // 创建TCP服务器Socket
        winsock2::Socket server(AF_INET, SOCK_STREAM, IPPROTO_TCP, AI_PASSIVE);
      
        // 绑定端口并监听
        server.Bind(8080).Listen(5);
      
        std::cout << "Server listening on port 8080..." << std::endl;
      
        while (true) {
            // 接受客户端连接
            auto client = server.Accept();
          
            // 在新线程中处理客户端
            std::thread(handle_client, std::move(client)).detach();
        }
      
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
  
    return 0;
}
```

## API参考

### 初始化

```cpp
// 初始化Winsock2库（必须最先调用）
winsock2::WSAData::Initialize();
```

### Socket类

#### 构造函数
```cpp
// 创建Socket
Socket(int ai_family,        // AF_INET 或 AF_INET6
       int ai_socktype,      // SOCK_STREAM 或 SOCK_DGRAM
       int ai_protocol,      // IPPROTO_TCP 或 IPPROTO_UDP
       int ai_flags = 0);    // 附加标志
```

#### 客户端操作

```cpp
// 连接到服务器
Socket& Connect(const std::string& addr, std::uint16_t port);

// 发送数据
int Send(const std::string& data);  // 返回发送的字节数

// 接收数据
bool Recv(bool is_blocked, std::string& output);
```

#### 服务器操作

```cpp
// 绑定到本地端口
Socket& Bind(std::uint16_t port);

// 开始监听连接
Socket& Listen(int max_count);  // max_count: 最大等待连接数

// 接受客户端连接
Socket Accept();

// 关闭发送通道
void Shutdown();
```

### 异常类型

库定义了以下异常类型（均继承自`std::runtime_error`）：

- `WSAStartupFailedError` - Winsock初始化失败
- `InvalidSocketError` - Socket无效
- `GetaddrinfoFailedError` - 地址解析失败
- `SocketCreatingFailedError` - Socket创建失败
- `SocketConnectingFailedError` - 连接失败
- `SocketSendingFailedError` - 发送数据失败
- `SocketRecvFailedError` - 接收数据失败
- `SocketBindingFailedError` - 绑定端口失败
- `SocketListeningFailedError` - 监听失败
- `SocketAcceptingFailedError` - 接受连接失败
- `SocketShutdownFailedError` - 关闭连接失败

## 构建说明

### 要求
- Windows操作系统
- C++20或更高版本（需要`<format>`支持）
- Visual Studio 2019+ 或 MinGW-w64

### 项目设置

**Visual Studio:**
1. 将`winsock2-cpp.hpp`添加到项目
2. 在项目属性中，链接到`ws2_32.lib`：
   - 链接器 → 输入 → 附加依赖项：添加`ws2_32.lib`

**CMake:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(MyNetworkProject)

add_executable(my_app main.cpp)

# 链接Winsock2库
target_link_libraries(my_app ws2_32)
```

**MinGW命令行:**
```bash
g++ -o myapp.exe main.cpp -lws2_32 -std=c++20
```

## 注意事项

1. **单次实现**：`WINSOCK2_CPP_IMPLEMENTATION`宏在整个项目中只能定义一次
2. **异常处理**：所有网络操作都可能抛出异常，建议使用try-catch块
3. **线程安全**：Socket对象不是线程安全的，每个线程应使用自己的Socket实例
4. **资源管理**：Socket使用移动语义，移走后原对象不再拥有socket资源

## License

此代码库遵循MIT许可证。详见文件中的版权声明。

## 作者

pzjw0066 - 初始创建和维护

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <codecvt>

#pragma comment(lib, "ws2_32.lib")

class 网络服务器类
{
private:
    SOCKET 监听Socket = INVALID_SOCKET;
    class 任务栏窗口类* 任务栏窗口 = nullptr;
    std::thread* 网络服务器_线程 = nullptr;
    bool 运行中 = false;

private:
    std::wstring_convert<std::codecvt_utf8<wchar_t>> 字符转换;

public:
    网络服务器类(class 任务栏窗口类*, unsigned short);
    ~网络服务器类();

private:
    void 监听线程函数(unsigned short 端口);
    void 处理连接(SOCKET 客户端Socket);
    
    // 业务处理函数
    void 字体(const std::string& body);
    void 颜色(const std::string& body);
    void 样式(const std::string& body);
    void 大小(const std::string& body);
    void 歌词(const std::string& body);
    void 对齐(const std::string& body);
    void 位置(const std::string& body);
    void 边距(const std::string& body);
    void 屏幕(const std::string& body);
    void 关闭(const std::string& body);

    // 简易 JSON 解析辅助函数
    std::string JsonGetString(const std::string& json, const std::string& key);
    int JsonGetInt(const std::string& json, const std::string& key);
    float JsonGetFloat(const std::string& json, const std::string& key);
    bool JsonGetBool(const std::string& json, const std::string& key);
    unsigned int JsonGetHex(const std::string& json, const std::string& key);
};

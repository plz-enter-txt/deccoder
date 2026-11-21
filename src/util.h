// ===================== src/util.h =====================
#pragma once
#include <string>
#include <filesystem>
#include <chrono>

namespace dec {
    // 全局语言标志：true 输出中文，false 输出英文。
    // 由 main.cpp 在启动时根据 $LANG / $LC_ALL 设置，decfile.cpp 等模块共享。
    extern bool gUseZh;

    // 双语文本选择，供不包含 main.cpp 的模块使用
    inline const char* tr(const char* zh, const char* en) { return gUseZh ? zh : en; }

    std::string nowIso();
    std::string fileTimeIso(const std::filesystem::path& p);
    std::string publicIp(); // 注意这里的结尾分号
} // namespace dec

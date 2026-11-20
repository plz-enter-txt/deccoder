#pragma once
#include "crypto.h"
#include "json.h"
#include <filesystem>
#include <string>
#include <functional>

namespace dec {

    inline constexpr uint32_t kFormatVersion = 2;
    inline constexpr size_t   kMacLen = 32;

    inline constexpr uint8_t kFlagHeaderEncrypted = 0x01;
    inline constexpr uint8_t kFlagHasPassword     = 0x02;

    enum class DecError {
        Ok = 0,
        NotDecFile,
        VersionTooNew,
        NeedPassword,
        WrongPassword,
        BadHeader,
        IoError,
        IntegrityFailed
    };

    const char* decErrorText(DecError e);

    struct HeaderInfo {
        Json        json;
        Bytes       salt;
        Nonce12     headerNonce{};
        uint32_t    iterations = 0;
        Key32       key{};
        bool        hasPassword = false;
        uint64_t    headerLen = 0;
        uint64_t    bodyOffset = 0;
        uint32_t    version = 1;
    };

    DecError readHeader(const std::filesystem::path& p, const std::string& password, HeaderInfo& out);

    // 进度阶段：一次操作里可能会先后经历多个阶段（如先校验再解密、先打包再加密）
    enum class ProgressPhase {
        Pack,     // 打包目录
        Encrypt,  // 加密正文
        Verify,   // 解密前的 HMAC 校验
        Decrypt,  // 解密正文
        Unpack    // 解包归档
    };

    // 进度回调：(阶段, 已完成字节数, 总字节数)
    using ProgressCb = std::function<void(ProgressPhase phase, uint64_t current, uint64_t total)>;

    bool encryptFile(const std::filesystem::path& src,
                     const std::filesystem::path& dst,
                     const std::string& password,
                     const std::string& remarks,
                     bool is_archive,
                     std::string& err,
                     ProgressCb progressCb = nullptr);

    DecError decryptBody(const std::filesystem::path& src,
                         const HeaderInfo& h,
                         const std::filesystem::path& dst,
                         ProgressCb progressCb = nullptr);

    bool rewriteHeader(const std::filesystem::path& p, const HeaderInfo& h,
                       const Json& newHeader, std::string& err);

    bool packDirectory(const std::filesystem::path& srcDir, const std::filesystem::path& outArchive, std::string& err,
                       ProgressCb progressCb = nullptr);
    bool unpackArchive(const std::filesystem::path& archiveFile, const std::filesystem::path& outDir, std::string& err,
                       ProgressCb progressCb = nullptr);

} // namespace dec

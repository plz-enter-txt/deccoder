#include <iomanip> // 新增，用于 std::setw
// ... 原样保留其他头文件 ...

// 在 main.cpp 中加入打印进度条的函数
#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty(_fileno(stdout))
#else
#define ISATTY isatty(STDOUT_FILENO)
#endif

static void printProgress(const std::string& action, uint64_t current, uint64_t total) {
    if (total == 0) return;
    int percent = (int)((double)current / total * 100.0);
    static int last_percent = -1;

    if (percent == last_percent && current != total) return;
    last_percent = percent;

    if (!ISATTY) return;

    int barWidth = 30;
    int pos = (int)(barWidth * (percent / 100.0));

    std::cout << "\r" << action << " [" << std::setw(3) << percent << "%] [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::flush;

    if (current == total) {
        std::cout << std::endl;
        last_percent = -1;
    }
}

// ---------------- 操作：加密 ----------------
static int doEncrypt(...) {
    // ...
    // 如果是目录，先打包成临时文件
    if (fs::is_directory(path, ec)) {
        // ...
        std::cout << T("正在加密归档...\n", "Encrypting archive...\n");
        bool ok = encryptFile(tmpArchive, dst, password, remarks, true, err, [](uint64_t cur, uint64_t tot){
            printProgress(use_zh ? "正在加密" : "Encrypting", cur, tot);
        });
        // ...
    } else {
        // 普通文件
        std::string err;
        if (encryptFile(path, dst, password, remarks, false, err, [](uint64_t cur, uint64_t tot){
            printProgress(use_zh ? "正在加密" : "Encrypting", cur, tot);
        })) {
            // ...
        }
        // ...
    }

    // ---------------- 操作：解密 ----------------
    static int doDecrypt(...) {
        // ...
        if (is_archive) {
            // ...
            fs::path tmpArchive = fs::temp_directory_path() / (origName + ".dec_tmp");
            e = decryptBody(src, h, tmpArchive, [](uint64_t cur, uint64_t tot){
                printProgress(use_zh ? "正在解密" : "Decrypting", cur, tot);
            });
            // ...
        } else {
            // ...
            e = decryptBody(src, h, dst, [](uint64_t cur, uint64_t tot){
                printProgress(use_zh ? "正在解密" : "Decrypting", cur, tot);
            });
            // ...
        }
    }

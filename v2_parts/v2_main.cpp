#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "../common/tests_common.h"
#include "v2_preprocess_impl.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::cout << "V2: улучшенная версия (BOM/CRLF/normalize/кэш) + flatten\n";

    // РЕЖИМ 1: тесты
    if (argc == 1) {
        common::RunAllTests("V2", &v2::Preprocess);
        return 0;
    }

    // РЕЖИМ 2: flatten-утилита
    // v2.exe --flatten <in> <out> [include_dir...]
    if (std::string(argv[1]) == "--flatten") {
        if (argc < 4) {
            std::cerr << "usage: v2 --flatten <in_file> <out_file> [include_dir...]\n";
            return 2;
        }
        fs::path in_file = argv[2];
        fs::path out_file = argv[3];

        std::vector<fs::path> include_dirs;
        for (int i = 4; i < argc; ++i) include_dirs.push_back(fs::path(argv[i]));

        bool ok = v2::FlattenProject(in_file, out_file, include_dirs);
        return ok ? 0 : 1;
    }

    // РЕЖИМ 3: ТЗ-утилита
    // v2.exe <in> <out> [include_dir...]
    if (argc < 3) {
        std::cerr << "usage: v2 <in_file> <out_file> [include_dir...]\n";
        return 2;
    }

    fs::path in_file = argv[1];
    fs::path out_file = argv[2];
    std::vector<fs::path> include_dirs;
    for (int i = 3; i < argc; ++i) include_dirs.push_back(fs::path(argv[i]));

    bool ok = v2::Preprocess(in_file, out_file, include_dirs);
    return ok ? 0 : 1;
}

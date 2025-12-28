#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "../common/tests_common.h"
#include "v1_preprocess_impl.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // РЕЖИМ 1: тесты (без аргументов)
    if (argc == 1) {
    	
    	std::cout << "V1: минимальная реализация + flatten (только #include \"...\")\n";
        common::RunAllTests("V1", &v1::Preprocess);
        return 0;
    }

    // РЕЖИМ 2: flatten-утилита
    // v1.exe --flatten <in> <out> [include_dir...]
    if (std::string(argv[1]) == "--flatten") {
        if (argc < 4) {
            std::cerr << "usage: v1 --flatten <in_file> <out_file> [include_dir...]\n";
            return 2;
        }
        fs::path in_file = argv[2];
        fs::path out_file = argv[3];

        std::vector<fs::path> include_dirs;
        for (int i = 4; i < argc; ++i) include_dirs.push_back(fs::path(argv[i]));

        bool ok = v1::FlattenProject(in_file, out_file, include_dirs);
        return ok ? 0 : 1;
    }

    // РЕЖИМ 3: “чистый” ТЗ-режим утилиты
    // v1.exe <in> <out> [include_dir...]
    if (argc < 3) {
        std::cerr << "usage: v1 <in_file> <out_file> [include_dir...]\n";
        return 2;
    }

    fs::path in_file = argv[1];
    fs::path out_file = argv[2];
    std::vector<fs::path> include_dirs;
    for (int i = 3; i < argc; ++i) include_dirs.push_back(fs::path(argv[i]));

    bool ok = v1::Preprocess(in_file, out_file, include_dirs);
    return ok ? 0 : 1;
}

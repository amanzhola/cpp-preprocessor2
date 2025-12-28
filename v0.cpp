#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace std;
namespace fs = std::filesystem;

static int Run(const string& cmd) {
    cout << cmd << endl;
    return system(cmd.c_str());
}

static void EnsureDir(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p, ec);
}

int main() {
    // ВАЖНО: консоль Windows часто в CP866. В README добавляем "chcp 65001".

    EnsureDir("build");

    // 1) Собираем V1
    if (Run("g++ -std=gnu++17 v1_parts/v1_main.cpp -o v1.exe") != 0) {
        cerr << "Ошибка! Не удалось собрать V1\n";
        return 1;
    }

    // 2) Запускаем V1 (тесты)
    if (Run("v1.exe") != 0) {
        cerr << "Ошибка! Тесты V1 провалены\n";
        return 1;
    }

    // 3) V1 склеивает V2 (flatten: раскрываем только #include "...")
    if (Run("v1.exe --flatten v2_parts/v2_main.cpp build/v2_flat.cpp v2_parts common") != 0) {
        cerr << "Ошибка! V1 не смог склеить V2\n";
        return 1;
    }

    // 4) Собираем V2 из build/v2_flat.cpp
    if (Run("g++ -std=gnu++17 build/v2_flat.cpp -o v2.exe") != 0) {
        cerr << "Ошибка! Не удалось собрать V2 (v2_flat.cpp)\n";
        return 1;
    }

    // 5) Запускаем V2 (тесты)
    if (Run("v2.exe") != 0) {
        cerr << "Ошибка! Тесты V2 провалены\n";
        return 1;
    }

    return 0;
}

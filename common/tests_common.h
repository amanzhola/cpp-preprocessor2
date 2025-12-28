#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace common {
namespace fs = std::filesystem;

// Читаем файл целиком в строку
inline std::string GetFileContents(const fs::path& file) {
    std::ifstream stream(file);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

// Печатаем ровно 3 строки "как в тренажёре".
// Дополнительный вывод (например unknown include file ...) тесты НЕ печатают.
// Но при этом мы проверяем, что сообщение корректное: перехватываем std::cout.
struct CoutCapture {
    std::streambuf* old = nullptr;
    std::ostringstream ss;

    void Begin() {
        old = std::cout.rdbuf(ss.rdbuf());
    }
    std::string End() {
        std::cout.rdbuf(old);
        return ss.str();
    }
};

// Тип функции Preprocess
using PreprocessFn = bool(*)(const fs::path&, const fs::path&, const std::vector<fs::path>&);

// Создаём тестовые файлы, как в условии.
inline void PrepareSampleFiles() {
    std::error_code err;
    fs::remove_all("sources", err);

    fs::create_directories(fs::path("sources") / "include2" / "lib", err);
    fs::create_directories(fs::path("sources") / "include1", err);
    fs::create_directories(fs::path("sources") / "dir1" / "subdir", err);

    {
        std::ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
             << "#include \"dir1/b.h\"\n"
             << "// text between b.h and c.h\n"
             << "#include \"dir1/d.h\"\n"
             << "\n"
             << "int SayHello() {\n"
             << "    cout << \"hello, world!\" << endl;\n"
             << "#   include<dummy.txt>\n"
             << "}\n";
    }
    {
        std::ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
             << "#include \"subdir/c.h\"\n"
             << "// text from b.h after include";
    }
    {
        std::ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
             << "#include <std1.h>\n"
             << "// text from c.h after include\n";
    }
    {
        std::ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
             << "#include \"lib/std2.h\"\n"
             << "// text from d.h after include\n";
    }
    {
        std::ofstream file("sources/include1/std1.h");
        file << "// std1\n";
    }
    {
        std::ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n";
    }
}

// Проверяем пример из условия:
// - функция должна вернуть false (из-за dummy.txt)
// - файл sources/a.in должен содержать строки до ошибки
// - сообщение "unknown include file ..." должно быть выведено, но мы его не печатаем в терминал
inline void TestSample(PreprocessFn fn) {
    PrepareSampleFiles();

    const std::vector<fs::path> include_dirs = { fs::path("sources/include1"), fs::path("sources/include2") };

    CoutCapture cap;
    cap.Begin();

    bool ok = fn(fs::path("sources/a.cpp"), fs::path("sources/a.in"), include_dirs);

    std::string captured = cap.End();

    // Должно быть false из-за dummy.txt
    assert(ok == false);

    // Проверяем, что сообщение об ошибке правильное (строка 8)
    // В разных ОС слеши могут быть \ или /, поэтому проверяем по подстрокам.
    assert(captured.find("unknown include file dummy.txt") != std::string::npos);
    assert(captured.find("at file") != std::string::npos);
    assert(captured.find("sources") != std::string::npos);
    assert(captured.find("at line 8") != std::string::npos);

    std::ostringstream expected;
    expected << "// this comment before include\n"
             << "// text from b.h before include\n"
             << "// text from c.h before include\n"
             << "// std1\n"
             << "// text from c.h after include\n"
             << "// text from b.h after include\n"
             << "// text between b.h and c.h\n"
             << "// text from d.h before include\n"
             << "// std2\n"
             << "// text from d.h after include\n"
             << "\n"
             << "int SayHello() {\n"
             << "    cout << \"hello, world!\" << endl;\n";

    assert(GetFileContents("sources/a.in") == expected.str());
}

inline void RunAllTests(const char* /*version_name*/, PreprocessFn fn) {
    // "как в тренажёре"
    std::cout << "Анализируем и компилируем решение...\n";
    std::cout << "Запускаем тесты...\n";

    TestSample(fn);

    std::cout << "Успех!\n";
}

} // namespace common

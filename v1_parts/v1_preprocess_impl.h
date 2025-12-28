#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace v1 {
namespace fs = std::filesystem;

// =======================
// РЕЖИМ 1 (ТЗ): раскрываем и "..." и <...> по include_directories
// =======================

inline bool PreprocessOne_TZ(const fs::path& in_file,
                            std::ostream& out,
                            const std::vector<fs::path>& include_directories) {
    std::ifstream in(in_file);
    if (!in) return false;

    static std::regex incl_quotes{R"inc(\s*#\s*include\s*"([^"]*)"\s*)inc"};
    static std::regex incl_angle {R"inc(\s*#\s*include\s*<([^>]*)>\s*)inc"};
	
    std::string line;
    int line_num = 0;

    while (std::getline(in, line)) {
        ++line_num;
    
        std::smatch m;

        // #include "..."
        if (std::regex_match(line, m, incl_quotes)) {
            const std::string token = m[1].str();
            const fs::path rel = fs::path(token);

            // 1) рядом с текущим файлом, 2) include_directories
            std::vector<fs::path> candidates;
            candidates.push_back(in_file.parent_path() / rel);
            for (const auto& dir : include_directories) candidates.push_back(dir / rel);

            bool ok = false;
            for (const auto& cand : candidates) {
                std::ifstream test(cand);
                if (test) {
                    if (!PreprocessOne_TZ(cand, out, include_directories)) return false;
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                std::cout << "unknown include file " << token
                          << " at file " << in_file.string()
                          << " at line " << line_num << std::endl;
                return false;
            }
            continue;
        }

        // #include <...>  (в ТЗ-режиме ищем по include_directories)
        if (std::regex_match(line, m, incl_angle)) {
            const std::string token = m[1].str();
            const fs::path rel = fs::path(token);

            bool ok = false;
            for (const auto& dir : include_directories) {
                fs::path cand = dir / rel;
                std::ifstream test(cand);
                if (test) {
                    if (!PreprocessOne_TZ(cand, out, include_directories)) return false;
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                std::cout << "unknown include file " << token
                          << " at file " << in_file.string()
                          << " at line " << line_num << std::endl;
                return false;
            }
            continue;
        }

        out << line << '\n';
    }

    return true;
}

inline bool Preprocess(const fs::path& in_file,
                       const fs::path& out_file,
                       const std::vector<fs::path>& include_directories) {
    // ТЗ: если вход не открылся — out не трогаем, ничего не печатаем
    std::ifstream probe(in_file);
    if (!probe) return false;

    std::ofstream out(out_file);
    if (!out) return false;

    return PreprocessOne_TZ(in_file, out, include_directories);
}

// =======================
// РЕЖИМ 2 (FLATTEN): раскрываем только "..." , а <...> оставляем как есть
// =======================

inline bool PreprocessOne_Flatten(const fs::path& in_file,
                                 std::ostream& out,
                                 const std::vector<fs::path>& include_directories) {
    std::ifstream in(in_file);
    if (!in) return false;

    static std::regex incl_quotes{R"inc(\s*#\s*include\s*"([^"]*)"\s*)inc"};
    static std::regex incl_angle {R"inc(\s*#\s*include\s*<([^>]*)>\s*)inc"};
    static const std::regex pragma_once_re(R"inc(\s*#\s*pragma\s+once\s*)inc");

    std::string line;
    int line_num = 0;

    while (std::getline(in, line)) {
        ++line_num;
        
        // убираем #pragma once при flatten (чтобы не было warning в итоговом .cpp)
	    if (std::regex_match(line, pragma_once_re)) {
	        continue;
	    }
    
        std::smatch m;

        if (std::regex_match(line, m, incl_quotes)) {
            const std::string token = m[1].str();
            const fs::path rel = fs::path(token);

            std::vector<fs::path> candidates;
            candidates.push_back(in_file.parent_path() / rel);
            for (const auto& dir : include_directories) candidates.push_back(dir / rel);

            bool ok = false;
            for (const auto& cand : candidates) {
                std::ifstream test(cand);
                if (test) {
                    if (!PreprocessOne_Flatten(cand, out, include_directories)) return false;
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                std::cout << "unknown include file " << token
                          << " at file " << in_file.string()
                          << " at line " << line_num << std::endl;
                return false;
            }
            continue;
        }

        // КЛЮЧЕВОЕ ПРАВИЛО FLATTEN:
        // <...> не раскрываем, оставляем компилятору.
        if (std::regex_match(line, m, incl_angle)) {
            out << line << '\n';
            continue;
        }

        out << line << '\n';
    }

    return true;
}

inline bool FlattenProject(const fs::path& in_file,
                           const fs::path& out_file,
                           const std::vector<fs::path>& include_directories) {
    std::ifstream probe(in_file);
    if (!probe) return false;

    std::ofstream out(out_file);
    if (!out) return false;

    return PreprocessOne_Flatten(in_file, out, include_directories);
}

} // namespace v1

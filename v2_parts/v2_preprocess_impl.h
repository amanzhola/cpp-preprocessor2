#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace v2 {
namespace fs = std::filesystem;

inline void StripUtf8BOM(std::string& s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

inline void RStripCR(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

inline fs::path Normalize(const fs::path& p) {
    return p.lexically_normal();
}

// =======================
// РЕЖИМ 1 (ТЗ): раскрываем "..." и <...> по include_directories
// + улучшения: BOM/CRLF, normalize, кэш
// =======================

inline bool Preprocess_TZ(const fs::path& in_file,
                          const fs::path& out_file,
                          const std::vector<fs::path>& include_directories) {
    std::ifstream probe(in_file);
    if (!probe.is_open()) return false;

    std::ofstream out(out_file);
    if (!out.is_open()) return false;

    const std::regex re_quote(R"inc(\s*#\s*include\s*"([^"]*)"\s*)inc", std::regex_constants::optimize);
    const std::regex re_angle(R"inc(\s*#\s*include\s*<([^>]*)>\s*)inc", std::regex_constants::optimize);

    std::unordered_map<std::string, fs::path> cache;

    std::function<bool(const fs::path&, int)> expand = [&](const fs::path& current, int depth) -> bool {
        if (depth > 200) {
            std::cout << "unknown include file TOO_DEEP at file " << current.string()
                      << " at line 1\n";
            return false;
        }

        std::ifstream in(current);
        if (!in.is_open()) return false;

        std::string line;
        std::size_t line_no = 0;

        while (std::getline(in, line)) {
            ++line_no;
            if (line_no == 1) StripUtf8BOM(line);
            RStripCR(line);

            std::smatch m;

            // "..."
            if (std::regex_match(line, m, re_quote)) {
                const std::string token = m[1].str();
                const fs::path rel(token);
                const fs::path here = current.parent_path();

                const std::string key = std::string("Q|") + here.string() + "|" + token;
                if (auto it = cache.find(key); it != cache.end()) {
                    if (!expand(it->second, depth + 1)) return false;
                    continue;
                }

                std::vector<fs::path> candidates;
                candidates.push_back(Normalize(here / rel));
                for (const auto& dir : include_directories) candidates.push_back(Normalize(dir / rel));

                bool ok = false;
                for (const auto& cand : candidates) {
                    std::ifstream test(cand);
                    if (test.is_open()) {
                        cache[key] = cand;
                        if (!expand(cand, depth + 1)) return false;
                        ok = true;
                        break;
                    }
                }

                if (!ok) {
                    std::cout << "unknown include file " << token
                              << " at file " << current.string()
                              << " at line " << line_no << "\n";
                    return false;
                }
                continue;
            }

            // <...> (ТЗ-режим: ищем по include_directories)
            if (std::regex_match(line, m, re_angle)) {
                const std::string token = m[1].str();
                const fs::path rel(token);

                const std::string key = std::string("A|") + token;
                if (auto it = cache.find(key); it != cache.end()) {
                    if (!expand(it->second, depth + 1)) return false;
                    continue;
                }

                bool ok = false;
                for (const auto& dir : include_directories) {
                    fs::path cand = Normalize(dir / rel);
                    std::ifstream test(cand);
                    if (test.is_open()) {
                        cache[key] = cand;
                        if (!expand(cand, depth + 1)) return false;
                        ok = true;
                        break;
                    }
                }

                if (!ok) {
                    std::cout << "unknown include file " << token
                              << " at file " << current.string()
                              << " at line " << line_no << "\n";
                    return false;
                }
                continue;
            }

            out << line << '\n';
        }

        return true;
    };

    return expand(in_file, 0);
}

inline bool Preprocess(const fs::path& in_file,
                       const fs::path& out_file,
                       const std::vector<fs::path>& include_directories) {
    return Preprocess_TZ(in_file, out_file, include_directories);
}

// =======================
// РЕЖИМ 2 (FLATTEN): раскрываем только "..." , а <...> оставляем как есть
// + улучшения: BOM/CRLF, normalize, кэш
// =======================

inline bool FlattenProject(const fs::path& in_file,
                           const fs::path& out_file,
                           const std::vector<fs::path>& include_directories) {
    std::ifstream probe(in_file);
    if (!probe.is_open()) return false;

    std::ofstream out(out_file);
    if (!out.is_open()) return false;

    const std::regex re_quote(R"inc(\s*#\s*include\s*"([^"]*)"\s*)inc", std::regex_constants::optimize);
    const std::regex re_angle(R"inc(\s*#\s*include\s*<([^>]*)>\s*)inc", std::regex_constants::optimize);

    std::unordered_map<std::string, fs::path> cache;

    std::function<bool(const fs::path&, int)> expand = [&](const fs::path& current, int depth) -> bool {
        if (depth > 200) return false;

        std::ifstream in(current);
        if (!in.is_open()) return false;

        std::string line;
        std::size_t line_no = 0;

        while (std::getline(in, line)) {
            ++line_no;
            if (line_no == 1) StripUtf8BOM(line);
            RStripCR(line);

            std::smatch m;

            // "..."
            if (std::regex_match(line, m, re_quote)) {
                const std::string token = m[1].str();
                const fs::path rel(token);
                const fs::path here = current.parent_path();

                const std::string key = std::string("Q|") + here.string() + "|" + token;
                if (auto it = cache.find(key); it != cache.end()) {
                    if (!expand(it->second, depth + 1)) return false;
                    continue;
                }

                std::vector<fs::path> candidates;
                candidates.push_back(Normalize(here / rel));
                for (const auto& dir : include_directories) candidates.push_back(Normalize(dir / rel));

                bool ok = false;
                for (const auto& cand : candidates) {
                    std::ifstream test(cand);
                    if (test.is_open()) {
                        cache[key] = cand;
                        if (!expand(cand, depth + 1)) return false;
                        ok = true;
                        break;
                    }
                }
                if (!ok) return false;
                continue;
            }

            // <...> НЕ раскрываем — оставляем компилятору
            if (std::regex_match(line, m, re_angle)) {
                out << line << '\n';
                continue;
            }

            out << line << '\n';
        }

        return true;
    };

    return expand(in_file, 0);
}

} // namespace v2

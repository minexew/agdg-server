#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

namespace agdg {
    class FileUtils {
    public:
        static std::string get_contents(const std::string& path) {
            std::ifstream ifs(path);

            if (!ifs.is_open())
                throw std::runtime_error("failed to open file '" + path + "' for input");

            return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
        }

        static bool try_get_contents(const std::string& path, std::string& contents_out) {
            std::ifstream ifs(path);

            if (!ifs.is_open())
                return false;

            contents_out = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
            return true;
        }
    };
}

#pragma once

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace agdg {
    // TODO: separate declarataion/implementation when you're bored

    class FileUtils {
    public:
        // Only guaranteed to work for UTF-8 text, mostly because std::string makes little sense for binary data
        static std::string get_contents(const std::string& path) {
            std::ifstream ifs(path, std::ios::binary);

            if (!ifs.is_open())
                throw std::runtime_error("failed to open file '" + path + "' for input");

            return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
        }

        static void get_contents(const std::string& path, std::vector<uint8_t>& vector_out) {
            std::ifstream ifs(path, std::ios::binary);

            if (!ifs.is_open())
                throw std::runtime_error("failed to open file '" + path + "' for input");

            vector_out.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
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

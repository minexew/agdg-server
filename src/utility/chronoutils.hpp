#pragma once

#include <chrono>
#include <iomanip>
#include <string>
#include <sstream>

namespace agdg {
    class ChronoUtils {
    public:
        static constexpr auto iso8601_format = "%Y-%m-%dT%T";
        static constexpr auto date_format = "%Y-%m-%d";

        static void from_iso8601(std::chrono::system_clock::time_point& tp_out, const std::string& str) {
            struct tm tm;

            std::istringstream iss(str);
            iss >> std::get_time(&tm, iso8601_format);

            tp_out = std::chrono::system_clock::from_time_t(mktime(&tm));
        }

        static std::string to_iso8601(std::chrono::system_clock::time_point tp) {
            time_t t = std::chrono::system_clock::to_time_t(tp);

            std::ostringstream oss;
#ifdef _WIN32
            std::tm tm;
            localtime_s(&tm, &t);
            oss << std::put_time(&tm, iso8601_format);
#else
            oss << std::put_time(localtime(&t), iso8601_format);
#endif
            return oss.str();
        }

        static std::string to_date_string(std::chrono::system_clock::time_point tp) {
            time_t t = std::chrono::system_clock::to_time_t(tp);

            std::ostringstream oss;
#ifdef _WIN32
            std::tm tm;
            localtime_s(&tm, &t);
            oss << std::put_time(&tm, date_format);
#else
            oss << std::put_time(localtime(&t), date_format);
#endif
            return oss.str();
        }
    };
}

#pragma once

#include <agdg/types.hpp>
#include <rapidjson/document.h>

#include <string>

namespace agdg {
    class IContentManager {
    public:
        static unique_ptr<IContentManager> create();
        virtual ~IContentManager() {}

        //virtual std::string get_asset_as_string(const std::string& path) = 0;
        virtual SHA3_224 put(const rapidjson::Document& document, bool cache) = 0;
        virtual SHA3_224 put_asset(const std::string& path) = 0;

        //virtual void rehash_all_content() = 0;
    };
}

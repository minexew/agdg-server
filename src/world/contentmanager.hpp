#pragma once

#include <agdg/types.hpp>
#include <rapidjson/document.h>

#include <string>

#include <gsl/span>

namespace agdg {
    class IContentManager {
    public:
        static unique_ptr<IContentManager> create();
        virtual ~IContentManager() {}

        //virtual std::string get_asset_as_string(const std::string& path) = 0;
        virtual SHA3_224 put(const rapidjson::Document& document, bool cache) = 0;
        virtual SHA3_224 put_asset(const std::string& path) = 0;

        // An overlay is a set of (path => hash) mappings delivered as JSON
        // Currently this is used to pack .obj+.mtl+texture for props
        // The argument is a list of pairs (local_path, path_in_overlay)
        virtual SHA3_224 put_overlay(gsl::span<std::pair<std::string, std::string>> paths) = 0;

        //virtual void rehash_all_content() = 0;
    };
}

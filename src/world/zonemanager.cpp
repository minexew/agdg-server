#include <world/zonemanager.hpp>

#include <utility/contentscanner.hpp>
#include <agdg/logging.hpp>
#include <utility/hashutils.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/document.h>

#include <memory>
#include <unordered_map>

namespace agdg {
    // TODO: this will be moved, of course
    class Zone : public IZone {
    public:
        Zone(IContentManager* content_mgr, const std::string& path) {
            rapidjson::Document d;
            RapidJsonUtils::load_json(d, path.c_str());

            try {
                RapidJsonUtils::get_value(name, d, "name");
            }
            catch (std::runtime_error& ex) {
                throw std::runtime_error(path + ": format error: "s + ex.what());
            }

            resolve_dependencies(content_mgr, d);
            hash = content_mgr->put(d, false);
        }

        const std::string& get_name() override { return name; }
        const SHA3_224& get_hash() override { return hash;  }

    private:
        void resolve_dependencies(IContentManager* content_mgr, rapidjson::Document& d) {
            auto dependencies_json = rapidjson::Value(rapidjson::kArrayType);

            const auto& props = d.FindMember("props");

            if (props != d.MemberEnd() && props->value.IsArray()) {                         // TODO: json_format_silent_failure
                for (auto it = props->value.Begin(); it != props->value.End(); it++) {
                    auto& prop = *it;

                    if (prop.IsObject()) {                                                  // TODO: json_format_silent_failure
                        std::string path;
                        if (getString(prop, "model", path)) {
                            auto hash = content_mgr->put_asset(path);
                            auto hash_str = HashUtils::hash_to_hex_string(hash);

                            prop.FindMember("model")->value.SetString(hash_str.c_str(), hash_str.size(), d.GetAllocator());
                            dependencies_json.PushBack(rapidjson::Value(hash_str.c_str(), hash_str.size(), d.GetAllocator()), d.GetAllocator());
                        }
                    }
                }
            }

            d.AddMember("dependencies", std::move(dependencies_json), d.GetAllocator());
        }

        std::string name;
        SHA3_224 hash;
    };

    class ZoneManager : public IZoneManager {
    public:
        ZoneManager(IContentManager* content_mgr) : content_mgr(content_mgr) {}

        IZone* get_zone_by_id(const std::string& id) override {
            auto z = zones.find(id);

            return (z != zones.end()) ? z->second.get() : nullptr;
        }

        void reload_content() override {
            ContentScanner::ScanDirectory("world/zones", [this](auto path, auto filename) {
                std::string zoneName(filename, strlen(filename) - 5);

                auto z = zones.find(zoneName);

                if (z != zones.end())
                    g_log->info("replacing zone %s on-the-fly", zoneName.c_str());

                // FIXME: might throw
                zones[zoneName] = make_unique<Zone>(content_mgr, path);
            }, ".json");
        }

    private:
        IContentManager* content_mgr;

        std::unordered_map<std::string, std::unique_ptr<Zone>> zones;
    };

    unique_ptr<IZoneManager> IZoneManager::create(IContentManager* content_mgr) {
        return make_unique<ZoneManager>(content_mgr);
    }
}

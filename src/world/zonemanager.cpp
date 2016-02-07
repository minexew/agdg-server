#include <world/zonemanager.hpp>

#include <utility/contentscanner.hpp>
#include <utility/logging.hpp>
#include <utility/hashutils.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <memory>
#include <unordered_map>

namespace agdg {
	class Zone : public IZone {
	public:
		Zone(IContentManager* content_mgr, const std::string& path) {
			std::string json = ContentScanner::GetFileContents(path); //content_mgr->get_asset_as_string(path);

			rapidjson::Document d;
			rapidjson::ParseResult ok = d.Parse(json.c_str());

			if (!ok)
				throw std::runtime_error(path + ": JSON parse error: " + rapidjson::GetParseError_En(ok.Code()) + " (" + std::to_string(ok.Offset()) + ")");

			try {
				RapidJsonUtils::get_value(name, d, "name");
			}
			catch (std::runtime_error& ex) {
				throw std::runtime_error(path + ": format error: "s + ex.what());
			}

			resolve_dependencies(content_mgr, d);
			hash = content_mgr->put(d, false);
		}

		virtual const std::string& GetName() override { return name; }
		virtual const SHA3_224& GetHash() override { return hash;  }

	private:
		void resolve_dependencies(IContentManager* content_mgr, rapidjson::Document& d) {
			d.AddMember("dependencies", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		}

		std::string name;
		SHA3_224 hash;
	};

	class ZoneManager : public IZoneManager {
	public:
		ZoneManager(IContentManager* content_mgr) : content_mgr(content_mgr) {}

		virtual IZone* GetZoneById(const std::string& id) override {
			auto z = zones.find(id);

			return (z != zones.end()) ? z->second.get() : nullptr;
		}

		virtual void ReloadContent() override {
			ContentScanner::ScanDirectory("world/zones", [this](auto path, auto filename) {
				std::string zoneName(filename, strlen(filename) - 5);

				auto z = zones.find(zoneName);

				if (z != zones.end())
					g_log->Log("replacing zone %s on-the-fly", zoneName.c_str());

				// FIXME: might throw
				zones[zoneName] = make_unique<Zone>(content_mgr, path);
			}, ".json");
		}

	private:
		IContentManager* content_mgr;

		std::unordered_map<std::string, std::unique_ptr<Zone>> zones;
	};

	unique_ptr<IZoneManager> IZoneManager::Create(IContentManager* content_mgr) {
		return make_unique<ZoneManager>(content_mgr);
	}
}

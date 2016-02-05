#include <world/zonemanager.hpp>

#include <utility/contentscanner.hpp>
#include <utility/logging.hpp>
#include <utility/hashutils.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/document.h>

#include <memory>
#include <unordered_map>

namespace agdg {
	class Zone : public IZone {
	public:
		Zone(IContentManager* contentMgr, const std::string& path) {
			// TODO: error handling
			std::string json = ContentScanner::GetFileContents(path);
			HashUtils::HashString(json, hash);
			contentMgr->CacheAsset(hash, path, json);

			rapidjson::Document d;
			d.Parse(json.c_str());

			if (d.GetParseError() != rapidjson::kParseErrorNone)
				return;

			getString(d, "name", name);
		}

		virtual const std::string& GetName() override { return name; }
		virtual const SHA3_224& GetHash() override { return hash;  }

	private:
		std::string name;
		SHA3_224 hash;
	};

	class ZoneManager : public IZoneManager {
	public:
		ZoneManager(IContentManager* contentMgr) : contentMgr(contentMgr) {}

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

				zones[zoneName] = make_unique<Zone>(contentMgr, path);
			}, ".json");
		}

	private:
		IContentManager* contentMgr;

		std::unordered_map<std::string, std::unique_ptr<Zone>> zones;
	};

	unique_ptr<IZoneManager> IZoneManager::Create(IContentManager* contentMgr) {
		return make_unique<ZoneManager>(contentMgr);
	}
}

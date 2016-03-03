#pragma once

#include <agdg/types.hpp>
#include <world/contentmanager.hpp>

#include <string>

namespace agdg {
	class IZone {
	public:
		virtual const std::string& get_name() = 0;
		virtual const SHA3_224& get_hash() = 0;
	};

	class IZoneManager {
	public:
		static unique_ptr<IZoneManager> create(IContentManager* content_mgr);
		virtual ~IZoneManager() {}

		virtual IZone* get_zone_by_id(const std::string& id) = 0;

		virtual void reload_content() = 0;
	};
}

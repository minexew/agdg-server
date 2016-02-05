#pragma once

#include <agdg/types.hpp>
#include <world/contentmanager.hpp>

#include <string>

namespace agdg {
	class IZone {
	public:
		virtual const std::string& GetName() = 0;
		virtual const SHA3_224& GetHash() = 0;
	};

	class IZoneManager {
	public:
		static unique_ptr<IZoneManager> Create(IContentManager* contentMgr);
		virtual ~IZoneManager() {}

		virtual IZone* GetZoneById(const std::string& id) = 0;

		virtual void ReloadContent() = 0;
	};
}

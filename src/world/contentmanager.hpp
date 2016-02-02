#pragma once

#include <agdg/types.hpp>

#include <string>

namespace agdg {
	class IContentManager {
	public:
		static IContentManager* Create();
		virtual ~IContentManager() {}

		// This needs to be redone because it's not Realm's job to serve content
		virtual bool GetCachedAsset(const SHA3_224& hash, std::string& data_out) = 0;
		virtual void CacheAsset(const SHA3_224& hash, const std::string& path, const std::string& content) = 0;
	};
}

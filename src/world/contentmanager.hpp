#pragma once

#include <agdg/types.hpp>

#include <string>

namespace agdg {
	class IContentManager {
	public:
		static unique_ptr<IContentManager> Create();
		virtual ~IContentManager() {}

		// FIXME: Get rid of this bull
		virtual bool GetCachedAsset(const SHA3_224& hash, std::string& data_out) = 0;
		virtual void CacheAsset(const SHA3_224& hash, const std::string& path, const std::string& content) = 0;
	};
}

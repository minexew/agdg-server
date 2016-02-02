#include <world/contentmanager.hpp>

#include <unordered_map>

namespace agdg {
	class ContentManager : public IContentManager {
	public:
		virtual bool GetCachedAsset(const SHA3_224& hash, std::string& data_out) {
			auto it = cachedAssets.find(hash);

			if (it == cachedAssets.end())
				return false;

			data_out = it->second;
			return true;
		}

		virtual void CacheAsset(const SHA3_224& hash, const std::string& path, const std::string& content) {
			cachedAssets[hash] = content;
		}

	protected:
		std::unordered_map<SHA3_224, std::string> cachedAssets;
	};

	IContentManager* IContentManager::Create() {
		return new ContentManager();
	}
}

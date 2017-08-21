#include <world/contentmanager.hpp>

#include <agdg/config.hpp>
#include <agdg/logging.hpp>

#include <utility/fileutils.hpp>
#include <utility/hashutils.hpp>

#include <rapidjson/writer.h>

#include <fstream>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#endif

namespace agdg {
#ifdef _WIN32
    static bool file_exists(const char* path) {
        return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
    }
#else
    static bool file_exists(const char* path) {
        FILE* file = fopen(path, "rb");

        if (file) {
            fclose(file);
            return true;
        }
        else
            return false;
    }
#endif

    static void put_file_contents(const std::string& path, const uint8_t* bytes, size_t length) {
        std::ofstream ofs(path);

        if (!ofs.good())
            throw std::runtime_error("failed to open " + path + " for writing");

        ofs.write((const char*)bytes, length);
    }

    class ContentManager : public IContentManager {
    public:
        ContentManager() {
            g_config->get_value(content_output_dir, "contentOutputDir");
        }

        //virtual std::string get_asset_as_string(const std::string& path) override;

        SHA3_224 put(const uint8_t* bytes, size_t length, bool cache) /*override*/ {
            SHA3_224 hash;
            HashUtils::hash_bytes(bytes, length, hash);

            std::string filename = content_output_dir + "/" + HashUtils::hash_to_hex_string(hash);

            if (!file_exists(filename.c_str())) {
                g_log->info("writing %s", filename.c_str());
                put_file_contents(filename, bytes, length);
            }

            return hash;
        }

        SHA3_224 put(const rapidjson::Document& document, bool cache) override {
            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> wr(buf);

            document.Accept(wr);

            return put((const uint8_t*) buf.GetString(), buf.GetSize(), cache);
        }

        SHA3_224 put_asset(const std::string& path) override {
            // TODO: cache, etc etc etc
            std::string contents = FileUtils::get_contents(path);

            return put(reinterpret_cast<const uint8_t*>(contents.data()), contents.size(), true);
        }

    protected:
        std::string content_output_dir;
    };

    unique_ptr<IContentManager> IContentManager::create() {
        return make_unique<ContentManager>();
    }
}

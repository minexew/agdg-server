#include <agdg/config.hpp>

#include <rapidjson/filereadstream.h>

#include <fstream>

namespace agdg {
	static void s_CopyFile(const char* srcName, const char* dstName) {
		std::ifstream src(srcName, std::ios::binary);

		if (!src.good())
			throw std::runtime_error((std::string) "can't open " + srcName + " for reading");

		std::ofstream dst(dstName, std::ios::binary);

		if (!dst.good())
			throw std::runtime_error((std::string) "can't open " + srcName + " for writing");

		dst << src.rdbuf();
	}

	class Config : public IConfig {
	public:
		virtual void Init() override {
			static const std::string config_dir = "config/";

			LoadConfig(master, config_dir + "master.json", config_dir + "master.default.json");
		}

		virtual void EnumerateServices(std::function<void(const std::string&, const rapidjson::Value&)> callback) override {
			const auto& services = master["services"];

			if (!services.IsObject())
				return;

			for (auto it = services.MemberBegin(); it != services.MemberEnd(); it++) {
				callback(it->name.GetString(), it->value);
			}
		}

	private:
		void LoadConfig(rapidjson::Document& d, const std::string& fileName, const std::string& defaultFileName) {
			FILE* f = fopen(fileName.c_str(), "rb");

			if (!f) {
				s_CopyFile(defaultFileName.c_str(), fileName.c_str());
				f = fopen(fileName.c_str(), "rb");
			}

			if (!f)
				throw std::runtime_error((std::string) "can't open config file " + fileName);

			char buffer[4096];
			rapidjson::FileReadStream stream(f, buffer, sizeof(buffer));
			d.ParseStream(stream);

			if (d.GetParseError() != rapidjson::kParseErrorNone)
				throw std::runtime_error((std::string) "syntax error in config file " + fileName);
		}

		rapidjson::Document master;
	};

	static Config s_config;
	IConfig* g_config = &s_config;
}
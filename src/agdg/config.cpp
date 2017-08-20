#include <agdg/config.hpp>

#include <rapidjson/filereadstream.h>
#include <utility/rapidjsonutils.hpp>

#include <fstream>
#include <stdexcept>

// TODO: migrate off fopen

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
		void init(const char* master_config_or_null) override {
			static const std::string config_dir = "config/";

			LoadConfig(master, master_config_or_null ? master_config_or_null : (config_dir + "master.json"), config_dir + "master.default.json");
		}

		void enumerate_services(std::function<void(const std::string&, const rapidjson::Value&)> callback) override {
			const auto& services = master["services"];

			if (!services.IsObject())
				return;

			for (auto it = services.MemberBegin(); it != services.MemberEnd(); it++) {
				callback(it->name.GetString(), it->value);
			}
		}

		void get_value(std::string& output, const char* field_name) override {
			RapidJsonUtils::get_value(output, master, field_name);
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

			fclose(f);

			if (d.GetParseError() != rapidjson::kParseErrorNone)
				throw std::runtime_error((std::string) "syntax error in config file " + fileName);
		}

		rapidjson::Document master;
	};

	static Config s_config;
	IConfig* g_config = &s_config;
}
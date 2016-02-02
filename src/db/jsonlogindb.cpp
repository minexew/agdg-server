#include <db/db.hpp>

#include <utility/logging.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include <rhash/sha3.h>

#include <sstream>

// TODO: migrate off fopen

namespace agdg {
	class JsonLoginDB : public IJsonLoginDB {
	public:
		JsonLoginDB(const std::string& dir) : dir(dir) {}

		virtual bool CreateAccount(const std::string& username, const std::string& password) override {
			// FIXME: error reporting
			if (!ValidateUsername(username))
				return false;

			rapidjson::Document d;
			if (LoadAccount(username, d))
				return false;		/// Fail IF the account exists

			const auto filename = dir + username + ".json";
			FILE* f = fopen(filename.c_str(), "wb");

			if (!f)
				return false;

			char buffer[4096];
			rapidjson::FileWriteStream stream(f, buffer, sizeof(buffer));
			rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);

			writer.StartObject();
			writer.String("password_sha3_512");
			writer.String(StringToSHA3_512String(password).c_str());
			writer.EndObject();

			fclose(f);
			return true;
		}

		virtual bool VerifyCredentials(const std::string& username, const std::string& password, const std::string& hostname) override {
			// FIXME: error reporting
			if (!ValidateUsername(username))
				return false;

			rapidjson::Document d;
			if (!LoadAccount(username, d))
				return false;

			std::string passwordHash, allowedHostname;

			if (getString(d, "allowedHostname", allowedHostname) && hostname != allowedHostname) {
				g_log->Log("rejecting user '%s' from '%s' (allowed hostnames: '%s')", username.c_str(), hostname.c_str(), allowedHostname.c_str());
				return false;
			}

			getString(d, "password_sha3_512", passwordHash);

			if (StringToSHA3_512String(password) != passwordHash) {
				g_log->Log("rejecting user '%s' from '%s' (invalid password)", username.c_str(), hostname.c_str());
				return false;
			}

			return true;
		}

	private:
		bool ValidateUsername(const std::string& username) {
			if (username.size() < 2)
				return false;

			for (auto c : username) {
				if (!isalnum(c))
					return false;
			}

			return true;
		}

		bool LoadAccount(const std::string& username, rapidjson::Document& d) {
			const auto filename = dir + username + ".json";
			FILE* f = fopen(filename.c_str(), "rb");

			if (!f)
				return false;

			char buffer[4096];
			rapidjson::FileReadStream stream(f, buffer, sizeof(buffer));
			d.ParseStream(stream);

			if (d.GetParseError() != rapidjson::kParseErrorNone) {
				//throw std::runtime_error((std::string) "syntax error in config file " + fileName);
				g_log->Log("error in JSON file '%s'", filename.c_str());
			}

			fclose(f);
			return true;
		}

		std::string StringToSHA3_512String(const std::string& str) {
			sha3_ctx ctx;
			uint8_t hash[64];

			rhash_sha3_512_init(&ctx);
			rhash_sha3_update(&ctx, (const uint8_t*) str.c_str(), str.size());
			rhash_sha3_final(&ctx, hash);

			char string[129];

			for (int i = 0; i < sizeof(hash); i++)
				snprintf(string + i * 2, 3, "%02x", hash[i]);

			return string;
		}

		std::string dir;
	};

	IJsonLoginDB* IJsonLoginDB::Create(const std::string& dir) {
		return new JsonLoginDB(dir);
	}
}

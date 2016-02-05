#include <db/db.hpp>

#include <utility/hashutils.hpp>
#include <utility/logging.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

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
			writer.String(HashUtils::StringToSHA3_512Hex(password).c_str());
			writer.EndObject();

			fclose(f);
			return true;
		}

		virtual bool VerifyCredentials(const std::string& username, const std::string& password, const std::string& hostname,
				AccountSnapshot& snapshot_out) override {
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

			if (HashUtils::StringToSHA3_512Hex(password) != passwordHash) {
				g_log->Log("rejecting user '%s' from '%s' (invalid password)", username.c_str(), hostname.c_str());
				return false;
			}

			snapshot_out.name = username;
			snapshot_out.trusted = false;
			getBool(d, "trusted", snapshot_out.trusted);
			return true;
		}

	private:
		bool LoadAccount(const std::string& username, rapidjson::Document& d) {
			const auto filename = dir + username + ".json";
			FILE* f = fopen(filename.c_str(), "rb");		// FIXME: get rid of fopen

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

		std::string dir;
	};

	unique_ptr<IJsonLoginDB> IJsonLoginDB::Create(const std::string& dir) {
		return make_unique<JsonLoginDB>(dir);
	}
}

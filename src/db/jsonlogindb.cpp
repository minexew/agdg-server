#include <db/db.hpp>

#include <utility/atomicreplacement.hpp>
#include <utility/hashutils.hpp>
#include <utility/logging.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include <fstream>
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
			writer.String(HashUtils::string_to_sha3_512_hex_string(password).c_str());
			writer.EndObject();

			fclose(f);
			return true;
		}

		virtual void get_news(std::vector<db::NewsEntry>& news_out) override {
			std::lock_guard<std::mutex> lg(news_mutex);

			news_out.resize(news.size());
			std::copy(news.begin(), news.end(), news_out.begin());
		}

		virtual void post_news(std::string&& title_html, std::string&& contents_html) override {
			std::lock_guard<std::mutex> lg(news_mutex);

			news.emplace_back(std::chrono::system_clock::now(), std::move(title_html), std::move(contents_html));

			save_news_unguarded();
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

			if (HashUtils::string_to_sha3_512_hex_string(password) != passwordHash) {
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

		void save_news_unguarded() {
			// TODO: could be done asynchronously
			AtomicReplacement replacement(dir + "_news.json");
			std::ofstream file(replacement.c_str());

			if (!file.is_open()) {
				g_log->error("failed to open '%s' for writing", replacement.c_str());
				// TODO: exception?
				return;
			}

			RapidJsonOstream stream(file);
			rapidjson::Writer<RapidJsonOstream> writer(stream);

			writer.StartArray();

			for (const auto& entry : news) {
				writer.StartObject();
				writer.String("title");
				writer.String(entry.title_html.c_str(), entry.title_html.size());
				writer.String("contents");
				writer.String(entry.contents_html.c_str(), entry.contents_html.size());
				writer.EndObject();
			}

			writer.EndArray();

			file.close();
			replacement.commit();
		}

		std::string dir;

		std::vector<db::NewsEntry> news;
		std::mutex news_mutex;
	};

	unique_ptr<IJsonLoginDB> IJsonLoginDB::Create(const std::string& dir) {
		return make_unique<JsonLoginDB>(dir);
	}
}

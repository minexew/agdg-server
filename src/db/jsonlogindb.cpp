#include <db/db.hpp>

#include <utility/atomicreplacement.hpp>
#include <utility/chronoutils.hpp>
#include <utility/hashutils.hpp>
#include <agdg/logging.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <fstream>
#include <mutex>

// TODO: migrate off fopen

namespace agdg {
	class JsonLoginDB : public IJsonLoginDB {
	public:
		JsonLoginDB(const std::string& dir) : dir(dir), news_filename(dir + "_news.json") {
			load_news_unguarded();
		}

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
			std::reverse_copy(news.begin(), news.end(), news_out.begin());
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
				g_log->warning("rejecting user '%s' from '%s' (allowed hostnames: '%s')", username.c_str(), hostname.c_str(), allowedHostname.c_str());
				return false;
			}

			getString(d, "password_sha3_512", passwordHash);

			if (HashUtils::string_to_sha3_512_hex_string(password) != passwordHash) {
				g_log->warning("rejecting user '%s' from '%s' (invalid password)", username.c_str(), hostname.c_str());
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

			if (!RapidJsonUtils::try_load_json(d, filename.c_str()))
				return false;

			return true;
		}

		void load_news_unguarded() {
			rapidjson::Document d;
			if (!RapidJsonUtils::try_load_json(d, news_filename.c_str()))
				return;

			auto& array = d;

			if (!array.IsArray())
				throw std::runtime_error("expected array in '" + news_filename + "'");

			try {
				for (auto it = array.Begin(); it != array.End(); it++) {
					// FIXME: check *it.IsObject()
					news.emplace_back();
					RapidJsonUtils::get_value(news.back().when_posted, *it, "when_posted");
					RapidJsonUtils::get_value(news.back().title_html, *it, "title");
					RapidJsonUtils::get_value(news.back().contents_html, *it, "contents");
				}
			}
			catch (std::runtime_error& ex) {
				throw std::runtime_error(ex.what() + " in '"s + news_filename + "'");
			}
		}

		void save_news_unguarded() {
			// TODO: could be done asynchronously
			AtomicReplacement replacement(news_filename);
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
				writer.String("when_posted");
				writer.String(ChronoUtils::to_iso8601(entry.when_posted).c_str());
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
		std::string news_filename = dir + "_news.json";

		std::vector<db::NewsEntry> news;
		std::mutex news_mutex;
	};

	unique_ptr<IJsonLoginDB> IJsonLoginDB::create(const std::string& dir) {
		return make_unique<JsonLoginDB>(dir);
	}
}

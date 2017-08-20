#include <login/loginserver.hpp>

#include <agdg/config.hpp>
#include <db/db.hpp>
#include <tokenmanager.hpp>
#include <utility/chronoutils.hpp>
#include <utility/hashutils.hpp>
#include <agdg/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>
#include <utility/rapidjsonutils.hpp>
#include <websocketpp_configuration.hpp>

#include <reflection/basic_templates.hpp>
#include <reflection/magic.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <websocketpp/server.hpp>

namespace agdg {
	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	enum { kExpectedClientVersion = 1 };

	class LoginSession;
	class LoginServer;

	typedef websocketpp::server<configuration<LoginSession>> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

	struct RealmInfo {
		std::string name, url;

		REFL_BEGIN("LoginServer.RealmInfo", 1)
			REFL_MUST_CONFIG(name)
			REFL_MUST_CONFIG(url)
		REFL_END
	};

	class LoginProtocol {
	public:
		LoginProtocol(connection_ptr con) : con(con) {}

		void send_error(const std::string& error) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("error");
			writer.String("error");
			writer.String(error.c_str(), error.size());
			writer.EndObject();

			con->send(s.GetString());
		}

		void send_login_success(const std::string& token, const std::vector<RealmInfo>& realms) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("success");
			writer.String("token");
			writer.String(token.c_str(), token.size());
			writer.String("realms");
			writer.StartArray();

			for (const auto& realm : realms) {
				writer.StartObject();
				writer.String("name");
				writer.String(realm.name.c_str(), realm.name.size());
				writer.String("url");
				writer.String(realm.url.c_str(), realm.url.size());
				writer.EndObject();
			}

			writer.EndArray();
			writer.EndObject();

			con->send(s.GetString());
		}

		void send_hello(const std::string& serverName, bool anonymousLogin,
				const std::vector<db::NewsEntry>& news) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("hello");
			writer.String("serverName");
			writer.String(serverName.c_str(), serverName.size());
			writer.String("anonymousLogin");
			writer.Bool(anonymousLogin);
			writer.String("news");
			writer.StartArray();

			for (const auto& entry : news) {
				writer.StartObject();
				writer.String("when_posted");
				writer.String(ChronoUtils::to_date_string(entry.when_posted).c_str());
				writer.String("title");
				writer.String(entry.title_html.c_str(), entry.title_html.size());
				writer.String("contents");
				writer.String(entry.contents_html.c_str(), entry.contents_html.size());
				writer.EndObject();
			}

			writer.EndArray();

			writer.EndObject();

			con->send(s.GetString());
		}

		void send_reject(int expectedVersion) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("reject");
			//writer.String("expectedVersion");
			//writer.Int(expectedVersion);
			writer.EndObject();

			con->send(s.GetString());
		}

		void send_server_closed(std::string& message) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("server_closed");
			writer.String("message");
			writer.String(message.c_str(), message.size());
			writer.EndObject();

			con->send(s.GetString());
		}

	private:
		connection_ptr con;
	};

	class LoginSession {
	public:
		LoginSession(LoginServer* server, connection_ptr con) : server(server), con(con), protocol(con) {}

		void on_message(const rapidjson::Document& d);

	private:
		LoginServer* server;
		connection_ptr con;
		LoginProtocol protocol;

		// TODO: std::optional :)
		int clientVersion = 0;
	};

	class LoginServer : public ILoginServer {
	public:
		LoginServer(const std::string& serviceName, const rapidjson::Value& d) {
			configure(*this, d);
			configureArray(realms, d, "realms");

			db = IJsonLoginDB::create("db/" + serviceName + "/");
		}

		virtual void init() override {
			server.init_asio();
			server.set_reuse_addr(true);

			server.clear_access_channels(websocketpp::log::alevel::all);

			server.set_open_handler(bind(&LoginServer::on_open, this, _1));

			server.listen(listenPort);
			server.start_accept();
		}

		virtual void start() override {
			thread = std::thread(&LoginServer::run, this);
		}

		virtual void stop() override {
			// FIXME: end all connections

			server.stop();

			thread.join();
		}

		virtual void close_server(const std::string& message) override {
			std::lock_guard<std::mutex> lg(server_closure_message_mutex);

			server_closed = true;
			server_closure_message = message;
		}

		virtual void post_news(std::string&& title_html, std::string&& contents_html) override {
			db->post_news(std::move(title_html), std::move(contents_html));
		}

		virtual void reopen_server() override {
			server_closed = false;
		}

		void get_news(std::vector<db::NewsEntry>& news) { db->get_news(news); }
		const std::vector<RealmInfo>& get_realms() const { return realms;  }
		const std::string& get_server_name() const { return serverName; }

		bool is_server_closed(std::string& message_out) {
			if (server_closed) {
				std::lock_guard<std::mutex> lg(server_closure_message_mutex);

				message_out = server_closure_message;
				return true;
			}
			else
				return false;
		}

		bool login(connection_ptr con, const std::string& username, const std::string& password, AccountSnapshot& snapshot_out) {
			if (!ValidateUsername(username))
				return false;

			if (anonymousLogin) {
				snapshot_out.name = username;
				snapshot_out.trusted = false;
				return true;
			}
			else
				return db->VerifyCredentials(username, password, con->get_host(), snapshot_out);
		}

		bool uses_anonymous_login() const { return anonymousLogin; }

	private:
		static void on_message(LoginSession* session, connection_hdl hdl, Server::message_ptr msg) {
			rapidjson::Document d;
			d.Parse(msg->get_payload().c_str());

			if (d.GetParseError() == rapidjson::kParseErrorNone) {
				try {
					session->on_message(d);
				}
				catch (const std::exception& ex) {
					g_log->error("LoginServer on_message exception: %s", ex.what());
				}
			}
		}

		void on_open(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			con->instance = make_unique<LoginSession>(this, con);

			con->set_message_handler(bind(&LoginServer::on_message, con->instance.get(), _1, _2));
		}

		void run() {
			server.run();
		}

		std::thread thread;
		Server server;

		std::unique_ptr<ILoginDB> db;

		std::string serverName;
		int listenPort;
		bool anonymousLogin;

		bool server_closed = false;
		std::string server_closure_message;
		std::mutex server_closure_message_mutex;

		std::vector<RealmInfo> realms;

		REFL_BEGIN("LoginServer", 1)
			REFL_MUST_CONFIG(serverName)
			REFL_MUST_CONFIG(listenPort)
			REFL_MUST_CONFIG(anonymousLogin)
		REFL_END
	};

	void LoginSession::on_message(const rapidjson::Document& d) {
		if (!clientVersion) {
			int clientVersion;

			if (!getInt(d, "clientVersion", clientVersion))
				return;

			if (clientVersion == kExpectedClientVersion) {
				std::string closure_message;

				if (server->is_server_closed(closure_message))
					protocol.send_server_closed(closure_message);
				else {
					std::vector<db::NewsEntry> news;
					server->get_news(news);

					protocol.send_hello(server->get_server_name(), server->uses_anonymous_login(), news);
					this->clientVersion = clientVersion;
				}
			}
			else {
				protocol.send_reject(kExpectedClientVersion);
			}
		}
		else {
			std::string username, password;

			if (!getString(d, "username", username) || !getString(d, "password", password))
				return;

			auto& socket = con->get_raw_socket();
			auto hostname = socket.remote_endpoint().address().to_string();

			g_log->info("Logging in user %s from %s (password length %d)", username.c_str(), hostname.c_str(), password.size());

			// FIXME: this really should be done asynchronously
			AccountSnapshot account_snapshot;
			bool ok = server->login(con, username, password, account_snapshot);

			if (ok) {
				auto token = g_token_manager.assign_account_token(account_snapshot);
				protocol.send_login_success(HashUtils::hash_to_hex_string(token), server->get_realms());
			}
			else {
				protocol.send_error("Invalid username or password");
			}
		}
	}

	unique_ptr<ILoginServer> ILoginServer::create(const std::string& serviceName, const rapidjson::Value& config) {
		return make_unique<LoginServer>(serviceName, config);
	}
}

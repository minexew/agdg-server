#include <login/loginserver.hpp>

#include <agdg/config.hpp>
#include <utility/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>
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

	class LoginProtocol {
	public:
		static void SendHello(connection_ptr con, const std::string& serverName) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("hello");
			writer.String("serverName");
			writer.String(serverName.c_str(), serverName.size());
			writer.EndObject();

			con->send(s.GetString());
		}

		static void SendReject(connection_ptr con, int expectedVersion) {
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();
			writer.String("type");
			writer.String("reject");
			writer.String("expectedVersion");
			writer.Int(expectedVersion);
			writer.EndObject();

			con->send(s.GetString());
		}
	};

	class LoginSession {
	public:
		LoginSession(LoginServer* server, connection_ptr con) : server(server), con(con) {}

		void OnMessage(const rapidjson::Document& d);

	private:
		LoginServer* server;
		connection_ptr con;

		int clientVersion = 0;
	};

	class LoginServer : public ILoginServer {
	public:
		LoginServer(const rapidjson::Value& d) {
			configure(*this, d);
			configureArray(realms, d, "realms");
		}

		virtual void Init() override {
			server.init_asio();

			server.clear_access_channels(websocketpp::log::alevel::all);

			server.set_open_handler(bind(&LoginServer::OnOpen, this, _1));

			server.listen(listenPort);
			server.start_accept();
		}

		virtual void Start() override {
			thread = std::thread(&LoginServer::Run, this);
		}

		virtual void Stop() override {
			// FIXME: end all connections

			server.stop();

			thread.join();
		}

		const std::string& GetServerName() { return serverName; }

	private:
		static void ForwardMessage(LoginSession* session, connection_hdl hdl, Server::message_ptr msg) {
			rapidjson::Document d;
			d.Parse(msg->get_payload().c_str());

			if (d.GetParseError() == rapidjson::kParseErrorNone)
				session->OnMessage(d);
		}

		void OnOpen(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			con->instance.reset(new LoginSession(this, con));

			con->set_message_handler(bind(&LoginServer::ForwardMessage, con->instance.get(), _1, _2));
		}

		void Run() {
			server.run();
		}

		struct Realm {
			std::string name, url;

			REFL_BEGIN("LoginServer.Realm", 1)
				REFL_MUST_CONFIG(name)
				REFL_MUST_CONFIG(url)
			REFL_END
		};

		std::thread thread;
		Server server;

		std::string serverName;
		int listenPort;
		std::vector<Realm> realms;

		REFL_BEGIN("LoginServer", 1)
			REFL_MUST_CONFIG(serverName)
			REFL_MUST_CONFIG(listenPort)
		REFL_END
	};

	void LoginSession::OnMessage(const rapidjson::Document& d) {
		if (!clientVersion) {
			int clientVersion;

			if (!getInt(d, "clientVersion", clientVersion))
				return;

			if (clientVersion == kExpectedClientVersion) {
				LoginProtocol::SendHello(con, server->GetServerName());
				this->clientVersion = clientVersion;
			}
			else {
				LoginProtocol::SendReject(con, kExpectedClientVersion);
			}
		}
		else {
			std::string username, password;

			if (!getString(d, "username", username) || !getString(d, "password", password))
				return;

			g_log->Log("Logging in user %s (pw length %d)", username.c_str(), password.size());
		}
	}

	ILoginServer* ILoginServer::Create(const rapidjson::Value& config) {
		return new LoginServer(config);
	}
}

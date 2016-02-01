#include <login/loginserver.hpp>

#include <utility/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>

#include <reflection/basic_templates.hpp>
#include <reflection/magic.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace agdg {
	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	struct Realm {
		std::string name, url;

		REFL_BEGIN("LoginServer.Realm", 1)
			REFL_MUST_CONFIG(name)
			REFL_MUST_CONFIG(url)
		REFL_END
	};

	class LoginSession {
	public:
		void OnMessage() {
		}
	};

	// This needs to be done so that we can have our own class for the session
	struct custom_config : public websocketpp::config::asio {
		typedef websocketpp::config::asio core;

		typedef core::concurrency_type concurrency_type;
		typedef core::request_type request_type;
		typedef core::response_type response_type;
		typedef core::message_type message_type;
		typedef core::con_msg_manager_type con_msg_manager_type;
		typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;
		typedef core::alog_type alog_type;
		typedef core::elog_type elog_type;
		typedef core::rng_type rng_type;
		typedef core::transport_type transport_type;
		typedef core::endpoint_base endpoint_base;

		typedef LoginSession connection_base;
	};

	typedef websocketpp::server<custom_config> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

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
			server.stop();

			thread.join();
		}

	private:
		static void ForwardMessage(LoginSession* con, connection_hdl hdl, Server::message_ptr msg) {
			con->OnMessage();
		}

		void OnOpen(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);
			con->set_message_handler(bind(&LoginServer::ForwardMessage, con.get(), _1, _2));
		}

		void Run() {
			server.run();
		}

		std::thread thread;
		Server server;

		int listenPort;
		std::vector<Realm> realms;

		REFL_BEGIN("LoginServer", 1)
			REFL_MUST_CONFIG(listenPort)
		REFL_END
	};

	ILoginServer* ILoginServer::Create(const rapidjson::Value& config) {
		return new LoginServer(config);
	}
}

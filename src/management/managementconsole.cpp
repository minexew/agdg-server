#include <management/managementconsole.hpp>

#include <agdg/serverlifecycle.hpp>
#include <login/loginserver.hpp>
#include <agdg/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>
#include <utility/rapidjsonutils.hpp>

#include <reflection/magic.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace agdg {
	// TODO: JSON (de)serialization should probably be based off a schema

	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	class ManagementConsoleSession {
	public:
		void on_message(const rapidjson::Document& d) {
			const auto& command = d["command"];

			if (!command.IsString())
				return;

			std::string cmd = command.GetString();

			// FIXME: d["message"] asserts!
			if (cmd == "close_server")
				g_serverLifecycle->close_server(d["message"].GetString());
			else if (cmd == "post_news") {
				std::string title, contents;
				auto login = get_login(d);

				RapidJsonUtils::get_value(title, d, "title");
				RapidJsonUtils::get_value(contents, d, "contents");

				login->post_news(std::move(title), std::move(contents));
			}
			else if (cmd == "stop")
				g_serverLifecycle->request_shutdown();
		}

	private:
		ILoginServer* get_login(const rapidjson::Document& d) {
			std::string service_name;
			RapidJsonUtils::get_value(service_name, d, "serviceName");

			auto login = dynamic_cast<ILoginServer*>(g_serverLifecycle->find_service_by_name(service_name));

			if (!login)
				throw std::runtime_error("invalid ILoginServer " + service_name);

			return login;
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

		typedef ManagementConsoleSession connection_base;
	};

	typedef websocketpp::server<custom_config> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

	class ManagementConsoleProtocol {
	public:
		static void SerializeLogEvent(rapidjson::StringBuffer& s, LogTimestamp timestamp, const std::string& message) {
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);
			
			writer.StartObject();
			writer.String("type");
			writer.String("logEvent");
			writer.String("timestamp");
			writer.Int64(timestamp);
			writer.String("message");
			writer.String(message.c_str(), message.size());
			writer.EndObject();
		}
	};

	class ManagementConsole : public IManagementConsole {
	public:
		ManagementConsole(const rapidjson::Value& d) {
			if (!d.IsObject())
				throw std::runtime_error("ManagementConsole: configuration error - expected object");

			configure(*this, d);

			try {
				RapidJsonUtils::get_array(allowed_ips, d, "allowedIPs");
			}
			catch (std::runtime_error& ex) {
				throw std::runtime_error("ManagementConsole: configuration error - "s + ex.what());
			}
		}

		virtual void init() override {
			server.init_asio();
			server.set_reuse_addr(true);

			server.clear_access_channels(websocketpp::log::alevel::all);

			server.set_open_handler(bind(&ManagementConsole::on_open, this, _1));
			server.set_validate_handler(bind(&ManagementConsole::validate_handler, this, _1));

			server.listen(listenPort);
			server.start_accept();
		}

		virtual void start() override {
			thread = std::thread(&ManagementConsole::run, this);
		}

		virtual void stop() override {
			// FIXME: end all connections

			server.stop();

			thread.join();
		}

	private:
		static void forward_message(ManagementConsoleSession* con, connection_hdl hdl, Server::message_ptr msg) {
			rapidjson::Document d;
			d.Parse(msg->get_payload().c_str());

			if (d.GetParseError() == rapidjson::kParseErrorNone) {
				try {
					con->on_message(d);
				}
				catch (const std::exception& ex) {
					g_log->error("ManagementConsole OnMessage exception: %s", ex.what());
				}
			}
		}

		bool is_allowed_client_addr(const std::string& client_addr) {
			for (auto& addr : allowed_ips)
				if (client_addr == addr)
					return true;

			return false;
		}

		void on_open(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);
			con->set_message_handler(bind(&ManagementConsole::forward_message, con.get(), _1, _2));

			g_log->get_all_messages([con](auto timestamp, auto message) {
				// Walk the log and send all previous entries to the client
				rapidjson::StringBuffer s;

				ManagementConsoleProtocol::SerializeLogEvent(s, timestamp, message);
				con->send(s.GetString());
			});
		}

		void run() {
			server.run();
		}

		bool validate_handler(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			auto& socket = con->get_raw_socket();
			auto client_addr = socket.remote_endpoint().address().to_string();

			// Only allow trusted connections
			if (is_allowed_client_addr(client_addr)) {
				g_log->info("ManagementConsole: ACCEPTING connection from %s", client_addr.c_str());
				return true;
			}

			g_log->warning("ManagementConsole: REJECTING connection from %s", client_addr.c_str());
			return false;
		}

		std::thread thread;
		Server server;

		int listenPort;
		std::vector<std::string> allowed_ips;

		REFL_BEGIN("ManagementConsole", 1)
			REFL_MUST_CONFIG(listenPort)
		REFL_END
	};

	unique_ptr<IManagementConsole> IManagementConsole::create(const std::string& serviceName, const rapidjson::Value& config) {
		return make_unique<ManagementConsole>(config);
	}
}

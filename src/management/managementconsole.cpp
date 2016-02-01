#include <management/managementconsole.hpp>

#include <utility/logging.hpp>
#include <serverlifecycle.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace agdg {
	// TODO: JSON (de)serialization should probably be based off a schema

	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	enum { kConfigManagementPort = 9001 };

	class ManagementConsoleSession {
	public:
		void OnMessage(const rapidjson::Document& d) {
			const auto& command = d["command"];

			if (!command.IsString())
				return;

			if (strcmp(command.GetString(), "stop") == 0)
				g_serverLifecycle->Stop();
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
		virtual void Init() override {
			server.init_asio();

			server.clear_access_channels(websocketpp::log::alevel::all);

			server.set_open_handler(bind(&ManagementConsole::OnOpen, this, _1));
			server.set_validate_handler(bind(&ManagementConsole::ValidateHandler, this, _1));

			server.listen(kConfigManagementPort);
			server.start_accept();
		}

		virtual void Start() override {
			thread = std::thread(&ManagementConsole::Run, this);
		}

		virtual void Stop() override {
			server.stop();

			thread.join();
		}

	private:
		static void ForwardMessage(ManagementConsoleSession* con, connection_hdl hdl, Server::message_ptr msg) {
			rapidjson::Document d;
			d.Parse(msg->get_payload().c_str());

			if (d.GetParseError() == rapidjson::kParseErrorNone)
				con->OnMessage(d);
		}

		void OnOpen(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);
			con->set_message_handler(bind(&ManagementConsole::ForwardMessage, con.get(), _1, _2));

			g_log->GetAllMessages([con](auto timestamp, auto message) {
				// Walk the log and send all previous entries to the client
				rapidjson::StringBuffer s;

				ManagementConsoleProtocol::SerializeLogEvent(s, timestamp, message);
				con->send(s.GetString());
			});
		}

		void Run() {
			server.run();
		}

		bool ValidateHandler(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			const auto& hostname = con->get_host();

			// Only allow localhost connections
			if (hostname == "127.0.0.1" || hostname == "localhost") {
				g_log->Log("ManagementConsole: ACCEPTING connection from %s", hostname.c_str());
				return true;
			}

			g_log->Log("ManagementConsole: REJECTING connection from %s", hostname.c_str());
			return false;
		}

		std::thread thread;
		Server server;
	};

	static ManagementConsole s_mgmtConsole;
	IManagementConsole* g_mgmtConsole = &s_mgmtConsole;
}

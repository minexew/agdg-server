#pragma once

#include <realm/realmserver.hpp>
#include <realm/zoneinstance.hpp>
#include <scripting/realm_dom.hpp>
#include <world/zonemanager.hpp>
#include <websocketpp_configuration.hpp>

#include <reflection/basic_templates.hpp>
#include <reflection/basic_types.hpp>
#include <reflection/magic.hpp>

#include <websocketpp/server.hpp>

#include <chrono>

namespace agdg {
	using namespace std::chrono;

	class Realm;
	class RealmSession;

	typedef websocketpp::server<configuration<RealmSession>> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

	constexpr auto kPingInterval = milliseconds(5000);
	constexpr auto kPingTimeout = milliseconds(1000);

	constexpr auto kDefaultAssumedLatency = 200;

	class RealmServer : public IRealmServer {
	public:
		RealmServer(const std::string& serviceName, const rapidjson::Value& d);
		~RealmServer();

		void init() override;
		void start() override;
		void stop() override;

		// FIXME: implement
		void close_server(const std::string& message) override {}
		void reopen_server() override {}

		//IContentManager* get_content_manager() { return content_mgr.get(); }
		//IZoneManager* GetZoneManager() { return zoneMgr.get(); }
		ZoneInstance* get_world_zone() { return world_zone.get(); }

	private:
		void on_close(websocketpp::connection_hdl hdl);
		void on_message(RealmSession* session, websocketpp::connection_hdl hdl, Server::message_ptr msg);
		void on_open(websocketpp::connection_hdl hdl);

		void on_tick();

		void run();
		void try_run_catch();

		std::thread thread;
		Server server;
		bool should_stop;

		std::unique_ptr<Realm> realm;

		//std::unique_ptr<ILoginDB> db;

		std::unique_ptr<IContentManager> content_mgr;
		std::unique_ptr<IZoneManager> zone_mgr;

		std::unique_ptr<ZoneInstance> world_zone;

		std::vector<RealmSession*> all_sessions;

		std::string serverName;
		int listenPort;

		REFL_BEGIN("RealmServer", 1)
			REFL_MUST_CONFIG(listenPort)
		REFL_END
	};
}

#include <realm/realm.hpp>
#include <realm/realmserverimpl.hpp>
#include <realm/realmsession.hpp>

#include <agdg/config.hpp>
#include <utility/hashutils.hpp>
#include <agdg/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>
#include <utility/rapidjsonutils.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

// TODO: preallocate message structs (both C->S & S->C) to reuse buffers in std::string, vector etc.
// Maybe use fixed-size buffers in some cases

namespace agdg {
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	RealmServer::RealmServer(const std::string& serviceName, const rapidjson::Value& d) {
		configure(*this, d);

		content_mgr = IContentManager::create();
		zone_mgr = IZoneManager::create(content_mgr.get());
		zone_mgr->reload_content();

		//db.reset(IJsonRealmDB::Create("db/" + serviceName + "/"));
	}

	RealmServer::~RealmServer() {
		// must go down before the context
		realm.reset();
	}

	void RealmServer::init() {
		/*
#ifdef WITH_V8
		init_v8();
#endif

		server.init_asio();
		server.set_reuse_addr(true);

		server.clear_access_channels(websocketpp::log::alevel::all);

		server.set_open_handler(bind(&RealmServer::on_open, this, _1));
		server.set_close_handler(bind(&RealmServer::on_close, this, _1));

		hooks->on_realm_init();

		IZone* test_zone = zone_mgr->get_zone_by_id("test_zone");
		world_zone = ZoneInstance::create(hooks.get(), test_zone);

		server.listen(listenPort);
		server.start_accept();
		*/
	}

#ifdef WITH_V8
	
#endif

	void RealmServer::on_close(websocketpp::connection_hdl hdl) {
		connection_ptr con = server.get_con_from_hdl(hdl);

		all_sessions.erase(std::remove(all_sessions.begin(), all_sessions.end(), con->instance.get()));
		con->instance->close();
		con->instance.reset();
	}

	void RealmServer::on_message(RealmSession* session, websocketpp::connection_hdl hdl, Server::message_ptr msg) {
		auto data = (const uint8_t*) msg->get_payload().c_str();
		size_t length = msg->get_payload().size();

		try {
			// do not parse the commands here, RealmSession will take care of it
			session->on_message(data, length);
		}
		catch (const std::exception& ex) {
			g_log->error("RealmServer on_message exception: %s", ex.what());
		}
	}

	void RealmServer::on_open(websocketpp::connection_hdl hdl) {
		connection_ptr con = server.get_con_from_hdl(hdl);

		con->instance = make_unique<RealmSession>(realm.get(), this, con);
		all_sessions.push_back(con->instance.get());

		con->set_message_handler(bind(&RealmServer::on_message, this, con->instance.get(), _1, _2));
	}

	void RealmServer::on_tick() {
		realm->on_tick();

		for (auto session : all_sessions)
			session->on_tick();
	}

	void RealmServer::run() {
		realm = Realm::create();

		server.init_asio();
		server.set_reuse_addr(true);

		server.clear_access_channels(websocketpp::log::alevel::all);

		server.set_open_handler(bind(&RealmServer::on_open, this, _1));
		server.set_close_handler(bind(&RealmServer::on_close, this, _1));

		realm->on_realm_init();

		IZone* test_zone = zone_mgr->get_zone_by_id("test_zone");
		world_zone = ZoneInstance::create(realm.get(), test_zone);

		server.listen(listenPort);
		server.start_accept();

		auto last_update = std::chrono::high_resolution_clock::now();
		const auto tick_time_ms = 10;

		while (true) {
			for (size_t count = 0; count < 50; count++)
				if (server.poll_one() == 0)
					break;

			// TODO: maybe use something more efficient?
			auto now = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);

			if (duration.count() >= tick_time_ms) {
				last_update = now;		// FIXME: should be last_update += tick_time

				this->on_tick();
			}

			usleep(1000);
		}
	}

	void RealmServer::start() {
		thread = std::thread(&RealmServer::run, this);
	}

	void RealmServer::stop() {
		// FIXME: end all connections

		server.stop();

		thread.join();
	}

	unique_ptr<IRealmServer> IRealmServer::create(const std::string& serviceName, const rapidjson::Value& config) {
		return make_unique<RealmServer>(serviceName, config);
	}
}

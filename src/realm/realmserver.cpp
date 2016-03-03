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

		IZone* test_zone = zone_mgr->get_zone_by_id("test_zone");
		world_zone = ZoneInstance::create(test_zone);

		//db.reset(IJsonRealmDB::Create("db/" + serviceName + "/"));
	}

	void RealmServer::init() {
		server.init_asio();
		server.set_reuse_addr(true);

		server.clear_access_channels(websocketpp::log::alevel::all);

		server.set_open_handler(bind(&RealmServer::on_open, this, _1));
		server.set_close_handler(bind(&RealmServer::on_close, this, _1));

		server.listen(listenPort);
		server.start_accept();
	}

	void RealmServer::on_close(websocketpp::connection_hdl hdl) {
		connection_ptr con = server.get_con_from_hdl(hdl);

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

		con->instance = make_unique<RealmSession>(this, con);

		con->set_message_handler(bind(&RealmServer::on_message, this, con->instance.get(), _1, _2));
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

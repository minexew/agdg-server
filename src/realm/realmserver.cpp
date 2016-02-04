#include <realm/realmserver.hpp>

#include <agdg/config.hpp>
#include <db/db.hpp>
#include <realm/realmprotocol.hpp>
#include <realm/zoneinstance.hpp>
#include <utility/hashutils.hpp>
#include <utility/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>
#include <utility/rapidjsonutils.hpp>
#include <websocketpp_configuration.hpp>
#include <world/zonemanager.hpp>

#include <reflection/basic_templates.hpp>
#include <reflection/magic.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <websocketpp/server.hpp>

// TODO: preallocate message structs (both C->S & S->C) to reuse buffers in std::string, vector etc.
// Maybe use fixed-size buffers in some cases

namespace agdg {
	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	class RealmSession;
	class RealmServer;

	typedef websocketpp::server<configuration<RealmSession>> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

	// TODO: get completely rid of streambuf (and ostream) and use more efficient direct access
	class ByteVectorBuf : public std::streambuf {
	public:
		virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override {
			size_t pos = bytes.size();
			bytes.resize(pos + n);
			memcpy(&bytes[pos], s, n);
			return n;
		}

		virtual int_type overflow(int_type c) {
			size_t pos = bytes.size();
			bytes.resize(pos + 1);
			bytes[pos] = c;
			return c;
		}

		void Clear() {
			bytes.resize(0);
		}

		const uint8_t* GetBytes() { return &bytes[0]; }
		const size_t GetLength() { return bytes.size(); }

	private:
		std::vector<uint8_t> bytes;
	};

	class RealmSession : private ZoneInstanceListener {
	public:
		RealmSession(RealmServer* server, connection_ptr con) : server(server), con(con) {}

		void close();
		void on_message(int id, const uint8_t* buffer, size_t length);

		void handle(CChatSay& msg);
		void handle(CEnterWorld& msg);
		void handle(CHello& msg);
		void handle(CPlayerMovement& msg);
		void handle(CZoneLoaded& msg);

		template <typename T>
		void send(T& message) {
			//messageBuffer.Clear();
			messageBuf.Clear();
			std::ostream os(&messageBuf);

			if (message.Encode(os))
				con->send(messageBuf.GetBytes(), messageBuf.GetLength());
		}

	private:
		virtual void on_chat(int eid, const std::string& text) override;
		virtual void on_entity_despawn(int eid) override;
		virtual void on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) override;
		virtual void on_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) override;

		RealmServer* server;
		connection_ptr con;

		ZoneInstance* inst = nullptr;
		PlayerCharacter* pc = nullptr;
		Entity* player_entity = nullptr;
		int player_eid = 0;

		bool tokenValidated = false;
		bool trusted = false;

		//ByteVectorStream messageBuffer;
		ByteVectorBuf messageBuf;
	};

	class RealmServer : public IRealmServer {
	public:
		RealmServer(const std::string& serviceName, const rapidjson::Value& d) {
			configure(*this, d);

			content_mgr.reset(IContentManager::Create());
			zone_mgr.reset(IZoneManager::Create(content_mgr.get()));
			zone_mgr->ReloadContent();

			IZone* test_zone = zone_mgr->GetZoneById("test_zone");
			world_zone.reset(ZoneInstance::Create(test_zone));

			//db.reset(IJsonRealmDB::Create("db/" + serviceName + "/"));
		}

		virtual void Init() override {
			server.init_asio();

			server.clear_access_channels(websocketpp::log::alevel::all);

			server.set_open_handler(bind(&RealmServer::on_open, this, _1));
			server.set_close_handler(bind(&RealmServer::on_close, this, _1));

			server.listen(listenPort);
			server.start_accept();
		}

		virtual void Start() override {
			thread = std::thread(&RealmServer::Run, this);
		}

		virtual void Stop() override {
			// FIXME: end all connections

			server.stop();

			thread.join();
		}

		//IContentManager* get_content_manager() { return content_mgr.get(); }
		//IZoneManager* GetZoneManager() { return zoneMgr.get(); }
		ZoneInstance* get_world_zone() { return world_zone.get(); }

	private:
		void on_close(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			con->instance->close();
			con->instance.reset();
		}

		void on_message_received(RealmSession* session, connection_hdl hdl, Server::message_ptr msg) {
			auto data = (const uint8_t*) msg->get_payload().c_str();
			size_t length = msg->get_payload().size();

			if (length >= 1)
				session->on_message(data[0], data + 1, length - 1);
		}

		void on_open(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			con->instance.reset(new RealmSession(this, con));

			con->set_message_handler(bind(&RealmServer::on_message_received, this, con->instance.get(), _1, _2));
		}

		void Run() {
			server.run();
		}

		std::thread thread;
		Server server;

		//std::unique_ptr<ILoginDB> db;

		std::unique_ptr<IContentManager> content_mgr;
		std::unique_ptr<IZoneManager> zone_mgr;

		std::unique_ptr<ZoneInstance> world_zone;

		std::string serverName;
		int listenPort;

		REFL_BEGIN("RealmServer", 1)
			REFL_MUST_CONFIG(listenPort)
		REFL_END
	};

	void RealmSession::close() {
		if (player_entity) {
			inst->unsubscribe(this);
			inst->remove_entity(player_eid);

			// FIXME: proper ownership management
			delete player_entity;
			player_entity = nullptr;
		}
	}

	void RealmSession::handle(CChatSay& msg) {
		if (!inst || !player_eid)
			return;

		if (!trusted) {
			// FIXME: filter out HTML
		}

		g_log->Log("<admin> %s", msg.text.c_str());
		inst->broadcast_chat(player_eid, msg.text);
	}

	void RealmSession::handle(CHello& msg) {
		// FIXME: validate token (asynchronously?)
		tokenValidated = true;

		SHello reply = { { "TestChar" } };
		send(reply);
	}

	void RealmSession::handle(CEnterWorld& msg) {
		//g_log->Log("character %s entering world", msg.characterName.c_str());
		
		if (inst != nullptr)
			return;

		pc = new PlayerCharacter;

		// FIXME: can be NULL etc.
		inst = server->get_world_zone();
		auto zone = inst->get_zone();

		SLoadZone reply = { zone->GetName(), zone->GetHash() };
		send(reply);
	}

	void RealmSession::handle(CPlayerMovement& msg) {
		// FIXME: validate movement

		if (!inst || !player_entity)
			return;

		player_entity->set_pos_dir_velocity(msg.pos, msg.dir, msg.velocity);
		inst->broadcast_entity_update(player_eid, msg.pos, msg.dir, msg.velocity);
	}

	void RealmSession::handle(CZoneLoaded& msg) {
		if (!inst || !pc)
			return;

		SZoneState reply;

		// create player entity
		player_entity = Entity::create_player_entity(pc);

		inst->iterate_entities([this, &reply](int eid, auto ent) {
			reply.entities.emplace_back();
			auto& ent_state = reply.entities.back();
			ent_state.eid = eid;
			ent_state.name = "<???>";		// FIXME: will each entity have a name?
			ent_state.flags = 0;			// FIXME
			ent_state.pos = ent->get_pos();
			ent_state.dir = ent->get_dir();
		});

		player_eid = inst->add_entity(player_entity);

		reply.playerEid = player_eid;
		reply.playerPos = player_entity->get_pos();
		reply.playerDir = player_entity->get_dir();
		send(reply);

		SChatSay hello{ 0, "<strong>Welcome to AGDG MMO.</strong>" };
		send(hello);

		inst->subscribe(this);
	}

	void RealmSession::on_chat(int eid, const std::string& text) {
		SChatSay msg{ eid, text };
		send(msg);
	}

	void RealmSession::on_entity_despawn(int eid) {
		SEntityDespawn msg{ eid };
		send(msg);
	}

	void RealmSession::on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) {
		SEntitySpawn msg;
		msg.entity.eid = eid;
		msg.entity.name = "<???>";		// FIXME: will each entity have a name?
		msg.entity.flags = 0;			// FIXME
		msg.entity.pos = pos;
		msg.entity.dir = dir;
		send(msg);
	}

	void RealmSession::on_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) {
		if (eid != player_eid) {
			SEntityUpdate msg;
			msg.eid = eid;
			msg.pos = pos;
			msg.dir = dir;
			msg.velocity = velocity;
			send(msg);
		}
	}

	void RealmSession::on_message(int id, const uint8_t* buffer, size_t length) {
		if (!tokenValidated && id != kCHello)
			return;

#define handle_message(name_) case k##name_: {\
	name_ msg;\
	if (!msg.Decode(buffer, length)) return;\
	this->handle(msg);\
	break;\
}

		switch (id) {
		handle_message(CChatSay)
		handle_message(CHello)
		handle_message(CEnterWorld)
		handle_message(CPlayerMovement)
		handle_message(CZoneLoaded)
		default:
			;
		}
	}

	IRealmServer* IRealmServer::Create(const std::string& serviceName, const rapidjson::Value& config) {
		return new RealmServer(serviceName, config);
	}
}

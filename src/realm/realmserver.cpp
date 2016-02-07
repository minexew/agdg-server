#include <realm/realmserver.hpp>

#include <agdg/config.hpp>
#include <db/db.hpp>
#include <realm/realmprotocol.hpp>
#include <realm/zoneinstance.hpp>
#include <tokenmanager.hpp>
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
	using namespace std::chrono;
	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	class RealmSession;
	class RealmServer;

	typedef websocketpp::server<configuration<RealmSession>> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

	static const auto kPingInterval = milliseconds(5000);
	static const auto kPingTimeout = milliseconds(1000);

	enum { kDefaultAssumedLatency = 200 };

	void escape_html(const std::string& data, std::string& output) {
		output.reserve(data.size() + data.size() / 4);

		for (size_t pos = 0; pos != data.size(); ++pos) {
			switch (data[pos]) {
			case '&':  output.append("&amp;");       break;
			case '\"': output.append("&quot;");      break;
			case '\'': output.append("&apos;");      break;
			case '<':  output.append("&lt;");        break;
			case '>':  output.append("&gt;");        break;
			default:   output.append(&data[pos], 1); break;
			}
		}
	}

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

	class Ping {
	public:
		void arrived();
		void update(RealmSession& session);

		auto get_measured_latency() const { return measured_latency; }

	private:
		enum class State { not_pinged, first_ping_in_progress, pinged, next_ping_in_progress };

		State state = State::not_pinged;
		steady_clock::time_point last_ping;
		milliseconds measured_latency;
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
		void handle(CPong& msg);
		void handle(CZoneLoaded& msg);

		template <typename T>
		void send(const T& message) {
			//messageBuffer.Clear();
			messageBuf.Clear();
			std::ostream os(&messageBuf);

			if (message.Encode(os))
				con->send(messageBuf.GetBytes(), messageBuf.GetLength());
		}

	private:
		virtual void on_chat(int eid, const std::string& text, bool html) override;
		virtual void on_entity_despawn(int eid) override;
		virtual void on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) override;
		virtual void on_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, int half_latency) override;

		RealmServer* server;
		connection_ptr con;

		Ping ping;
		int client_latency = kDefaultAssumedLatency;

		ZoneInstance* inst = nullptr;
		unique_ptr<PlayerCharacter> pc;
		unique_ptr<Entity> player_entity;
		int player_eid = 0;

		AccountSnapshot account_snapshot;
		bool tokenValidated = false;

		//ByteVectorStream messageBuffer;
		ByteVectorBuf messageBuf;
	};

	class RealmServer : public IRealmServer {
	public:
		RealmServer(const std::string& serviceName, const rapidjson::Value& d) {
			configure(*this, d);

			content_mgr = IContentManager::Create();
			zone_mgr = IZoneManager::Create(content_mgr.get());
			zone_mgr->ReloadContent();

			IZone* test_zone = zone_mgr->GetZoneById("test_zone");
			world_zone = ZoneInstance::Create(test_zone);

			//db.reset(IJsonRealmDB::Create("db/" + serviceName + "/"));
		}

		virtual void Init() override {
			server.init_asio();
			server.set_reuse_addr(true);

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

		// FIXME: implement
		virtual void close_server(const std::string& message) override {}
		virtual void reopen_server() override {}

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

			con->instance = make_unique<RealmSession>(this, con);

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

	void Ping::arrived() {
		measured_latency = duration_cast<milliseconds>(steady_clock::now() - last_ping);
		state = State::pinged;
	}

	void Ping::update(RealmSession& session) {
		auto current_clock = steady_clock::now();

		switch (state) {
		case State::not_pinged:
		case State::pinged:
			if (state == State::not_pinged || current_clock > last_ping + kPingInterval) {
				session.send(SPing{});
				last_ping = current_clock;

				state = (state == State::not_pinged ? State::first_ping_in_progress : State::next_ping_in_progress);
			}
			break;

		case State::first_ping_in_progress:
		case State::next_ping_in_progress:
			if (current_clock > last_ping + kPingTimeout) {
				// TODO
			}
			break;
		}
	}

	void RealmSession::close() {
		if (player_entity) {
			inst->unsubscribe(this);
			inst->remove_entity(player_eid);
			inst->broadcast_chat(0, "<b>" + pc->get_name() + "</b> left.", true);

			// CHECKME: ownership management OK?
			player_entity.reset();
		}
	}

	void RealmSession::handle(CChatSay& msg) {
		if (!inst || !pc || !player_eid)
			return;

		g_log->Log("<%s> %s", pc->get_name().c_str(), msg.text.c_str());
		inst->broadcast_chat(player_eid, msg.text, account_snapshot.trusted);
	}

	void RealmSession::handle(CHello& msg) {
		// FIXME: validate token (asynchronously?)
		if (!g_token_manager.validate_token(msg.token, account_snapshot))
			return;

		tokenValidated = true;

		SHello reply = { { account_snapshot.name } };
		send(reply);
	}

	void RealmSession::handle(CEnterWorld& msg) {
		//g_log->Log("character %s entering world", msg.characterName.c_str());
		
		if (inst != nullptr)
			return;

		if (msg.characterName != account_snapshot.name)
			return;

		// FIXME: load actual character data etc.
		pc = make_unique<PlayerCharacter>(msg.characterName);

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

		player_entity->set_pos_dir(msg.pos, msg.dir);
		inst->broadcast_entity_update(player_eid, msg.pos, msg.dir, client_latency / 2);

		ping.update(*this);
	}

	void RealmSession::handle(CPong& msg) {
		ping.arrived();
		client_latency = (int)ping.get_measured_latency().count();

		g_log->Log("%s has a latency of %dms", pc->get_name().c_str(), client_latency);
	}

	void RealmSession::handle(CZoneLoaded& msg) {
		if (!inst || !pc)
			return;

		SZoneState reply;

		// create player entity
		player_entity = Entity::create_player_entity(pc.get());

		inst->iterate_entities([this, &reply](int eid, auto entity) {
			reply.entities.emplace_back();
			auto& ent_state = reply.entities.back();
			ent_state.eid = eid;
			ent_state.name = entity->get_name();
			ent_state.flags = 0;			// FIXME
			ent_state.pos = entity->get_pos();
			ent_state.dir = entity->get_dir();
		});

		player_eid = inst->add_entity(player_entity.get());

		reply.playerEid = player_eid;
		reply.playerName = pc->get_name();
		reply.playerPos = player_entity->get_pos();
		reply.playerDir = player_entity->get_dir();
		send(reply);

		SChatSay hello{ 0, "<strong>Welcome to AGDG MMO.</strong>", true };
		send(hello);

		inst->broadcast_chat(0, "<strong>" + pc->get_name() + "</strong> joined.", true);
		inst->subscribe(this);
	}

	void RealmSession::on_chat(int eid, const std::string& text, bool html) {
		SChatSay msg{ eid, text, html };
		send(msg);
	}

	void RealmSession::on_entity_despawn(int eid) {
		SEntityDespawn msg{ eid };
		send(msg);
	}

	void RealmSession::on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) {
		SEntitySpawn msg;
		msg.entity.eid = eid;
		msg.entity.name = entity->get_name();
		msg.entity.flags = 0;			// FIXME
		msg.entity.pos = pos;
		msg.entity.dir = dir;
		send(msg);
	}

	void RealmSession::on_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, int half_latency) {
		if (eid != player_eid) {
			SEntityUpdate msg;
			msg.eid = eid;
			msg.pos = pos;
			msg.dir = dir;
			msg.latency = half_latency + this->client_latency / 2;
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
		handle_message(CPong)
		handle_message(CZoneLoaded)
		default:
			;
		}
	}

	unique_ptr<IRealmServer> IRealmServer::Create(const std::string& serviceName, const rapidjson::Value& config) {
		return make_unique<RealmServer>(serviceName, config);
	}
}

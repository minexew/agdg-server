#include <realm/realmsession.hpp>

#include <agdg/logging.hpp>
#include <realm/realm.hpp>

namespace agdg {
	PlayerEntityImpl::PlayerEntityImpl(Realm* realm, PlayerCharacter* pc, RealmSession* session) : Entity(true), pc(pc), session(session), pos{{0, 0, 0.5f}}, dir{} {
		dom = realm->get_dom()->create_entity_dom(this);
	}

	void PlayerEntityImpl::on_entity_did_say(Entity* entity, const std::string& message, bool html) {
		session->on_entity_did_say(entity, message, html);
	}

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
				session.queue(SPing{});
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

	void RealmSession::flush_queue() {
		con->send(&send_queue[0], send_queue.size());
		send_queue.resize(0);
	}

	void RealmSession::handle(CChatSay& msg) {
		if (!inst || !pc || !player_entity)
			return;

		g_log->info("<%s> %s", pc->get_name().c_str(), msg.text.c_str());
		player_entity->say(msg.text, account_snapshot.trusted);
	}

	void RealmSession::handle(CHello& msg) {
		// FIXME: validate token (asynchronously?)
		if (!g_token_manager.validate_token(msg.token, account_snapshot))
			return;

		tokenValidated = true;

		SHello reply = { { account_snapshot.name } };
		queue(reply);
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

		SLoadZone reply = { zone->get_name(), zone->get_hash() };
		queue(reply);
	}

	void RealmSession::handle(CPlayerMovement& msg) {
		// FIXME: validate movement

		if (!inst || !player_entity)
			return;

		player_entity->set_pos_dir(msg.pos, msg.dir);
		inst->broadcast_entity_update(player_entity.get(), msg.pos, msg.dir, client_latency / 2);

		ping.update(*this);
	}

	void RealmSession::handle(CPong& msg) {
		ping.arrived();
		client_latency = (int)ping.get_measured_latency().count();

		g_log->info("%s has a latency of %dms", pc->get_name().c_str(), client_latency);
	}

	void RealmSession::handle(CZoneLoaded& msg) {
		if (!inst || !pc)
			return;

		SZoneState reply;

		// create player entity
		//player_entity = Entity::create_player_entity(realm, pc.get());
		player_entity = make_unique<PlayerEntityImpl>(realm, pc.get(), this);

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
		queue(reply);

		SChatSay hello{ 0, "<strong>Welcome to AGDG MMO.</strong>", true };
		queue(hello);

		inst->broadcast_chat(0, "<strong>" + pc->get_name() + "</strong> joined.", true);
		inst->subscribe(this);

		inst->get_dom()->on_player_did_enter(player_entity.get());
	}

	void RealmSession::handle_command(int code, int cookie, const uint8_t* payload, size_t payload_length) {
		if (!tokenValidated && code != CHello::code)
			return;

#define HANDLE_COMMAND(name_) case name_::code: {\
	name_ msg;\
	if (!msg.decode(payload, payload_length)) return;\
	this->handle(msg);\
	break;\
}

		switch (code) {
		HANDLE_COMMAND(CChatSay);
		HANDLE_COMMAND(CHello);
		HANDLE_COMMAND(CEnterWorld);
		HANDLE_COMMAND(CPlayerMovement);
		HANDLE_COMMAND(CPong);
		HANDLE_COMMAND(CZoneLoaded);
		default:
			;
		}
	}

	void RealmSession::on_entity_did_say(Entity* entity, const std::string& message, bool html) {
		SChatSay msg{ entity ? entity->get_eid() : 0, message, html };
		queue(msg);
	}

	void RealmSession::on_entity_despawn(int eid) {
		SEntityDespawn msg{ eid };
		queue(msg);
	}

	void RealmSession::on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) {
		SEntitySpawn msg;
		msg.entity.eid = eid;
		msg.entity.name = entity->get_name();
		msg.entity.flags = 0;			// FIXME
		msg.entity.pos = pos;
		msg.entity.dir = dir;
		queue(msg);
	}

	void RealmSession::on_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) {
		if (entity != player_entity.get()) {
			SEntityUpdate msg;
			msg.eid = entity->get_eid();
			msg.pos = pos;
			msg.dir = dir;
			msg.latency = half_latency + this->client_latency / 2;
			queue(msg);
		}
	}

	void RealmSession::on_message(const uint8_t* message, size_t length) {
		while (length) {
			uint8_t code, cookie;
			uint16_t payload_length;

			if (!read_command_header(message, length, code, cookie, payload_length)
					|| payload_length > length) {
				set_command_error_flag();
				return;
			}

			handle_command(code, cookie, message, payload_length);

			message += payload_length;
			length -= payload_length;
		}
	}

	void RealmSession::on_tick() {
		if (player_entity)
			player_entity->on_tick();
	}

	void RealmSession::set_command_error_flag() {
		if (flag_command_error == 0) {
			auto& socket = con->get_raw_socket();
			auto hostname = socket.remote_endpoint().address().to_string();

			g_log->warning("command error (%s)", hostname.c_str());
		}

		if (++flag_command_error == 100) {
			drop_connection();
		}
	}
}

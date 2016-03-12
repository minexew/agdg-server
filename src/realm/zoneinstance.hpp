#pragma once

#include <agdg/types.hpp>
#include <world/zonemanager.hpp>

namespace agdg {
	class EntityDOM;
	class Realm;
	class RealmDOM;
	class ZoneInstanceDOM;

	class PlayerCharacter {
	public:
		PlayerCharacter(const std::string& name) : name(name) {}

		const std::string& get_name() { return name; }

	private:
		std::string name;
	};

	class Entity {
	public:
		static unique_ptr<Entity> create_player_entity(Realm* realm, PlayerCharacter* pc);
		virtual ~Entity() {}

		virtual int get_eid() = 0;
		virtual const std::string& get_name() = 0;

		virtual EntityDOM* get_dom() = 0;

		virtual const glm::vec3& get_dir() = 0;
		virtual const glm::vec3& get_pos() = 0;

		virtual void set_eid(int eid) = 0;
		virtual void set_pos_dir(const glm::vec3& pos, const glm::vec3& dir) = 0;
	};

	class ZoneInstanceListener {
	public:
		virtual void on_chat(Entity* entity, const std::string& text, bool html) = 0;
		virtual void on_entity_despawn(int eid) = 0;
		virtual void on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) = 0;
		virtual void on_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) = 0;
	};

	class ZoneInstance {
	public:
		static unique_ptr<ZoneInstance> create(Realm* realm, IZone* zone);

		virtual int add_entity(Entity* entity) = 0;
		virtual void remove_entity(int eid) = 0;

		virtual void subscribe(ZoneInstanceListener* listener) = 0;
		virtual void unsubscribe(ZoneInstanceListener* listener) = 0;

		virtual void broadcast_chat(Entity* entity, const std::string& text, bool html) = 0;
		virtual void broadcast_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) = 0;

		virtual ZoneInstanceDOM* get_dom() = 0;

		virtual int get_id() = 0;
		virtual IZone* get_zone() = 0;
		virtual void iterate_entities(std::function<void(int eid, Entity* entity)> callback) = 0;
	};
}

#pragma once

#include <agdg/types.hpp>
#include <world/zonemanager.hpp>

namespace agdg {
	class PlayerCharacter {
	public:
		PlayerCharacter(const std::string& name) : name(name) {}

		const std::string& get_name() { return name; }

	private:
		std::string name;
	};

	class Entity {
	public:
		static Entity* create_player_entity(PlayerCharacter* pc);

		virtual const std::string& get_name() = 0;

		virtual const glm::vec3& get_dir() = 0;
		virtual const glm::vec3& get_pos() = 0;

		virtual void set_pos_dir_velocity(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) = 0;
	};

	class ZoneInstanceListener {
	public:
		virtual void on_chat(int eid, const std::string& text) = 0;
		virtual void on_entity_despawn(int eid) = 0;
		virtual void on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) = 0;
		virtual void on_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) = 0;
	};

	class ZoneInstance {
	public:
		static ZoneInstance* Create(IZone* zone);

		virtual int add_entity(Entity* entity) = 0;
		virtual void remove_entity(int eid) = 0;

		virtual void subscribe(ZoneInstanceListener* listener) = 0;
		virtual void unsubscribe(ZoneInstanceListener* listener) = 0;

		virtual void broadcast_chat(int eid, const std::string& text) = 0;
		virtual void broadcast_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) = 0;

		virtual IZone* get_zone() = 0;
		virtual void iterate_entities(std::function<void(int eid, Entity* entity)> callback) = 0;
	};
}

#pragma once

#include <agdg/types.hpp>

namespace agdg {
	class EntityDOM;
	class Realm;
	class ZoneInstance;

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

		int get_eid() { return eid; }
		virtual const std::string& get_name() = 0;
		ZoneInstance* get_zone_instance() { return zone_instance; }

		virtual EntityDOM* get_dom() = 0;

		virtual const glm::vec3& get_dir() = 0;
		virtual const glm::vec3& get_pos() = 0;

		void set_eid(int eid) { this->eid = eid; }
		virtual void set_pos_dir(const glm::vec3& pos, const glm::vec3& dir) = 0;
		void set_zone_instance(ZoneInstance* instance) { this->zone_instance = instance; }

		void say(const std::string& message, bool html);

	protected:
		int eid = -1;
		ZoneInstance* zone_instance = nullptr;
	};
}

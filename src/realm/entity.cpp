#include <realm/zoneinstance.hpp>

namespace agdg {
	class EntityImpl : public Entity {
	public:
		virtual const glm::vec3& get_dir() { return dir; }
		virtual const glm::vec3& get_pos() { return pos; }

		virtual void set_pos_dir_velocity(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) override {
			this->pos = pos;
			this->dir = dir;
			this->velocity = velocity;
		}

	private:
		glm::vec3 pos, dir, velocity;
	};

	Entity* Entity::create_player_entity(PlayerCharacter* pc) {
		return new EntityImpl();
	}
}

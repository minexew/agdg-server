#include <realm/zoneinstance.hpp>

namespace agdg {
	class EntityImpl : public Entity {
	public:
		EntityImpl(PlayerCharacter* pc) : pc(pc), pos{0, 0, 0.5f}, dir{}, velocity{} {}

		virtual const std::string& get_name() { return pc->get_name(); }

		virtual const glm::vec3& get_dir() { return dir; }
		virtual const glm::vec3& get_pos() { return pos; }

		virtual void set_pos_dir_velocity(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) override {
			this->pos = pos;
			this->dir = dir;
			this->velocity = velocity;
		}

	private:
		PlayerCharacter* pc;

		glm::vec3 pos, dir, velocity;
	};

	Entity* Entity::create_player_entity(PlayerCharacter* pc) {
		return new EntityImpl(pc);
	}
}

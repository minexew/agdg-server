#include <realm/zoneinstance.hpp>

namespace agdg {
	class EntityImpl : public Entity {
	public:
		EntityImpl(PlayerCharacter* pc) : pc(pc), pos{0, 0, 0.5f}, dir{} {}

		virtual const std::string& get_name() override { return pc->get_name(); }

		virtual const glm::vec3& get_dir() override { return dir; }
		virtual const glm::vec3& get_pos() override { return pos; }

		virtual void set_pos_dir(const glm::vec3& pos, const glm::vec3& dir) override {
			this->pos = pos;
			this->dir = dir;
		}

	private:
		PlayerCharacter* pc;

		glm::vec3 pos, dir;
	};

	unique_ptr<Entity> Entity::create_player_entity(PlayerCharacter* pc) {
		return make_unique<EntityImpl>(pc);
	}
}

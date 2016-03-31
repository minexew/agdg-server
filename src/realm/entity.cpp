#include <realm/entity.hpp>

#include <agdg/logging.hpp>
#include <realm/realm.hpp>
#include <realm/zoneinstance.hpp>
#include <scripting/realm_dom.hpp>

namespace agdg {
	class EntityImpl : public Entity {
	public:
		EntityImpl(Realm* realm, PlayerCharacter* pc) : pc(pc), pos{{0, 0, 0.5f}}, dir{} {
			dom = realm->get_dom()->create_entity_dom(this);
		}

		virtual const std::string& get_name() override { return pc->get_name(); }

		virtual EntityDOM* get_dom() override { return dom.get(); }

		virtual const glm::vec3& get_dir() override { return dir; }
		virtual const glm::vec3& get_pos() override { return pos; }

		virtual void set_pos_dir(const glm::vec3& pos, const glm::vec3& dir) override {
			this->pos = pos;
			this->dir = dir;
		}

	private:
		PlayerCharacter* pc;

		glm::vec3 pos, dir;

		unique_ptr<EntityDOM> dom;
	};

	void Entity::say(const std::string& message, bool html) {
		if (zone_instance)
			zone_instance->broadcast_chat(this, message, html);
		else
			g_log->warning("entity '%s' not in instance - tried to say '%s'", get_name().c_str(), message.c_str());
	}

	unique_ptr<Entity> Entity::create_player_entity(Realm* realm, PlayerCharacter* pc) {
		return make_unique<EntityImpl>(realm, pc);
	}
}

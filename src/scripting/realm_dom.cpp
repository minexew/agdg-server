#include <scripting/realm_dom.hpp>

#include <agdg/logging.hpp>
#include <realm/entity.hpp>
#include <realm/realm.hpp>
#include <realm/zoneinstance.hpp>
#include <utility/fileutils.hpp>
#include <v8scripting/bindingutils.hpp>

namespace agdg {
	class EntityDOMImpl : public EntityDOM {
	public:
	};

	class ZoneInstanceDOMImpl : public ZoneInstanceDOM {
	public:
		bool on_will_chat(Entity* entity, const std::string& message) override {
			return true;
		}

		void on_did_chat(Entity* entity, const std::string& message) override {}
		void on_player_did_enter(Entity *player) override {}
	};

	class RealmDOMImpl : public RealmDOM {
	public:
		RealmDOMImpl() {}

		unique_ptr<EntityDOM> create_entity_dom(Entity* entity) override {
			return make_unique<EntityDOMImpl>();
		}

		unique_ptr<ZoneInstanceDOM> create_zone_instance_dom(ZoneInstance* zone_instance) override {
			return make_unique<ZoneInstanceDOMImpl>();
		}

		void on_realm_init() override {}
		void on_tick() override {}

		void on_zone_instance_create(ZoneInstance* instance) override {}
	};

	unique_ptr<RealmDOM> RealmDOM::create(ScriptContext* ctx, Realm* realm) {
		return make_unique<RealmDOMImpl>();
	}
}

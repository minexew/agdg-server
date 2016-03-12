#pragma once

#include <realm/zoneinstance.hpp>
#include <v8scripting/v8scripthandler.hpp>

namespace agdg {
	class Realm;
	class V8Context;
	typedef V8Context ScriptContext;

	class EntityDOM {
	public:
		virtual ~EntityDOM() {}
	};

	class ZoneInstanceDOM {
	public:
		virtual ~ZoneInstanceDOM() {}

		virtual bool on_chat(Entity* entity, const std::string& message) = 0;
		//virtual void on_player_will_enter(Entity* player) = 0;
		virtual void on_player_has_entered(Entity* player) = 0;
	};

	class RealmDOM {
	public:
		static unique_ptr<RealmDOM> create(ScriptContext* ctx, Realm* realm);
		virtual ~RealmDOM() {}

		virtual unique_ptr<EntityDOM> create_entity_dom(Entity* entity) = 0;
		virtual unique_ptr<ZoneInstanceDOM> create_zone_instance_dom(ZoneInstance* zone_instance) = 0;

		virtual void on_realm_init() = 0;
		virtual void on_zone_instance_create(ZoneInstance* instance) = 0;
	};
}

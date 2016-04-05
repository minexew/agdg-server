#pragma once

#include <agdg/types.hpp>
#include <scripting/realm_dom.hpp>

namespace agdg {
	class Realm {
	public:
		static unique_ptr<Realm> create();
		virtual ~Realm() {}

		virtual void on_realm_init() = 0;
		virtual void on_tick() = 0;

		virtual RealmDOM* get_dom() = 0;
	};
}

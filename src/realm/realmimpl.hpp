#pragma once

#include <realm/realm.hpp>

#ifdef WITH_V8
#include <v8scripting/v8scripthandler.hpp>
#endif

namespace agdg {
	class RealmImpl : public Realm {
	public:
		RealmImpl();
		~RealmImpl();

		RealmDOM* get_dom() override { return dom.get(); }
		void on_realm_init() override { dom->on_realm_init(); }
		void on_tick() override { dom->on_tick(); }

	private:
#ifdef WITH_V8
		std::unique_ptr<V8ScriptHandler> v8handler;
		std::unique_ptr<V8Context> v8context;
#endif

		// Careful about the destruction order w.r.t. v8context
		unique_ptr<RealmDOM> dom;
	};
}

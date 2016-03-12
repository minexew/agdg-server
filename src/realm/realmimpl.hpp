#pragma once

#include <realm/realm.hpp>

#ifdef WITH_V8
#include <v8scripting/v8scripthandler.hpp>
#endif

namespace agdg {
	class RealmImpl : public Realm {
	public:
		RealmImpl() { init_v8(); }
		~RealmImpl();

		virtual RealmDOM* get_dom() override { return dom.get(); }
		virtual void on_realm_init() override { dom->on_realm_init(); }

	private:
		unique_ptr<RealmDOM> dom;

#ifdef WITH_V8
		void init_v8();
		std::unique_ptr<V8ScriptHandler> v8handler;
		std::unique_ptr<V8Context> v8context;
#endif
	};
}

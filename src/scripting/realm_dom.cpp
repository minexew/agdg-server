#include <scripting/realm_dom.hpp>

#include <realm/realm.hpp>
#include <v8scripting/bindingutils.hpp>

namespace agdg {
	// TODO: simplify Inst-Template entanglement
	// TODO: try to remove <Inst> in register_callback
	// TODO: split up Realm DOM!

	class EntityDOMTemplate;
	class ZoneInstanceDOMTemplate;

	enum class EntityCB {
		dummy_,
		max,
	};

	enum RealmCB {
		realm_init,
		zone_instance_create,
		max,
	};

	enum class ZoneInstanceCB {
		chat,
		player_has_entered,
		//zoneinstance_player_will_enter,
		max,
	};

	class EntityDOMImpl : public EntityDOM, public V8CallbackCollector<EntityCB> {
	public:
		EntityDOMImpl(EntityDOMTemplate& tpl, Entity* entity)
				;/*: V8CallbackCollector(tpl) {
			tpl->get_instance(weak, entity);
		}*/

		v8::Local<v8::Object> get_instance() { return v8::Local<v8::Object>::New(tpl->isolate, weak.ref->v8_instance); }

		V8WeakRefOwner<Entity> weak;
	};

	class ZoneInstanceDOMImpl : public ZoneInstanceDOM, public V8CallbackCollector<ZoneInstanceCB> {
	public:
		ZoneInstanceDOMImpl(ZoneInstanceDOMTemplate& tpl, ZoneInstance* zone_instance)
				;/*: V8CallbackCollector(tpl) {
			tpl->get_instance(weak, zone_instance);
		}*/

		static bool bool_and(bool a, bool b) { return a && b; }

		virtual bool on_chat(Entity* entity, const std::string& message) override {
			if (callable(ZoneInstanceCB::chat)) {
				V8Scope scope(tpl->ctx);
				auto entity_obj = entity
						? v8::Local<v8::Value>(static_cast<EntityDOMImpl*>(entity->get_dom())->get_instance())
						: v8::Local<v8::Value>(v8::Null(tpl->isolate));
				return call<bool, bool_and>(ZoneInstanceCB::chat, entity_obj, message);
			}
			else
				return true;
		}

		//virtual void on_player_will_enter(Entity* player) override {}

		virtual void on_player_has_entered(Entity* player) override {
			if (callable(ZoneInstanceCB::player_has_entered)) {
				V8Scope scope(tpl->ctx);
				auto player_obj = static_cast<EntityDOMImpl*>(player->get_dom())->get_instance();
				call(ZoneInstanceCB::player_has_entered, player_obj);
			}
		}

		static void broadcast_chat(ZoneInstance* zone, const v8::FunctionCallbackInfo<v8::Value>& info) {
			v8::HandleScope handle_scope(info.GetIsolate());

			v8::String::Utf8Value message(info[1]);
			if (!*message) return;
			bool html = false;
			V8Utils::from_js_value(info[2], html);
			zone->broadcast_chat(nullptr, *message, html);
		}

		v8::Local<v8::Object> get_instance() { return v8::Local<v8::Object>::New(tpl->isolate, weak.ref->v8_instance); }

		V8WeakRefOwner<ZoneInstance> weak;
	};

	class EntityDOMTemplate : public V8TypedClassTemplate<Entity> {
	public:
		EntityDOMTemplate(V8Context* ctx) : V8TypedClassTemplate(ctx) {
			V8Scope scope(ctx);

			set_property_getter<int, &Entity::get_eid>("eid");
			set_property_getter<const std::string&, &Entity::get_name>("name");
		}
	};

	class ZoneInstanceDOMTemplate : public V8ClassTemplateWithCallbacks<ZoneInstance> {
	public:
		ZoneInstanceDOMTemplate(V8Context* ctx) : V8ClassTemplateWithCallbacks(ctx) {
			V8Scope scope(ctx);

			register_callback<ZoneInstanceDOMImpl>(ZoneInstanceCB::chat, "onChat");
			register_callback<ZoneInstanceDOMImpl>(ZoneInstanceCB::player_has_entered , "onPlayerHasEntered");
			//register_hook(zoneinstance_player_will_enter, "onPlayerWillEnter");

			set_method<ZoneInstanceDOMImpl::broadcast_chat>("broadcastChat");
			set_property_getter<int, &ZoneInstance::get_id>("id");
		}
	};

	class RealmDOMImpl : public RealmDOM, public V8ClassTemplateWithCallbacks<Realm>,
			public V8CallbackCollector<RealmCB> {
	public:
		RealmDOMImpl(V8Context* ctx, Realm* realm)
				: V8ClassTemplateWithCallbacks(ctx),
				  V8CallbackCollector(this),
				entity_tpl(ctx),
				zone_instance_tpl(ctx) {
			V8Scope scope(ctx);

			register_callback<RealmDOMImpl>(RealmCB::realm_init, "onRealmInit");
			register_callback<RealmDOMImpl>(RealmCB::zone_instance_create, "onZoneInstanceCreate");

			auto context = ctx->get_v8_context();
			v8::Local<v8::Object> global = context->Global();
			global->Set(v8::String::NewFromUtf8(isolate, "realm"), get_instance(weak, realm));
		}

		virtual unique_ptr<EntityDOM> create_entity_dom(Entity* entity) override {
			return make_unique<EntityDOMImpl>(entity_tpl, entity);
		}

		virtual unique_ptr<ZoneInstanceDOM> create_zone_instance_dom(ZoneInstance* zone_instance) override {
			return make_unique<ZoneInstanceDOMImpl>(zone_instance_tpl, zone_instance);
		}

		virtual void on_realm_init() override {
			call(RealmCB::realm_init);
		}

		virtual void on_zone_instance_create(ZoneInstance* instance) override;

		EntityDOMTemplate entity_tpl;
		ZoneInstanceDOMTemplate zone_instance_tpl;
		V8WeakRefOwner<Realm> weak;
	};

	EntityDOMImpl::EntityDOMImpl(EntityDOMTemplate& tpl, Entity* entity) : V8CallbackCollector(&tpl) {
		V8Scope scope(tpl.ctx);		// FIXME: ugh...
		tpl.get_instance(weak, entity);
	}

	ZoneInstanceDOMImpl::ZoneInstanceDOMImpl(ZoneInstanceDOMTemplate& tpl, ZoneInstance* zone_instance)
			: V8CallbackCollector(&tpl) {
		V8Scope scope(tpl.ctx);
		tpl.get_instance(weak, zone_instance);
	}

	void RealmDOMImpl::on_zone_instance_create(ZoneInstance* instance) {
		if (callable(RealmCB::zone_instance_create)) {
			V8Scope scope(ctx);
			auto instance_obj = static_cast<ZoneInstanceDOMImpl*>(instance->get_dom())->get_instance();
			call(RealmCB::zone_instance_create , instance_obj);
		}
	}

	unique_ptr<RealmDOM> RealmDOM::create(ScriptContext* ctx, Realm* realm) {
		return make_unique<RealmDOMImpl>(ctx, realm);
	}
}

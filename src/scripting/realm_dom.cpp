#include <scripting/realm_dom.hpp>

#include <realm/entity.hpp>
#include <realm/realm.hpp>
#include <realm/zoneinstance.hpp>
#include <utility/fileutils.hpp>
#include <v8scripting/bindingutils.hpp>

namespace agdg {
	// TODO: try to remove <Inst> in register_callback
	// TODO: try to move enums into classes
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
		will_chat,
		did_chat,
		player_did_enter,
		//zoneinstance_player_will_enter,
		max,
	};

	class TestEntityImpl : public Entity {
	public:
		TestEntityImpl(Realm* realm, const std::string& name, const glm::vec3& pos) : name(name), pos(pos), dir{} {
			dom = realm->get_dom()->create_entity_dom(this);
		}

		virtual const std::string& get_name() override { return name; }

		virtual EntityDOM* get_dom() override { return dom.get(); }

		virtual const glm::vec3& get_dir() override { return dir; }
		virtual const glm::vec3& get_pos() override { return pos; }

		virtual void set_pos_dir(const glm::vec3& pos, const glm::vec3& dir) override {
			this->pos = pos;
			this->dir = dir;
		}

	private:
		std::string name;

		glm::vec3 pos, dir;

		unique_ptr<EntityDOM> dom;
	};

	class EntityDOMImpl : public EntityDOM, public V8CallbackCollector<EntityCB> {
	public:
		EntityDOMImpl(V8TypedClassTemplate<Entity>* tpl, Entity* entity) : V8CallbackCollector(tpl), tpl(tpl), entity(entity) {}

		v8::Local<v8::Object> get_instance() { return tpl->get_instance(weak, entity); }

		static void say(Entity* entity, const v8::FunctionCallbackInfo<v8::Value>& info) {
			v8::HandleScope handle_scope(info.GetIsolate());	// FIXME: argument checking

			std::string message;
			V8Utils::from_js_value(info[0], message);
			bool html = false;
			V8Utils::from_js_value(info[1], html);
			entity->say(message, html);
		}

		V8TypedClassTemplate<Entity>* tpl;
		Entity* entity;
		V8WeakRefOwner<Entity> weak;
	};

	class ZoneInstanceDOMImpl : public ZoneInstanceDOM, public V8CallbackCollector<ZoneInstanceCB> {
	public:
		ZoneInstanceDOMImpl(V8TypedClassTemplate<ZoneInstance>* tpl, ZoneInstance* zi) : V8CallbackCollector(tpl), tpl(tpl), zi(zi) {}

		static bool bool_and(bool a, bool b) { return a && b; }

		virtual bool on_will_chat(Entity* entity, const std::string& message) override {
			if (callable(ZoneInstanceCB::will_chat)) {
				V8Scope scope(tpl->ctx);
				auto entity_obj = entity
						? v8::Local<v8::Value>(static_cast<EntityDOMImpl*>(entity->get_dom())->get_instance())
						: v8::Local<v8::Value>(v8::Null(tpl->isolate));
				return call_with_return<bool, bool_and>(ZoneInstanceCB::will_chat, entity_obj, message);
			}
			else
				return true;
		}

		virtual void on_did_chat(Entity* entity, const std::string& message) override {
			if (callable(ZoneInstanceCB::did_chat)) {
				V8Scope scope(tpl->ctx);
				auto entity_obj = entity
								  ? v8::Local<v8::Value>(static_cast<EntityDOMImpl*>(entity->get_dom())->get_instance())
								  : v8::Local<v8::Value>(v8::Null(tpl->isolate));
				return call(ZoneInstanceCB::did_chat, entity_obj, message);
			}
		}

		virtual void on_player_did_enter(Entity *player) override {
			if (callable(ZoneInstanceCB::player_did_enter)) {
				V8Scope scope(tpl->ctx);
				auto player_obj = static_cast<EntityDOMImpl*>(player->get_dom())->get_instance();
				call(ZoneInstanceCB::player_did_enter, player_obj);
			}
		}

		static void broadcast_chat(ZoneInstance* instance, const v8::FunctionCallbackInfo<v8::Value>& info) {
			v8::HandleScope handle_scope(info.GetIsolate());	// FIXME: argument checking

			v8::String::Utf8Value message(info[1]);
			if (!*message) return;
			bool html = false;
			V8Utils::from_js_value(info[2], html);
			instance->broadcast_chat(nullptr, *message, html);
		}

		static void spawn_test_entity(ZoneInstance* instance, const v8::FunctionCallbackInfo<v8::Value>& info) {
			v8::HandleScope handle_scope(info.GetIsolate());	// FIXME: argument checking

			std::string name;
			glm::vec3 pos;

			if (!V8Utils::from_js_value(info[0], name)
					|| !V8Utils::from_js_value(info[1], pos))
				return;

			// FIXME: entity memory management
			auto entity = new TestEntityImpl(instance->get_realm(), name, pos);
			instance->add_entity(entity);

			auto entity_obj = static_cast<EntityDOMImpl*>(entity->get_dom())->get_instance();
			info.GetReturnValue().Set(V8Utils::to_js_value(info.GetIsolate(), entity_obj));
		}

		v8::Local<v8::Object> get_instance() { return tpl->get_instance(weak, zi); }

		V8TypedClassTemplate<ZoneInstance>* tpl;
		ZoneInstance* zi;
		V8WeakRefOwner<ZoneInstance> weak;
	};

	class EntityDOMTemplate : public V8TypedClassTemplate<Entity> {
	public:
		EntityDOMTemplate(V8Context* ctx) : V8TypedClassTemplate(ctx) {
			V8Scope scope(ctx);

			set_property_getter<int, &Entity::get_eid>("eid");
			set_property_getter<const std::string&, &Entity::get_name>("name");
			set_property_getter<const glm::vec3&, &Entity::get_pos>("pos");

			set_method<EntityDOMImpl::say>("say");
		}
	};

	class ZoneInstanceDOMTemplate : public V8ClassTemplateWithCallbacks<ZoneInstance> {
	public:
		ZoneInstanceDOMTemplate(V8Context* ctx) : V8ClassTemplateWithCallbacks(ctx) {
			V8Scope scope(ctx);

			register_callback<ZoneInstanceDOMImpl>(ZoneInstanceCB::will_chat, "willChat");
			register_callback<ZoneInstanceDOMImpl>(ZoneInstanceCB::did_chat, "didChat");
			register_callback<ZoneInstanceDOMImpl>(ZoneInstanceCB::player_did_enter , "playerDidEnter");
			//register_hook(zoneinstance_player_will_enter, "onPlayerWillEnter");

			set_property_getter<int, &ZoneInstance::get_id>("id");

			set_method<ZoneInstanceDOMImpl::broadcast_chat>("broadcastChat");
			set_method<ZoneInstanceDOMImpl::spawn_test_entity>("spawnTestEntity");
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

			set_method<std::string, RealmDOMImpl::loadFileAsString>("loadFileAsString");

			auto context = ctx->get_v8_context();
			v8::Local<v8::Object> global = context->Global();
			global->Set(v8::String::NewFromUtf8(isolate, "realm"), get_instance(weak, realm));
		}

		virtual unique_ptr<EntityDOM> create_entity_dom(Entity* entity) override {
			return make_unique<EntityDOMImpl>(&entity_tpl, entity);
		}

		virtual unique_ptr<ZoneInstanceDOM> create_zone_instance_dom(ZoneInstance* zone_instance) override {
			return make_unique<ZoneInstanceDOMImpl>(&zone_instance_tpl, zone_instance);
		}

		virtual void on_realm_init() override {
			call(RealmCB::realm_init);
		}

		virtual void on_zone_instance_create(ZoneInstance* instance) override;

		static std::string loadFileAsString(Realm* realm, const v8::FunctionCallbackInfo<v8::Value>& info) {
			std::string filename;
			V8Utils::from_js_value(info[0], filename);
			// FIXME: exception handling
			return FileUtils::get_contents(filename);
		}

		EntityDOMTemplate entity_tpl;
		ZoneInstanceDOMTemplate zone_instance_tpl;
		V8WeakRefOwner<Realm> weak;
	};

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

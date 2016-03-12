#include <realm/realm.hpp>
#include <realm/zoneinstance.hpp>

#include <scripting/realm_dom.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace agdg {
	class ZoneInstanceImpl : public ZoneInstance {
	public:
		ZoneInstanceImpl(Realm* realm, IZone* zone) : zone(zone), instance_id(1001) {
			dom = realm->get_dom()->create_zone_instance_dom(this);
			realm->get_dom()->on_zone_instance_create(this);
		}

		virtual int add_entity(Entity* entity) override {
			int eid = next_eid++;
			entity->set_eid(eid);
			entities[eid] = entity;

			//dom->on_player_will_enter(entity);

			broadcast_entity_spawn(eid, entity, entity->get_pos(), entity->get_dir());

			return eid;
		}

		virtual void remove_entity(int eid) override {
			entities.erase(eid);

			broadcast_entity_despawn(eid);
		}

		virtual void subscribe(ZoneInstanceListener* listener) override{
			listeners.push_back(listener);
		}

		virtual void unsubscribe(ZoneInstanceListener* listener) override {
			listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
		}

		virtual void broadcast_chat(Entity* entity, const std::string& text, bool html) override {
			if (!dom->on_chat(entity, text))
				return;

			for (auto listener : listeners)
				listener->on_chat(entity, text, html);
		}

		virtual void broadcast_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) override {
			for (auto listener : listeners)
				listener->on_entity_update(entity, pos, dir, half_latency);
		}

		virtual ZoneInstanceDOM* get_dom() override {
			return dom.get();
		}

		virtual int get_id() override {
			return instance_id;
		}

		virtual IZone* get_zone() override {
			return zone;
		}

		virtual void iterate_entities(std::function<void(int eid, Entity* entity)> callback) override {
			for (auto kv : entities) {
				callback(kv.first, kv.second);
			}
		}

	private:
		void broadcast_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) {
			for (auto listener : listeners)
				listener->on_entity_spawn(eid, entity, pos, dir);
		}

		void broadcast_entity_despawn(int eid) {
			for (auto listener : listeners)
				listener->on_entity_despawn(eid);
		}

		IZone* zone;
		int instance_id;

		std::unordered_map<int, Entity*> entities;
		std::vector<ZoneInstanceListener*> listeners;

		unique_ptr<ZoneInstanceDOM> dom;

		int next_eid = 1;
	};

	unique_ptr<ZoneInstance> ZoneInstance::create(Realm* realm, IZone* zone) {
		return make_unique<ZoneInstanceImpl>(realm, zone);
	}
}

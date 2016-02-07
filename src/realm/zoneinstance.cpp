#include <realm/zoneinstance.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace agdg {
	class ZoneInstanceImpl : public ZoneInstance {
	public:
		ZoneInstanceImpl(IZone* zone) : zone(zone) {}

		virtual int add_entity(Entity* entity) override {
			int eid = next_eid++;
			entities[eid] = entity;

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

		virtual void broadcast_chat(int eid, const std::string& text) override {
			for (auto listener : listeners)
				listener->on_chat(eid, text);
		}

		virtual void broadcast_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, int half_latency) override {
			for (auto listener : listeners)
				listener->on_entity_update(eid, pos, dir, half_latency);
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
		std::unordered_map<int, Entity*> entities;
		std::vector<ZoneInstanceListener*> listeners;

		int next_eid = 1;
	};

	unique_ptr<ZoneInstance> ZoneInstance::Create(IZone* zone) {
		return make_unique<ZoneInstanceImpl>(zone);
	}
}

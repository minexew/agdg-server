#include <realm/zoneinstance.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace agdg {
	class ZoneInstanceImpl : public ZoneInstance {
	public:
		ZoneInstanceImpl(IZone* zone) : zone(zone) {}

		virtual int add_entity(Entity* entity) {
			int eid = next_eid++;
			entities[eid] = entity;

			broadcast_entity_spawn(eid, entity, entity->get_pos(), entity->get_dir());

			return eid;
		}

		virtual void remove_entity(int eid) {
			entities.erase(eid);

			broadcast_entity_despawn(eid);
		}

		virtual void subscribe(ZoneInstanceListener* listener) {
			listeners.push_back(listener);
		}

		virtual void unsubscribe(ZoneInstanceListener* listener) {
			listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
		}

		virtual void broadcast_chat(int eid, const std::string& text) override {
			for (auto listener : listeners)
				listener->on_chat(eid, text);
		}

		virtual void broadcast_entity_update(int eid, const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& velocity) {
			for (auto listener : listeners)
				listener->on_entity_update(eid, pos, dir, velocity);
		}

		virtual IZone* get_zone() {
			return zone;
		}

		virtual void iterate_entities(std::function<void(int eid, Entity* ent)> callback) {
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

	ZoneInstance* ZoneInstance::Create(IZone* zone) {
		return new ZoneInstanceImpl{ zone };
	}
}

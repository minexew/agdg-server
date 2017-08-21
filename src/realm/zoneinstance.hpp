#pragma once

#include <agdg/types.hpp>
#include <world/zonemanager.hpp>

namespace agdg {
    class Entity;
    class Realm;
    class ZoneInstanceDOM;

    class ZoneInstanceListener {
    public:
        virtual void on_chat(Entity* entity, const std::string& text, bool html) = 0;
        virtual void on_entity_despawn(int eid) = 0;
        virtual void on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) = 0;
        virtual void on_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) = 0;
    };

    class ZoneInstance {
    public:
        static unique_ptr<ZoneInstance> create(Realm* realm, IZone* zone);
        virtual ~ZoneInstance() {}

        virtual int add_entity(Entity* entity) = 0;
        virtual void remove_entity(int eid) = 0;

        virtual void subscribe(ZoneInstanceListener* listener) = 0;
        virtual void unsubscribe(ZoneInstanceListener* listener) = 0;

        virtual void broadcast_chat(Entity* entity, const std::string& text, bool html) = 0;
        virtual void broadcast_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) = 0;

        virtual ZoneInstanceDOM* get_dom() = 0;

        // TODO: devirtualize getters?
        virtual int get_id() = 0;
        virtual Realm* get_realm() = 0;
        virtual IZone* get_zone() = 0;
        virtual void iterate_entities(std::function<void(int eid, Entity* entity)> callback) = 0;
    };
}

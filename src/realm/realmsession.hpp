#pragma once

#include <realm/entity.hpp>
#include <realm/realmprotocol.hpp>
#include <realm/realmserverimpl.hpp>
#include <tokenmanager.hpp>

namespace agdg {
    class Ping {
    public:
        void arrived();
        void update(RealmSession& session);

        auto get_measured_latency() const { return measured_latency; }

    private:
        enum class State { not_pinged, first_ping_in_progress, pinged, next_ping_in_progress };

        State state = State::not_pinged;
        steady_clock::time_point last_ping;
        milliseconds measured_latency;
    };

    class PlayerEntityImpl : public Entity {
    public:
        PlayerEntityImpl(Realm* realm, PlayerCharacter* pc, RealmSession* session);

        const std::string& get_name() override { return pc->get_name(); }

        EntityDOM* get_dom() override { return dom.get(); }

        const glm::vec3& get_dir() override { return dir; }
        const glm::vec3& get_pos() override { return pos; }

        void set_pos_dir(const glm::vec3& pos, const glm::vec3& dir) override {
            this->pos = pos;
            this->dir = dir;
        }

        void on_tick() override {
        }

        void on_entity_did_say(Entity* entity, const std::string& message, bool html) override;

    private:
        PlayerCharacter* pc;
        RealmSession* session;

        glm::vec3 pos, dir;

        unique_ptr<EntityDOM> dom;
    };

    class RealmSession : private ZoneInstanceListener {
    public:
        RealmSession(Realm* realm, RealmServer* server, connection_ptr con)
                : realm(realm), server(server), con(con) {}

        void close();
        void on_entity_did_say(Entity* entity, const std::string& text, bool html);
        void on_message(const uint8_t* message, size_t length);
        void on_tick();

        void handle_command(int code, int cookie, const uint8_t* payload, size_t payload_length);

    private:
        void on_chat(Entity* entity, const std::string& text, bool html) override {}
        void on_entity_despawn(int eid) override;
        void on_entity_spawn(int eid, Entity* entity, const glm::vec3& pos, const glm::vec3& dir) override;
        void on_entity_update(Entity* entity, const glm::vec3& pos, const glm::vec3& dir, int half_latency) override;

        void drop_connection() {} // FIXME
        void flush_queue();

        void handle(CChatSay& msg);
        void handle(CEnterWorld& msg);
        void handle(CHello& msg);
        void handle(CPlayerMovement& msg);
        void handle(CPong& msg);
        void handle(CZoneLoaded& msg);

        template <typename T>
        void queue(const T& message, uint8_t cookie = 0) {
            if (!message.encode(send_queue, cookie)) {
                throw std::runtime_error("message encode failed");
            }

            flush_queue();
        }

        void set_command_error_flag();

        Realm* realm;
        RealmServer* server;

        // connection-related structures
        connection_ptr con;
        std::vector<uint8_t> send_queue;

        int flag_command_error = 0;

        // account
        AccountSnapshot account_snapshot;
        bool tokenValidated = false;

        // latency & compensation
        Ping ping;
        int client_latency = kDefaultAssumedLatency;

        // entity in world
        ZoneInstance* inst = nullptr;
        unique_ptr<PlayerCharacter> pc;
        unique_ptr<Entity> player_entity;
        int player_eid = 0;

        friend class Ping;
    };
}

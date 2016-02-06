enum { kCHello = 1 };
enum { kCEnterWorld = 2 };
enum { kCZoneLoaded = 3 };
enum { kCPlayerMovement = 4 };
enum { kCPong = 5 };
enum { kCChatSay = 30 };
enum { kSHello = 1 };
enum { kSLoadZone = 2 };
enum { kSZoneState = 3 };
enum { kSPing = 4 };
enum { kSEntitySpawn = 20 };
enum { kSEntityDespawn = 21 };
enum { kSEntityUpdate = 22 };
enum { kSChatSay = 30 };

struct CHello {
    SHA3_224	token;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CEnterWorld {
    std::string	characterName;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CZoneLoaded {
    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CPlayerMovement {
    glm::vec3	pos;
    glm::vec3	dir;
    glm::vec3	velocity;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CPong {
    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CChatSay {
    std::string	text;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SHello {
    std::vector<std::string>	characters;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SLoadZone {
    std::string	zoneName;
    SHA3_224	zoneRef;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SZoneState {
    struct Entity {
    uint32_t	eid;
    uint32_t	flags;
    std::string	name;
    glm::vec3	pos;
    glm::vec3	dir;
    };
    int32_t	playerEid;
    std::string	playerName;
    glm::vec3	playerPos;
    glm::vec3	playerDir;
    std::vector<Entity>	entities;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SPing {
    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SEntitySpawn {
    struct Entity {
    uint32_t	eid;
    uint32_t	flags;
    std::string	name;
    glm::vec3	pos;
    glm::vec3	dir;
    };
    Entity	entity;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SEntityDespawn {
    uint32_t	eid;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SEntityUpdate {
    uint32_t	eid;
    glm::vec3	pos;
    glm::vec3	dir;
    glm::vec3	velocity;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SChatSay {
    uint32_t	eid;
    std::string	text;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};

enum { kSEntityUpdate = 4 };
enum { kCEnterWorld = 2 };
enum { kCZoneLoaded = 3 };
enum { kCPlayerMovement = 4 };
enum { kSHello = 1 };
enum { kCHello = 1 };
enum { kSZoneState = 3 };
enum { kSAsset = 5 };
enum { kSLoadZone = 2 };
enum { kCRequestAsset = 5 };

struct SEntityUpdate {
    uint32_t	eid;
    glm::vec3	pos;
    glm::vec3	dir;
    glm::vec3	velocity;

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
struct SHello {
    std::vector<std::string>	characters;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CHello {
    SHA3_224	token;

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
    std::vector<Entity>	entities;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SAsset {
    SHA3_224	hash;
    std::string	data;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct SLoadZone {
    std::string	zoneName;
    SHA3_224	zoneRef;
    glm::vec3	playerPos;
    glm::vec3	playerDir;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};
struct CRequestAsset {
    SHA3_224	hash;

    bool Decode(const uint8_t* buffer, size_t length);
    bool Encode(std::ostream& out);
};

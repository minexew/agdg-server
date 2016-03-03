
struct CHello {
    enum { code = 1 };
    SHA3_224	token;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct CEnterWorld {
    enum { code = 2 };
    std::string	characterName;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct CZoneLoaded {
    enum { code = 3 };
    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct CPlayerMovement {
    enum { code = 4 };
    glm::vec3	pos;
    glm::vec3	dir;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct CPong {
    enum { code = 5 };
    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct CChatSay {
    enum { code = 30 };
    std::string	text;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SHello {
    enum { code = 1 };
    std::vector<std::string>	characters;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SLoadZone {
    enum { code = 2 };
    std::string	zoneName;
    SHA3_224	zoneRef;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SZoneState {
    enum { code = 3 };
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

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SPing {
    enum { code = 4 };
    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SEntitySpawn {
    enum { code = 20 };
    struct Entity {
    int32_t	eid;
    uint32_t	flags;
    std::string	name;
    glm::vec3	pos;
    glm::vec3	dir;
    };
    Entity	entity;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SEntityDespawn {
    enum { code = 21 };
    int32_t	eid;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SEntityUpdate {
    enum { code = 22 };
    int32_t	eid;
    glm::vec3	pos;
    glm::vec3	dir;
    uint32_t	latency;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};
struct SChatSay {
    enum { code = 30 };
    int32_t	eid;
    std::string	text;
    bool	html;

    bool decode(const uint8_t* buffer, size_t length);
    bool encode(std::vector<uint8_t>& out, uint8_t cookie) const;
};

bool CHello::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, token)) return false;
    return true;
}

bool CHello::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, CHello::code, cookie);
    if (!offset) return false;
    if (!write(out, token)) return false;
    return end(out, offset);
}

bool CEnterWorld::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, characterName)) return false;
    return true;
}

bool CEnterWorld::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, CEnterWorld::code, cookie);
    if (!offset) return false;
    if (!write(out, characterName)) return false;
    return end(out, offset);
}

bool CZoneLoaded::decode(const uint8_t* buffer, size_t length) {
    return true;
}

bool CZoneLoaded::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, CZoneLoaded::code, cookie);
    if (!offset) return false;
    return end(out, offset);
}

bool CPlayerMovement::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, pos)) return false;
    if (!read(buffer, length, dir)) return false;
    return true;
}

bool CPlayerMovement::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, CPlayerMovement::code, cookie);
    if (!offset) return false;
    if (!write(out, pos)) return false;
    if (!write(out, dir)) return false;
    return end(out, offset);
}

bool CPong::decode(const uint8_t* buffer, size_t length) {
    return true;
}

bool CPong::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, CPong::code, cookie);
    if (!offset) return false;
    return end(out, offset);
}

bool CChatSay::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, text)) return false;
    return true;
}

bool CChatSay::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, CChatSay::code, cookie);
    if (!offset) return false;
    if (!write(out, text)) return false;
    return end(out, offset);
}

bool SHello::decode(const uint8_t* buffer, size_t length) {
    uint32_t characters_count;
    if (!read(buffer, length, characters_count)) return false;

    for (size_t i = 0; i < characters_count; i++) {
        characters.emplace_back();
    if (!read(buffer, length, characters.back())) return false;
    }

    return true;
}

bool SHello::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SHello::code, cookie);
    if (!offset) return false;
    if (!write<uint32_t>(out, characters.size())) return false;

    for (size_t i = 0; i < characters.size(); i++) {
    if (!write(out, characters[i])) return false;
    }

    return end(out, offset);
}

bool SLoadZone::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, zoneName)) return false;
    if (!read(buffer, length, zoneRef)) return false;
    return true;
}

bool SLoadZone::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SLoadZone::code, cookie);
    if (!offset) return false;
    if (!write(out, zoneName)) return false;
    if (!write(out, zoneRef)) return false;
    return end(out, offset);
}

bool SZoneState::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, playerEid)) return false;
    if (!read(buffer, length, playerName)) return false;
    if (!read(buffer, length, playerPos)) return false;
    if (!read(buffer, length, playerDir)) return false;
    uint32_t entities_count;
    if (!read(buffer, length, entities_count)) return false;

    for (size_t i = 0; i < entities_count; i++) {
        entities.emplace_back();
    if (!read(buffer, length, entities.back().eid)) return false;
    if (!read(buffer, length, entities.back().flags)) return false;
    if (!read(buffer, length, entities.back().name)) return false;
    if (!read(buffer, length, entities.back().pos)) return false;
    if (!read(buffer, length, entities.back().dir)) return false;
    }

    return true;
}

bool SZoneState::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SZoneState::code, cookie);
    if (!offset) return false;
    if (!write(out, playerEid)) return false;
    if (!write(out, playerName)) return false;
    if (!write(out, playerPos)) return false;
    if (!write(out, playerDir)) return false;
    if (!write<uint32_t>(out, entities.size())) return false;

    for (size_t i = 0; i < entities.size(); i++) {
    if (!write(out, entities[i].eid)) return false;
    if (!write(out, entities[i].flags)) return false;
    if (!write(out, entities[i].name)) return false;
    if (!write(out, entities[i].pos)) return false;
    if (!write(out, entities[i].dir)) return false;
    }

    return end(out, offset);
}

bool SPing::decode(const uint8_t* buffer, size_t length) {
    return true;
}

bool SPing::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SPing::code, cookie);
    if (!offset) return false;
    return end(out, offset);
}

bool SEntitySpawn::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, entity.eid)) return false;
    if (!read(buffer, length, entity.flags)) return false;
    if (!read(buffer, length, entity.name)) return false;
    if (!read(buffer, length, entity.pos)) return false;
    if (!read(buffer, length, entity.dir)) return false;
    return true;
}

bool SEntitySpawn::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SEntitySpawn::code, cookie);
    if (!offset) return false;
    if (!write(out, entity.eid)) return false;
    if (!write(out, entity.flags)) return false;
    if (!write(out, entity.name)) return false;
    if (!write(out, entity.pos)) return false;
    if (!write(out, entity.dir)) return false;
    return end(out, offset);
}

bool SEntityDespawn::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, eid)) return false;
    return true;
}

bool SEntityDespawn::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SEntityDespawn::code, cookie);
    if (!offset) return false;
    if (!write(out, eid)) return false;
    return end(out, offset);
}

bool SEntityUpdate::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, eid)) return false;
    if (!read(buffer, length, pos)) return false;
    if (!read(buffer, length, dir)) return false;
    if (!read(buffer, length, latency)) return false;
    return true;
}

bool SEntityUpdate::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SEntityUpdate::code, cookie);
    if (!offset) return false;
    if (!write(out, eid)) return false;
    if (!write(out, pos)) return false;
    if (!write(out, dir)) return false;
    if (!write(out, latency)) return false;
    return end(out, offset);
}

bool SChatSay::decode(const uint8_t* buffer, size_t length) {
    if (!read(buffer, length, eid)) return false;
    if (!read(buffer, length, text)) return false;
    if (!read(buffer, length, html)) return false;
    return true;
}

bool SChatSay::encode(std::vector<uint8_t>& out, uint8_t cookie) const {
    auto offset = begin(out, SChatSay::code, cookie);
    if (!offset) return false;
    if (!write(out, eid)) return false;
    if (!write(out, text)) return false;
    if (!write(out, html)) return false;
    return end(out, offset);
}


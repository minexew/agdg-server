bool SEntityUpdate::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, eid)) return false;
    if (!Read(buffer, length, pos)) return false;
    if (!Read(buffer, length, dir)) return false;
    if (!Read(buffer, length, velocity)) return false;
    return true;
}

bool SEntityUpdate::Encode(std::ostream& out) {
    Begin(out, kSEntityUpdate);
    if (!Write(out, eid)) return false;
    if (!Write(out, pos)) return false;
    if (!Write(out, dir)) return false;
    if (!Write(out, velocity)) return false;
    return true;
}

bool CEnterWorld::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, characterName)) return false;
    return true;
}

bool CEnterWorld::Encode(std::ostream& out) {
    Begin(out, kCEnterWorld);
    if (!Write(out, characterName)) return false;
    return true;
}

bool CZoneLoaded::Decode(const uint8_t* buffer, size_t length) {
    return true;
}

bool CZoneLoaded::Encode(std::ostream& out) {
    Begin(out, kCZoneLoaded);
    return true;
}

bool CPlayerMovement::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, pos)) return false;
    if (!Read(buffer, length, dir)) return false;
    if (!Read(buffer, length, velocity)) return false;
    return true;
}

bool CPlayerMovement::Encode(std::ostream& out) {
    Begin(out, kCPlayerMovement);
    if (!Write(out, pos)) return false;
    if (!Write(out, dir)) return false;
    if (!Write(out, velocity)) return false;
    return true;
}

bool SHello::Decode(const uint8_t* buffer, size_t length) {
    uint32_t characters_count;
    if (!Read(buffer, length, characters_count)) return false;

    for (size_t i = 0; i < characters_count; i++) {
        characters.emplace_back();
    if (!Read(buffer, length, characters.back())) return false;
    }

    return true;
}

bool SHello::Encode(std::ostream& out) {
    Begin(out, kSHello);
    if (!Write<uint32_t>(out, characters.size())) return false;

    for (size_t i = 0; i < characters.size(); i++) {
    if (!Write(out, characters[i])) return false;
    }

    return true;
}

bool CHello::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, token)) return false;
    return true;
}

bool CHello::Encode(std::ostream& out) {
    Begin(out, kCHello);
    if (!Write(out, token)) return false;
    return true;
}

bool SZoneState::Decode(const uint8_t* buffer, size_t length) {
    uint32_t entities_count;
    if (!Read(buffer, length, entities_count)) return false;

    for (size_t i = 0; i < entities_count; i++) {
        entities.emplace_back();
    if (!Read(buffer, length, entities.back().eid)) return false;
    if (!Read(buffer, length, entities.back().flags)) return false;
    if (!Read(buffer, length, entities.back().name)) return false;
    if (!Read(buffer, length, entities.back().pos)) return false;
    if (!Read(buffer, length, entities.back().dir)) return false;
    }

    return true;
}

bool SZoneState::Encode(std::ostream& out) {
    Begin(out, kSZoneState);
    if (!Write<uint32_t>(out, entities.size())) return false;

    for (size_t i = 0; i < entities.size(); i++) {
    if (!Write(out, entities[i].eid)) return false;
    if (!Write(out, entities[i].flags)) return false;
    if (!Write(out, entities[i].name)) return false;
    if (!Write(out, entities[i].pos)) return false;
    if (!Write(out, entities[i].dir)) return false;
    }

    return true;
}

bool SAsset::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, hash)) return false;
    if (!Read(buffer, length, data)) return false;
    return true;
}

bool SAsset::Encode(std::ostream& out) {
    Begin(out, kSAsset);
    if (!Write(out, hash)) return false;
    if (!Write(out, data)) return false;
    return true;
}

bool SLoadZone::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, zoneName)) return false;
    if (!Read(buffer, length, zoneRef)) return false;
    if (!Read(buffer, length, playerPos)) return false;
    if (!Read(buffer, length, playerDir)) return false;
    return true;
}

bool SLoadZone::Encode(std::ostream& out) {
    Begin(out, kSLoadZone);
    if (!Write(out, zoneName)) return false;
    if (!Write(out, zoneRef)) return false;
    if (!Write(out, playerPos)) return false;
    if (!Write(out, playerDir)) return false;
    return true;
}

bool CRequestAsset::Decode(const uint8_t* buffer, size_t length) {
    if (!Read(buffer, length, hash)) return false;
    return true;
}

bool CRequestAsset::Encode(std::ostream& out) {
    Begin(out, kCRequestAsset);
    if (!Write(out, hash)) return false;
    return true;
}


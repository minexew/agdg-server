#pragma once

#include <agdg/types.hpp>

#include <cstdlib>
#include <unordered_map>

namespace agdg {
    // FIXME: move this!
    template <typename T>
    bool ValidateUsername(const T& username) {
        if (username.size() < 2)
            return false;

        for (auto c : username) {
            if (!isalnum(c))
                return false;
        }

        return true;
    }

    struct AccountSnapshot {
        std::string name;
        bool trusted;
    };

    class TokenManager {
    public:
        SHA3_224 assign_account_token(const AccountSnapshot& snapshot) {
            SHA3_224 token;
            // This is obviously unsecure and terrible.
            for (size_t i = 0; i < sizeof(token.bytes); i++) {
                token.bytes[i] = rand() & 0xff;
            }

            // TODO: token expiration
            tokens[token] = snapshot;

            return token;
        }

        bool validate_token(const SHA3_224& token, AccountSnapshot& account_out) {
            auto it = tokens.find(token);

            if (it == tokens.end())
                return false;

            account_out = it->second;
            tokens.erase(it);
            return true;
        }

    private:
        std::unordered_map<SHA3_224, AccountSnapshot> tokens;
    };

    extern TokenManager g_token_manager;
}

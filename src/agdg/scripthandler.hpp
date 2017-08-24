#pragma once

#include <agdg/types.hpp>

namespace agdg {
#ifdef WITH_V8
    class V8Context;
    typedef V8Context ScriptContext;
#else
    struct ScriptContext {};
#endif

    class ScriptHandler {
    public:
        virtual ~ScriptHandler() {}
    };
}

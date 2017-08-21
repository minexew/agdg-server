#include <realm/realmimpl.hpp>

namespace agdg {
    RealmImpl::RealmImpl() {
#ifdef WITH_V8
        v8handler = V8ScriptHandler::create("");
        v8context = v8handler->create_context();

        //V8Scope scope(v8context.get());
        dom = RealmDOM::create(v8context.get(), this);

        // TODO: this shouldn't be hardcoded
        v8context->run_file("scripts/dialogue.js");
        v8context->run_file("world/startup.js");
#else
        dom = RealmDOM::create(nullptr, this);
#endif
    }

    RealmImpl::~RealmImpl() {
        // must be destoryed before the context -- or not? (what about references from JS to realm?)
        dom.reset();
    }

    unique_ptr<Realm> Realm::create() {
        return make_unique<RealmImpl>();
    }
}

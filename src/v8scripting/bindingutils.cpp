#include <v8scripting/bindingutils.hpp>

namespace agdg {
    V8ClassTemplate::V8ClassTemplate(V8Context* ctx) : ctx(ctx) {
        V8Scope scope(ctx);

        auto context = ctx->get_v8_context();
        isolate = context->GetIsolate();
        p_context.Reset(isolate, context);

        v8::Local<v8::ObjectTemplate> template_ = v8::ObjectTemplate::New(isolate);
        template_->SetInternalFieldCount(1);

        p_template.Reset(isolate, template_);
    }

    V8ClassTemplate::~V8ClassTemplate() {
        p_template.Reset();
        p_context.Reset();
    }

    void V8ClassTemplate::set_function(const char* name, v8::FunctionCallback callback, v8::Local<v8::Value> data) {
        //V8Scope scope(ctx);

        auto template_ = v8::Local<v8::ObjectTemplate>::New(isolate, p_template);

        template_->Set(v8::String::NewFromUtf8(isolate, name), v8::FunctionTemplate::New(isolate, callback, data));
    }
}

#pragma once

#include <agdg/scripthandler.hpp>
#include <agdg/types.hpp>

// TODO: try to avoid this
#include "libplatform/libplatform.h"
#include "v8.h"

namespace agdg {
	class V8Context {
	public:
		virtual ~V8Context() {}

		virtual void run_script(const char* source, const char* origin) = 0;
		virtual void run_file(const char* path) = 0;

		virtual v8::Isolate* get_isolate() = 0;
		virtual v8::Local<v8::Context> get_v8_context() = 0;
	};

	class V8ScriptHandler : public ScriptHandler {
	public:
		static unique_ptr<V8ScriptHandler> create(const char* base_path);

		virtual unique_ptr<V8Context> create_context() = 0;
	};

	struct V8Scope {
		V8Scope(V8Context* context)
				: isolate_scope(context->get_isolate()), handle_scope(context->get_isolate()),
				context_scope(context->get_v8_context()) {}

		v8::Isolate::Scope isolate_scope;
		v8::HandleScope handle_scope;
		v8::Context::Scope context_scope;
	};
}

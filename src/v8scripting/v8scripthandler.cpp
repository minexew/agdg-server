#include <v8scripting/v8scripthandler.hpp>

#include <agdg/logging.hpp>
#include <utility/fileutils.hpp>

#include "libplatform/libplatform.h"
#include "v8.h"

#include <cstdlib>

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
	virtual void* Allocate(size_t length) override {
		void* data = AllocateUninitialized(length);
		return data == NULL ? data : memset(data, 0, length);
	}

	virtual void* AllocateUninitialized(size_t length) override { return malloc(length); }
	virtual void Free(void* data, size_t) override { free(data); }
};

const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
	v8::HandleScope handle_scope(isolate);
	v8::String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Handle<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		fprintf(stderr, "%s\n", exception_string);
	} else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
		fprintf(stderr, "%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			fprintf(stderr, " ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			fprintf(stderr, "^");
		}
		fprintf(stderr, "\n");
		v8::String::Utf8Value stack_trace(try_catch->StackTrace());
		if (stack_trace.length() > 0) {
			const char* stack_trace_string = ToCString(stack_trace);
			fprintf(stderr, "%s\n", stack_trace_string);
		}
	}
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions) {
	v8::HandleScope handle_scope(isolate);
	v8::TryCatch try_catch(isolate);
	v8::ScriptOrigin origin(name);
	v8::Local<v8::Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Script> script;

	if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
    	// Print errors that happened during compilation.
		if (report_exceptions)
			ReportException(isolate, &try_catch);
		return false;
	}

	v8::Local<v8::Value> result;
	if (!script->Run(context).ToLocal(&result)) {
		if (report_exceptions)
			ReportException(isolate, &try_catch);
		return false;
	}
	return true;
}


namespace agdg {
	class V8ContextImpl : public V8Context {
	public:
		V8ContextImpl(const v8::Isolate::CreateParams& create_params) {
			isolate = v8::Isolate::New(create_params);
			init_context(isolate);
		}

		~V8ContextImpl() {
			p_context.Reset();
			isolate->Dispose();
		}

		virtual void run_script(const char* source, const char* origin) override;

		virtual void run_file(const char* path) override {
			std::string s = FileUtils::get_contents(path);
			return run_script(s.c_str(), path);
		}

		virtual v8::Isolate* get_isolate() override {
			return isolate;
		}

		virtual v8::Local<v8::Context> get_v8_context() override {
			return v8::Local<v8::Context>::New(isolate, p_context);
		}

	private:
		void init_context(v8::Isolate* isolate);

		v8::Isolate* isolate;
		v8::Persistent<v8::Context> p_context;
	};

	class V8ScriptHandlerImpl : public V8ScriptHandler {
	public:
		V8ScriptHandlerImpl(const char* base_path);
		~V8ScriptHandlerImpl();

	private:
		virtual unique_ptr<V8Context> create_context() override;

		ArrayBufferAllocator allocator;

		std::unique_ptr<v8::Platform> platform;
	};

	V8ScriptHandlerImpl::V8ScriptHandlerImpl(const char* base_path) {
		v8::V8::InitializeICUDefaultLocation(base_path);
		v8::V8::InitializeExternalStartupData(base_path);
		platform.reset(v8::platform::CreateDefaultPlatform());
		v8::V8::InitializePlatform(platform.get());
		v8::V8::Initialize();
	}

	V8ScriptHandlerImpl::~V8ScriptHandlerImpl() {
		v8::V8::Dispose();
		v8::V8::ShutdownPlatform();
	}

	unique_ptr<V8Context> V8ScriptHandlerImpl::create_context() {
		v8::Isolate::CreateParams create_params;
		create_params.array_buffer_allocator = &allocator;
		return make_unique<V8ContextImpl>(create_params);
	}

	void log_string(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (!args.Length())
			return;

		v8::String::Utf8Value str(args[0]);

		if (*str)
			g_log->script(*str);
	}

	void V8ContextImpl::init_context(v8::Isolate* isolate) {
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate);

		v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);

		global->Set(v8::String::NewFromUtf8(isolate, "log_string"),
					v8::FunctionTemplate::New(isolate, log_string));

		v8::Local<v8::Context> local_context = v8::Context::New(isolate, NULL, global);
		p_context.Reset(isolate, local_context);
	}

	void V8ContextImpl::run_script(const char* source, const char* origin) {
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate);

		v8::Handle<v8::String> source_ = v8::String::NewFromUtf8(isolate, source);
		v8::Handle<v8::Value> origin_ = v8::String::NewFromUtf8(isolate, origin);

		v8::Local<v8::Context> local_context =
				v8::Local<v8::Context>::New(isolate, p_context);
		v8::Context::Scope context_scope(local_context);
		ExecuteString(isolate, source_, origin_, true, true);
	}

	unique_ptr<V8ScriptHandler> V8ScriptHandler::create(const char* base_path) {
		return make_unique<V8ScriptHandlerImpl>(base_path);
	}
}

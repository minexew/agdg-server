#pragma once

#include <v8scripting/v8scripthandler.hpp>

#include "libplatform/libplatform.h"
#include "v8.h"

#include <string>
#include <vector>

namespace agdg {
	class V8Utils {
	public:
		static bool from_js_value(v8::Local<v8::Value> js_value, bool& value_out) {
			value_out = js_value->BooleanValue();
			return true;
		}

		static bool from_js_value(v8::Local<v8::Value> js_value, glm::vec3& value_out) {
			if (!js_value->IsArray())
				return false;

			auto array = v8::Local<v8::Array>::Cast(js_value);

			for (int i = 0; i < 3; i++)
				value_out.xyz[i] = array->Get(i)->NumberValue();

			return true;
		}

		static bool from_js_value(v8::Local<v8::Value> js_value, std::string& value_out) {
			v8::String::Utf8Value utf8(js_value);
			value_out = *utf8 ? *utf8 : "";
			return true;
		}

		static v8::Local<v8::Value> to_js_value(v8::Isolate* isolate, int value) {
			return v8::Integer::New(isolate, value);
		}

		static v8::Local<v8::Value> to_js_value(v8::Isolate* isolate, const glm::vec3& value) {
			auto array = v8::Array::New(isolate, 3);

			for (int i = 0; i < 3; i++)
				array->Set(i, v8::Number::New(isolate, value.xyz[i]));

			return array;
		}

		static v8::Local<v8::Value> to_js_value(v8::Isolate* isolate, const std::string& value) {
			return v8::String::NewFromUtf8(isolate, value.c_str(), v8::String::kNormalString, value.size());
		}

		static v8::Local<v8::Value> to_js_value(v8::Isolate* isolate, v8::Local<v8::Value> value) {
			return value;
		}
	};

	class V8ClassTemplate {
	public:
		// a V8Context must be established when calling this!
		V8ClassTemplate(V8Context* ctx);
		~V8ClassTemplate();

		void set_function(const char* name, v8::FunctionCallback callback, v8::Local<v8::Value> data);

	public:
		V8Context* ctx;

		v8::Isolate* isolate;
		v8::Persistent<v8::Context> p_context;
		v8::Persistent<v8::ObjectTemplate> p_template;
	};

	template <class C>
	struct V8WeakRef {
		V8WeakRef(v8::Isolate* isolate, C* native_instance, v8::Local<v8::Object> instance)
				: native_instance(native_instance) {
			v8_instance.Reset(isolate, instance);
			v8_instance.SetWeak(this, on_gc);
		}

		void release() {
			native_instance = nullptr;
			v8_instance.Reset();
		}

		static void on_gc(const v8::WeakCallbackData<v8::Object, V8WeakRef<C>>& data) {
			printf("V8WeakRef on_gc\n");
			delete data.GetParameter();
		}

		C* native_instance;
		v8::Persistent<v8::Object> v8_instance;
	};

	template <class C>
	struct V8WeakRefOwner {
		~V8WeakRefOwner() {
			reset();
		}

		void reset() {
			if (ref) {
				ref->release();
				ref = nullptr;
			}
		}

		void reset(v8::Isolate* isolate, C* native_instance, v8::Local<v8::Object> instance) {
			reset();

			ref = new V8WeakRef<C>(isolate, native_instance, instance);
			instance->SetInternalField(0, v8::External::New(isolate, ref));
		}

		V8WeakRef<C>* ref = nullptr;
	};

	template <class C>
	class V8TypedClassTemplate : public V8ClassTemplate {
	public:
		typedef V8WeakRef<C> Ref;
		V8TypedClassTemplate(V8Context* ctx) : V8ClassTemplate(ctx) {}

		template <typename ReturnType, ReturnType (method)(C*, const v8::FunctionCallbackInfo<v8::Value>& info)>
		void set_method(const char* name) {
			auto template_ = v8::Local<v8::ObjectTemplate>::New(isolate, p_template);

			template_->Set(v8::String::NewFromUtf8(isolate, name), v8::Function::New(isolate,
					[](const v8::FunctionCallbackInfo<v8::Value>& info) {
				auto isolate = info.GetIsolate();

				auto ref_external = v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0));
				auto ref = static_cast<Ref*>(ref_external->Value());

				if (ref->native_instance != nullptr)
					info.GetReturnValue().Set(V8Utils::to_js_value(isolate, method(ref->native_instance, info)));
			}));
		}

		template <void (method)(C*, const v8::FunctionCallbackInfo<v8::Value>& info)>
		void set_method(const char* name) {
			auto template_ = v8::Local<v8::ObjectTemplate>::New(isolate, p_template);

			template_->Set(v8::String::NewFromUtf8(isolate, name), v8::Function::New(isolate,
					[](const v8::FunctionCallbackInfo<v8::Value>& info) {
				auto ref_external = v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0));
				auto ref = static_cast<Ref*>(ref_external->Value());

				if (ref->native_instance != nullptr)
					method(ref->native_instance, info);
			}));
		}

		template <typename ReturnType, ReturnType (C::*getter)()>
		void set_property_getter(const char* name) {
			auto template_ = v8::Local<v8::ObjectTemplate>::New(isolate, p_template);

			template_->SetAccessor(v8::String::NewFromUtf8(isolate, name),
					[](v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
				auto isolate = info.GetIsolate();

				auto ref_external = v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0));
				auto ref = static_cast<Ref*>(ref_external->Value());

				if (ref->native_instance != nullptr)
					info.GetReturnValue().Set(V8Utils::to_js_value(isolate, (ref->native_instance->*getter)()));
			});
		}

		/*template <typename ReturnType, ReturnType (*getter)(C*)>
		void set_property_getter(const char* name) {
			auto template_ = v8::Local<v8::ObjectTemplate>::New(isolate, p_template);

			template_->SetAccessor(v8::String::NewFromUtf8(isolate, name),
					[](v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
				auto isolate = info.GetIsolate();

				auto ref_external = v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0));
				auto ref = static_cast<Owner*>(ref_external->Value());

				if (ref->native_instance != nullptr)
					info.GetReturnValue().Set(V8Utils::to_js_value(isolate, getter(ref->native_instance)));
			});
		}*/

		v8::Local<v8::Object> get_instance(V8WeakRefOwner<C>& owner, C* native_instance) {
			if (!owner.ref) {
				auto template_ = v8::Local<v8::ObjectTemplate>::New(isolate, p_template);

				v8::Local<v8::Object> instance = template_->NewInstance();

				owner.reset(isolate, native_instance, instance);

				return instance;
			}
			else
				return v8::Local<v8::Object>::New(isolate, owner.ref->v8_instance);
		}
	};

	template <class C>
	class V8ClassTemplateWithCallbacks : public V8TypedClassTemplate<C> {
	public:
		V8ClassTemplateWithCallbacks(V8Context* ctx) : V8TypedClassTemplate<C>(ctx) {}

		template <class Inst, class CallbackEnum>
		void register_callback(CallbackEnum which, const char* name);
	};

	template <class CallbackEnum>
	class V8CallbackCollector {
	public:
		V8CallbackCollector(V8ClassTemplate* tpl) : tpl(tpl) {}

		void add_callback(CallbackEnum which, v8::Local<v8::Function> func);

		bool callable(CallbackEnum which) { return callbacks[static_cast<int>(which)].get() != nullptr; }

		template<class... Types> void call(CallbackEnum which, Types... args) {
			auto callback = callbacks[static_cast<int>(which)].get();

			if (!callback)
				return;

			auto isolate = tpl->isolate;

			v8::Isolate::Scope isolate_scope(isolate);
			v8::HandleScope handle_scope(isolate);

			auto context = v8::Local<v8::Context>::New(isolate, tpl->p_context);
			v8::Context::Scope context_scope(context);

			v8::Local<v8::Value> global = context->Global();
			v8::Local<v8::Value> call_args[] = { V8Utils::to_js_value(isolate, args)... };

			for (; callback; callback = callback->next.get()) {
				v8::Local<v8::Function> function = v8::Local<v8::Function>::New(isolate, callback->function);
				function->Call(global, sizeof...(args), call_args);
			}
		}

		template<class ReturnValue, ReturnValue(*fold)(ReturnValue, ReturnValue),
				class... Types> ReturnValue call(CallbackEnum which, Types... args) {
			auto callback = callbacks[static_cast<int>(which)].get();

			if (!callback)
				return ReturnValue();

			auto isolate = tpl->isolate;

			v8::Isolate::Scope isolate_scope(isolate);
			v8::HandleScope handle_scope(isolate);

			auto context = v8::Local<v8::Context>::New(isolate, tpl->p_context);
			v8::Context::Scope context_scope(context);

			v8::Local<v8::Value> global = context->Global();
			v8::Local<v8::Value> call_args[] = { V8Utils::to_js_value(isolate, args)... };

			bool first = true;
			ReturnValue theReturn;

			for (; callback; callback = callback->next.get()) {
				v8::Local<v8::Function> function = v8::Local<v8::Function>::New(isolate, callback->function);
				v8::Local<v8::Value> result = function->Call(global, sizeof...(args), call_args);

				if (first) {
					V8Utils::from_js_value(result, theReturn);
					first = false;
				}
				else {
					ReturnValue oneReturn;
					V8Utils::from_js_value(result, oneReturn);
					theReturn = fold(theReturn, oneReturn);
				}
			}

			return theReturn;
		}

	protected:
		struct Callback {
			v8::Persistent<v8::Function> function;
			unique_ptr<Callback> next;

			Callback(v8::Isolate* isolate, v8::Local<v8::Function> function, unique_ptr<Callback>&& next)
					: function(isolate, function), next(std::move(next)) {}

			~Callback() {
				function.Reset();
			}
		};

		V8ClassTemplate* tpl;
		unique_ptr<Callback> callbacks[static_cast<size_t>(CallbackEnum::max)];
	};

	template <class C>
	template <class Inst, class CallbackEnum>
	void V8ClassTemplateWithCallbacks<C>::register_callback(CallbackEnum which, const char* name) {
		V8Scope scope(this->ctx);

		this->V8ClassTemplate::set_function(name, [](const v8::FunctionCallbackInfo<v8::Value>& info) {
			auto isolate = info.GetIsolate();

			// get callback index from function internal data
			CallbackEnum which = static_cast<CallbackEnum>(
					info.Data()->ToInt32(info.GetIsolate())->Value());

			// get class instance pointer
			auto field = v8::Local<v8::External>::Cast(info.Holder()->GetInternalField(0));
			void* ptr = field->Value();
			auto this_ = static_cast<V8WeakRef<C>*>(ptr)->native_instance;

			if (info.Length() != 1 || !info[0]->IsFunction()) {
				isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(
						isolate, "expected function argument")));
				return;
			}

			v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(info[0]);
			static_cast<Inst*>(this_->get_dom())->add_callback(which, function);

			// TODO: return value
		}, v8::Integer::New(this->isolate, static_cast<int>(which)));
	}

	template <class CallbackEnum>
	void V8CallbackCollector<CallbackEnum>::add_callback(CallbackEnum which, v8::Local<v8::Function> func) {
		auto i = static_cast<int>(which);

		if (!callbacks[i])
			callbacks[i] = make_unique<Callback>(tpl->isolate, func, nullptr);
		else
			// WRONG WRONG WRONG
			callbacks[i]->next = make_unique<Callback>(tpl->isolate, func, nullptr);
	}
}

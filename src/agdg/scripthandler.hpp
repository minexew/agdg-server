#pragma once

#include <agdg/types.hpp>

namespace agdg {
#ifdef WITH_V8
	class V8ScriptContext;
	typedef V8ScriptContext ScriptContext;
#endif

	class ScriptHandler {
	public:
		virtual ~ScriptHandler() {}
	};
}

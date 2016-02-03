#pragma once

namespace agdg {
	class IServerLifecycle {
	public:
		virtual void Run() = 0;
		virtual void Start() = 0;
		virtual void Stop() = 0;
	};

	extern IServerLifecycle* g_serverLifecycle;
}

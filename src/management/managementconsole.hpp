#pragma once

namespace agdg {
	class IManagementConsole {
	public:
		virtual void Init() = 0;
		virtual void Start() = 0;
		virtual void Stop() = 0;
	};

	extern IManagementConsole* g_mgmtConsole;
}

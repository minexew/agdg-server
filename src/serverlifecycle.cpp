#include <serverlifecycle.hpp>

#include <management/managementconsole.hpp>
#include <utility/logging.hpp>

#include <thread>

namespace agdg {
	class ServerLifecycle : public IServerLifecycle {
	public:
		virtual void Run() override {
			while (isStarted && !shouldStop)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			DoStop();
		}

		virtual void Start() override {
			if (isStarted)
				return;

			// FIXME: error handling
			g_log->Log("Starting Management Console");
			g_mgmtConsole->Init();
			g_mgmtConsole->Start();

			isStarted = true;
		}

		virtual void Stop() override {
			if (isStarted)
				shouldStop = true;
		}

	private:
		void DoStop() {
			if (!isStarted)
				return;

			g_log->Log("Shutting down");

			g_mgmtConsole->Stop();
			isStarted = false;
		}

		bool isStarted = false;
		bool shouldStop = false;
	};

	static ServerLifecycle s_serverLifecycle;
	IServerLifecycle* g_serverLifecycle = &s_serverLifecycle;
}

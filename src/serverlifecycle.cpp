#include <serverlifecycle.hpp>

#include <agdg/config.hpp>
#include <agdg/service.hpp>
#include <utility/logging.hpp>

// All Services: shouldn't really be here, low-priority fix
#include <login/loginserver.hpp>
#include <management/managementconsole.hpp>

#include <thread>
#include <vector>

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
			g_log->Log("Starting Services");

			g_config->EnumerateServices([this](const auto& name, const auto& d) {
				std::unique_ptr<IService> service(InstantiateService(name, d));
				service->Init();
				service->Start();

				services.push_back(std::move(service));
			});

			g_log->Log("Server is running");

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

			g_log->Log("Stopping services");

			for (auto& s : services)
				s->Stop();

			isStarted = false;
		}

		IService* InstantiateService(const std::string& name, const rapidjson::Value& d) {
			std::string class_(d["class"].GetString());

			g_log->Log("Starting service '%s' of class %s", name.c_str(), class_.c_str());

			if (class_ == "ILoginServer")
				return ILoginServer::Create(name, d);
			else if (class_ == "IManagementConsole")
				return IManagementConsole::Create(name, d);
			else
				throw std::runtime_error((std::string) "unknown service class " + class_.c_str());
		}

		std::vector<std::unique_ptr<IService>> services;

		bool isStarted = false;
		bool shouldStop = false;
	};

	static ServerLifecycle s_serverLifecycle;
	IServerLifecycle* g_serverLifecycle = &s_serverLifecycle;
}

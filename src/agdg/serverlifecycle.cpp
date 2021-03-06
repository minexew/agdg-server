#include <agdg/serverlifecycle.hpp>

#include <agdg/config.hpp>
#include <agdg/service.hpp>
#include <agdg/logging.hpp>

// All Services: shouldn't really be here, low-priority fix
#include <login/loginserver.hpp>
#include <management/managementconsole.hpp>
#include <realm/realmserver.hpp>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace agdg {
    class ServerLifecycle : public IServerLifecycle {
    public:
        void run() override {
            while (!shouldStop)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        void start() override {
            // FIXME: error handling
            g_log->info("Starting services");

            g_config->enumerate_services([this](const auto& name, const auto& d) {
                auto service = this->instantiate_service(name, d);
                service->init();
                service->start();

                services[name] = std::move(service);
            });

            g_log->info("Server is running");
        }

        void close_server(const std::string& message) override {
            std::lock_guard<std::mutex> lg(services_mutex);
            g_log->info("Closing server with reason '%s'", message.c_str());

            for (auto& s : services)
                s.second->close_server(message);
        }

        IService* find_service_by_name(const std::string& name) override {
            std::lock_guard<std::mutex> lg(services_mutex);

            auto it = services.find(name);

            if (it == services.end())
                return nullptr;
            else
                return it->second.get();
        }

        void reopen_server() override {
            std::lock_guard<std::mutex> lg(services_mutex);
            g_log->info("Reopening server");

            for (auto& s : services)
                s.second->reopen_server();
        }

        void request_shutdown() override {
            shouldStop = true;
        }

        void stop() override {
            std::lock_guard<std::mutex> lg(services_mutex);
            g_log->info("Stopping services");

            for (auto& s : services)
                s.second->stop();
        }

    private:
        unique_ptr<IService> instantiate_service(const std::string& name, const rapidjson::Value& d) {
            std::string class_(d["class"].GetString());

            g_log->info("Starting service '%s' of class %s", name.c_str(), class_.c_str());

            if (class_ == "ILoginServer")
                return ILoginServer::create(name, d);
            else if (class_ == "IManagementConsole")
                return IManagementConsole::create(name, d);
            else if (class_ == "IRealmServer")
                return IRealmServer::create(name, d);
            else
                throw std::runtime_error((std::string) "unknown service class " + class_.c_str());
        }

        std::unordered_map<std::string, std::unique_ptr<IService>> services;
        std::mutex services_mutex;

        bool shouldStop = false;
    };

    static ServerLifecycle s_serverLifecycle;
    IServerLifecycle* g_serverLifecycle = &s_serverLifecycle;
}

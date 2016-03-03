#include <agdg/config.hpp>
#include <agdg/serverlifecycle.hpp>
#include <agdg/logging.hpp>
#include <tokenmanager.hpp>

#include <reflection/base.hpp>

#include <websocketpp/error.hpp>

#include <iostream>

#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#endif

namespace reflection {
	class DefaultErrorHandler : public IErrorHandler {
	public:
		virtual void error(const char* errorCode, const char* description) override {
			throw std::runtime_error((std::string) errorCode + ": " + description);
		}
	};

	DefaultErrorHandler defaultErr;
	IErrorHandler* err = &defaultErr;
}

namespace agdg {
	TokenManager g_token_manager;

	static void ctrl_c_handler(int signal) {
		g_serverLifecycle->request_shutdown();
	}

	static void init_ctrl_c_handler() {
#ifndef _WIN32
		struct sigaction sigIntHandler;

		sigIntHandler.sa_handler = ctrl_c_handler;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;

		sigaction(SIGINT, &sigIntHandler, NULL);
#endif
	}

	int main(int argc, char** argv) {
		try {
			const char* config_name = nullptr;

			if (argc >= 2)
				config_name = argv[1];

			g_config->init(config_name);

			init_ctrl_c_handler();

			g_serverLifecycle->start();
			g_serverLifecycle->run();
		}
		catch (const websocketpp::exception& e) {
			std::cout << e.what() << std::endl;
		}
		catch (const std::exception& ex) {
			g_log->error("exception: %s", ex.what());
		}
		catch (...) {
			std::cout << "other exception" << std::endl;
		}

		g_serverLifecycle->stop();

		//std::cerr << "Press any key to close." << std::endl;
		//std::cin.get();
		return 0;
	}
}

int main(int argc, char** argv) {
	return agdg::main(argc, argv);
}

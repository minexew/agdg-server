#include <agdg/config.hpp>
#include <agdg/serverlifecycle.hpp>
#include <tokenmanager.hpp>

#include <reflection/base.hpp>

#include <websocketpp/error.hpp>

#include <iostream>

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

	int main(int argc, char** argv) {
		try {
			const char* config_name = nullptr;

			if (argc >= 2)
				config_name = argv[1];

			g_config->Init(config_name);

			g_serverLifecycle->Start();
			g_serverLifecycle->Run();
		}
		catch (const websocketpp::exception& e) {
			std::cout << e.what() << std::endl;
		}
		catch (const std::exception& ex) {
			std::cerr << ex.what() << std::endl;
		}
		catch (...) {
			std::cout << "other exception" << std::endl;
		}

		//std::cerr << "Press any key to close." << std::endl;
		//std::cin.get();
		return 0;
	}
}

int main(int argc, char** argv) {
	return agdg::main(argc, argv);
}

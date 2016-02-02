#include <agdg/config.hpp>
#include <serverlifecycle.hpp>

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
	int main(int argc, char** argv) {
		try {
			g_config->Init();

			g_serverLifecycle->Start();
			g_serverLifecycle->Run();
		}
		catch (websocketpp::exception const & e) {
			std::cout << e.what() << std::endl;
			std::cin.get();
		}
		catch (std::exception ex) {
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

#include <serverlifecycle.hpp>

#include <websocketpp/error.hpp>

#include <iostream>

namespace agdg {
	int main(int argc, char** argv) {
		try {
			g_serverLifecycle->Start();
			g_serverLifecycle->Run();
		}
		catch (websocketpp::exception const & e) {
			std::cout << e.what() << std::endl;
			std::cin.get();
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

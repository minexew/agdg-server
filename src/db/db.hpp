#pragma once

#include <memory>
#include <string>

#include <tokenmanager.hpp>

namespace agdg {
	namespace db {
		/*class Account {
		private:
			std::string username;
		};*/

		/*class Character {
		private:
			std::string name;
		};*/
	}

	class ILoginDB {
	public:
		virtual ~ILoginDB() {}

		//virtual db::Account* RetrieveAccount();

		virtual bool CreateAccount(const std::string& username, const std::string& password) = 0;

		// This is wrong, but will be redone
		virtual bool VerifyCredentials(const std::string& username, const std::string& password,
			const std::string& hostname, AccountSnapshot& snapshot_out) = 0;
	};

	class IJsonLoginDB : public ILoginDB {
	public:
		static unique_ptr<IJsonLoginDB> Create(const std::string& dir);
	};
}

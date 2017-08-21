#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

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

        struct NewsEntry {
            std::chrono::system_clock::time_point when_posted;
            std::string title_html;
            std::string contents_html;

            NewsEntry() {}

            NewsEntry(std::chrono::system_clock::time_point when_posted,
                    std::string&& title_html, std::string&& contents_html)
                    : when_posted(when_posted), title_html(std::move(title_html)), contents_html(std::move(contents_html)) {
            }
        };
    }

    class ILoginDB {
    public:
        virtual ~ILoginDB() {}

        //virtual db::Account* RetrieveAccount();

        virtual bool CreateAccount(const std::string& username, const std::string& password) = 0;

        // This is wrong, but will be redone
        virtual bool VerifyCredentials(const std::string& username, const std::string& password,
            const std::string& hostname, AccountSnapshot& snapshot_out) = 0;

        // TODO: this might be needlessly inefficient (not that it matters a lot)
        // returns newest first
        virtual void get_news(std::vector<db::NewsEntry>& news_out) = 0;
        virtual void post_news(std::string&& title_html, std::string&& contents_html) = 0;
    };

    class IJsonLoginDB : public ILoginDB {
    public:
        static unique_ptr<IJsonLoginDB> create(const std::string& dir);
    };
}

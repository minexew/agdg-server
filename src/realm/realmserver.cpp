#include <realm/realmserver.hpp>

#include <agdg/config.hpp>
#include <db/db.hpp>
#include <realm/realmprotocol.hpp>
#include <utility/hashutils.hpp>
#include <utility/logging.hpp>
#include <utility/rapidjsonconfigmanager.hpp>
#include <utility/rapidjsonutils.hpp>
#include <websocketpp_configuration.hpp>
#include <world/zonemanager.hpp>

#include <reflection/basic_templates.hpp>
#include <reflection/magic.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <websocketpp/server.hpp>

namespace agdg {
	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;

	class RealmSession;
	class RealmServer;

	typedef websocketpp::server<configuration<RealmSession>> Server;
	typedef Server::connection_ptr connection_ptr;
	typedef Server::message_ptr message_ptr;

	/*class ByteVectorStream : std::ostream {
	public:
		virtual std::ostream& write(const char* s, std::streamsize n) override {
			size_t pos = bytes.size();
			bytes.resize(pos + n);
			memcpy(&bytes[pos], s, n);
		}

		void Clear() {
			bytes.resize(0);
		}

		const uint8_t* GetBytes() { return &bytes[0]; }

	private:
		std::vector<uint8_t> bytes;
	};*/

	class ByteVectorBuf : public std::streambuf {
	public:
		virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override {
			size_t pos = bytes.size();
			bytes.resize(pos + n);
			memcpy(&bytes[pos], s, n);
			return n;
		}

		virtual int_type overflow(int_type c) {
			size_t pos = bytes.size();
			bytes.resize(pos + 1);
			bytes[pos] = c;
			return c;
		}

		void Clear() {
			bytes.resize(0);
		}

		const uint8_t* GetBytes() { return &bytes[0]; }
		const size_t GetLength() { return bytes.size(); }

	private:
		std::vector<uint8_t> bytes;
	};

	class RealmSession {
	public:
		RealmSession(RealmServer* server, connection_ptr con) : server(server), con(con) {}

		void OnMessage(int id, const uint8_t* buffer, size_t length);

		void HandleMessage(CEnterWorld& msg);
		void HandleMessage(CHello& msg);
		void HandleMessage(CRequestAsset& msg);
		void HandleMessage(CZoneLoaded& msg);

		template <typename T>
		void SendReply(T& reply) {
			//messageBuffer.Clear();
			messageBuf.Clear();
			std::ostream os(&messageBuf);

			if (reply.Encode(os))
				con->send(messageBuf.GetBytes(), messageBuf.GetLength());
		}

	private:
		RealmServer* server;
		connection_ptr con;

		bool tokenValidated = false;

		//ByteVectorStream messageBuffer;
		ByteVectorBuf messageBuf;
	};

	class RealmServer : public IRealmServer {
	public:
		RealmServer(const std::string& serviceName, const rapidjson::Value& d) {
			configure(*this, d);

			contentMgr.reset(IContentManager::Create());
			zoneMgr.reset(IZoneManager::Create(contentMgr.get()));
			zoneMgr->ReloadContent();

			//db.reset(IJsonRealmDB::Create("db/" + serviceName + "/"));
		}

		virtual void Init() override {
			server.init_asio();

			server.clear_access_channels(websocketpp::log::alevel::all);

			server.set_open_handler(bind(&RealmServer::OnOpen, this, _1));

			server.listen(listenPort);
			server.start_accept();
		}

		virtual void Start() override {
			thread = std::thread(&RealmServer::Run, this);
		}

		virtual void Stop() override {
			// FIXME: end all connections

			server.stop();

			thread.join();
		}

		IContentManager* GetContentManager() { return contentMgr.get(); }
		IZoneManager* GetZoneManager() { return zoneMgr.get(); }

	private:
		void ForwardMessage(RealmSession* session, connection_hdl hdl, Server::message_ptr msg) {
			auto data = (const uint8_t*) msg->get_payload().c_str();
			size_t length = msg->get_payload().size();

			if (length >= 1)
				session->OnMessage(data[0], data + 1, length - 1);
		}

		void OnOpen(connection_hdl hdl) {
			connection_ptr con = server.get_con_from_hdl(hdl);

			con->instance.reset(new RealmSession(this, con));

			con->set_message_handler(bind(&RealmServer::ForwardMessage, this, con->instance.get(), _1, _2));
		}

		void Run() {
			server.run();
		}

		std::thread thread;
		Server server;

		//std::unique_ptr<ILoginDB> db;

		std::unique_ptr<IContentManager> contentMgr;
		std::unique_ptr<IZoneManager> zoneMgr;

		std::string serverName;
		int listenPort;

		REFL_BEGIN("RealmServer", 1)
			REFL_MUST_CONFIG(listenPort)
		REFL_END
	};

	void RealmSession::HandleMessage(CHello& msg) {
		// FIXME: validate token (asynchronously?)
		tokenValidated = true;

		SHello reply = { { "TestChar" } };
		SendReply(reply);
	}

	void RealmSession::HandleMessage(CEnterWorld& msg) {
		g_log->Log("character %s entering world", msg.characterName.c_str());
		
		IZone* zone = server->GetZoneManager()->GetZoneById("test_zone");
		// FIXME: can be NULL

		SLoadZone reply = { zone->GetName(), zone->GetHash(), glm::vec3{}, glm::vec3{} };
		SendReply(reply);
	}

	void RealmSession::HandleMessage(CRequestAsset& msg) {
		SAsset reply;

		// FIXME: this should be done by a content server, not Realm!!
		if (!server->GetContentManager()->GetCachedAsset(msg.hash, reply.data)) {
			g_log->Log("failed to retrieve cached asset %s", HashUtils::HashToHexString(msg.hash).c_str());
			return;
		}

		reply.hash = msg.hash;
		SendReply(reply);
	}

	void RealmSession::HandleMessage(CZoneLoaded& msg) {
		SZoneState reply;

		// TODO: iterate entities in zone

		reply.entities.push_back({ 1, 0, "Test Entity", glm::vec3{2, 2, 1}, glm::vec3{} });
		
		SendReply(reply);
	}

	void RealmSession::OnMessage(int id, const uint8_t* buffer, size_t length) {
		if (!tokenValidated && id != kCHello)
			return;

#define handle_message(name_) case k##name_: {\
	name_ msg;\
	if (!msg.Decode(buffer, length)) return;\
	this->HandleMessage(msg);\
	break;\
}

		switch (id) {
		handle_message(CHello)
		handle_message(CEnterWorld)
		handle_message(CRequestAsset)
		handle_message(CZoneLoaded)
		default:
			;
		}
	}

	IRealmServer* IRealmServer::Create(const std::string& serviceName, const rapidjson::Value& config) {
		return new RealmServer(serviceName, config);
	}
}

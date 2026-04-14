#pragma once

#include <memory>
#include <string>
#include <vector>
#include <pqxx/pqxx>

#include "browse.h"

// All done by Codex

namespace webAPI {
    class socialAPI;
    class SimpleESA;

    struct DbConfig {
        std::string host;
        int port;
        std::string dbname;
        std::string user;
        std::string password;
        /**
         * Delete data or just mark data as disabled
         *
         * true means delete, default is false
         *
         * Set by env: DB_DELETE
         */
        bool deleteOrDisable = false;
    };

    struct ClientRow {
        std::string client_id;
        std::string esa_key_enc;
        int status = 1;
        std::string created_at;
        std::string last_seen;
    };

    struct HandlerRow {
        std::string client_id;
        std::string platform;
        BrowseWorkingContext browse;
        std::string data_json;
        std::string updated_at;
    };

    class postgres {
    public:
        explicit postgres(DbConfig config);

        inline bool init() {
            return connect() && ensureSchema();
        }

        void setConfig(DbConfig config);

        bool connect();
        void disconnect();
        [[nodiscard]] bool isConnected() const;

        [[nodiscard]] bool ensureSchema() const;

        [[nodiscard]] bool upsertClient(const std::string& client_id, const std::string& esa_key_enc) const;
        [[nodiscard]] bool updateClientLastSeen(const std::string& client_id) const;
        [[nodiscard]] bool deleteClient(const std::string& client_id) const;
        [[nodiscard]] bool updateClientDeleteStrategy() const;

        [[nodiscard]] bool setAdminRole(const std::string& client_id, bool enabled) const;
        [[nodiscard]] bool clearAllAdmins() const;
        [[nodiscard]] bool isAdmin(const std::string& client_id) const;

        [[nodiscard]] bool upsertHandler(const socialAPI* handler, const std::string& client_id) const;
        [[nodiscard]] bool upsertHandler(const HandlerRow& row) const;
        [[nodiscard]] bool deleteHandler(const std::string& client_id, const std::string& platform) const;
        bool listClientHandlers(const std::string& client_id, std::vector<HandlerRow>& out) const;

        bool clientsInit(std::vector<ClientRow>& clients, std::vector<HandlerRow>& handlers) const;

    private:
        [[nodiscard]] std::string buildConnectionString() const;
        [[nodiscard]] bool execute(const std::string& sql) const;
        [[nodiscard]] bool ensureCrypto() const;
        [[nodiscard]] std::string encryptDb(const std::string& plain) const;
        [[nodiscard]] std::string decryptDb(const std::string& cipher) const;

        DbConfig config_;
        bool connected_ = false;
        std::unique_ptr<pqxx::connection> connection_;
        #ifndef DEVELOP
            mutable std::unique_ptr<webAPI::SimpleESA> crypto_;
        #endif
    };
}

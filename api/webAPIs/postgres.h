#pragma once

#include <memory>
#include <string>
#include <vector>
#include <pqxx/pqxx>

#include "socialAPI.h"

// All done by Codex

namespace socialAPI {

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

    struct CookieRow {
        std::string client_id;
        std::string platform;
        std::string cookie;
        std::string updated_at;
        bool disabled = false;
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

        [[nodiscard]] bool upsertCookie(const std::string& client_id,
                          const std::string& platform,
                          const std::string& cookie_enc) const;
        [[nodiscard]] bool deleteCookie(const std::string& client_id, const std::string& platform) const;
        bool getCookie(const std::string& client_id,
                       const std::string& platform,
                       CookieRow& out) const;
        bool listClientCookies(const std::string& client_id, std::vector<CookieRow>& out) const;

        bool clientsInit(std::vector<ClientRow>& clients, std::vector<CookieRow>& cookies) const;

    private:
        [[nodiscard]] std::string buildConnectionString() const;
        [[nodiscard]] bool execute(const std::string& sql) const;
        [[nodiscard]] bool ensureCrypto() const;
        [[nodiscard]] std::string encryptDb(const std::string& plain) const;
        [[nodiscard]] std::string decryptDb(const std::string& cipher) const;

        DbConfig config_;
        bool connected_ = false;
        std::unique_ptr<pqxx::connection> connection_;
        mutable std::unique_ptr<webAPI::SimpleESA> crypto_;
    };
}

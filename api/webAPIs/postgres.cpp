#include "postgres.h"
#include <algorithm>
#include <pqxx/pqxx>
#include <sodium.h>

#include "../config.h"
#include "../Util.h"
#include "../develop/flags.h"

namespace {
    std::string decodeKey(const std::string& key) {
        if (key.size() == crypto_secretbox_KEYBYTES) {
            return key;
        }
        std::vector<unsigned char> bin(key.size());
        size_t bin_len = 0;
        if (sodium_base642bin(bin.data(), bin.size(), key.c_str(), key.size(),
                              nullptr, &bin_len, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0) {
            return "";
        }
        if (bin_len != crypto_secretbox_KEYBYTES) {
            return "";
        }
        return std::string(reinterpret_cast<const char*>(bin.data()), bin_len);
    }
}

namespace webAPI {

    postgres::postgres(DbConfig config) : config_(std::move(config)) {}

    void postgres::setConfig(DbConfig config) {
        config_ = std::move(config);
    }

    bool postgres::connect() {
        try {
            connection_ = std::make_unique<pqxx::connection>(buildConnectionString());
            connected_ = connection_->is_open();
        } catch (const std::exception& e) {
            warn("Connect to database encounters an error: ");
            warn(e.what());
            warn("Connection string: ");
            warn(buildConnectionString().c_str());
            connected_ = false;
        }
        return connected_;
    }

    void postgres::disconnect() {
        connected_ = false;
        connection_.reset();
    }

    bool postgres::isConnected() const {
        return connected_ && connection_ && connection_->is_open();
    }

    bool postgres::ensureSchema() const {
        if (!isConnected()) {
            return false;
        }
        const std::vector<std::string> statements = {
        #ifdef DEVELOP
            R"sql(CREATE TABLE IF NOT EXISTS develop_clients (
  client_id   text PRIMARY KEY,
  esa_key_enc bytea NOT NULL,
  created_at  timestamptz NOT NULL DEFAULT now(),
  last_seen   timestamptz NOT NULL DEFAULT now(),
  status      smallint NOT NULL DEFAULT 1
))sql",
            R"sql(CREATE TABLE IF NOT EXISTS develop_client_roles (
  client_id text NOT NULL REFERENCES develop_clients(client_id) ON DELETE CASCADE,
  role      text NOT NULL,
  PRIMARY KEY (client_id, role)
))sql",
            R"sql(CREATE TABLE IF NOT EXISTS develop_client_socials (
  client_id  text NOT NULL REFERENCES develop_clients(client_id) ON DELETE CASCADE,
  platform   text NOT NULL,
  cookie     text NOT NULL DEFAULT '',
  browse_ua  text NOT NULL DEFAULT '',
  data       jsonb NOT NULL DEFAULT '{}'::jsonb,
  updated_at timestamptz NOT NULL DEFAULT now(),
  PRIMARY KEY (client_id, platform)
))sql",
            R"sql(CREATE INDEX IF NOT EXISTS develop_idx_clients_last_seen ON develop_clients(last_seen))sql",
            R"sql(CREATE INDEX IF NOT EXISTS develop_idx_socials_platform ON develop_client_socials(platform))sql",
            R"sql(ALTER TABLE develop_client_socials ADD COLUMN IF NOT EXISTS browse_ua text NOT NULL DEFAULT '')sql",
            R"sql(CREATE TABLE IF NOT EXISTS develop_retention_policy (
  name      text PRIMARY KEY,
  mode      text NOT NULL,
  updated_at timestamptz NOT NULL DEFAULT now()
))sql"
        #else
            R"sql(CREATE TABLE IF NOT EXISTS clients (
  client_id   text PRIMARY KEY,
  esa_key_enc bytea NOT NULL,
  created_at  timestamptz NOT NULL DEFAULT now(),
  last_seen   timestamptz NOT NULL DEFAULT now(),
  status      smallint NOT NULL DEFAULT 1
))sql",
            R"sql(CREATE TABLE IF NOT EXISTS client_roles (
  client_id text NOT NULL REFERENCES clients(client_id) ON DELETE CASCADE,
  role      text NOT NULL,
  PRIMARY KEY (client_id, role)
))sql",
            R"sql(CREATE TABLE IF NOT EXISTS client_socials (
  client_id  text NOT NULL REFERENCES clients(client_id) ON DELETE CASCADE,
  platform   text NOT NULL,
  cookie     bytea NOT NULL DEFAULT ''::bytea,
  browse_ua  text NOT NULL DEFAULT '',
  data       jsonb NOT NULL DEFAULT '{}'::jsonb,
  updated_at timestamptz NOT NULL DEFAULT now(),
  PRIMARY KEY (client_id, platform)
))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_clients_last_seen ON clients(last_seen))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_socials_platform ON client_socials(platform))sql",
            R"sql(ALTER TABLE client_socials ADD COLUMN IF NOT EXISTS cookie bytea NOT NULL DEFAULT ''::bytea)sql",
            R"sql(ALTER TABLE client_socials ADD COLUMN IF NOT EXISTS browse_ua text NOT NULL DEFAULT '')sql",
            R"sql(CREATE TABLE IF NOT EXISTS retention_policy (
  name      text PRIMARY KEY,
  mode      text NOT NULL,
  updated_at timestamptz NOT NULL DEFAULT now()
))sql"
        #endif
        };

        return std::ranges::all_of(statements, [this](const auto& sql) {
            return execute(sql);
        });
    }

    bool postgres::upsertClient(const std::string& client_id, const std::string& esa_key_enc) const {
        if (!isConnected()) {
            return false;
        }
        const auto encrypted = encryptDb(esa_key_enc);
        if (encrypted.empty() && !esa_key_enc.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            txn.exec(
                "INSERT INTO develop_clients (client_id, esa_key_enc, created_at, last_seen, status) "
                "VALUES ($1, decode($2, 'base64'), now(), now(), 1) "
                "ON CONFLICT (client_id) DO UPDATE SET "
                "esa_key_enc = EXCLUDED.esa_key_enc, "
                "last_seen = now()",
                pqxx::params{client_id, encrypted}
            );
        #else
            txn.exec(
                "INSERT INTO clients (client_id, esa_key_enc, created_at, last_seen, status) "
                "VALUES ($1, decode($2, 'base64'), now(), now(), 1) "
                "ON CONFLICT (client_id) DO UPDATE SET "
                "esa_key_enc = EXCLUDED.esa_key_enc, "
                "last_seen = now()",
                pqxx::params{client_id, encrypted}
            );
        #endif
            txn.commit();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::updateClientLastSeen(const std::string& client_id) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto result = txn.exec(
                "UPDATE develop_clients SET last_seen = now() WHERE client_id = $1",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "UPDATE clients SET last_seen = now() WHERE client_id = $1",
                pqxx::params{client_id}
            );
        #endif
            txn.commit();
            return result.affected_rows() > 0;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::deleteClient(const std::string& client_id) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto result = txn.exec(
                "UPDATE develop_clients SET status = 0 WHERE client_id = $1",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "UPDATE clients SET status = 0 WHERE client_id = $1",
                pqxx::params{client_id}
            );
        #endif
            txn.commit();
            return result.affected_rows() > 0;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::updateClientDeleteStrategy() const {
        if (!isConnected()) {
            return false;
        }
        const char* mode_str = config_.deleteOrDisable ? "true" : "false";
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            txn.exec(
                "INSERT INTO develop_retention_policy (name, mode, updated_at) VALUES ('client', $1, now()) "
                "ON CONFLICT (name) DO UPDATE SET mode = EXCLUDED.mode, updated_at = now()",
                pqxx::params{mode_str}
            );
        #else
            txn.exec(
                "INSERT INTO retention_policy (name, mode, updated_at) VALUES ('client', $1, now()) "
                "ON CONFLICT (name) DO UPDATE SET mode = EXCLUDED.mode, updated_at = now()",
                pqxx::params{mode_str}
            );
        #endif
            txn.commit();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::setAdminRole(const std::string& client_id, bool enabled) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            if (enabled) {
            #ifdef DEVELOP
                txn.exec(
                    "INSERT INTO develop_client_roles (client_id, role) VALUES ($1, 'admin') "
                    "ON CONFLICT DO NOTHING",
                    pqxx::params{client_id}
                );
            #else
                txn.exec(
                    "INSERT INTO client_roles (client_id, role) VALUES ($1, 'admin') "
                    "ON CONFLICT DO NOTHING",
                    pqxx::params{client_id}
                );
            #endif
            } else {
            #ifdef DEVELOP
                txn.exec(
                    "DELETE FROM develop_client_roles WHERE client_id = $1 AND role = 'admin'",
                    pqxx::params{client_id}
                );
            #else
                txn.exec(
                    "DELETE FROM client_roles WHERE client_id = $1 AND role = 'admin'",
                    pqxx::params{client_id}
                );
            #endif
            }
            txn.commit();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::clearAllAdmins() const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            txn.exec("DELETE FROM develop_client_roles WHERE role = 'admin'");
        #else
            txn.exec("DELETE FROM client_roles WHERE role = 'admin'");
        #endif
            txn.commit();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::isAdmin(const std::string& client_id) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto result = txn.exec(
                "SELECT 1 FROM develop_client_roles WHERE client_id = $1 AND role = 'admin' LIMIT 1",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "SELECT 1 FROM client_roles WHERE client_id = $1 AND role = 'admin' LIMIT 1",
                pqxx::params{client_id}
            );
        #endif
            txn.commit();
            return !result.empty();
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::upsertHandler(const socialAPI* handler, const std::string& client_id) const {
        if (handler == nullptr || client_id.empty()) {
            return false;
        }
        HandlerRow row;
        row.client_id = client_id;
        row.platform = handler->support();
        {
            std::vector<HandlerRow> existing;
            if (listClientHandlers(client_id, existing)) {
                for (const auto& item : existing) {
                    if (item.platform == row.platform) {
                        row.browse = item.browse;
                        row.data_json = item.data_json;
                        row.updated_at = item.updated_at;
                        break;
                    }
                }
            }
        }
        handler->writeToDataBase(row);
        return upsertHandler(row);
    }

    bool postgres::upsertHandler(const HandlerRow& row) const {
        if (!isConnected() || row.client_id.empty() || row.platform.empty()) {
            return false;
        }
        const auto encrypted = encryptDb(row.browse.cookie);
        if (encrypted.empty() && !row.browse.cookie.empty()) {
            return false;
        }
        const auto json = row.data_json.empty() ? "{}" : row.data_json;
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            txn.exec(
                "INSERT INTO develop_client_socials (client_id, platform, cookie, browse_ua, data, updated_at) "
                "VALUES ($1, $2, $3, $4, $5::jsonb, now()) "
                "ON CONFLICT (client_id, platform) DO UPDATE SET "
                "cookie = EXCLUDED.cookie, "
                "browse_ua = EXCLUDED.browse_ua, "
                "data = EXCLUDED.data, "
                "updated_at = now()",
                pqxx::params{row.client_id, row.platform, row.browse.cookie, row.browse.ua, json}
            );
        #else
            txn.exec(
                "INSERT INTO client_socials (client_id, platform, cookie, browse_ua, data, updated_at) "
                "VALUES ($1, $2, decode($3, 'base64'), $4, $5::jsonb, now()) "
                "ON CONFLICT (client_id, platform) DO UPDATE SET "
                "cookie = EXCLUDED.cookie, "
                "browse_ua = EXCLUDED.browse_ua, "
                "data = EXCLUDED.data, "
                "updated_at = now()",
                pqxx::params{row.client_id, row.platform, encrypted, row.browse.ua, json}
            );
        #endif
        #if MORE_DETAILS
            say("写入数据库");
        #endif
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            warn("写入数据库失败，报错：",false);
            warn(e.what());
            return false;
        }
    }

    bool postgres::deleteHandler(const std::string& client_id, const std::string& platform) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto result = txn.exec(
                "DELETE FROM develop_client_socials WHERE client_id = $1 AND platform = $2",
                pqxx::params{client_id, platform}
            );
        #else
            const auto result = txn.exec(
                "DELETE FROM client_socials WHERE client_id = $1 AND platform = $2",
                pqxx::params{client_id, platform}
            );
        #endif
            txn.commit();
            return result.affected_rows() > 0;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::listClientHandlers(const std::string& client_id, std::vector<HandlerRow>& out) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto result = txn.exec(
                "SELECT client_id, platform, cookie, browse_ua, data::text AS data, updated_at "
                "FROM develop_client_socials WHERE client_id = $1 ORDER BY platform",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "SELECT client_id, platform, encode(cookie, 'base64') AS cookie, browse_ua, data::text AS data, updated_at "
                "FROM client_socials WHERE client_id = $1 ORDER BY platform",
                pqxx::params{client_id}
            );
        #endif
            txn.commit();
            out.clear();
            out.reserve(result.size());
            for (const auto& row : result) {
                HandlerRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                entry.platform = row["platform"].as<std::string>();
                const auto encrypted = row["cookie"].as<std::string>();
            #ifdef DEVELOP
                entry.browse.cookie = encrypted;
            #else
                const auto decrypted = decryptDb(encrypted);
                if (decrypted.empty() && !encrypted.empty()) {
                    return false;
                }
                entry.browse.cookie = decrypted;
            #endif
                entry.browse.ua = row["browse_ua"].as<std::string>();
                entry.data_json = row["data"].as<std::string>();
                entry.updated_at = row["updated_at"].as<std::string>();
                out.push_back(std::move(entry));
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string postgres::buildConnectionString() const {
        std::string conn;
        conn += "host=" + config_.host;
        conn += " port=" + std::to_string(config_.port);
        conn += " dbname=" + config_.dbname;
        conn += " user=" + config_.user;
        conn += " password=" + config_.password;
        conn += " sslmode=" + config<string>(POSTGRES_SSL_MODE);
        return conn;
    }

    bool postgres::execute(const std::string& sql) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            txn.exec(sql);
            txn.commit();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::clientsInit(std::vector<ClientRow>& clients, std::vector<HandlerRow>& handlers) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto client_result = txn.exec(
                "SELECT client_id, encode(esa_key_enc, 'base64') AS esa_key_enc, status, created_at, last_seen FROM develop_clients"
            );
        #else
            const auto client_result = txn.exec(
                "SELECT client_id, encode(esa_key_enc, 'base64') AS esa_key_enc, status, created_at, last_seen FROM clients"
            );
        #endif
        #ifdef DEVELOP
            const auto handler_result = txn.exec(
                "SELECT client_id, platform, cookie, browse_ua, data::text AS data, updated_at "
                "FROM develop_client_socials"
            );
        #else
            const auto handler_result = txn.exec(
                "SELECT client_id, platform, encode(cookie, 'base64') AS cookie, browse_ua, data::text AS data, updated_at "
                "FROM client_socials"
            );
        #endif
            txn.commit();

            clients.clear();
            clients.reserve(client_result.size());
            for (const auto& row : client_result) {
                ClientRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                const auto enc_key = row["esa_key_enc"].as<std::string>();
                const auto dec_key = decryptDb(enc_key);
                if (dec_key.empty() && !enc_key.empty()) {
                    return false;
                }
                entry.esa_key_enc = dec_key;
                entry.status = row["status"].as<int>();
                entry.created_at = row["created_at"].as<std::string>();
                entry.last_seen = row["last_seen"].as<std::string>();
                clients.push_back(std::move(entry));
            }

            handlers.clear();
            handlers.reserve(handler_result.size());
            for (const auto& row : handler_result) {
                HandlerRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                entry.platform = row["platform"].as<std::string>();
                const auto enc_cookie = row["cookie"].as<std::string>();
            #ifdef DEVELOP
                entry.browse.cookie = enc_cookie;
            #else
                const auto dec_cookie = decryptDb(enc_cookie);
                if (dec_cookie.empty() && !enc_cookie.empty()) {
                    return false;
                }
                entry.browse.cookie = dec_cookie;
            #endif
                entry.browse.ua = row["browse_ua"].as<std::string>();
                entry.data_json = row["data"].as<std::string>();
                entry.updated_at = row["updated_at"].as<std::string>();
                handlers.push_back(std::move(entry));
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::ensureCrypto() const {
    #ifdef DEVELOP
        return true;
    #else
        if (crypto_) {
            return true;
        }
        const auto key = config<std::string>(POSTGRES_ENCRYPT_KEY);
        if (key.empty()) {
            return false;
        }
        const auto raw = decodeKey(key);
        if (raw.empty()) {
            return false;
        }
        crypto_ = std::make_unique<webAPI::SimpleESA>(raw, true);
        return true;
    #endif
    }

    std::string postgres::encryptDb(const std::string& plain) const {
    #ifdef DEVELOP
        return plain;
    #else
        if (plain.empty()) {
            return plain;
        }
        if (!ensureCrypto()) {
            return "";
        }
        return crypto_->encrypt(plain);
    #endif
    }

    std::string postgres::decryptDb(const std::string& cipher) const {
    #ifdef DEVELOP
        return cipher;
    #else
        if (cipher.empty()) {
            return cipher;
        }
        if (!ensureCrypto()) {
            return "";
        }
        return crypto_->decrypt(cipher);
    #endif
    }
}

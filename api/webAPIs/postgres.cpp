#include "postgres.h"
#include <algorithm>
#include <cpr/error.h>
#include <pqxx/pqxx>
#include <sodium.h>

#include "../config.h"
#include "../Util.h"

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

namespace socialAPI {

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
            R"sql(CREATE TABLE IF NOT EXISTS client_cookies (
  client_id  text NOT NULL REFERENCES clients(client_id) ON DELETE CASCADE,
  platform   text NOT NULL,
  cookie bytea NOT NULL,
  updated_at timestamptz NOT NULL DEFAULT now(),
  disabled   boolean NOT NULL DEFAULT false,
  PRIMARY KEY (client_id, platform)
))sql",
            R"sql(CREATE TABLE IF NOT EXISTS client_socials (
  client_id  text NOT NULL REFERENCES clients(client_id) ON DELETE CASCADE,
  platform   text NOT NULL,
  data       jsonb NOT NULL DEFAULT '{}'::jsonb,
  updated_at timestamptz NOT NULL DEFAULT now(),
  PRIMARY KEY (client_id, platform)
))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_clients_last_seen ON clients(last_seen))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_cookies_enabled ON client_cookies(disabled))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_socials_platform ON client_socials(platform))sql",
            R"sql(CREATE TABLE IF NOT EXISTS retention_policy (
  name      text PRIMARY KEY,
  mode      text NOT NULL,
  updated_at timestamptz NOT NULL DEFAULT now()
))sql"
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
            txn.exec(
                "INSERT INTO clients (client_id, esa_key_enc, created_at, last_seen, status) "
                "VALUES ($1, $2, now(), now(), 1) "
                "ON CONFLICT (client_id) DO UPDATE SET "
                "esa_key_enc = EXCLUDED.esa_key_enc, "
                "last_seen = now()",
                pqxx::params{client_id, encrypted}
            );
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
            const auto result = txn.exec(
                "UPDATE clients SET last_seen = now() WHERE client_id = $1",
                pqxx::params{client_id}
            );
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
            const auto result = txn.exec(
                "UPDATE clients SET status = 0 WHERE client_id = $1",
                pqxx::params{client_id}
            );
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
            txn.exec(
                "INSERT INTO retention_policy (name, mode, updated_at) VALUES ('client', $1, now()) "
                "ON CONFLICT (name) DO UPDATE SET mode = EXCLUDED.mode, updated_at = now()",
                pqxx::params{mode_str}
            );
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
                txn.exec(
                    "INSERT INTO client_roles (client_id, role) VALUES ($1, 'admin') "
                    "ON CONFLICT DO NOTHING",
                    pqxx::params{client_id}
                );
            } else {
                txn.exec(
                    "DELETE FROM client_roles WHERE client_id = $1 AND role = 'admin'",
                    pqxx::params{client_id}
                );
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
            txn.exec("DELETE FROM client_roles WHERE role = 'admin'");
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
            const auto result = txn.exec(
                "SELECT 1 FROM client_roles WHERE client_id = $1 AND role = 'admin' LIMIT 1",
                pqxx::params{client_id}
            );
            txn.commit();
            return !result.empty();
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::upsertCookie(const std::string& client_id,
                                const std::string& platform,
                                const std::string& cookie_enc) const {
        if (!isConnected()) {
            return false;
        }
        const auto encrypted = encryptDb(cookie_enc);
        if (encrypted.empty() && !cookie_enc.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            txn.exec(
                "INSERT INTO client_cookies (client_id, platform, cookie, updated_at, disabled) "
                "VALUES ($1, $2, $3, now(), false) "
                "ON CONFLICT (client_id, platform) DO UPDATE SET "
                "cookie = EXCLUDED.cookie, "
                "updated_at = now(), "
                "disabled = false",
                pqxx::params{client_id, platform, encrypted}
            );
            txn.commit();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::deleteCookie(const std::string& client_id, const std::string& platform) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            const auto result = txn.exec(
                "DELETE FROM client_cookies WHERE client_id = $1 AND platform = $2",
                pqxx::params{client_id, platform}
            );
            txn.commit();
            return result.affected_rows() > 0;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::getCookie(const std::string& client_id,
                             const std::string& platform,
                             CookieRow& out) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            const auto result = txn.exec(
                "SELECT client_id, platform, cookie, updated_at, disabled "
                "FROM client_cookies WHERE client_id = $1 AND platform = $2 LIMIT 1",
                pqxx::params{client_id, platform}
            );
            txn.commit();
            if (result.empty()) {
                return false;
            }
            const auto& row = result[0];
            out.client_id = row["client_id"].as<std::string>();
            out.platform = row["platform"].as<std::string>();
            const auto encrypted = row["cookie"].as<std::string>();
            const auto decrypted = decryptDb(encrypted);
            if (decrypted.empty() && !encrypted.empty()) {
                return false;
            }
            out.cookie = decrypted;
            out.updated_at = row["updated_at"].as<std::string>();
            out.disabled = row["disabled"].as<bool>();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::listClientCookies(const std::string& client_id, std::vector<CookieRow>& out) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            const auto result = txn.exec(
                "SELECT client_id, platform, cookie, updated_at, disabled "
                "FROM client_cookies WHERE client_id = $1 ORDER BY platform",
                pqxx::params{client_id}
            );
            txn.commit();
            out.clear();
            out.reserve(result.size());
            for (const auto& row : result) {
                CookieRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                entry.platform = row["platform"].as<std::string>();
                const auto encrypted = row["cookie"].as<std::string>();
                const auto decrypted = decryptDb(encrypted);
                if (decrypted.empty() && !encrypted.empty()) {
                    return false;
                }
                entry.cookie = decrypted;
                entry.updated_at = row["updated_at"].as<std::string>();
                entry.disabled = row["disabled"].as<bool>();
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

    bool postgres::clientsInit(std::vector<ClientRow>& clients, std::vector<CookieRow>& cookies) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            const auto client_result = txn.exec(
                "SELECT client_id, esa_key_enc, status, created_at, last_seen FROM clients"
            );
            const auto cookie_result = txn.exec(
                "SELECT client_id, platform, cookie, updated_at, disabled "
                "FROM client_cookies WHERE disabled = false"
            );
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

            cookies.clear();
            cookies.reserve(cookie_result.size());
            for (const auto& row : cookie_result) {
                CookieRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                entry.platform = row["platform"].as<std::string>();
                const auto enc_cookie = row["cookie"].as<std::string>();
                const auto dec_cookie = decryptDb(enc_cookie);
                if (dec_cookie.empty() && !enc_cookie.empty()) {
                    return false;
                }
                entry.cookie = dec_cookie;
                entry.updated_at = row["updated_at"].as<std::string>();
                entry.disabled = row["disabled"].as<bool>();
                cookies.push_back(std::move(entry));
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::ensureCrypto() const {
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
    }

    std::string postgres::encryptDb(const std::string& plain) const {
        if (plain.empty()) {
            return plain;
        }
        if (!ensureCrypto()) {
            return "";
        }
        return crypto_->encrypt(plain);
    }

    std::string postgres::decryptDb(const std::string& cipher) const {
        if (cipher.empty()) {
            return cipher;
        }
        if (!ensureCrypto()) {
            return "";
        }
        return crypto_->decrypt(cipher);
    }
}

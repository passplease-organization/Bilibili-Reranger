#include "postgres.h"
#include <algorithm>
#include <pqxx/pqxx>
#include <sodium.h>

#include "socialAPI.h"
#include "../utils/config.h"
#include "../utils/Util.h"
#include "../utils/BilibiliInterface.h"
#include "../develop/flags.h"
#include "../pluginInterface.h"

namespace {
    std::string decodeKey(const std::string& key) {
        if (key.size() == crypto_secretbox_KEYBYTES) {
            return key;
        }
        std::vector<unsigned char> bin(key.size());
        size_t bin_len = 0;
        if (sodium_base642bin(bin.data(), bin.size(), key.c_str(), key.size(),
                              "\r\n\t ", &bin_len, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0) {
            return "";
        }
        if (bin_len != crypto_secretbox_KEYBYTES) {
            return "";
        }
        return std::string(reinterpret_cast<const char*>(bin.data()), bin_len);
    }

    std::string normalizeStoredEsaKey(const std::string& key) {
        return decodeKey(key);
    }

    std::string videoKeyFromVideo(const webAPI::Video& video) {
        const std::string url = video.url();
        if (!url.empty())
            return url;
        if (video.mid() != WRONG_MID)
            return std::to_string(video.mid());
        return std::string(video.title()) + "\n" + video.author();
    }

#ifdef DEVELOP
    constexpr const char* kPreCrawlVideosTable = "develop_client_precrawl_videos";
    constexpr const char* kPurgeExpiredClientsFunction = "purge_expired_develop_clients";
#else
    constexpr const char* kPreCrawlVideosTable = "client_precrawl_videos";
    constexpr const char* kPurgeExpiredClientsFunction = "purge_expired_clients";
#endif
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
            cppUtil::warn("Connect to database encounters an error: ");
            cppUtil::warn(e.what());
            cppUtil::warn("Connection string: ");
            cppUtil::warn(buildConnectionString());
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
            R"sql(ALTER TABLE develop_clients ADD COLUMN IF NOT EXISTS created_at timestamptz NOT NULL DEFAULT now())sql",
            R"sql(ALTER TABLE develop_clients ADD COLUMN IF NOT EXISTS last_seen timestamptz NOT NULL DEFAULT now())sql",
            R"sql(ALTER TABLE develop_clients ADD COLUMN IF NOT EXISTS status smallint NOT NULL DEFAULT 1)sql",
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
))sql",
            R"sql(DO $$
BEGIN
  IF EXISTS (
    SELECT 1
    FROM information_schema.columns
    WHERE table_schema = current_schema()
      AND table_name = 'develop_client_precrawl_videos'
      AND column_name = 'task_keyword'
  ) THEN
    DROP TABLE develop_client_precrawl_videos;
  END IF;
END;
$$)sql",
            R"sql(CREATE TABLE IF NOT EXISTS develop_client_precrawl_videos (
  client_id          text NOT NULL,
  platform           text NOT NULL,
  video_key          text NOT NULL,
  video              jsonb NOT NULL,
  tags               jsonb NOT NULL DEFAULT '[]'::jsonb,
  crawled_at         timestamptz NOT NULL DEFAULT now(),
  recommend_count    integer NOT NULL DEFAULT 0,
  score              smallint NOT NULL DEFAULT 0,
  PRIMARY KEY (client_id, platform, video_key)
))sql",
            R"sql(CREATE INDEX IF NOT EXISTS develop_idx_precrawl_tags_gin ON develop_client_precrawl_videos USING GIN(tags jsonb_path_ops))sql",
            R"sql(CREATE INDEX IF NOT EXISTS develop_idx_precrawl_tag_score_lookup ON develop_client_precrawl_videos(client_id, platform, score DESC, crawled_at ASC))sql",
            R"sql(CREATE INDEX IF NOT EXISTS develop_idx_precrawl_client_platform_oldest ON develop_client_precrawl_videos(client_id, platform, crawled_at))sql",
            R"sql(ALTER TABLE develop_client_precrawl_videos ADD COLUMN IF NOT EXISTS tags jsonb NOT NULL DEFAULT '[]'::jsonb)sql",
            R"sql(ALTER TABLE develop_client_precrawl_videos ADD COLUMN IF NOT EXISTS recommend_count integer NOT NULL DEFAULT 0)sql",
            R"sql(ALTER TABLE develop_client_precrawl_videos ADD COLUMN IF NOT EXISTS score smallint NOT NULL DEFAULT 0)sql",
            R"sql(DELETE FROM develop_client_precrawl_videos p
WHERE NOT EXISTS (
  SELECT 1 FROM develop_clients c WHERE c.client_id = p.client_id
))sql",
            R"sql(DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM pg_constraint
    WHERE conname = 'fk_develop_precrawl_client'
      AND conrelid = 'develop_client_precrawl_videos'::regclass
  ) THEN
    ALTER TABLE develop_client_precrawl_videos
      ADD CONSTRAINT fk_develop_precrawl_client
      FOREIGN KEY (client_id) REFERENCES develop_clients(client_id)
      ON DELETE CASCADE;
  END IF;
END;
$$)sql",
            R"sql(CREATE OR REPLACE FUNCTION purge_expired_develop_clients()
RETURNS integer
LANGUAGE plpgsql
AS $$
DECLARE
  deleted_count integer;
BEGIN
  DELETE FROM develop_clients
  WHERE last_seen < now() - interval '3 days';
  GET DIAGNOSTICS deleted_count = ROW_COUNT;

  DELETE FROM develop_client_precrawl_videos p
  WHERE NOT EXISTS (
    SELECT 1 FROM develop_clients c WHERE c.client_id = p.client_id
  );

  RETURN deleted_count;
END;
$$)sql",
            R"sql(CREATE OR REPLACE FUNCTION purge_expired_develop_client_precrawl_videos()
RETURNS trigger
LANGUAGE plpgsql
AS $$
BEGIN
  DELETE FROM develop_client_precrawl_videos
  WHERE crawled_at < now() - interval '3 days';
  RETURN NULL;
END;
$$)sql",
            R"sql(DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM pg_trigger
    WHERE tgname = 'trg_purge_expired_develop_client_precrawl_videos'
      AND tgrelid = 'develop_client_precrawl_videos'::regclass
  ) THEN
    CREATE TRIGGER trg_purge_expired_develop_client_precrawl_videos
    AFTER INSERT OR UPDATE ON develop_client_precrawl_videos
    FOR EACH STATEMENT
    EXECUTE FUNCTION purge_expired_develop_client_precrawl_videos();
  END IF;
END;
$$)sql"
#else
            R"sql(CREATE TABLE IF NOT EXISTS clients (
  client_id   text PRIMARY KEY,
  esa_key_enc bytea NOT NULL,
  created_at  timestamptz NOT NULL DEFAULT now(),
  last_seen   timestamptz NOT NULL DEFAULT now(),
  status      smallint NOT NULL DEFAULT 1
))sql",
            R"sql(ALTER TABLE clients ADD COLUMN IF NOT EXISTS created_at timestamptz NOT NULL DEFAULT now())sql",
            R"sql(ALTER TABLE clients ADD COLUMN IF NOT EXISTS last_seen timestamptz NOT NULL DEFAULT now())sql",
            R"sql(ALTER TABLE clients ADD COLUMN IF NOT EXISTS status smallint NOT NULL DEFAULT 1)sql",
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
))sql",
            R"sql(DO $$
BEGIN
  IF EXISTS (
    SELECT 1
    FROM information_schema.columns
    WHERE table_schema = current_schema()
      AND table_name = 'client_precrawl_videos'
      AND column_name = 'task_keyword'
  ) THEN
    DROP TABLE client_precrawl_videos;
  END IF;
END;
$$)sql",
            R"sql(CREATE TABLE IF NOT EXISTS client_precrawl_videos (
  client_id          text NOT NULL,
  platform           text NOT NULL,
  video_key          text NOT NULL,
  video              jsonb NOT NULL,
  tags               jsonb NOT NULL DEFAULT '[]'::jsonb,
  crawled_at         timestamptz NOT NULL DEFAULT now(),
  recommend_count    integer NOT NULL DEFAULT 0,
  score              smallint NOT NULL DEFAULT 0,
  PRIMARY KEY (client_id, platform, video_key)
))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_precrawl_tags_gin ON client_precrawl_videos USING GIN(tags jsonb_path_ops))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_precrawl_tag_score_lookup ON client_precrawl_videos(client_id, platform, score DESC, crawled_at ASC))sql",
            R"sql(CREATE INDEX IF NOT EXISTS idx_precrawl_client_platform_oldest ON client_precrawl_videos(client_id, platform, crawled_at))sql",
            R"sql(ALTER TABLE client_precrawl_videos ADD COLUMN IF NOT EXISTS tags jsonb NOT NULL DEFAULT '[]'::jsonb)sql",
            R"sql(ALTER TABLE client_precrawl_videos ADD COLUMN IF NOT EXISTS recommend_count integer NOT NULL DEFAULT 0)sql",
            R"sql(ALTER TABLE client_precrawl_videos ADD COLUMN IF NOT EXISTS score smallint NOT NULL DEFAULT 0)sql",
            R"sql(DO $$
BEGIN
  IF EXISTS (
    SELECT 1
    FROM pg_constraint
    WHERE conname = 'fk_precrawl_develop_client'
      AND conrelid = 'client_precrawl_videos'::regclass
  ) THEN
    ALTER TABLE client_precrawl_videos
      DROP CONSTRAINT fk_precrawl_develop_client;
  END IF;
END;
$$)sql",
            R"sql(DELETE FROM client_precrawl_videos p
WHERE NOT EXISTS (
  SELECT 1 FROM clients c WHERE c.client_id = p.client_id
))sql",
            R"sql(DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM pg_constraint
    WHERE conname = 'fk_precrawl_client'
      AND conrelid = 'client_precrawl_videos'::regclass
  ) THEN
    ALTER TABLE client_precrawl_videos
      ADD CONSTRAINT fk_precrawl_client
      FOREIGN KEY (client_id) REFERENCES clients(client_id)
      ON DELETE CASCADE;
  END IF;
END;
$$)sql",
            R"sql(CREATE OR REPLACE FUNCTION purge_expired_clients()
RETURNS integer
LANGUAGE plpgsql
AS $$
DECLARE
  deleted_count integer;
BEGIN
  DELETE FROM clients
  WHERE last_seen < now() - interval '3 days';
  GET DIAGNOSTICS deleted_count = ROW_COUNT;

  DELETE FROM client_precrawl_videos p
  WHERE NOT EXISTS (
    SELECT 1 FROM clients c WHERE c.client_id = p.client_id
  );

  RETURN deleted_count;
END;
$$)sql",
            R"sql(CREATE OR REPLACE FUNCTION purge_expired_client_precrawl_videos()
RETURNS trigger
LANGUAGE plpgsql
AS $$
BEGIN
  DELETE FROM client_precrawl_videos
  WHERE crawled_at < now() - interval '3 days';
  RETURN NULL;
END;
$$)sql",
            R"sql(DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM pg_trigger
    WHERE tgname = 'trg_purge_expired_client_precrawl_videos'
      AND tgrelid = 'client_precrawl_videos'::regclass
  ) THEN
    CREATE TRIGGER trg_purge_expired_client_precrawl_videos
    AFTER INSERT OR UPDATE ON client_precrawl_videos
    FOR EACH STATEMENT
    EXECUTE FUNCTION purge_expired_client_precrawl_videos();
  END IF;
END;
$$)sql"
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
                "last_seen = now(), "
                "status = 1",
                pqxx::params{client_id, encrypted}
            );
        #else
            txn.exec(
                "INSERT INTO clients (client_id, esa_key_enc, created_at, last_seen, status) "
                "VALUES ($1, decode($2, 'base64'), now(), now(), 1) "
                "ON CONFLICT (client_id) DO UPDATE SET "
                "esa_key_enc = EXCLUDED.esa_key_enc, "
                "last_seen = now(), "
                "status = 1",
                pqxx::params{client_id, encrypted}
            );
        #endif
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres execute exception: ", e.what());
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
                "UPDATE develop_clients SET last_seen = now() "
                "WHERE client_id = $1 AND status = 1 "
                "AND last_seen >= now() - interval '3 days'",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "UPDATE clients SET last_seen = now() "
                "WHERE client_id = $1 AND status = 1 "
                "AND last_seen >= now() - interval '3 days'",
                pqxx::params{client_id}
            );
        #endif
            txn.commit();
            return result.affected_rows() > 0;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool postgres::clientActive(const std::string& client_id) const {
        if (!isConnected()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto result = txn.exec(
                "SELECT 1 FROM develop_clients "
                "WHERE client_id = $1 AND status = 1 "
                "AND last_seen >= now() - interval '3 days' "
                "LIMIT 1",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "SELECT 1 FROM clients "
                "WHERE client_id = $1 AND status = 1 "
                "AND last_seen >= now() - interval '3 days' "
                "LIMIT 1",
                pqxx::params{client_id}
            );
        #endif
            txn.commit();
            return !result.empty();
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
            txn.exec(
                std::string("DELETE FROM ") + kPreCrawlVideosTable + " WHERE client_id = $1",
                pqxx::params{client_id}
            );
        #ifdef DEVELOP
            const auto result = txn.exec(
                "DELETE FROM develop_clients WHERE client_id = $1",
                pqxx::params{client_id}
            );
        #else
            const auto result = txn.exec(
                "DELETE FROM clients WHERE client_id = $1",
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
        const char* mode_str = "delete";
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
            // 旧值加载失败时必须拒绝写入，否则 handler 未提供新 cookie 时会
            // 用空值覆盖数据库里已有的 cookie
            if (!listClientHandlers(client_id, existing)) {
                cppUtil::warn("读取数据库已有handler失败，放弃写入，避免覆盖旧数据");
                return false;
            }
            for (const auto& item : existing) {
                if (item.platform == row.platform) {
                    row.browse = item.browse;
                    row.data_json = item.data_json;
                    row.updated_at = item.updated_at;
                    break;
                }
            }
        }
        const auto& old_cookie = row.browse.cookie;
        handler->writeToDataBase(row);
        if (row.browse.cookie.empty()) {
            if (!old_cookie.empty())
                cppUtil::warn("handler未提供新cookie（浏览器验证失败或主动清空），保留数据库原cookie");
            else
                cppUtil::warn("handler未提供新cookie且数据库无旧值，将写入空cookie");
        }
        return upsertHandler(row);
    }

    bool postgres::upsertHandler(const HandlerRow& row) const {
        if (!isConnected() || row.client_id.empty() || row.platform.empty()) {
            return false;
        }
        const auto encrypted = encryptDb(row.browse.cookie);
        if (encrypted.empty() && !row.browse.cookie.empty()) {
            cppUtil::warn("cookie加密失败，拒绝写入数据库");
            return false;
        }
        const auto json = row.data_json.empty() ? "{}" : row.data_json;
        if (config<bool>(DETAILS))
            cppUtil::say("更新数据库handler，客户端：",row.client_id,"，cookie：",row.browse.cookie);
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
            txn.commit();
        #if MORE_DETAILS
            cppUtil::say("写入数据库");
        #endif
        // 写后回读验证：连接异常/状态损坏时 commit 可能"假成功"，
        // 必须确认数据真实落库且内容一致才返回 true
        try {
            pqxx::work verify(*connection_);
        #ifdef DEVELOP
            const auto result = verify.exec(
                "SELECT cookie FROM develop_client_socials WHERE client_id = $1 AND platform = $2",
                pqxx::params{row.client_id, row.platform}
            );
        #else
            const auto result = verify.exec(
                "SELECT encode(cookie, 'base64') AS cookie FROM client_socials WHERE client_id = $1 AND platform = $2",
                pqxx::params{row.client_id, row.platform}
            );
        #endif
            verify.commit();
            if (result.empty()) {
                cppUtil::warn("写入数据库后回读不到记录，判定写入失败");
                return false;
            }
            const auto stored_enc = result[0]["cookie"].as<std::string>();
        #ifdef DEVELOP
            const auto stored_cookie = stored_enc;
        #else
            const auto stored_cookie = decryptDb(stored_enc);
        #endif
            if (stored_cookie != row.browse.cookie) {
                cppUtil::warn({false, nullptr}, "写入数据库后回读不一致，判定写入失败，回读cookie长度：");
                cppUtil::warn(stored_cookie.size(), "，预期长度：", row.browse.cookie.size());
                return false;
            }
            cppUtil::say("写入数据库成功");
            return true;
        } catch (const std::exception& e) {
            cppUtil::warn({false, nullptr}, "写入数据库后回读失败，报错：");
            cppUtil::warn(e.what());
            return false;
        }
        } catch (const std::exception& e) {
            cppUtil::warn({false, nullptr}, "写入数据库失败，报错：");
            cppUtil::warn(e.what());
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

    bool postgres::upsertPreCrawlVideos(const std::string& client_id, const std::string& platform,
                                        const std::vector<PreCrawlVideoRow>& videos) const {
        if (!isConnected() || client_id.empty() || platform.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            txn.exec(std::string("DELETE FROM ") + kPreCrawlVideosTable + " WHERE crawled_at < now() - interval '3 days'");
            for (const auto& video : videos) {
                if (video.video_key.empty() || video.video_json.empty()) {
                    return false;
                }
                const Json tags_json = video.tags;
                txn.exec(
                    std::string("INSERT INTO ") + kPreCrawlVideosTable + " "
                    "(client_id, platform, video_key, video, tags, crawled_at, score) "
                    "VALUES ($1, $2, $3, $4::jsonb, $5::jsonb, now(), $6) "
                    "ON CONFLICT (client_id, platform, video_key) "
                    "DO UPDATE SET "
                    "video = EXCLUDED.video, "
                    "tags = EXCLUDED.tags, "
                    "crawled_at = now(), "
                    "score = EXCLUDED.score",
                    pqxx::params{
                        client_id,
                        platform,
                        video.video_key,
                        video.video_json,
                        tags_json.dump(),
                        video.score
                    }
                );
            }
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres upsertPreCrawlVideos exception: ", e.what());
            return false;
        }
    }

    bool postgres::listPreCrawlVideos(const std::string& client_id, const std::string& platform,
                                      const crawlTask::Task* task, std::vector<PreCrawlVideoRow>& out,
                                      const int limit, const int offset) const {
        if (!isConnected() || client_id.empty() || platform.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            txn.exec(std::string("DELETE FROM ") + kPreCrawlVideosTable + " WHERE crawled_at < now() - interval '3 days'");
            pqxx::result result;
            const auto tag = task == nullptr || task->keyword == nullptr ? std::string() : std::string(task->keyword);
            const auto tag_json = Json::array({tag}).dump();
            const auto select_sql =
                "SELECT client_id, platform, video_key, video::text AS video, tags::text AS tags, "
                "crawled_at, recommend_count, score "
                "FROM " + std::string(kPreCrawlVideosTable) + " "
                "WHERE client_id = $1 AND platform = $2 ";
            if (!tag.empty() && limit > 0) {
                if (offset > 0) {
                    result = txn.exec(
                        select_sql +
                        "AND tags @> $3::jsonb "
                        "ORDER BY score DESC, crawled_at ASC "
                        "LIMIT $4 OFFSET $5",
                        pqxx::params{
                            client_id,
                            platform,
                            tag_json,
                            limit,
                            offset
                        }
                    );
                } else {
                    result = txn.exec(
                        select_sql +
                        "AND tags @> $3::jsonb "
                        "ORDER BY score DESC, crawled_at ASC "
                        "LIMIT $4",
                        pqxx::params{
                            client_id,
                            platform,
                            tag_json,
                            limit
                        }
                    );
                }
            } else if (!tag.empty()) {
                result = txn.exec(
                    select_sql +
                    "AND tags @> $3::jsonb "
                    "ORDER BY score DESC, crawled_at ASC",
                    pqxx::params{
                        client_id,
                        platform,
                        tag_json
                    }
                );
            } else if (limit > 0) {
                if (offset > 0) {
                    result = txn.exec(
                        select_sql +
                        "ORDER BY score DESC, crawled_at ASC "
                        "LIMIT $3 OFFSET $4",
                        pqxx::params{client_id, platform, limit, offset}
                    );
                } else {
                    result = txn.exec(
                        select_sql +
                        "ORDER BY score DESC, crawled_at ASC "
                        "LIMIT $3",
                        pqxx::params{client_id, platform, limit}
                    );
                }
            } else {
                result = txn.exec(
                    select_sql +
                    "ORDER BY score DESC, crawled_at ASC",
                    pqxx::params{client_id, platform}
                );
            }
            txn.commit();

            out.clear();
            out.reserve(result.size());
            for (const auto& row : result) {
                PreCrawlVideoRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                entry.platform = row["platform"].as<std::string>();
                entry.video_key = row["video_key"].as<std::string>();
                entry.video_json = row["video"].as<std::string>();
                entry.tags = Json::parse(row["tags"].as<std::string>()).get<std::vector<std::string>>();
                entry.crawled_at = row["crawled_at"].as<std::string>();
                entry.recommend_count = row["recommend_count"].as<int>();
                entry.score = row["score"].as<int>();
                out.push_back(std::move(entry));
            }

            return true;
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres listPreCrawlVideos exception: ", e.what());
            return false;
        }
    }

    bool postgres::countPreCrawlVideos(const std::string& client_id, const std::string& platform,
                                       const crawlTask::Task* task, std::size_t& out) const {
        if (!isConnected() || client_id.empty() || platform.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            txn.exec(std::string("DELETE FROM ") + kPreCrawlVideosTable + " WHERE crawled_at < now() - interval '3 days'");
            const auto tag = task == nullptr || task->keyword == nullptr ? std::string() : std::string(task->keyword);
            const auto result = tag.empty()
                ? txn.exec(
                    "SELECT count(*) AS count "
                    "FROM " + std::string(kPreCrawlVideosTable) + " "
                    "WHERE client_id = $1 AND platform = $2",
                    pqxx::params{client_id, platform}
                )
                : txn.exec(
                    "SELECT count(*) AS count "
                    "FROM " + std::string(kPreCrawlVideosTable) + " "
                    "WHERE client_id = $1 AND platform = $2 "
                    "AND tags @> $3::jsonb",
                    pqxx::params{client_id, platform, Json::array({tag}).dump()}
                );
            txn.commit();
            out = result[0]["count"].as<std::size_t>();
            return true;
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres countPreCrawlVideos exception: ", e.what());
            return false;
        }
    }

    bool postgres::purgeExpiredPreCrawlVideos() const {
        if (!isConnected()) {
            return false;
        }
        return execute(std::string("DELETE FROM ") + kPreCrawlVideosTable + " WHERE crawled_at < now() - interval '3 days'");
    }

    bool postgres::purgeExpiredClients() const {
        if (!isConnected()) {
            return false;
        }
        return execute(std::string("SELECT ") + kPurgeExpiredClientsFunction + "()");
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

    bool postgres::incrementBatchRecommendCount(const std::string& client_id,
                                                  const std::vector<Video>& videos) const {
        if (!isConnected() || client_id.empty() || videos.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            for (const auto& video : videos) {
                const auto video_key = videoKeyFromVideo(video);
                if (video_key.empty()) continue;
                txn.exec(
                    std::string("UPDATE ") + kPreCrawlVideosTable + " "
                    "SET recommend_count = recommend_count + 1 "
                    "WHERE client_id = $1 AND video_key = $2",
                    pqxx::params{client_id, video_key}
                );
            }
            txn.exec(
                std::string("DELETE FROM ") + kPreCrawlVideosTable + " "
                "WHERE client_id = $1 AND recommend_count >= 5",
                pqxx::params{client_id}
            );
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres incrementBatchRecommendCount exception: ", e.what());
            return false;
        }
    }

    bool postgres::deletePreCrawlVideo(const std::string& client_id,
                                        const Video& video) const {
        if (!isConnected() || client_id.empty()) {
            return false;
        }
        const auto video_key = videoKeyFromVideo(video);
        if (video_key.empty()) {
            return false;
        }
        try {
            pqxx::work txn(*connection_);
            txn.exec(
                std::string("DELETE FROM ") + kPreCrawlVideosTable + " "
                "WHERE client_id = $1 AND video_key = $2",
                pqxx::params{client_id, video_key}
            );
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres deletePreCrawlVideo exception: ", e.what());
            return false;
        }
    }

    bool postgres::clientsInit(std::vector<ClientRow>& clients, std::vector<HandlerRow>& handlers) const {
        if (!isConnected()) {
            cppUtil::warn("Postgres clientsInit aborted: database is not connected");
            return false;
        }
        try {
            pqxx::work txn(*connection_);
        #ifdef DEVELOP
            const auto client_result = txn.exec(
                "SELECT client_id, encode(esa_key_enc, 'base64') AS esa_key_enc, status, created_at, last_seen "
                "FROM develop_clients "
                "WHERE status = 1 AND last_seen >= now() - interval '3 days'"
            );
        #else
            const auto client_result = txn.exec(
                "SELECT client_id, regexp_replace(encode(esa_key_enc, 'base64'), E'[\\n\\r]+', '', 'g') AS esa_key_enc, status, created_at, last_seen "
                "FROM clients "
                "WHERE status = 1 AND last_seen >= now() - interval '3 days'"
            );
        #endif
        #ifdef DEVELOP
            const auto handler_result = txn.exec(
                "SELECT client_id, platform, cookie, browse_ua, data::text AS data, updated_at "
                "FROM develop_client_socials s "
                "WHERE EXISTS ("
                "  SELECT 1 FROM develop_clients c "
                "  WHERE c.client_id = s.client_id "
                "  AND c.status = 1 "
                "  AND c.last_seen >= now() - interval '3 days'"
                ")"
            );
        #else
            const auto handler_result = txn.exec(
                "SELECT client_id, platform, regexp_replace(encode(cookie, 'base64'), E'[\\n\\r]+', '', 'g') AS cookie, browse_ua, data::text AS data, updated_at "
                "FROM client_socials s "
                "WHERE EXISTS ("
                "  SELECT 1 FROM clients c "
                "  WHERE c.client_id = s.client_id "
                "  AND c.status = 1 "
                "  AND c.last_seen >= now() - interval '3 days'"
                ")"
            );
        #endif
            txn.commit();

            clients.clear();
            clients.reserve(client_result.size());
            for (const auto& row : client_result) {
                ClientRow entry;
                entry.client_id = row["client_id"].as<std::string>();
                const auto enc_key = row["esa_key_enc"].as<std::string>();
                const auto decrypted_key = decryptDb(enc_key);
                if (decrypted_key.empty() && !enc_key.empty()) {
                    cppUtil::warn((std::string("Postgres clientsInit failed to decrypt client key, client_id=") + entry.client_id +
                        ", enc_key_length=" + std::to_string(enc_key.size())).c_str());
                    return false;
                }
                entry.esa_key_enc = normalizeStoredEsaKey(decrypted_key);
                if (entry.esa_key_enc.empty() && !decrypted_key.empty()) {
                    cppUtil::warn((std::string("Postgres clientsInit failed to normalize client key, client_id=") + entry.client_id +
                        ", decrypted_key_length=" + std::to_string(decrypted_key.size())).c_str());
                    return false;
                }
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
                    cppUtil::warn((std::string("Postgres clientsInit failed to decrypt handler cookie, client_id=") + entry.client_id +
                        ", platform=" + entry.platform + ", enc_cookie_length=" + std::to_string(enc_cookie.size())).c_str());
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
        } catch (const std::exception& e) {
            cppUtil::warn("Postgres clientsInit exception: ", e.what());
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
            cppUtil::warn("Postgres crypto key is empty");
            return false;
        }
        const auto raw = decodeKey(key);
        if (raw.empty()) {
            cppUtil::warn("Postgres crypto key is invalid");
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

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/pqxx>

namespace {

std::string env_or(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return value;
}

std::string sslmode() {
    const char* value = std::getenv("DB_SSLMODE");
    if (value != nullptr && *value != '\0') {
        return value;
    }
    return env_or("POSTGRES_SSL_MODE", "disable");
}

std::string connection_string() {
    return "host=" + env_or("DB_HOST", "postgres") +
           " port=" + env_or("DB_PORT", "5432") +
           " dbname=" + env_or("DB_NAME", "postgres") +
           " user=" + env_or("DB_USER", "noname") +
           " password=" + env_or("DB_PASSWORD", "nopassword") +
           " sslmode=" + sslmode();
}

void print_result(const std::string& title, const pqxx::result& result) {
    std::cout << "\n-- " << title << "\n";
    if (result.empty()) {
        std::cout << "(no rows)\n";
        return;
    }
    for (const auto& row : result) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i != 0) {
                std::cout << " | ";
            }
            std::cout << row[i].c_str();
        }
        std::cout << "\n";
    }
}

bool relation_exists(pqxx::work& txn, const std::string& relation) {
    return txn.exec(
        "SELECT to_regclass($1) IS NOT NULL",
        pqxx::params{relation}
    )[0][0].as<bool>();
}

void print_query_if_exists(pqxx::work& txn, const std::string& title,
                           const std::string& relation, const std::string& sql) {
    if (!relation_exists(txn, relation)) {
        std::cout << "\n-- " << title << "\n" << relation << " does not exist\n";
        return;
    }
    print_result(title, txn.exec(sql));
}

void inspect_client(pqxx::work& txn, const std::string& client_id) {
    std::cout << "\n== Client " << client_id << "\n";
    if (relation_exists(txn, "clients")) {
        print_result("clients match", txn.exec(
            "SELECT client_id, last_seen::text, status::text "
            "FROM clients WHERE client_id = $1",
            pqxx::params{client_id}
        ));
    }
    if (relation_exists(txn, "develop_clients")) {
        print_result("develop_clients match", txn.exec(
            "SELECT client_id, last_seen::text, status::text "
            "FROM develop_clients WHERE client_id = $1",
            pqxx::params{client_id}
        ));
    }
    if (relation_exists(txn, "client_socials")) {
        print_result("client_socials match", txn.exec(
            "SELECT client_id, platform, length(cookie::text)::text AS cookie_text_length, "
            "browse_ua <> '' AS has_ua, updated_at::text "
            "FROM client_socials WHERE client_id = $1 ORDER BY platform",
            pqxx::params{client_id}
        ));
    }
    if (relation_exists(txn, "develop_client_socials")) {
        print_result("develop_client_socials match", txn.exec(
            "SELECT client_id, platform, length(cookie)::text AS cookie_text_length, "
            "browse_ua <> '' AS has_ua, updated_at::text "
            "FROM develop_client_socials WHERE client_id = $1 ORDER BY platform",
            pqxx::params{client_id}
        ));
    }
    if (relation_exists(txn, "client_precrawl_videos")) {
        print_result("client_precrawl_videos match", txn.exec(
            "SELECT client_id, platform, count(*)::text AS rows, "
            "min(crawled_at)::text AS oldest, max(crawled_at)::text AS newest "
            "FROM client_precrawl_videos WHERE client_id = $1 "
            "GROUP BY client_id, platform ORDER BY platform",
            pqxx::params{client_id}
        ));
    }
    if (relation_exists(txn, "develop_client_precrawl_videos")) {
        print_result("develop_client_precrawl_videos match", txn.exec(
            "SELECT client_id, platform, count(*)::text AS rows, "
            "min(crawled_at)::text AS oldest, max(crawled_at)::text AS newest "
            "FROM develop_client_precrawl_videos WHERE client_id = $1 "
            "GROUP BY client_id, platform ORDER BY platform",
            pqxx::params{client_id}
        ));
    }
}

void rollback_insert_check(pqxx::work& txn, const std::string& client_table,
                           const std::string& precrawl_table) {
    if (!relation_exists(txn, client_table) || !relation_exists(txn, precrawl_table)) {
        std::cout << "\n-- rollback insert check: " << precrawl_table << "\n";
        std::cout << "skipped because " << client_table << " or " << precrawl_table
                  << " does not exist\n";
        return;
    }

    std::cout << "\n-- rollback insert check: " << precrawl_table << "\n";
    const auto clients = txn.exec(
        "SELECT client_id FROM " + client_table + " ORDER BY last_seen DESC LIMIT 20"
    );
    if (clients.empty()) {
        std::cout << "skipped because " << client_table << " has no rows\n";
        return;
    }

    for (const auto& row : clients) {
        const auto client_id = row["client_id"].as<std::string>();
        txn.exec(
            "INSERT INTO " + precrawl_table + " "
            "(client_id, platform, task_keyword, task_mode, task_published_day, "
            "task_video_count, task, video_key, video, crawled_at) "
            "VALUES ($1, 'Bilibili', 'probe', 0, 0, 1, '{}'::jsonb, $2, '{}'::jsonb, now()) "
            "ON CONFLICT (client_id, platform, task_keyword, task_mode, task_published_day, video_key) "
            "DO UPDATE SET crawled_at = now()",
            pqxx::params{client_id, "probe-" + client_id}
        );
        std::cout << "insert ok for " << client_id << "\n";
    }
}

void print_overview(pqxx::work& txn) {
    print_result("current_database/current_user", txn.exec(
        "SELECT current_database(), current_user, inet_server_addr()::text, inet_server_port()"
    ));

    print_result("precrawl tables", txn.exec(R"sql(
        SELECT table_name
        FROM information_schema.tables
        WHERE table_schema = 'public'
          AND table_name LIKE '%precrawl%'
        ORDER BY table_name
    )sql"));

    print_result("client tables", txn.exec(R"sql(
        SELECT table_name
        FROM information_schema.tables
        WHERE table_schema = 'public'
          AND table_name IN (
            'clients',
            'develop_clients',
            'client_socials',
            'develop_client_socials',
            'client_precrawl_videos',
            'develop_client_precrawl_videos'
          )
        ORDER BY table_name
    )sql"));

    print_result("precrawl constraints", txn.exec(R"sql(
        SELECT con.conname,
               conrelid::regclass::text AS table_name,
               CASE WHEN confrelid = 0 THEN '-' ELSE confrelid::regclass::text END AS referenced_table,
               pg_get_constraintdef(con.oid) AS definition
        FROM pg_constraint con
        JOIN pg_class cls ON cls.oid = con.conrelid
        WHERE cls.relname IN ('client_precrawl_videos', 'develop_client_precrawl_videos')
        ORDER BY table_name, conname
    )sql"));

    print_result("purge functions", txn.exec(R"sql(
        SELECT p.proname
        FROM pg_proc p
        JOIN pg_namespace n ON n.oid = p.pronamespace
        WHERE n.nspname = 'public'
          AND p.proname LIKE 'purge_expired%client%'
        ORDER BY p.proname
    )sql"));

    print_result("triggers", txn.exec(R"sql(
        SELECT tgname, tgrelid::regclass::text, tgfoid::regproc::text
        FROM pg_trigger
        JOIN pg_class cls ON cls.oid = tgrelid
        WHERE NOT tgisinternal
          AND cls.relname IN ('client_precrawl_videos', 'develop_client_precrawl_videos')
        ORDER BY tgrelid::regclass::text, tgname
    )sql"));

    print_query_if_exists(txn, "clients count", "clients",
                          "SELECT count(*)::text FROM clients");
    print_query_if_exists(txn, "develop_clients count", "develop_clients",
                          "SELECT count(*)::text FROM develop_clients");
    print_query_if_exists(txn, "client_precrawl_videos count", "client_precrawl_videos",
                          "SELECT count(*)::text FROM client_precrawl_videos");
    print_query_if_exists(txn, "develop_client_precrawl_videos count", "develop_client_precrawl_videos",
                          "SELECT count(*)::text FROM develop_client_precrawl_videos");

    if (relation_exists(txn, "clients") && relation_exists(txn, "client_precrawl_videos")) {
        print_result("release orphan precrawl rows", txn.exec(R"sql(
            SELECT p.client_id, count(*)::text
            FROM client_precrawl_videos p
            WHERE NOT EXISTS (SELECT 1 FROM clients c WHERE c.client_id = p.client_id)
            GROUP BY p.client_id
            ORDER BY count(*) DESC, p.client_id
            LIMIT 20
        )sql"));
    }
    if (relation_exists(txn, "develop_clients") && relation_exists(txn, "develop_client_precrawl_videos")) {
        print_result("develop orphan precrawl rows", txn.exec(R"sql(
            SELECT p.client_id, count(*)::text
            FROM develop_client_precrawl_videos p
            WHERE NOT EXISTS (SELECT 1 FROM develop_clients c WHERE c.client_id = p.client_id)
            GROUP BY p.client_id
            ORDER BY count(*) DESC, p.client_id
            LIMIT 20
        )sql"));
    }

    print_query_if_exists(txn, "recent clients", "clients", R"sql(
        SELECT client_id, last_seen::text, status::text
        FROM clients
        ORDER BY last_seen DESC
        LIMIT 20
    )sql");
    print_query_if_exists(txn, "recent develop clients", "develop_clients", R"sql(
        SELECT client_id, last_seen::text, status::text
        FROM develop_clients
        ORDER BY last_seen DESC
        LIMIT 20
    )sql");
    print_query_if_exists(txn, "client socials", "client_socials", R"sql(
        SELECT client_id, platform, length(cookie::text)::text AS cookie_text_length,
               browse_ua <> '' AS has_ua, updated_at::text
        FROM client_socials
        ORDER BY updated_at DESC
        LIMIT 20
    )sql");
    print_query_if_exists(txn, "develop client socials", "develop_client_socials", R"sql(
        SELECT client_id, platform, length(cookie)::text AS cookie_text_length,
               browse_ua <> '' AS has_ua, updated_at::text
        FROM develop_client_socials
        ORDER BY updated_at DESC
        LIMIT 20
    )sql");
}

} // namespace

int main(int argc, char** argv) {
    bool insert_check = false;
    std::vector<std::string> client_ids;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--insert-check") {
            insert_check = true;
        } else if (arg == "--client-id") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--client-id requires a value");
            }
            client_ids.emplace_back(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: run_db_probe.sh [--insert-check] [--client-id ID]\n";
            return 0;
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }

    pqxx::connection conn(connection_string());
    pqxx::work txn(conn);

    print_overview(txn);
    for (const auto& client_id : client_ids) {
        inspect_client(txn, client_id);
    }
    if (insert_check) {
        rollback_insert_check(txn, "clients", "client_precrawl_videos");
        rollback_insert_check(txn, "develop_clients", "develop_client_precrawl_videos");
        txn.abort();
        std::cout << "\nrolled back insert check\n";
        return 0;
    }

    txn.commit();
    return 0;
}

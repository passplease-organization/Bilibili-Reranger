---
name: backend-postgres-debug
description: Debug this backend repository's Postgres database state, especially crawler client rows, handler/social rows, pre-crawl video storage, Release vs DEVELOP table separation, foreign-key failures on client_precrawl_videos, and Docker Compose database connectivity from .devcontainer/docker-compose.yml.
---

# Backend Postgres Debug

## Quick Start

Use the bundled probe first when investigating Postgres-backed crawler problems:

```bash
.codex/skills/backend-postgres-debug/scripts/run_db_probe.sh
```

Run a rollback insert check when `client_precrawl_videos` foreign keys or writes look suspicious:

```bash
.codex/skills/backend-postgres-debug/scripts/run_db_probe.sh --insert-check
```

Inspect one client across client, social, and pre-crawl tables:

```bash
.codex/skills/backend-postgres-debug/scripts/run_db_probe.sh --client-id '<client_id>'
```

The script compiles `scripts/db_probe.cpp` into `/tmp/backend_postgres_db_probe` and runs it. It does not modify project files. `--insert-check` writes probe rows inside one transaction and rolls them back.

## Connection Defaults

The development Compose file `.devcontainer/docker-compose.yml` defines the database service as `postgres`. Unless the environment overrides them, use:

```text
DB_HOST=postgres
DB_PORT=5432
DB_NAME=postgres
DB_USER=noname
DB_PASSWORD=nopassword
DB_SSLMODE=disable
```

The script reads `DB_HOST`, `DB_PORT`, `DB_NAME`, `DB_USER`, `DB_PASSWORD`, and `DB_SSLMODE`. It also accepts `POSTGRES_SSL_MODE` as a fallback for sslmode.

The dev container may not have `psql`, `docker`, or `pip`. Prefer the bundled C++ probe because the repo already has vcpkg-provided `pqxx`/`libpq` dependencies under `/usr/local/vcpkg/installed/x64-linux`.

## What To Check

For Release-mode storage, expect:

- `clients` exists.
- `client_precrawl_videos` exists.
- `fk_precrawl_client` references `clients(client_id)`.
- `client_precrawl_videos` has no orphan rows.
- A frontend `client_id` appears in `clients`.
- A crawler-capable Bilibili client also has a row in `client_socials`.

For DEVELOP/Test-mode storage, expect:

- `develop_clients` exists.
- `develop_client_precrawl_videos` exists.
- `fk_develop_precrawl_client` references `develop_clients(client_id)`.

If `--insert-check` succeeds for the frontend client IDs, the pre-crawl table and foreign key are not blocking writes. Then investigate runtime flow: whether the running binary is current, whether the backend is connected to the same database, whether the client has a handler, and whether scheduled crawling actually produced videos before `syncPreCrawlDataBase()`.

## Safety

Do not print real cookies, ESA keys, admin keys, or `.env` secret values. The probe reports cookie byte/text lengths and boolean status only.

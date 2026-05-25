---
name: test-mode-client
description: Simulate this backend repository's Test-mode HTTP requests with a persistent local client id and ESA key. Use when Codex needs to call or debug backend endpoints documented in src/test/testCode.cpp and README_en.md, initialize a reusable Test client, poll session-based requests, or reproduce /key, /test, /set, /login, /init, /get, /all_category, crawl, and plugin request flows.
---

# Test Mode Client

## Overview

Use `scripts/test_mode_client.py` to talk to the local backend exactly like `src/test/testCode.cpp`: first exchange keys through `/key`, store the resulting `id`, encrypt client bodies with the stored ESA key, decrypt encrypted responses, and poll `/get` for session-based requests.

The state file lives at `state/client.json` inside this skill folder by default. It is local development state and may be regenerated at any time with `init-client --force`.

## Quick Start

Run the script from the repository root:

```bash
python3 .codex/skills/test-mode-client/scripts/test_mode_client.py init-client
python3 .codex/skills/test-mode-client/scripts/test_mode_client.py set-platform bilibili
python3 .codex/skills/test-mode-client/scripts/test_mode_client.py login
python3 .codex/skills/test-mode-client/scripts/test_mode_client.py init
python3 .codex/skills/test-mode-client/scripts/test_mode_client.py crawl math
```

Default backend URL: `http://localhost:23223`. Override with `--base-url` or `BILIBILI_TEST_BASE_URL`.

## Request Commands

Each backend request has a matching command:

- `/key` public key: `key-public`
- `/key` client exchange and state save: `key-exchange`
- Full client initialization: `init-client`
- `/key?id=...` admin login in Test mode: `admin-login`
- `/test`: `test-id`
- `/all_category`: `all-category`
- `/set`: `set-platform <platform>`
- `/login?test=true` plus `/get` polling: `login-status`
- `/login` plus `/get` polling: `login`
- `/init` plus `/get` polling: `init`
- `/get`: `get <session>`
- Root crawl request: `crawl <category>`
- `/plugin` list: `plugin-list`
- `/plugin` request: `plugin-request <plugin> --data-json '{...}'`
- Generic POST escape hatch: `request <path>`
- Test-mode sequence based on `src/test/testCode.cpp`: `test-flow`

## Important Behavior

- The backend program itself does not provide command help; do not rely on backend CLI discovery.
- Prefer the script commands above because endpoint names, parameter names, encryption, and session polling are fixed from `src/test/testCode.cpp` and README.
- `init-client` validates a stored client with `/test`; if the backend restarted and the id is stale, use `init-client --force` or let normal commands auto-initialize.
- `/init` token handling follows the current code: `token` is sent in the encrypted body even though README describes it as a URL parameter.
- Session commands poll `/get?id=<clientId>&session=<session>` until `finished` is true or the retry limit is exhausted.
- In Debug/Release builds, `admin-login` must use the current `admin_client_key` from the running backend's `config/mainConfig.toml`; the default `test` value only matches the Test build shortcut.
- If the backend or plugin closes a connection without an HTTP response, the script reports `status: 0` with a `reason` instead of printing a Python traceback.
- Current code requires a client for `/login`; a no-id `/login` request may return `Need Client Id !` even though README describes no-id platform discovery.

## Script Options

Use `python3 .codex/skills/test-mode-client/scripts/test_mode_client.py <command> --help` for script-side options. The common options are `--base-url`, `--state`, `--timeout`, `--no-auto-init`, and `--compact`.

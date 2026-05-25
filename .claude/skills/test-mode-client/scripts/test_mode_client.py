#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import ctypes
import ctypes.util
import http.client
import json
import os
import socket
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any


CRYPTO_BOX_PUBLICKEYBYTES = 32
CRYPTO_BOX_SEALBYTES = 48
CRYPTO_SECRETBOX_KEYBYTES = 32
CRYPTO_SECRETBOX_NONCEBYTES = 24
CRYPTO_SECRETBOX_MACBYTES = 16

DEFAULT_BASE_URL = os.environ.get("BILIBILI_TEST_BASE_URL", "http://localhost:23223")
DEFAULT_STATE_PATH = Path(__file__).resolve().parents[1] / "state" / "client.json"
DEFAULT_TIMEOUT = int(os.environ.get("BILIBILI_TEST_TIMEOUT", "300"))


class CliError(RuntimeError):
    pass


class Sodium:
    def __init__(self) -> None:
        candidates = [
            ctypes.util.find_library("sodium"),
            "libsodium.so",
            "libsodium.so.23",
            "/lib/x86_64-linux-gnu/libsodium.so.23",
        ]
        last_error: Exception | None = None
        self.lib = None
        for candidate in candidates:
            if not candidate:
                continue
            try:
                self.lib = ctypes.CDLL(candidate)
                break
            except OSError as exc:
                last_error = exc
        if self.lib is None:
            raise CliError(f"Cannot load libsodium: {last_error}")
        if self.lib.sodium_init() < 0:
            raise CliError("libsodium initialization failed")
        self.lib.crypto_box_seal.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_ulonglong,
            ctypes.c_void_p,
        ]
        self.lib.crypto_box_seal.restype = ctypes.c_int
        self.lib.crypto_secretbox_easy.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_ulonglong,
            ctypes.c_void_p,
            ctypes.c_void_p,
        ]
        self.lib.crypto_secretbox_easy.restype = ctypes.c_int
        self.lib.crypto_secretbox_open_easy.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_ulonglong,
            ctypes.c_void_p,
            ctypes.c_void_p,
        ]
        self.lib.crypto_secretbox_open_easy.restype = ctypes.c_int

    @staticmethod
    def b64encode(data: bytes) -> str:
        return base64.b64encode(data).decode("ascii")

    @staticmethod
    def b64decode(text: str) -> bytes:
        return base64.b64decode(text.encode("ascii"), validate=False)

    def seal_b64(self, public_key_b64: str, plain: bytes) -> str:
        public_key = self.b64decode(public_key_b64)
        if len(public_key) != CRYPTO_BOX_PUBLICKEYBYTES:
            raise CliError(f"Unexpected public key length: {len(public_key)}")
        cipher = ctypes.create_string_buffer(len(plain) + CRYPTO_BOX_SEALBYTES)
        plain_buf = ctypes.create_string_buffer(plain)
        public_buf = ctypes.create_string_buffer(public_key)
        ret = self.lib.crypto_box_seal(cipher, plain_buf, len(plain), public_buf)
        if ret != 0:
            raise CliError("crypto_box_seal failed")
        return self.b64encode(cipher.raw)

    def secretbox_encrypt_b64(self, plain: bytes, key: bytes) -> str:
        if len(key) != CRYPTO_SECRETBOX_KEYBYTES:
            raise CliError(f"Unexpected ESA key length: {len(key)}")
        nonce = os.urandom(CRYPTO_SECRETBOX_NONCEBYTES)
        cipher = ctypes.create_string_buffer(len(plain) + CRYPTO_SECRETBOX_MACBYTES)
        plain_buf = ctypes.create_string_buffer(plain)
        nonce_buf = ctypes.create_string_buffer(nonce)
        key_buf = ctypes.create_string_buffer(key)
        ret = self.lib.crypto_secretbox_easy(cipher, plain_buf, len(plain), nonce_buf, key_buf)
        if ret != 0:
            raise CliError("crypto_secretbox_easy failed")
        return self.b64encode(nonce + cipher.raw)

    def secretbox_decrypt_b64(self, cipher_b64: str, key: bytes) -> bytes:
        if len(key) != CRYPTO_SECRETBOX_KEYBYTES:
            raise CliError(f"Unexpected ESA key length: {len(key)}")
        data = self.b64decode(cipher_b64.strip())
        min_len = CRYPTO_SECRETBOX_NONCEBYTES + CRYPTO_SECRETBOX_MACBYTES
        if len(data) < min_len:
            raise CliError("Encrypted payload is too short")
        nonce = data[:CRYPTO_SECRETBOX_NONCEBYTES]
        cipher = data[CRYPTO_SECRETBOX_NONCEBYTES:]
        plain = ctypes.create_string_buffer(len(cipher) - CRYPTO_SECRETBOX_MACBYTES)
        cipher_buf = ctypes.create_string_buffer(cipher)
        nonce_buf = ctypes.create_string_buffer(nonce)
        key_buf = ctypes.create_string_buffer(key)
        ret = self.lib.crypto_secretbox_open_easy(plain, cipher_buf, len(cipher), nonce_buf, key_buf)
        if ret != 0:
            raise CliError("crypto_secretbox_open_easy failed")
        return plain.raw


@dataclass
class HttpResult:
    path: str
    params: dict[str, str]
    status: int
    reason: str
    encrypted_text: str
    text: str
    parsed: Any
    decrypted: bool

    def as_dict(self, include_encrypted: bool = False) -> dict[str, Any]:
        data: dict[str, Any] = {
            "request": {
                "path": self.path,
                "params": self.params,
            },
            "status": self.status,
            "reason": self.reason,
            "ok": 200 <= self.status < 300,
            "decrypted": self.decrypted,
            "text": self.text,
        }
        if self.parsed is not None:
            data["json"] = self.parsed
        if include_encrypted:
            data["encrypted_text"] = self.encrypted_text
        return data


def now_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def dump_json(data: Any) -> str:
    return json.dumps(data, ensure_ascii=False, separators=(",", ":"))


def parse_json_maybe(text: str) -> Any:
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return None


def parse_key_value(items: list[str] | None) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for item in items or []:
        if "=" not in item:
            raise CliError(f"Expected key=value, got: {item}")
        key, value = item.split("=", 1)
        parsed[key] = value
    return parsed


def read_body_arg(args: argparse.Namespace) -> Any | None:
    sources = [
        getattr(args, "body_json", None) is not None,
        getattr(args, "body_file", None) is not None,
        getattr(args, "body_text", None) is not None,
    ]
    if sum(1 for item in sources if item) > 1:
        raise CliError("Use only one of --body-json, --body-file, or --body-text")
    if getattr(args, "body_json", None) is not None:
        return json.loads(args.body_json)
    if getattr(args, "body_file", None) is not None:
        return json.loads(Path(args.body_file).read_text(encoding="utf-8"))
    if getattr(args, "body_text", None) is not None:
        return args.body_text
    return None


class TestModeClient:
    def __init__(
        self,
        base_url: str,
        state_path: Path,
        timeout: int,
        auto_init: bool,
        include_encrypted: bool,
    ) -> None:
        self.base_url = base_url.rstrip("/")
        self.state_path = state_path
        self.timeout = timeout
        self.auto_init = auto_init
        self.include_encrypted = include_encrypted
        self.crypto = Sodium()

    def load_state(self) -> dict[str, Any] | None:
        if not self.state_path.exists():
            return None
        return json.loads(self.state_path.read_text(encoding="utf-8"))

    def save_state(self, state: dict[str, Any]) -> None:
        self.state_path.parent.mkdir(parents=True, exist_ok=True)
        self.state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def key_bytes(self, state: dict[str, Any]) -> bytes:
        return self.crypto.b64decode(state["esa_key_b64"])

    def ensure_client(self, validate: bool = True) -> dict[str, Any]:
        state = self.load_state()
        if state is None or state.get("base_url") != self.base_url:
            if not self.auto_init:
                raise CliError("No client state for this base URL. Run init-client first.")
            return self.init_client(force=True, admin_login=True, validate_after=True)
        if validate and not self.validate_client(state):
            if not self.auto_init:
                raise CliError("Stored client is invalid. Run init-client --force.")
            return self.init_client(force=True, admin_login=True, validate_after=True)
        return state

    def post(
        self,
        path: str,
        params: dict[str, str] | None = None,
        body: Any | None = None,
        use_client: bool = False,
        encrypt_body: bool = True,
        decrypt_response: bool | None = None,
        state: dict[str, Any] | None = None,
    ) -> HttpResult:
        path = "/" + path.lstrip("/") if path else "/"
        params = dict(params or {})
        key: bytes | None = None
        if use_client:
            if state is None:
                state = self.ensure_client(validate=True)
            params.setdefault("id", state["client_id"])
            key = self.key_bytes(state)
        if decrypt_response is None:
            decrypt_response = use_client

        if body is None:
            data = b""
        elif isinstance(body, str):
            plain = body.encode("utf-8")
            data = (
                self.crypto.secretbox_encrypt_b64(plain, key).encode("utf-8")
                if use_client and encrypt_body and key is not None
                else plain
            )
        else:
            plain = dump_json(body).encode("utf-8")
            data = (
                self.crypto.secretbox_encrypt_b64(plain, key).encode("utf-8")
                if use_client and encrypt_body and key is not None
                else plain
            )

        query = urllib.parse.urlencode(params)
        url = f"{self.base_url}{path}"
        if query:
            url = f"{url}?{query}"
        request = urllib.request.Request(
            url,
            data=data,
            method="POST",
            headers={
                "Accept": "application/json",
                "Content-Type": "application/json; charset=utf-8",
            },
        )

        status = 0
        reason = ""
        response_body = b""
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as response:
                status = response.status
                reason = response.reason
                response_body = response.read()
        except urllib.error.HTTPError as exc:
            status = exc.code
            reason = exc.reason
            response_body = exc.read()
        except (TimeoutError, socket.timeout) as exc:
            return HttpResult(
                path=path,
                params=params,
                status=0,
                reason=f"Timeout: {exc}",
                encrypted_text="",
                text="",
                parsed=None,
                decrypted=False,
            )
        except http.client.RemoteDisconnected as exc:
            return HttpResult(
                path=path,
                params=params,
                status=0,
                reason=f"RemoteDisconnected: {exc}",
                encrypted_text="",
                text="",
                parsed=None,
                decrypted=False,
            )
        except urllib.error.URLError as exc:
            raise CliError(f"HTTP request failed: {exc.reason}") from exc

        encrypted_text = response_body.decode("utf-8", errors="replace")
        text = encrypted_text
        decrypted = False
        if decrypt_response and key is not None and encrypted_text:
            text = self.crypto.secretbox_decrypt_b64(encrypted_text, key).decode("utf-8", errors="replace")
            decrypted = True
        return HttpResult(
            path=path,
            params=params,
            status=status,
            reason=reason,
            encrypted_text=encrypted_text,
            text=text,
            parsed=parse_json_maybe(text),
            decrypted=decrypted,
        )

    def key_public(self) -> HttpResult:
        return self.post("/key", decrypt_response=False)

    def init_client(
        self,
        force: bool,
        admin_login: bool,
        validate_after: bool,
        admin_value: str = "test",
    ) -> dict[str, Any]:
        if not force:
            state = self.load_state()
            if state is not None and state.get("base_url") == self.base_url:
                if not validate_after or self.validate_client(state):
                    return state

        public_result = self.key_public()
        if public_result.status != 200 or not isinstance(public_result.parsed, dict):
            raise CliError(f"Cannot get public key: {public_result.text}")
        public_key = public_result.parsed.get("key")
        if not isinstance(public_key, str):
            raise CliError("/key public response does not contain key")

        raw_key = os.urandom(CRYPTO_SECRETBOX_KEYBYTES)
        sealed_key = self.crypto.seal_b64(public_key, raw_key)
        exchange_result = self.post("/key", body={"key": sealed_key}, decrypt_response=False)
        if exchange_result.status != 200:
            raise CliError(f"Cannot exchange client key: {exchange_result.text}")
        decrypted = self.crypto.secretbox_decrypt_b64(exchange_result.text, raw_key).decode("utf-8", errors="replace")
        payload = json.loads(decrypted)
        client_id = payload.get("id")
        if not isinstance(client_id, str) or not client_id:
            raise CliError("/key exchange response does not contain id")

        state = {
            "schema_version": 1,
            "base_url": self.base_url,
            "client_id": client_id,
            "esa_key_b64": self.crypto.b64encode(raw_key),
            "created_at": now_iso(),
            "updated_at": now_iso(),
            "last_verified_at": None,
        }
        self.save_state(state)

        if admin_login:
            admin_result = self.admin_login(admin_value=admin_value, state=state)
            if isinstance(admin_result.parsed, dict):
                returned_id = admin_result.parsed.get("id")
                returned_key = admin_result.parsed.get("key")
                if isinstance(returned_id, str) and returned_id:
                    state["client_id"] = returned_id
                if isinstance(returned_key, str) and returned_key:
                    state["esa_key_b64"] = returned_key
                state["updated_at"] = now_iso()
                self.save_state(state)

        if validate_after and not self.validate_client(state):
            raise CliError("Initialized client did not pass /test validation")
        return state

    def validate_client(self, state: dict[str, Any]) -> bool:
        try:
            result = self.post("/test", use_client=True, state=state)
        except Exception:
            return False
        if result.status == 200 and result.text == "ID still valid !":
            state["last_verified_at"] = now_iso()
            state["updated_at"] = now_iso()
            self.save_state(state)
            return True
        return False

    def admin_login(self, admin_value: str, state: dict[str, Any] | None = None) -> HttpResult:
        if state is None:
            state = self.ensure_client(validate=True)
        body = {"key": state["esa_key_b64"], "admin": admin_value}
        return self.post("/key", body=body, use_client=True, state=state)

    def poll_session(
        self,
        first_result: HttpResult,
        retries: int,
        interval: float,
    ) -> dict[str, Any]:
        if not isinstance(first_result.parsed, dict) or not isinstance(first_result.parsed.get("session"), str):
            return {
                "session": None,
                "error": "first response does not contain a session",
                "first": first_result.as_dict(include_encrypted=self.include_encrypted),
                "last": None,
                "polls": [],
            }
        session = first_result.parsed["session"]
        polls: list[dict[str, Any]] = []
        last: HttpResult | None = None
        for _ in range(retries):
            last = self.post("/get", params={"session": session}, use_client=True)
            polls.append(last.as_dict(include_encrypted=self.include_encrypted))
            if isinstance(last.parsed, dict) and last.parsed.get("finished") is True:
                break
            time.sleep(interval)
        return {
            "session": session,
            "first": first_result.as_dict(include_encrypted=self.include_encrypted),
            "last": last.as_dict(include_encrypted=self.include_encrypted) if last is not None else None,
            "polls": polls,
        }


def emit(args: argparse.Namespace, data: Any) -> None:
    if getattr(args, "compact", False):
        print(json.dumps(data, ensure_ascii=False, separators=(",", ":")))
    else:
        print(json.dumps(data, ensure_ascii=False, indent=2))


def state_public(state: dict[str, Any] | None, show_key: bool = False) -> dict[str, Any] | None:
    if state is None:
        return None
    copied = dict(state)
    if not show_key and "esa_key_b64" in copied:
        copied["esa_key_b64"] = "<hidden>"
    return copied


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Simulate backend Test-mode requests.")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL, help="Backend URL, default: %(default)s")
    parser.add_argument("--state", type=Path, default=DEFAULT_STATE_PATH, help="Client state path")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT, help="HTTP timeout in seconds")
    parser.add_argument("--no-auto-init", action="store_true", help="Do not initialize a client automatically")
    parser.add_argument("--include-encrypted", action="store_true", help="Include encrypted response text in output")
    parser.add_argument("--compact", action="store_true", help="Print compact JSON")

    sub = parser.add_subparsers(dest="command", required=True)

    init_client = sub.add_parser("init-client", help="Create or validate the persistent client id")
    init_client.add_argument("--force", action="store_true", help="Always exchange a fresh client id")
    init_client.add_argument("--skip-admin-login", action="store_true", help="Skip Test-mode admin login")
    init_client.add_argument("--admin-value", default="test", help="Admin value sent to /key")

    key_exchange = sub.add_parser("key-exchange", help="Alias of init-client --force")
    key_exchange.add_argument("--skip-admin-login", action="store_true")
    key_exchange.add_argument("--admin-value", default="test")

    sub.add_parser("key-public", help="Call /key without a body")

    admin = sub.add_parser("admin-login", help="Call /key with id and encrypted admin body")
    admin.add_argument("--admin-value", default="test")

    test = sub.add_parser("test-id", help="Call /test")
    test.add_argument("--without-id", action="store_true", help="Call /test without id to reproduce wrong-id behavior")
    test.add_argument("--check-key", action="store_true", help="Send encrypted {key:<esa_key_b64>} body")

    sub.add_parser("all-category", help="Call /all_category")

    set_platform = sub.add_parser("set-platform", help="Call /set")
    set_platform.add_argument("platform", nargs="?", default="bilibili")

    login_status = sub.add_parser("login-status", help="Call /login?test=true and poll /get")
    login_status.add_argument("--retries", type=int, default=100)
    login_status.add_argument("--interval", type=float, default=5.0)

    login = sub.add_parser("login", help="Call /login and poll /get")
    login.add_argument("--platform", default="bilibili")
    login.add_argument("--width", type=int, default=1000)
    login.add_argument("--height", type=int, default=800)
    login.add_argument("--depth", type=int, default=16)
    login.add_argument("--no-poll", action="store_true")
    login.add_argument("--retries", type=int, default=100)
    login.add_argument("--interval", type=float, default=5.0)

    init = sub.add_parser("init", help="Call /init and poll /get")
    init.add_argument("--token")
    init.add_argument("--no-poll", action="store_true")
    init.add_argument("--retries", type=int, default=100)
    init.add_argument("--interval", type=float, default=5.0)

    get = sub.add_parser("get", help="Call /get")
    get.add_argument("session")

    crawl = sub.add_parser("crawl", help="Call root crawl request")
    crawl.add_argument("category", nargs="?", default="math")
    crawl.add_argument("--prepared", action="store_true")

    sub.add_parser("plugin-list", help="Call /plugin without a body")

    plugin_request = sub.add_parser("plugin-request", help="Call /plugin with plugin and data")
    plugin_request.add_argument("plugin")
    plugin_request.add_argument("--data-json", default="{}")

    request = sub.add_parser("request", help="Generic POST request")
    request.add_argument("path")
    request.add_argument("--param", action="append", help="URL param as key=value")
    request.add_argument("--body-json")
    request.add_argument("--body-file")
    request.add_argument("--body-text")
    request.add_argument("--no-client", action="store_true")
    request.add_argument("--plain-body", action="store_true")
    request.add_argument("--plain-response", action="store_true")

    flow = sub.add_parser("test-flow", help="Run the main flow from src/test/testCode.cpp")
    flow.add_argument("--platform", default="bilibili")
    flow.add_argument("--category", default="math")
    flow.add_argument("--include-login-status", action="store_true")
    flow.add_argument("--retries", type=int, default=100)
    flow.add_argument("--interval", type=float, default=5.0)

    state = sub.add_parser("state", help="Show stored client state")
    state.add_argument("--show-key", action="store_true")

    sub.add_parser("clear-state", help="Delete stored client state")
    return parser


def command_result(client: TestModeClient, args: argparse.Namespace) -> Any:
    if args.command == "init-client":
        state = client.init_client(
            force=args.force,
            admin_login=not args.skip_admin_login,
            validate_after=True,
            admin_value=args.admin_value,
        )
        return {"state": state_public(state)}

    if args.command == "key-exchange":
        state = client.init_client(
            force=True,
            admin_login=not args.skip_admin_login,
            validate_after=True,
            admin_value=args.admin_value,
        )
        return {"state": state_public(state)}

    if args.command == "key-public":
        return client.key_public().as_dict(include_encrypted=args.include_encrypted)

    if args.command == "admin-login":
        return client.admin_login(args.admin_value).as_dict(include_encrypted=args.include_encrypted)

    if args.command == "test-id":
        if args.without_id:
            result = client.post("/test", decrypt_response=False)
        else:
            state = client.ensure_client(validate=False)
            body = {"key": state["esa_key_b64"]} if args.check_key else None
            result = client.post("/test", body=body, use_client=True, state=state)
        return result.as_dict(include_encrypted=args.include_encrypted)

    if args.command == "all-category":
        return client.post("/all_category", decrypt_response=False).as_dict(include_encrypted=args.include_encrypted)

    if args.command == "set-platform":
        result = client.post("/set", params={"platform": args.platform}, use_client=True)
        return result.as_dict(include_encrypted=args.include_encrypted)

    if args.command == "login-status":
        first = client.post("/login", params={"test": "true"}, use_client=True)
        return client.poll_session(first, retries=args.retries, interval=args.interval)

    if args.command == "login":
        body = {
            "platform": args.platform,
            "screen": {
                "width": args.width,
                "height": args.height,
                "depth": args.depth,
            },
        }
        first = client.post("/login", body=body, use_client=True)
        if args.no_poll:
            return first.as_dict(include_encrypted=args.include_encrypted)
        return client.poll_session(first, retries=args.retries, interval=args.interval)

    if args.command == "init":
        body = {"token": args.token} if args.token else None
        first = client.post("/init", body=body, use_client=True)
        if args.no_poll:
            return first.as_dict(include_encrypted=args.include_encrypted)
        return client.poll_session(first, retries=args.retries, interval=args.interval)

    if args.command == "get":
        result = client.post("/get", params={"session": args.session}, use_client=True)
        return result.as_dict(include_encrypted=args.include_encrypted)

    if args.command == "crawl":
        params = {"category": args.category}
        if args.prepared:
            params["prepared"] = "true"
        result = client.post("/", params=params, use_client=True, encrypt_body=False)
        return result.as_dict(include_encrypted=args.include_encrypted)

    if args.command == "plugin-list":
        return client.post("/plugin", decrypt_response=False).as_dict(include_encrypted=args.include_encrypted)

    if args.command == "plugin-request":
        data = json.loads(args.data_json)
        result = client.post("/plugin", body={"plugin": args.plugin, "data": data}, use_client=True)
        return result.as_dict(include_encrypted=args.include_encrypted)

    if args.command == "request":
        result = client.post(
            args.path,
            params=parse_key_value(args.param),
            body=read_body_arg(args),
            use_client=not args.no_client,
            encrypt_body=not args.plain_body,
            decrypt_response=False if args.plain_response else None,
        )
        return result.as_dict(include_encrypted=args.include_encrypted)

    if args.command == "test-flow":
        output: dict[str, Any] = {}
        output["all_category"] = client.post("/all_category", decrypt_response=False).as_dict(
            include_encrypted=args.include_encrypted
        )
        state = client.init_client(force=True, admin_login=True, validate_after=True)
        output["init_client"] = {"state": state_public(state)}
        output["test_id"] = client.post("/test", use_client=True, state=state).as_dict(
            include_encrypted=args.include_encrypted
        )
        output["test_wrong_id"] = client.post("/test", decrypt_response=False).as_dict(
            include_encrypted=args.include_encrypted
        )
        output["admin_login"] = client.admin_login("test", state=state).as_dict(
            include_encrypted=args.include_encrypted
        )
        output["set_platform"] = client.post(
            "/set", params={"platform": args.platform}, use_client=True, state=state
        ).as_dict(include_encrypted=args.include_encrypted)
        if args.include_login_status:
            first_status = client.post("/login", params={"test": "true"}, use_client=True, state=state)
            output["login_status"] = client.poll_session(first_status, args.retries, args.interval)
        first_login = client.post(
            "/login",
            body={
                "platform": args.platform,
                "screen": {"width": 1000, "height": 800, "depth": 16},
            },
            use_client=True,
            state=state,
        )
        output["login"] = client.poll_session(first_login, args.retries, args.interval)
        first_init = client.post("/init", use_client=True, state=state)
        output["init"] = client.poll_session(first_init, args.retries, args.interval)
        output["crawl"] = client.post(
            "/", params={"category": args.category}, use_client=True, state=state, encrypt_body=False
        ).as_dict(include_encrypted=args.include_encrypted)
        return output

    if args.command == "state":
        return {"state": state_public(client.load_state(), show_key=args.show_key)}

    if args.command == "clear-state":
        if client.state_path.exists():
            client.state_path.unlink()
            return {"cleared": True, "state": str(client.state_path)}
        return {"cleared": False, "state": str(client.state_path)}

    raise CliError(f"Unhandled command: {args.command}")


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    client = TestModeClient(
        base_url=args.base_url,
        state_path=args.state,
        timeout=args.timeout,
        auto_init=not args.no_auto_init,
        include_encrypted=args.include_encrypted,
    )
    try:
        emit(args, command_result(client, args))
    except CliError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

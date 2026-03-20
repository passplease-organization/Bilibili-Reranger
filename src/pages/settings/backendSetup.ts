import {showPopup} from "@/component/utils/screen.ts";
import {SimpleESA} from "@/component/utils/SimpleESA.ts";
import {
  getBackendUrl,
  inputPlatform,
  nowPlatform,
  supportPlatform,
  targetPlatform
} from "@/component/settings/settings.ts";
import router from "@/router";

export interface client {
  esa: SimpleESA | null;
  id: string | null;
  valid: boolean;
}

let esa: SimpleESA | null = null;
let id: string | null = null;
let valid = false;
let setupPromise: Promise<client> | null = null;

export function getValid(): boolean {
  return valid;
}

export function getESA(): SimpleESA | null {
  return esa;
}

export function clientID(): string | null {
  return id;
}

export function decrypt(body: string,toJson?: boolean) {
  if (!esa) {
    throw new Error("ESA is not initialized.");
  }
  try {
    const back = esa.decrypt(body);
    return toJson ? JSON.parse(back) : back;
  } catch {
    return "";
  }
}

export function encrypt(crypted: unknown): string {
  if (!esa) {
    throw new Error("ESA is not initialized.");
  }
  if(typeof crypted === "string")
    return esa.encrypt(crypted);
  return esa.encrypt(JSON.stringify(crypted));
}

interface SessionNeededResponse{
  session: string;
}

export interface ResponseFromSession{
  ok: boolean;
  finished: boolean;
  data: unknown;
}

export async function fetchBackend(url: string,init?: RequestInit): Promise<Response> {
  const _init: RequestInit = init ? { ...init } : {};
  if (init?.body != null) {
    const body = init.body as unknown;
    let raw: string;
    if (typeof body === "string") {
      raw = body;
    } else if (
      body &&
      typeof body === "object" &&
      !(body instanceof ArrayBuffer) &&
      !(body instanceof Blob) &&
      !(body instanceof FormData) &&
      !(body instanceof ReadableStream) &&
      !(body instanceof URLSearchParams)
    ) {
      raw = JSON.stringify(body);
    } else {
      throw new Error("Unsupported body type for encrypted request.");
    }
    _init.body = encrypt(raw);
    if(!init.method){
      _init.method = "POST";
    }
    const headers = new Headers(init?.headers);
    headers.set("Content-Type", "text/plain");
    _init.headers = headers;
  }
  let base = getBackendUrl();
  if(!base.endsWith("/"))
    base = base + '/';
  let urlText = url;
  if (urlText.startsWith("/")) {
    urlText = urlText.slice(1);
  }
  const fullUrl = new URL(urlText, base);
  if (id) {
    fullUrl.searchParams.set("id", id);
  }
  const response = await fetch(fullUrl.toString(),_init)
  const POLL_TIMEOUT = 120_000;
  const POLL_INTERVAL = 15_000;
  const encrypted = await response.text();
  if (encrypted === "") {
    return new Response("", {
      status: response.status,
      statusText: response.statusText,
      headers: response.headers,
    });
  }
  const headers = new Headers(response.headers);
  headers.set("Content-Type", "application/json");
  const decrypted = decrypt(encrypted, false);

  let parsed: unknown;
  try {
    parsed = JSON.parse(decrypted as string);
  } catch {
    return new Response(decrypted, {
      status: response.status,
      statusText: response.statusText,
      headers,
    });
  }

  if (parsed && typeof parsed === "object" && "session" in parsed) {
    const sessionResp = parsed as SessionNeededResponse;
    const startTime = Date.now();
    while (Date.now() - startTime < POLL_TIMEOUT) {
      await new Promise(r => setTimeout(r, POLL_INTERVAL));
      const getFullUrl = new URL("get", base);
      if (id)
        getFullUrl.searchParams.set("id", id);
      getFullUrl.searchParams.set("session", sessionResp.session);
      const pollResponse = await fetch(getFullUrl.toString());
      const pollEncrypted = await pollResponse.text();
      if (pollEncrypted === "") {
        await new Promise(r => setTimeout(r, POLL_INTERVAL));
        continue;
      }
      const pollDecrypted = decrypt(pollEncrypted, true) as ResponseFromSession;
      if (pollDecrypted.finished) {
        return new Response(JSON.stringify(pollDecrypted.data), {
          status: pollResponse.status,
          statusText: pollResponse.statusText,
          headers,
        });
      }
    }
    return new Response(null, {
      status: 504,
      statusText: "Polling timeout",
      headers,
    });
  }

  return new Response(decrypted, {
    status: response.status,
    statusText: response.statusText,
    headers,
  });
}

export async function setup(): Promise<client> {
  if (setupPromise) {
    return setupPromise;
  }
  setupPromise = (async () => {
    const backend = getBackendUrl();
    const storedClientId = localStorage.getItem("clientID");
    const storedEsaKey = localStorage.getItem("esaKey");

    if (storedEsaKey && storedClientId) {
      const existingEsa = await SimpleESA.fromKey(storedEsaKey);
      const testResponse = await fetch(`${backend}/test?id=${storedClientId}`);
      if (testResponse.ok) {
        const encryptedResponse = await fetch(
          `${backend}/test?id=${storedClientId}`,
          {
            method: "POST",
            headers: { "Content-Type": "text/plain" },
            body: existingEsa.encrypt(JSON.stringify({key: storedEsaKey})),
          },
        );
        if (encryptedResponse.ok) {
          esa = existingEsa;
          id = storedClientId;
          valid = true;
          return { esa, id, valid };
        }
      }
    }

    const keyResponse = await fetch(`${backend}/key`);
    if (keyResponse.ok) {
      const { key: rsaKey } = await keyResponse.json();
      const esaKeyBase64 = await SimpleESA.generateKeyBase64();
      const newEsa = await SimpleESA.fromKey(esaKeyBase64);
      const registerResponse = await fetch(`${backend}/key`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          key: await newEsa.rsaEncrypt(rsaKey)
        }),
      });
      if (registerResponse.ok) {
        const payload = await registerResponse.text();
        const parsed = JSON.parse(newEsa.decrypt(payload)) as { id?: string };
        const newId = parsed.id ?? null;
        if (newId) {
          localStorage.setItem("esaKey", esaKeyBase64);
          localStorage.setItem("clientID", newId);
        }
        esa = newEsa;
        id = newId;
        valid = Boolean(newId);
        checkESA();
        return { esa, id, valid };
      }
    }

    esa = null;
    id = null;
    valid = false;
    checkESA();
    return { esa, id, valid };
  })();

  return setupPromise;
}

export async function initBackend(){
  const response = await fetchBackend("/login");
  if(response.ok){
    supportPlatform.value = JSON.parse(await response.text()) as string[];
    const r = await fetchBackend("/init");
    const r2 = await fetchBackend(`/set?platform=${nowPlatform.value}`);
    showPopup(`当前平台：${supportPlatform.value}，初始化情况：${r.ok}，当前平台为${nowPlatform.value}，后端平台配置情况：${r2.ok}`);
    if(!r.ok || !r2.ok)
      router.push("/login");
  }
}

function checkESA(){
  if(!valid)
    showPopup("后端连接失败，请检查！");
}

export async function setPlatform(): Promise<boolean> {
  const response: Response = await fetchBackend(`/set?platform=${inputPlatform.value}`);
  showPopup(`设置${response.ok ? '成功' : '失败'}`);
  if(response.ok) {
    nowPlatform.value = inputPlatform.value;
    inputPlatform.value = "";
    localStorage.setItem(targetPlatform,nowPlatform.value);
  }
  return response.ok;
}

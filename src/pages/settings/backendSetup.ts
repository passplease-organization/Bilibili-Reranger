import {hideLoading, showLoading, showPopup, updateLoading, type LoadingStateInput} from "@/component/utils/screen.ts";
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

export interface BackendRequestInit extends RequestInit {
  loading?: LoadingStateInput | null;
}

function isAbortLike(error: unknown): boolean {
  return error instanceof DOMException && error.name === "AbortError";
}

function createAbortableDelay(ms: number, signal?: AbortSignal): Promise<void> {
  return new Promise((resolve, reject) => {
    if (signal?.aborted) {
      reject(new DOMException("The operation was aborted.", "AbortError"));
      return;
    }
    const timer = window.setTimeout(() => {
      signal?.removeEventListener("abort", onAbort);
      resolve();
    }, ms);
    function onAbort(): void {
      window.clearTimeout(timer);
      signal?.removeEventListener("abort", onAbort);
      reject(new DOMException("The operation was aborted.", "AbortError"));
    }
    signal?.addEventListener("abort", onAbort);
  });
}

export function isAbortError(error: unknown): boolean {
  return isAbortLike(error);
}

export async function fetchBackend(url: string,init?: BackendRequestInit): Promise<Response> {
  const _init: RequestInit = init ? { ...init } : {};
  delete (_init as BackendRequestInit).loading;
  const controller = new AbortController();
  if (init?.signal) {
    if (init.signal.aborted) {
      controller.abort();
    } else {
      init.signal.addEventListener("abort", () => controller.abort(), { once: true });
    }
  }
  _init.signal = controller.signal;
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
    const loadingToken = init?.loading ? showLoading({
      ...init.loading,
      cancelLabel: init.loading.cancelLabel ?? "放弃当前等待",
      onCancel: () => {
        controller.abort();
        showPopup("已放弃当前等待请求", {
          type: "info",
        });
      },
    }) : null;
    try {
      while (Date.now() - startTime < POLL_TIMEOUT) {
        await createAbortableDelay(POLL_INTERVAL, controller.signal);
        const elapsedSeconds = Math.floor((Date.now() - startTime) / 1000);
        const minutes = Math.floor(elapsedSeconds / 60);
        const seconds = elapsedSeconds % 60;
        const elapsedLabel = `${minutes}:${String(seconds).padStart(2, "0")}`;
        const loadingState = init?.loading;
        if (loadingToken !== null && loadingState) {
          updateLoading(loadingToken, {
            ...loadingState,
            detail: `${loadingState.detail ?? "后端正在继续处理任务，请保持当前页面打开。"} 已等待 ${elapsedLabel}`,
            cancelLabel: loadingState.cancelLabel ?? "放弃当前等待",
            onCancel: () => {
              controller.abort();
              showPopup("已放弃当前等待请求", {
                type: "info",
              });
            },
          });
        }
        const getFullUrl = new URL("get", base);
        if (id)
          getFullUrl.searchParams.set("id", id);
        getFullUrl.searchParams.set("session", sessionResp.session);
        const pollResponse = await fetch(getFullUrl.toString(), {
          signal: controller.signal,
        });
        const pollEncrypted = await pollResponse.text();
        if (pollEncrypted === "") {
          await createAbortableDelay(POLL_INTERVAL, controller.signal);
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
    } finally {
      if (loadingToken !== null) {
        hideLoading(loadingToken);
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
  try {
    const response = await fetchBackend("/login", {
      loading: {
        title: "正在同步平台列表",
        detail: "后端正在读取可用平台与登录状态，请稍等。",
      },
    });
    if(response.ok){
      supportPlatform.value = JSON.parse(await response.text()) as string[];
      const r = await fetchBackend("/init", {
        loading: {
          title: "正在初始化推荐后端",
          detail: "后端正在准备推荐与抓取环境，这一步通常比普通请求更慢。",
        },
      });
      const r2 = await fetchBackend(`/set?platform=${nowPlatform.value}`, {
        loading: {
          title: "正在同步当前平台",
          detail: "后端正在切换当前平台设置，请稍等。",
        },
      });
      showPopup(`当前平台：${supportPlatform.value}，初始化情况：${r.ok}，当前平台为${nowPlatform.value}，后端平台配置情况：${r2.ok}`);
      if(!r.ok || !r2.ok)
        router.push("/login");
      return;
    }
    showPopup("初始化失败：获取平台列表失败，请检查后端状态", {
      type: "error",
      durationMs: 4200,
    });
  } catch (error) {
    if (isAbortLike(error)) {
      return;
    }
    showPopup("初始化失败：后端请求异常，请检查服务和代理配置", {
      type: "error",
      durationMs: 4200,
    });
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

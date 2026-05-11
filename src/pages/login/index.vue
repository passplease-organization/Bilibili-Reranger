<script setup lang="ts">
import {computed, ref} from "vue";
import Card from "@/component/Card.vue";
import {getBrowserUrl, inputPlatform, nowPlatform, supportPlatform} from "@/component/settings/settings.ts";
import {fetchBackend, isAbortError, setPlatform} from "@/pages/settings/backendSetup.ts";
import {showPopup} from "@/component/utils/screen.ts";

interface BackendScreen{
  width: number;
  height: number;
  depth: number;
}

interface LoginCacheEntry {
  url: string;
  token: string | null;
  savedAt: number;
}

const LOGIN_CACHE_KEY = "platform-login-cache";
const LOGIN_CACHE_TTL_MS = 2 * 60 * 1000;

let token: string | null;
const activeLoginPlatform = ref<string | null>(null);
const backendInitializing = ref(false);
const platformSetting = ref<string | null>(null);
const loginPending = computed(() => activeLoginPlatform.value !== null);

function readLoginCache(): Record<string, LoginCacheEntry> {
  const raw = localStorage.getItem(LOGIN_CACHE_KEY);
  if (!raw) {
    return {};
  }
  try {
    const parsed = JSON.parse(raw) as Record<string, LoginCacheEntry>;
    return parsed && typeof parsed === "object" ? parsed : {};
  } catch {
    return {};
  }
}

function writeLoginCache(cache: Record<string, LoginCacheEntry>): void {
  localStorage.setItem(LOGIN_CACHE_KEY, JSON.stringify(cache));
}

function getCachedLogin(platform: string): LoginCacheEntry | null {
  const cache = readLoginCache();
  const entry = cache[platform];
  if (!entry) {
    return null;
  }
  if (Date.now() - entry.savedAt > LOGIN_CACHE_TTL_MS) {
    delete cache[platform];
    writeLoginCache(cache);
    return null;
  }
  return entry;
}

function saveLoginCache(platform: string, entry: LoginCacheEntry): void {
  const cache = readLoginCache();
  cache[platform] = entry;
  writeLoginCache(cache);
}

function clearLoginCache(platform: string): void {
  const cache = readLoginCache();
  const entry = cache[platform];
  if (!entry) {
    showPopup(`${platform} 没有可清除的登录缓存`, {
      type: "info",
    });
    return;
  }
  if (token === entry.token) {
    token = null;
  }
  delete cache[platform];
  writeLoginCache(cache);
  showPopup(`已清除 ${platform} 的登录缓存`, {
    type: "success",
  });
}

function showBlockedPopup(url: string): void {
  showPopup("登录窗口被浏览器拦截，请手动打开链接继续登录", {
    type: "error",
    durationMs: 12000,
    link: {
      href: url,
      label: "打开登录链接",
    },
  });
}

function tryOpenLoginUrl(url: string): boolean {
  const opened = window.open(url, "_blank");
  if (!opened) {
    showBlockedPopup(url);
    return false;
  }
  return true;
}

function decoratePendingWindow(target: Window | null, platform: string): void {
  if (!target) {
    return;
  }
  try {
    const dark = document.documentElement.classList.contains("dark");
    const bgTop = dark ? "rgba(91,167,255,0.12)" : "rgba(0,114,245,0.10)";
    const bgBase = dark ? "#050914" : "#f4f8ff";
    const bgBase2 = dark ? "#0d1424" : "#edf4ff";
    const panelBg = dark ? "rgba(12,18,30,0.92)" : "rgba(255,255,255,0.92)";
    const panelBorder = dark ? "rgba(255,255,255,0.08)" : "rgba(255,255,255,0.78)";
    const panelShadow = dark ? "0 24px 60px rgba(0,0,0,0.34)" : "0 20px 50px rgba(15,23,42,0.12)";
    const titleColor = dark ? "rgba(255,255,255,0.94)" : "#0f172a";
    const textColor = dark ? "rgba(255,255,255,0.64)" : "rgba(15,23,42,0.68)";
    target.document.title = `${platform} 登录准备中`;
    target.document.body.innerHTML = `
      <style>
        html, body {
          margin: 0;
          min-height: 100%;
          background: ${bgBase};
        }
        body {
          min-height: 100vh;
        }
        @keyframes spin {
          to { transform: rotate(360deg); }
        }
      </style>
      <div style="margin:0;min-height:100vh;display:grid;place-items:center;background:radial-gradient(circle at top,${bgTop},transparent 28%),linear-gradient(180deg,${bgBase},${bgBase2});font-family:Geist,Inter,sans-serif;color:${titleColor};">
        <div style="display:flex;align-items:center;gap:16px;padding:24px 28px;border:1px solid ${panelBorder};border-radius:24px;background:${panelBg};box-shadow:${panelShadow};backdrop-filter:blur(14px);">
          <div style="position:relative;width:42px;height:42px;flex:0 0 42px;">
            <span style="position:absolute;inset:0;border-radius:50%;border:3px solid rgba(0,114,245,0.18);border-top-color:#0072f5;animation:spin 0.95s linear infinite;"></span>
            <span style="position:absolute;inset:9px;border-radius:50%;border:3px solid rgba(251,114,153,0.20);border-top-color:#fb7299;animation:spin 1.35s linear infinite reverse;"></span>
          </div>
          <div>
            <div style="font-size:18px;font-weight:650;letter-spacing:-0.03em;color:${titleColor};">正在准备 ${platform} 登录</div>
            <div style="margin-top:6px;font-size:14px;line-height:1.6;color:${textColor};">后端正在生成登录链接，请稍等片刻。</div>
          </div>
        </div>
      </div>
    `;
  } catch {}
}

async function requestLoginUrl(platform: string): Promise<LoginCacheEntry | null> {
  type BackendResponse = {
    url: string;
  }
  const response = await fetchBackend('/login',{
    body: JSON.stringify({
      platform: platform,
      screen: {
        width: window.screen.width,
        height: window.screen.height,
        depth: window.screen.colorDepth
      } as BackendScreen
    }),
    loading: {
      title: `正在准备 ${platform} 登录`,
      detail: "后端正在拉起浏览器登录环境，这一步可能需要几十秒。",
    },
  });
  if(!response.ok){
    showPopup("获取登录链接失败", {
      type: "error",
    });
    return null;
  }
  const r: BackendResponse = await response.json() as BackendResponse;
  const url = new URL(getBrowserUrl() + r.url);
  const entry: LoginCacheEntry = {
    url: url.toString(),
    token: url.searchParams.get("token"),
    savedAt: Date.now(),
  };
  saveLoginCache(platform, entry);
  return entry;
}

async function login(platform: string){
  activeLoginPlatform.value = platform;
  try {
  const cached = getCachedLogin(platform);
  if (cached) {
    token = cached.token;
    if (tryOpenLoginUrl(cached.url)) {
      showPopup("已使用缓存登录链接打开窗口", {
        type: "success",
      });
      return;
    }
  }

  const preOpenedWindow = window.open("", "_blank");
  decoratePendingWindow(preOpenedWindow, platform);
  const entry = await requestLoginUrl(platform);
  if (!entry) {
    token = null;
    preOpenedWindow?.close();
    return;
  }

  token = entry.token;

  if (preOpenedWindow) {
    try {
      preOpenedWindow.location.href = entry.url;
      showPopup("打开登录窗口成功", {
        type: "success",
      });
      return;
    } catch {
      preOpenedWindow.close();
    }
  }

  if (tryOpenLoginUrl(entry.url)) {
    showPopup("打开登录窗口成功", {
      type: "success",
    });
  }
  } catch (error) {
    if (isAbortError(error)) {
      showPopup("已取消登录链接请求", {
        type: "info",
      });
      return;
    }
    throw error;
  } finally {
    activeLoginPlatform.value = null;
  }
}

async function initBackend(){
  backendInitializing.value = true;
  try {
    const response = await fetchBackend('/init',{
      body: JSON.stringify({
        token
      }),
      loading: {
        title: "正在初始化后端",
        detail: "后端正在同步登录态并准备抓取环境，请稍等。",
      },
    });
    showPopup(`初始化${response.ok ? '成功' : '失败'}`);
    token = null;
  } catch (error) {
    token = null;
    if (isAbortError(error)) {
      showPopup("已取消初始化请求", {
        type: "info",
      });
      return;
    }
    throw error;
  } finally {
    backendInitializing.value = false;
  }
}

async function _setPlatform(platform: string){
  platformSetting.value = platform;
  inputPlatform.value = platform;
  try {
    showPopup(`设置平台${await setPlatform() ? '成功' : '失败'}`);
  } finally {
    platformSetting.value = null;
  }
}
</script>

<template>
  <div class="page-shell">
    <section class="page-hero">
      <span class="page-kicker">Access</span>
      <h1 class="page-title">登录平台并同步浏览器状态</h1>
      <p class="page-desc">每个平台都会打开独立登录窗口。完成授权后，再初始化后台并把当前平台设为观看来源。</p>
    </section>
    <div class="cards">
      <Card :size="`16px`" v-for="platform in supportPlatform" :title="`${platform} 登录配置`">
        <a class="base-content" style="--base-content-size: 16px;margin-bottom: 8px">配置您的登录状态</a>
        <div class="action-row">
          <button class="btn btn-primary" :disabled="loginPending" @click="login(platform)">
            <span v-if="activeLoginPlatform === platform" class="button-spinner" aria-hidden="true"></span>
            {{ activeLoginPlatform === platform ? "登录处理中" : "登录" }}
          </button>
          <button class="btn btn-ghost" :disabled="backendInitializing" @click="initBackend">
            <span v-if="backendInitializing" class="button-spinner" aria-hidden="true"></span>
            {{ backendInitializing ? "初始化中" : "初始化后台" }}
          </button>
        </div>
        <p class="status-hint">
          {{ activeLoginPlatform === platform ? "正在等待后端返回该平台的登录链接。" : "登录后再初始化后台，最后设为观看来源。" }}
        </p>
        <button style="display: block; margin-top: 10px" class="btn btn-ghost" @click="clearLoginCache(platform)">清除缓存</button>
        <button style="display: block; margin-top: 10px" class="btn btn-primary" :disabled="platformSetting !== null" @click="_setPlatform(platform)">
          <span v-if="platformSetting === platform" class="button-spinner" aria-hidden="true"></span>
          {{ platformSetting === platform ? "设置中" : "设为观看" }}
        </button>
      </Card>
    </div>
  </div>
</template>

<style scoped src="@/component/utils/base-text.css"></style>
<style scoped src="@/component/utils/base-button.css"></style>
<style>
.page-shell{
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.page-hero{
  width: min(720px, 100%);
  padding: 28px 28px 26px;
  border-radius: 28px;
  background:
    linear-gradient(180deg, rgba(11,14,22,0.58), rgba(11,14,22,0.42)),
    url("/signal-ribbon.svg") center/cover no-repeat;
  box-shadow: var(--surface-shadow);
  background-blend-mode: multiply, normal;
}

.page-kicker{
  display: inline-block;
  margin-bottom: 8px;
  color: rgba(255,255,255,0.74);
  font-family: var(--mono-font), monospace;
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.page-title{
  margin: 0;
  font-size: clamp(34px, 4vw, 48px);
  line-height: 1.02;
  letter-spacing: -0.08em;
  color: #ffffff;
}

.page-desc{
  margin: 12px 0 0;
  color: rgba(255,255,255,0.86);
  font-size: 16px;
  line-height: 1.7;
}

html.dark .page-hero {
  background:
    linear-gradient(180deg, rgba(4,6,10,0.76), rgba(4,6,10,0.62)),
    url("/signal-ribbon.svg") center/cover no-repeat;
  background-blend-mode: multiply, normal;
}

.cards{
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  align-items: start;
  gap: 16px;
}

.cards :deep(.card) {
  min-width: 0;
  max-width: none;
  width: 100%;
  margin: 0;
}

.action-row{
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.status-hint{
  margin: 10px 0 0;
  color: var(--dark-font-color);
  font-size: 13px;
  line-height: 1.55;
}

.button-spinner{
  display: inline-block;
  width: 14px;
  height: 14px;
  margin-right: 8px;
  border: 2px solid color-mix(in srgb, currentColor 28%, transparent);
  border-top-color: currentColor;
  border-radius: 50%;
  vertical-align: -2px;
  animation: button-spin 0.9s linear infinite;
}

.btn[disabled]{
  opacity: 0.7;
  cursor: wait;
}

@keyframes button-spin {
  to {
    transform: rotate(360deg);
  }
}

@media (max-width: 640px) {
  .cards {
    grid-template-columns: 1fr;
  }
}
</style>

<script setup lang="ts">
import ThemeButton from "@/component/theme/ThemeButton.vue";
import {IconSettings2} from "@tabler/icons-vue";
import {getValid, initBackend, setup} from "@/pages/settings/backendSetup.ts";
import {onMounted} from "vue";
import Popup from "@/component/utils/Popup.vue";

onMounted(async () => {
  await setup();
  if(getValid())
    await initBackend();
});
</script>

<template>
  <div class="app-shell">
    <header class="app-header">
      <div class="start-header">
        <router-link to="/" class="brand-link">
          <span class="brand-mark" aria-label="bilibili logo">
        <svg
            height="2.5em"
            style="flex: auto; line-height: 1"
            viewBox="0 0 24 24"
            width="3.5em"
            xmlns="http://www.w3.org/2000/svg"
        >
          <title>bilibili</title>
          <path
              clip-rule="evenodd"
              d="M4.977 3.561a1.31 1.31 0 111.818-1.884l2.828 2.728c.08.078.149.163.205.254h4.277a1.32 1.32 0 01.205-.254l2.828-2.728a1.31 1.31 0 011.818 1.884L17.82 4.66h.848A5.333 5.333 0 0124 9.992v7.34a5.333 5.333 0 01-5.333 5.334H5.333A5.333 5.333 0 010 17.333V9.992a5.333 5.333 0 015.333-5.333h.781L4.977 3.56zm.356 3.67a2.667 2.667 0 00-2.666 2.667v7.529a2.667 2.667 0 002.666 2.666h13.334a2.667 2.667 0 002.666-2.666v-7.53a2.667 2.667 0 00-2.666-2.666H5.333zm1.334 5.192a1.333 1.333 0 112.666 0v1.192a1.333 1.333 0 11-2.666 0v-1.192zM16 11.09c-.736 0-1.333.597-1.333 1.333v1.192a1.333 1.333 0 102.666 0v-1.192c0-.736-.597-1.333-1.333-1.333z"
              fill="#FB7299"
              fill-rule="evenodd"
          ></path>
        </svg>
          </span>
        </router-link>
        <nav class="primary-nav">
          <router-link to="/">首页</router-link>
          <router-link to="/login">登录</router-link>
        </nav>
      </div>
      <div class="end-header">
        <ThemeButton/>
        <router-link to="/settings" class="settings-link">
          <span class="setting-icon-shell" aria-hidden="true">
            <IconSettings2 class="setting-icon"/>
          </span>
          <span>设置</span>
        </router-link>
      </div>
    </header>
    <main class="app-main">
      <router-view />
    </main>
    <Popup />
  </div>
</template>

<style>
:root{
  --bg-dark: #0b0b0d;
  --bg-white: #ffffff;
  --important-font: "Geist", "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  --mono-font: "Geist Mono", "SFMono-Regular", "Consolas", monospace;
}

html{
  --background-color: var(--bg-white);
  --font-color: #171717;
  --dark-font-color: #666666;
  --focus-color: #0072f5;
  --dark-focus-color: rgba(0,114,245,0.16);
  --surface-color: rgba(255,255,255,0.92);
  --surface-muted: #fafafa;
  --surface-elevated: rgba(255,255,255,0.96);
  --surface-border: rgba(0, 0, 0, 0.08);
  --surface-shadow:
    rgba(0,0,0,0.08) 0 0 0 1px,
    rgba(0,0,0,0.04) 0 2px 2px,
    rgba(0,0,0,0.04) 0 8px 8px -8px,
    #fafafa 0 0 0 1px;
  --surface-shadow-soft:
    rgba(0,0,0,0.08) 0 0 0 1px,
    rgba(0,0,0,0.04) 0 2px 2px,
    #fafafa 0 0 0 1px;
  --focus-ring: 0 0 0 4px rgba(0,114,245,0.18);
  --page-gutter: clamp(18px, 3vw, 32px);
  --header-height: 80px;
}

html.dark {
  --background-color: var(--bg-dark);
  --focus-color: #5ba7ff;
  --dark-focus-color: rgba(91,167,255,0.18);
  --font-color: rgba(255,255,255,0.92);
  --dark-font-color: rgba(255,255,255,0.56);
  --surface-color: rgba(18,18,21,0.82);
  --surface-muted: rgba(255,255,255,0.03);
  --surface-elevated: rgba(20,20,24,0.94);
  --surface-border: rgba(255,255,255,0.12);
  --surface-shadow:
    rgba(255,255,255,0.08) 0 0 0 1px,
    rgba(0,0,0,0.36) 0 18px 38px -18px,
    rgba(255,255,255,0.04) 0 1px 0 inset;
  --surface-shadow-soft:
    rgba(255,255,255,0.08) 0 0 0 1px,
    rgba(0,0,0,0.22) 0 10px 20px -16px,
    rgba(255,255,255,0.03) 0 1px 0 inset;
  --focus-ring: 0 0 0 4px rgba(91,167,255,0.18);
}

html, body, #app {
  min-height: 100%;
}

body{
  margin: 0;
  background-color: var(--background-color);
  color: var(--font-color);
  font-family: var(--important-font), sans-serif;
  transition: background-color,color 0.5s ease-in;
  background-image:
    radial-gradient(circle at top left, color-mix(in srgb, var(--focus-color) 8%, transparent), transparent 28%),
    radial-gradient(circle at top right, color-mix(in srgb, var(--font-color) 4%, transparent), transparent 22%);
}

a {
  color: inherit;
}

#app {
  min-height: 100vh;
}

.app-shell {
  min-height: 100vh;
}

.app-header {
  position: sticky;
  top: 0;
  z-index: 50;
  display: flex;
  align-items: center;
  justify-content: space-between;
  min-height: var(--header-height);
  padding: 14px var(--page-gutter);
  box-sizing: border-box;
  backdrop-filter: blur(20px);
  background:
    linear-gradient(180deg, rgba(10,12,18,0.56), rgba(10,12,18,0.42)),
    url("/signal-ribbon.svg") center/cover no-repeat,
    color-mix(in srgb, var(--background-color) 82%, transparent);
  box-shadow: 0 1px 0 var(--surface-border);
  background-blend-mode: multiply, normal, normal;
}

.app-main {
  padding: 20px var(--page-gutter) 40px;
}

.start-header {
  display: flex;
  align-items: center;
  gap: clamp(18px, 2vw, 28px);
}

.end-header {
  display: flex;
  align-items: center;
  gap: 16px;
}

.brand-link {
  display: inline-flex;
  align-items: center;
  text-decoration: none;
}

.brand-mark {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 52px;
  height: 52px;
  border-radius: 14px;
}

.primary-nav {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 0;
}

.primary-nav > a,
.settings-link {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  border-radius: 10px;
  text-decoration: none;
  font-size: 16px;
  font-weight: 600;
  letter-spacing: -0.02em;
  color: var(--dark-font-color);
  transition: background-color 160ms ease, color 160ms ease;
}

.primary-nav > a.router-link-exact-active,
.primary-nav > a:hover,
.settings-link:hover {
  background: color-mix(in srgb, var(--font-color) 4%, transparent);
  color: var(--font-color);
}

.settings-link {
  color: var(--dark-font-color);
}

.setting-icon-shell {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  flex: 0 0 22px;
  transition: transform 180ms ease;
}

.setting-icon {
  height: 22px;
  width: 22px;
  display: block;
  transition: color 220ms ease;
}

.settings-link:hover .setting-icon-shell{
  transform: scale(1.12);
}

.settings-link:hover .setting-icon{
  color: var(--focus-color);
}

html.dark .app-header {
  background:
    linear-gradient(180deg, rgba(4,6,10,0.72), rgba(4,6,10,0.58)),
    url("/signal-ribbon.svg") center/cover no-repeat,
    color-mix(in srgb, var(--background-color) 80%, transparent);
  background-blend-mode: multiply, normal, normal;
}

@media (max-width: 900px) {
  .app-header {
    flex-direction: column;
    align-items: stretch;
    gap: 14px;
    padding-top: 16px;
    padding-bottom: 16px;
  }

  .start-header,
  .end-header {
    justify-content: space-between;
  }

  .primary-nav {
    margin-left: auto;
  }
}

@media (max-width: 640px) {
  .start-header {
    flex-wrap: wrap;
  }

  .brand-link {
    width: 100%;
  }

  .primary-nav {
    width: 100%;
    justify-content: space-between;
    border-radius: 18px;
  }

  .end-header {
    justify-content: space-between;
  }
}
</style>

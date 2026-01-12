<script setup lang="ts">
import { Cog6ToothIcon } from '@heroicons/vue/24/solid'
</script>
<script lang="ts">
const proxyKey: string = 'proxy_url'
export function setProxyUrl(proxy: string | null): void {
  if (proxy) localStorage.setItem(proxyKey, proxy)
  else localStorage.removeItem(proxyKey)
}
export function getProxyUrl(url: string | null, handler: string): string {
  const proxy = localStorage.getItem(proxyKey)
  if (url) {
    const encodedUrl = encodeURIComponent(url)
    if (proxy) {
      const separator = encodedUrl.includes('?') ? '&' : '?'
      return `${proxy}${separator}url=${encodedUrl}`
    }
    return `/api/${handler}?url=${encodedUrl}`
  } else {
    return proxy ? proxy : `/api/${handler}`
  }
}
</script>

<template>
  <div class="navbar bg-info/70 text-primary-content">
    <div class="navbar-start">
      <router-link to="/" class="hover-scale">
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
      </router-link>
      <ul class="menu menu-horizontal rounded-box space-x-4">
        <router-link to="/" class="hover-scale">
          <li>
            <a class="category">视频总页面</a>
          </li>
        </router-link>
        <router-link to="/login" class="hover-scale">
          <li>
            <a class="category">登录</a>
          </li>
        </router-link>
      </ul>
    </div>
    <div class="navbar-end">
      <ul class="menu menu-horizontal rounded-box">
        <li>
          <router-link to="/theme" class="category">
            <Cog6ToothIcon class="h-6 w-6" />
          </router-link>
        </li>
      </ul>
    </div>
  </div>
  <router-view />
</template>

<style>
.hover-scale {
  @apply hover:scale-110 delay-50 duration-200;
}
</style>
<style scoped>
.category {
  @apply bg-accent-content/20 hover:bg-base-300 hover:shadow transition delay-150 duration-300 hover:scale-110;
}
</style>

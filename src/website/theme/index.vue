<script setup lang="ts">
import ThemeCard from '@/components/ThemeCard.vue'
import MainContainer from '@/components/MainContainer.vue'

import { useTheme } from '@/website/theme/themesControl.ts'
const { theme: activeTheme, setTheme, themes: themeList } = useTheme()

import { getProxyUrl, setProxyUrl } from '@/website/App.vue'
import { nextTick, onMounted, ref } from 'vue'
import { getBackendUrl, initBackendUrl, setBackendUrl } from '@/website/index.vue'
const proxyURL = ref('')
const nowProxy = ref('')
const backendURL = ref('')
const nowBackend = ref('')
onMounted(() => {
  initBackendUrl()
  nowProxy.value = getProxyUrl(null, '')
  nowBackend.value = getBackendUrl()
})
function setProxy(url: string | null) {
  setProxyUrl(url)
  nowProxy.value = getProxyUrl(null, '')
  proxyURL.value = ''
}
function setBackend(url: string | null) {
  setBackendUrl(url)
  nowBackend.value = getBackendUrl()
  backendURL.value = ''
}
</script>

<template>
  <MainContainer>
    <h1 class="text-base-content">外观设置</h1>
    <div class="py-4"></div>
    <h2>
      <div class="status status-warning animate-bounce status-xl"></div>
      选择一个主题
    </h2>
    <div class="grid grid-cols-2 gap-4 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5">
      <div
        v-for="theme in themeList"
        :key="theme"
        class="overflow-hidden rounded-lg border border-base-content/20 hover:border-base-content/40 cursor-pointer transition-all duration-300"
        :class="{ 'outline outline-offset-2 outline-primary': activeTheme === theme }"
        @click="setTheme(theme)"
      >
        <ThemeCard :theme="theme"></ThemeCard>
      </div>
    </div>
    <div class="py-6"></div>
    <h1>代理配置</h1>
    <div class="py-4"></div>
    <h2>代理服务器配置</h2>
    <p>选择您自己的代理服务器用于加载B站视频图片等，若不设置，将使用Vercel的无服务器函数加载图片</p>
    <input
      type="url"
      v-model="proxyURL"
      :placeholder="nowProxy"
      class="input input-bordered w-1/2"
      @focusin="proxyURL = proxyURL ? proxyURL : nowProxy;nextTick(() => $event.currentTarget.select())"
      @focusout="proxyURL = proxyURL === nowProxy ? '' : proxyURL"
    />
    <button @click="setProxy(proxyURL)" class="btn btn-primary">保存</button>
    <button @click="setProxy(null)" class="btn btn-ghost">重置</button>
    <div class="py-4"></div>
    <h2>后端服务配置</h2>
    <p>配置推荐服务的后端，必须配置！！！若不设置，无法获取推荐的视频！！！</p>
    <input
      type="url"
      v-model="backendURL"
      :placeholder="nowBackend"
      class="input input-bordered w-1/2"
      @click="
        backendURL = backendURL ? backendURL : nowBackend;
        nextTick(() => $event.currentTarget.select())
      "
      @focusout="backendURL = backendURL === nowBackend ? '' : backendURL"
    />
    <button @click="setBackend(backendURL)" class="btn btn-primary">保存</button>
    <button @click="setBackend(null)" class="btn btn-ghost">重置</button>
  </MainContainer>
</template>
<style scoped>
h1 {
  @apply text-4xl font-bold rounded-full;
}
h2 {
  @apply text-3xl font-bold py-2 px-4 rounded-full;
}
p {
  @apply px-4 mb-4;
  font-size: 1.25rem;
}
</style>

<script setup lang="ts">
import ThemeCard from "@/components/ThemeCard.vue";
import MainContainer from "@/components/MainContainer.vue";

import { useTheme } from "@/website/theme/themesControl.ts";
const { theme: activeTheme, setTheme, themes: themeList } = useTheme();

import { getProxyUrl, setProxyUrl } from "@/website/App.vue";
import { nextTick, onMounted, Ref, ref } from "vue";
import { getBackendUrl, initBackendUrl, setBackendUrl } from "@/website/index.vue";
import SettingsCategory from "@/components/SettingsCategory.vue";
import type setCategory from "@/components/settingsInterface.ts";
const proxyURL: Ref<string> = ref<string>("");
const nowProxy: Ref<string> = ref<string>("");
const backendURL: Ref<string> = ref<string>("");
const nowBackend: Ref<string> = ref<string>("");
onMounted(() => {
  initBackendUrl();
  nowProxy.value = getProxyUrl(null, "");
  nowBackend.value = getBackendUrl();
});
function setProxy(url: string | null) {
  setProxyUrl(url);
  nowProxy.value = getProxyUrl(null, "");
  proxyURL.value = "";
}
function setBackend(url: string | null) {
  setBackendUrl(url);
  nowBackend.value = getBackendUrl();
  backendURL.value = "";
}

const settingsList: setCategory[] = [
  {
    name: `代理配置`,
    settings: [
      {
        title: `代理服务器配置`,
        description: `选择您自己的代理服务器用于加载B站视频图片等，若不设置，将使用Vercel的无服务器函数加载图片`,
        input: {
          type: `url`,
          placeholder: nowProxy,
          v_model: proxyURL,
          focusin: function (event: Event) {
            proxyURL.value = proxyURL.value ? proxyURL.value : nowProxy.value;
            nextTick(() => event.currentTarget.select());
          },
          focusout: function () {
            proxyURL.value = proxyURL.value === nowProxy.value ? `` : proxyURL.value;
          },
          save: {
            click: function () {
              setProxy(proxyURL.value);
            },
          },
          reset: {
            click: function () {
              setProxy(null);
            },
          },
        },
      },
      {
        title: `后端服务配置`,
        description: `配置推荐服务的后端，必须配置！！！若不设置，无法获取推荐的视频！！！`,
        input: {
          type: `url`,
          placeholder: nowBackend,
          v_model: backendURL,
          focusin: function (event: Event) {
            backendURL.value = backendURL.value ? backendURL.value : nowBackend.value;
            nextTick(() => event.currentTarget.select());
          },
          focusout: function () {
            backendURL.value = backendURL.value === nowBackend.value ? `` : backendURL.value;
          },
          save: {
            click: function () {
              setBackend(backendURL.value);
            },
          },
          reset: {
            click: function () {
              setBackend(null);
            },
          },
        },
      },
    ],
  },
];
</script>

<template>
  <MainContainer>
    <h1 class="text-base-content text-4xl font-bold rounded-full">外观设置</h1>
    <div class="h-4"></div>
    <h2 class="text-4xl font-bold rounded-full">
      <div class="status status-warning animate-bounce status-xl"></div>
      选择一个主题
    </h2>
    <div class="h-2"></div>
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
    <div class="h-5"></div>
    <SettingsCategory
      v-for="setting in settingsList"
      :key="setting.name"
      :categories="setting"
    />
  </MainContainer>
</template>
<style scoped></style>

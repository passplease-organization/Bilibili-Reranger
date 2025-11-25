<script setup lang="ts">
import ThemeCard from "@/components/ThemeCard.vue";
import MainContainer from "@/components/MainContainer.vue";

import { useTheme } from '@/website/theme/themesControl.ts';
const { theme: activeTheme, setTheme, themes: themeList } = useTheme();
</script>

<template>
  <MainContainer>
    <h2 class="text-2xl font-bold text-secondary py-2 px-4 rounded-full">
      <div class="status status-warning animate-bounce status-xl"></div>
      选择一个主题
    </h2>
    <div class="grid grid-cols-2 gap-4 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5">
      <div
        v-for="theme in themeList"
        :key="theme"
        class="overflow-hidden rounded-lg border border-base-content/20 hover:border-base-content/40 cursor-pointer transition-all duration-300"
        :class="{'outline outline-2 outline-offset-2 outline-primary': activeTheme === theme }"
        @click="setTheme(theme)"
      >
        <ThemeCard :theme="theme"></ThemeCard>
      </div>
    </div>
  </MainContainer>
</template>
<style></style>

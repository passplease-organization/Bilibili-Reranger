<script setup lang="ts">
import settings from "@/component/settings/settings.ts";
import SettingCategory from "@/component/settings/SettingCategory.vue";
import type {zoom_change} from "@/component/Card.vue";
import {reactive} from "vue";

const cards = reactive<Record<string, boolean>>({});
settings.forEach((setting,index) => {
  cards[`card-${index}`] = false;
});
function zoomed(id: string): boolean{
  return cards[id] || false;
}

function handleZoomChange(payload: zoom_change): void {
  if(payload.cardId){
    cards[payload.cardId] = payload.zoom;
  }
}
</script>

<template>
  <div class="settings-page">
    <section class="settings-hero settings-hero-panel">
      <span class="settings-kicker">Configuration</span>
      <h1 class="settings-title">系统设置与平台参数</h1>
      <p class="settings-desc">统一维护平台、后端和浏览器联动配置。放大单张卡片后，会临时占满整行，方便专注修改。</p>
    </section>
    <div class="categories">
      <SettingCategory
          :cardId="`card-${index}`"
          @zoom-change="handleZoomChange"
          v-for="(setting, index) in settings"
          :key="index"
          :setting="setting"
          class="category"
          :class="{zoom: zoomed(`card-${index}`)}"
      />
    </div>
  </div>
</template>

<style>
.settings-page {
  display: flex;
  flex-direction: column;
  gap: 24px;
  align-items: center;
}

.settings-hero {
  width: min(760px, 100%);
}

.settings-hero-panel {
  padding: 28px 28px 26px;
  border-radius: 28px;
  background:
    linear-gradient(180deg, rgba(255,255,255,0.16), rgba(255,255,255,0.06)),
    url("/aurora-console.svg") center/cover no-repeat;
  box-shadow: var(--surface-shadow);
}

.settings-kicker {
  display: inline-block;
  margin-bottom: 8px;
  color: rgba(255,255,255,0.72);
  font-family: var(--mono-font), monospace;
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.settings-title {
  margin: 0;
  font-size: clamp(34px, 4vw, 48px);
  line-height: 1.02;
  letter-spacing: -0.08em;
  color: #ffffff;
}

.settings-desc {
  margin: 12px 0 0;
  color: rgba(255,255,255,0.84);
  font-size: 16px;
  line-height: 1.7;
}

.categories {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  align-items: start;
  gap: 20px;
  width: min(1320px, 100%);
}

.category {
  min-width: 0;
  width: 100%;
}

.category .card {
  width: 100%;
  min-width: 0;
  max-width: none;
  margin: 0;
  box-sizing: border-box;
}

.category.zoom{
  grid-column: 1 / -1;
}

@media (max-width: 980px) {
  .categories {
    grid-template-columns: 1fr;
  }
}
</style>

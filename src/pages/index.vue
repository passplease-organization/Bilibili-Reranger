<script setup lang="ts">
import {onMounted, ref, type Ref} from "vue";
import {fetchBackend, getValid} from "@/pages/settings/backendSetup.ts";
import router from "@/router";
import {showPopup} from "@/component/utils/screen.ts";
import {nowPlatform} from "@/component/settings/settings.ts";
import type {Video, VideoJson} from "@/component/videos/interfaces.ts";
import VideoCard from "@/component/videos/VideoCard.vue";

const categories: Ref<string[] | null> = ref(null);
const categoriesLoading = ref(false);
const videosLoading = ref(false);
onMounted(async () => {
  if(!getValid()){
    await router.push("/login");
    return;
  }
  categoriesLoading.value = true;
  try {
    const response = await fetchBackend("/all_category");
    if(response.ok){
      const json = await response.json();
      categories.value = json[nowPlatform.value] || null;
      if(categories.value)
        return;
    }
  } finally {
    categoriesLoading.value = false;
  }
  showPopup("您的后端无效！请检查！");
});

const videos: Ref<Video[]> = ref([]);
const selectedCategory: Ref<string | null> = ref(null);
async function getVideos(category: string | null){
  selectedCategory.value = category;
  videosLoading.value = true;
  try {
    const response = await fetchBackend(category ? `/?category=${category}` : '/');
    if(response.ok || response.status == 500)
      try{
        const json = await response.json() as VideoJson | null;
        if(json){
          if(category)
            videos.value = json[category] || [];
          else
            videos.value = Object.values(json).flat();
          showPopup(`获取视频成功`);
          return;
        }
      } catch(e) {}
  } finally {
    videosLoading.value = false;
  }
  showPopup(`获取${category ?? "全部"}视频失败，请检查！`, {
    type: "error",
  });
}
</script>

<template>
  <div class="home-container">
    <div class="home-categories">
      <div class="home-sidebar-copy">
        <span class="home-sidebar-kicker">Discovery</span>
        <h1 class="home-sidebar-title">视频目录</h1>
        <p class="home-sidebar-desc">按分类切换来源，让当前平台的视频结果更快进入同一个工作台。</p>
        <p v-if="categoriesLoading" class="home-sidebar-status">分类同步中，请稍等...</p>
      </div>
      <div class="home-category-list">
        <label class="base-content home-category" :class="{ active: selectedCategory === null, busy: videosLoading && selectedCategory === null }" @click="getVideos(null)">
          <span class="home-category-label">
            <span v-if="videosLoading && selectedCategory === null" class="inline-spinner" aria-hidden="true"></span>
            全部视频
          </span>
        </label>
        <label v-for="category in categories" :key="category" class="base-content home-category" :class="{ active: selectedCategory === category, busy: videosLoading && selectedCategory === category }" @click="getVideos(category)">
          <span class="home-category-label">
            <span v-if="videosLoading && selectedCategory === category" class="inline-spinner" aria-hidden="true"></span>
            {{category}}
          </span>
        </label>
      </div>
    </div>
    <div class="home-videos">
      <div class="home-videos-head">
        <div>
          <span class="home-videos-kicker">Workspace</span>
          <h2 class="home-videos-title">{{ selectedCategory ?? "全部视频" }}</h2>
        </div>
        <div class="home-videos-meta">{{ videos.length }} 条结果</div>
      </div>
      <div v-if="videosLoading" class="home-videos-loading">
        <div class="home-videos-loading-visual" aria-hidden="true">
          <span class="home-videos-loading-ring home-videos-loading-ring-outer"></span>
          <span class="home-videos-loading-ring home-videos-loading-ring-middle"></span>
          <span class="home-videos-loading-ring home-videos-loading-ring-inner"></span>
          <span class="home-videos-loading-pulse"></span>
        </div>
        <p class="home-videos-loading-title">正在筛选并整理视频结果</p>
        <p class="home-videos-loading-desc">后端会持续轮询直到新结果准备完成。这个区域会保持活跃状态，避免页面看起来像是卡住。</p>
      </div>
      <a v-else-if="videos.length <= 0">请点击左侧设置视频</a>
      <div v-else class="home-video-grid">
        <VideoCard
          v-for="video in videos"
          :key="video.url"
          :video="video"
        />
      </div>
    </div>
  </div>
</template>

<style scoped src="@/component/utils/base-text.css"></style>
<style scoped>
.home-container {
  --category-sidebar-width: 156px;
  --category-sidebar-gap: 16px;
  --category-sidebar-top: calc(var(--header-height) + 16px);
}

.home-container {
  min-height: calc(100vh - var(--category-sidebar-top));
  padding-top: 12px;
  padding-bottom: 20px;
  background-image:
    radial-gradient(circle at 34% 16%, rgba(79, 140, 255, 0.12), transparent 26%),
    radial-gradient(circle at 82% 22%, rgba(255, 95, 162, 0.10), transparent 24%),
    radial-gradient(circle at 70% 76%, rgba(28, 181, 178, 0.10), transparent 30%);
}

.home-categories {
  position: fixed;
  top: var(--category-sidebar-top);
  left: 0;
  bottom: 24px;
  width: var(--category-sidebar-width);
  padding: 18px;
  overflow-y: auto;
  border-radius: 0 20px 20px 0;
  background-color: color-mix(in srgb, var(--surface-color) 90%, transparent);
  box-shadow: var(--surface-shadow);
  backdrop-filter: blur(16px);
}

.home-sidebar-copy {
  margin-bottom: 18px;
}

.home-sidebar-kicker,
.home-videos-kicker {
  display: inline-block;
  margin-bottom: 8px;
  color: var(--dark-font-color);
  font-family: var(--mono-font), monospace;
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.home-sidebar-title,
.home-videos-title {
  margin: 0;
  font-size: clamp(28px, 2.6vw, 36px);
  font-weight: 600;
  letter-spacing: -0.06em;
  line-height: 1.02;
}

.home-sidebar-desc {
  margin: 10px 0 0;
  color: var(--dark-font-color);
  font-size: 14px;
  line-height: 1.6;
}

.home-sidebar-status {
  margin: 12px 0 0;
  color: var(--focus-color);
  font-size: 12px;
  line-height: 1.5;
}

.home-category-list {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.home-category {
  --base-content-size: 14px;
  cursor: pointer;
  margin: 0;
  padding: 12px 14px;
  border-radius: 12px;
  background: transparent;
  color: var(--dark-font-color);
  text-shadow: none;
  transition:
    transform 0.2s ease,
    background-color 0.22s ease,
    color 0.22s ease,
    box-shadow 0.22s ease;
}

.home-category-label {
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.home-category.active {
  color: var(--focus-color);
  background: color-mix(in srgb, var(--focus-color) 10%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--focus-color) 18%, transparent);
}

.home-category.busy {
  cursor: wait;
}

.home-category:hover {
  transform: translateX(2px);
  background: color-mix(in srgb, var(--font-color) 4%, transparent);
}

.home-videos {
  min-width: 0;
  width: calc(100vw - var(--page-gutter) - var(--category-sidebar-width) - var(--category-sidebar-gap));
  max-width: 1680px;
  margin-left: calc(var(--category-sidebar-width) + var(--category-sidebar-gap));
  margin-right: 0;
  position: relative;
  box-sizing: border-box;
}

.home-videos-head {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 20px;
  padding: 28px 28px 24px;
  border-radius: 28px;
  background:
    linear-gradient(180deg, rgba(255,255,255,0.16), rgba(255,255,255,0.06)),
    url("/aurora-console.svg") center/cover no-repeat;
  box-shadow: var(--surface-shadow);
  color: rgba(255,255,255,0.96);
}

.home-videos-head :deep(*) {
  position: relative;
  z-index: 1;
}

.home-videos-meta {
  padding: 10px 14px;
  border-radius: 999px;
  background: rgba(255,255,255,0.14);
  box-shadow: inset 0 0 0 1px rgba(255,255,255,0.16);
  color: rgba(255,255,255,0.78);
  font-family: var(--mono-font), monospace;
  font-size: 12px;
}

.home-videos-kicker {
  color: rgba(255,255,255,0.72);
}

.home-videos-title {
  color: #ffffff;
}

.home-video-grid {
  display: grid;
  grid-template-columns: repeat(5, minmax(0, 1fr));
  gap: 24px 16px;
  align-items: stretch;
}

.inline-spinner,
.home-videos-loading-ring {
  border-radius: 50%;
  border-style: solid;
  border-top-color: currentColor;
}

.inline-spinner {
  width: 13px;
  height: 13px;
  border-width: 2px;
  border-color: color-mix(in srgb, currentColor 28%, transparent);
  animation: home-spin 0.9s linear infinite;
}

.home-videos-loading {
  display: grid;
  justify-items: center;
  gap: 14px;
  min-height: 320px;
  padding: 46px 28px;
  border: 1px solid color-mix(in srgb, var(--font-color) 10%, transparent);
  border-radius: 30px;
  background:
    radial-gradient(circle at 20% 20%, color-mix(in srgb, var(--focus-color) 10%, transparent), transparent 28%),
    radial-gradient(circle at 78% 30%, rgba(251, 114, 153, 0.10), transparent 26%),
    linear-gradient(180deg, color-mix(in srgb, var(--surface-color) 94%, transparent), color-mix(in srgb, var(--surface-elevated) 88%, transparent));
  box-shadow: var(--surface-shadow-soft);
  text-align: center;
  overflow: hidden;
}

.home-videos-loading-visual {
  position: relative;
  display: grid;
  place-items: center;
  width: 104px;
  height: 104px;
}

.home-videos-loading-ring {
  position: absolute;
  inset: 0;
  border-width: 3px;
}

.home-videos-loading-ring-outer {
  color: var(--focus-color);
  border-color: color-mix(in srgb, var(--focus-color) 18%, transparent);
  animation: home-spin 1.15s linear infinite;
}

.home-videos-loading-ring-middle {
  inset: 13px;
  color: #fb7299;
  border-color: color-mix(in srgb, #fb7299 20%, transparent);
  animation: home-spin-reverse 1.65s linear infinite;
}

.home-videos-loading-ring-inner {
  inset: 28px;
  color: #24b8a9;
  border-color: color-mix(in srgb, #24b8a9 22%, transparent);
  animation: home-spin 1.05s linear infinite;
}

.home-videos-loading-pulse {
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--focus-color), #fb7299);
  box-shadow: 0 0 0 0 color-mix(in srgb, var(--focus-color) 24%, transparent);
  animation: home-pulse 1.8s ease-out infinite;
}

.home-videos-loading-title,
.home-videos-loading-desc {
  margin: 0;
}

.home-videos-loading-title {
  font-size: 20px;
  font-weight: 640;
  letter-spacing: -0.03em;
}

.home-videos-loading-desc {
  max-width: 560px;
  color: var(--dark-font-color);
  font-size: 14px;
  line-height: 1.7;
}

.home-video-grid::before {
  content: "";
  position: absolute;
  inset: 132px 0 auto 0;
  height: 240px;
  border-radius: 32px;
  background:
    radial-gradient(circle at 15% 30%, rgba(79, 140, 255, 0.10), transparent 28%),
    radial-gradient(circle at 82% 22%, rgba(255, 95, 162, 0.10), transparent 26%),
    radial-gradient(circle at 52% 84%, rgba(28, 181, 178, 0.08), transparent 36%);
  filter: blur(12px);
  pointer-events: none;
  z-index: 0;
}

@keyframes home-spin {
  to {
    transform: rotate(360deg);
  }
}

@keyframes home-spin-reverse {
  to {
    transform: rotate(-360deg);
  }
}

@keyframes home-pulse {
  0% {
    transform: scale(0.9);
    box-shadow: 0 0 0 0 color-mix(in srgb, var(--focus-color) 26%, transparent);
  }
  70% {
    transform: scale(1);
    box-shadow: 0 0 0 18px color-mix(in srgb, var(--focus-color) 0%, transparent);
  }
  100% {
    transform: scale(0.92);
    box-shadow: 0 0 0 0 color-mix(in srgb, var(--focus-color) 0%, transparent);
  }
}

.home-video-grid > * {
  position: relative;
  z-index: 1;
}

@media (max-width: 1680px) {
  .home-video-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
}

@media (max-width: 1320px) {
  .home-video-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
}

@media (max-width: 900px) {
  .home-categories {
    position: static;
    width: auto;
    max-height: none;
    overflow: visible;
    backdrop-filter: none;
    margin-bottom: 20px;
    border-radius: 20px;
  }

  .home-videos {
    width: 100%;
    max-width: none;
    margin-left: 0;
    margin-right: 0;
    margin-top: 0;
  }

  .home-videos-head {
    align-items: start;
    flex-direction: column;
    padding: 22px 20px;
  }

  .home-video-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .home-video-grid {
    grid-template-columns: 1fr;
    gap: 18px;
  }
}
</style>

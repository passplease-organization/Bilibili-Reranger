<script setup lang="ts">
import {onMounted, ref, type Ref} from "vue";
import {fetchBackend, getValid} from "@/pages/settings/backendSetup.ts";
import router from "@/router";
import {showPopup} from "@/component/utils/screen.ts";
import {nowPlatform} from "@/component/settings/settings.ts";
import type {Video, VideoJson} from "@/component/videos/interfaces.ts";
import VideoCard from "@/component/videos/VideoCard.vue";

const categories: Ref<string[] | null> = ref(null);
onMounted(async () => {
  if(!getValid()){
    await router.push("/login");
  }
  const response = await fetchBackend("/all_category");
  if(response.ok){
    const json = await response.json();
    categories.value = json[nowPlatform.value] || null;
    if(categories.value)
      return;
  }
  showPopup("您的后端无效！请检查！");
});

const videos: Ref<Video[]> = ref([]);
const selectedCategory: Ref<string | null> = ref(null);
async function getVideos(category: string | null){
  selectedCategory.value = category;
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
  showPopup(`获取${category}视频失败，请检查！`);
}
</script>

<template>
  <div class="home-container">
    <div class="home-categories">
      <div class="home-sidebar-copy">
        <span class="home-sidebar-kicker">Discovery</span>
        <h1 class="home-sidebar-title">视频目录</h1>
        <p class="home-sidebar-desc">按分类切换来源，让当前平台的视频结果更快进入同一个工作台。</p>
      </div>
      <div class="home-category-list">
        <label class="base-content home-category" :class="{ active: selectedCategory === null }" @click="getVideos(null)">全部视频</label>
        <label v-for="category in categories" :key="category" class="base-content home-category" :class="{ active: selectedCategory === category }" @click="getVideos(category)">{{category}}</label>
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
      <a v-if="videos.length <= 0">请点击左侧设置视频</a>
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

.home-category.active {
  color: var(--focus-color);
  background: color-mix(in srgb, var(--focus-color) 10%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--focus-color) 18%, transparent);
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

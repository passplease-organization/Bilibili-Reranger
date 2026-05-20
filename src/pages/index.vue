<script setup lang="ts">
import {computed, onMounted, ref, type Ref} from "vue";
import {fetchBackend, getValid, isAbortError} from "@/pages/settings/backendSetup.ts";
import router from "@/router";
import {showPopup} from "@/component/utils/screen.ts";
import {nowPlatform} from "@/component/settings/settings.ts";
import type {FeedbackRequest, RawVideoPayload, Video, VideoJson} from "@/component/videos/interfaces.ts";
import VideoCard from "@/component/videos/VideoCard.vue";
import VideoFeedbackOverlay from "@/component/videos/VideoFeedbackOverlay.vue";

type Direction = "more" | "less";
type Intent = Direction | "none";

interface FeedbackTemplate {
  id: string;
  label: string;
  description: string;
}

const feedbackTemplates: FeedbackTemplate[] = [
  { id: "more-video", label: "多推这种视频", description: "整体增加当前视频类似内容" },
  { id: "less-video", label: "减少这种视频", description: "整体压低当前视频类似内容" },
  { id: "love-video-author", label: "喜欢这条和作者", description: "同时提升当前视频整体与当前作者" },
  { id: "reduce-video-author", label: "减少这条和作者", description: "同时压低当前视频整体与当前作者" },
  { id: "boost-tags", label: "增加当前标签", description: "把当前视频已知标签整体设为正向" },
  { id: "suppress-tags", label: "减少当前标签", description: "把当前视频已知标签整体设为负向" },
  { id: "hide-once", label: "仅隐藏本条", description: "不改整体偏好，只隐藏这条视频" },
  { id: "strong-like", label: "强烈喜欢", description: "喜爱 + 整体加强 + 作者加强" },
  { id: "strong-dislike", label: "强烈厌恶", description: "厌恶 + 整体减弱 + 作者减弱" },
];
const defaultOverallWeight = 8;
const defaultAuthorWeight = 6;
const defaultTagWeight = 8;

function readString(source: RawVideoPayload, key: string): string {
  const value = source[key];
  return typeof value === "string" ? value : "";
}

function readStringArray(value: unknown): string[] {
  if (!Array.isArray(value)) {
    return [];
  }
  const result = new Set<string>();
  for (const item of value) {
    if (typeof item === "string" && item.trim()) {
      result.add(item.trim());
      continue;
    }
    if (item && typeof item === "object") {
      for (const key of ["label", "name", "value", "text", "title"]) {
        const nested = (item as RawVideoPayload)[key];
        if (typeof nested === "string" && nested.trim()) {
          result.add(nested.trim());
          break;
        }
      }
    }
  }
  return Array.from(result);
}

function readNumber(source: RawVideoPayload, key: string): number {
  const value = source[key];
  return typeof value === "number" ? value : 0;
}

function normalizeWeightInput(raw: string, fallback: number): number {
  const value = Number(raw);
  if (!Number.isFinite(value)) {
    return fallback;
  }
  return Math.max(1, Math.min(16, Math.trunc(value)));
}

function uniq(values: string[]): string[] {
  return Array.from(new Set(values.filter(Boolean)));
}

function mapVideo(raw: unknown): Video | null {
  if (!raw || typeof raw !== "object" || Array.isArray(raw)) {
    return null;
  }
  const source = raw as RawVideoPayload;
  const url = readString(source, "url");
  const title = readString(source, "title");
  if (!url || !title) {
    return null;
  }
  const tags = uniq([
    ...readStringArray(source.tags),
    ...readStringArray(source.tag),
  ]);
  const keywords = uniq(readStringArray(source.keywords));
  return {
    author: readString(source, "author"),
    authorId: readString(source, "authorId") || readString(source, "mid") || readString(source, "uid"),
    category: readString(source, "category"),
    description: readString(source, "description"),
    keywords,
    popups: readNumber(source, "popups"),
    publishTime: readString(source, "publishTime"),
    tags,
    title,
    url,
    videoTime: readString(source, "videoTime"),
    videoURL: readString(source, "videoURL"),
    views: readNumber(source, "views"),
    raw: {...source},
  };
}

function normalizeVideos(source: unknown, category: string | null): Video[] {
  if (!source || typeof source !== "object" || Array.isArray(source)) {
    return [];
  }
  const json = source as VideoJson;
  const records = category
    ? json[category] ?? []
    : Object.values(json).flat();
  return records.map(mapVideo).filter((item): item is Video => item !== null);
}

const categories: Ref<string[] | null> = ref(null);
const categoriesLoading = ref(false);
const videosLoading = ref(false);
const activeFeedbackVideo: Ref<Video | null> = ref(null);
const feedbackSubmitting = ref(false);
const overallIntent = ref<Intent>("none");
const overallWeightInput = ref(String(defaultOverallWeight));
const favorite = ref(false);
const dislike = ref(false);
const hideOnce = ref(false);
const authorIntent = ref<Intent>("none");
const authorWeightInput = ref(String(defaultAuthorWeight));
const tagWeightInput = ref(String(defaultTagWeight));
const selectedMoreTags = ref<string[]>([]);
const selectedLessTags = ref<string[]>([]);

const overallWeight = computed(() => normalizeWeightInput(overallWeightInput.value, defaultOverallWeight));
const authorWeight = computed(() => normalizeWeightInput(authorWeightInput.value, defaultAuthorWeight));
const tagWeight = computed(() => normalizeWeightInput(tagWeightInput.value, defaultTagWeight));
const availableTags = computed(() => {
  const video = activeFeedbackVideo.value;
  if (!video) {
    return [];
  }
  return uniq([...video.tags, ...video.keywords, ...(video.category ? [video.category] : [])]);
});
const derivedLegacyScore = computed(() => {
  let score = 0;
  if (overallIntent.value === "more") {
    score = overallWeight.value;
  } else if (overallIntent.value === "less") {
    score = -overallWeight.value;
  }
  if (favorite.value) {
    score = Math.max(score, 12);
  }
  if (dislike.value) {
    score = Math.min(score, -12);
  }
  return Math.max(-16, Math.min(16, score));
});

function resetFeedbackDraft(): void {
  overallIntent.value = "none";
  overallWeightInput.value = String(defaultOverallWeight);
  favorite.value = false;
  dislike.value = false;
  hideOnce.value = false;
  authorIntent.value = "none";
  authorWeightInput.value = String(defaultAuthorWeight);
  tagWeightInput.value = String(defaultTagWeight);
  selectedMoreTags.value = [];
  selectedLessTags.value = [];
}

function openFeedback(video: Video): void {
  activeFeedbackVideo.value = video;
  resetFeedbackDraft();
}

function closeFeedback(): void {
  if (feedbackSubmitting.value) {
    return;
  }
  activeFeedbackVideo.value = null;
  resetFeedbackDraft();
}

function toggleTag(direction: Direction, value: string): void {
  const source = direction === "more" ? selectedMoreTags : selectedLessTags;
  const opposite = direction === "more" ? selectedLessTags : selectedMoreTags;
  opposite.value = opposite.value.filter((item) => item !== value);
  source.value = source.value.includes(value)
    ? source.value.filter((item) => item !== value)
    : [...source.value, value];
}

function applyFeedbackTemplate(templateId: string): void {
  const video = activeFeedbackVideo.value;
  if (!video) {
    return;
  }
  resetFeedbackDraft();
  switch (templateId) {
    case "more-video":
      overallIntent.value = "more";
      overallWeightInput.value = "8";
      break;
    case "less-video":
      overallIntent.value = "less";
      overallWeightInput.value = "8";
      break;
    case "love-video-author":
      overallIntent.value = "more";
      overallWeightInput.value = "8";
      if (video.author) {
        authorIntent.value = "more";
        authorWeightInput.value = "6";
      }
      break;
    case "reduce-video-author":
      overallIntent.value = "less";
      overallWeightInput.value = "8";
      if (video.author) {
        authorIntent.value = "less";
        authorWeightInput.value = "8";
      }
      break;
    case "boost-tags":
      selectedMoreTags.value = [...availableTags.value];
      tagWeightInput.value = "8";
      break;
    case "suppress-tags":
      selectedLessTags.value = [...availableTags.value];
      tagWeightInput.value = "8";
      break;
    case "hide-once":
      hideOnce.value = true;
      break;
    case "strong-like":
      overallIntent.value = "more";
      overallWeightInput.value = "12";
      favorite.value = true;
      if (video.author) {
        authorIntent.value = "more";
        authorWeightInput.value = "8";
      }
      break;
    case "strong-dislike":
      overallIntent.value = "less";
      overallWeightInput.value = "12";
      dislike.value = true;
      if (video.author) {
        authorIntent.value = "less";
        authorWeightInput.value = "8";
      }
      if (availableTags.value.length) {
        selectedLessTags.value = [...availableTags.value];
        tagWeightInput.value = "10";
      }
      break;
    default:
      break;
  }
}

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
        const json = await response.json() as unknown;
        const normalized = normalizeVideos(json, category);
        if(normalized.length || (json && typeof json === "object")){
          videos.value = normalized;
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

async function submitFeedback(): Promise<void> {
  const video = activeFeedbackVideo.value;
  if (!video || feedbackSubmitting.value) {
    return;
  }
  if (!nowPlatform.value) {
    showPopup("当前没有工作平台，无法提交反馈", {
      type: "error",
      durationMs: 3200,
    });
    return;
  }
  feedbackSubmitting.value = true;
  const payload: FeedbackRequest = {
    platform: nowPlatform.value,
    video: video.raw,
  };
  const overall: NonNullable<FeedbackRequest["overall"]> = {};
  if (overallIntent.value === "more") {
    overall.more = overallWeight.value;
  } else if (overallIntent.value === "less") {
    overall.less = overallWeight.value;
  }
  if (favorite.value) {
    overall.favorite = true;
  }
  if (dislike.value) {
    overall.dislike = true;
  }
  if (hideOnce.value) {
    overall.hideOnce = true;
  }
  if (Object.keys(overall).length > 0) {
    payload.overall = overall;
  }
  if (authorIntent.value !== "none" && video.author) {
    payload.author = {
      value: video.authorId || video.author,
      label: video.author,
      ...(authorIntent.value === "more"
        ? { more: authorWeight.value }
        : { less: authorWeight.value }),
    };
  }
  if (selectedMoreTags.value.length || selectedLessTags.value.length) {
    payload.tags = {};
    if (selectedMoreTags.value.length) {
      payload.tags.more = selectedMoreTags.value.map((value) => ({
        value,
        label: value,
        weight: tagWeight.value,
      }));
    }
    if (selectedLessTags.value.length) {
      payload.tags.less = selectedLessTags.value.map((value) => ({
        value,
        label: value,
        weight: tagWeight.value,
      }));
    }
  }
  if (video.category) {
    payload.category = {};
    if (selectedMoreTags.value.includes(video.category)) {
      payload.category.more = [{ value: video.category, label: video.category, weight: tagWeight.value }];
    }
    if (selectedLessTags.value.includes(video.category)) {
      payload.category.less = [{ value: video.category, label: video.category, weight: tagWeight.value }];
    }
    if (!payload.category.more?.length && !payload.category.less?.length) {
      delete payload.category;
    }
  }
  if (video.keywords.length) {
    const moreKeywords = selectedMoreTags.value.filter((value) => video.keywords.includes(value));
    const lessKeywords = selectedLessTags.value.filter((value) => video.keywords.includes(value));
    if (moreKeywords.length || lessKeywords.length) {
      payload.keywords = {};
      if (moreKeywords.length) {
        payload.keywords.more = moreKeywords.map((value) => ({ value, label: value, weight: tagWeight.value }));
      }
      if (lessKeywords.length) {
        payload.keywords.less = lessKeywords.map((value) => ({ value, label: value, weight: tagWeight.value }));
      }
    }
  }
  if (derivedLegacyScore.value !== 0) {
    payload.score = derivedLegacyScore.value;
  }
  if (!payload.overall && !payload.author && !payload.tags && !payload.category && !payload.keywords) {
    showPopup("至少选择一项反馈后再提交", {
      type: "info",
      durationMs: 2800,
    });
    feedbackSubmitting.value = false;
    return;
  }
  try {
    const response = await fetchBackend("/feedback", {
      method: "POST",
      body: JSON.stringify(payload),
    });
    if (!response.ok) {
      const message = await response.text();
      showPopup(message || "提交反馈失败", {
        type: "error",
        durationMs: 3600,
      });
      return;
    }
    showPopup(`已提交「${video.title}」反馈，请求体已包含整体、作者和标签倾向`, {
      type: "success",
      durationMs: 3200,
    });
    activeFeedbackVideo.value = null;
    resetFeedbackDraft();
  } catch (error) {
    if (isAbortError(error)) {
      showPopup("反馈请求已取消", {
        type: "info",
      });
      return;
    }
    showPopup(error instanceof Error ? error.message : "反馈请求异常", {
      type: "error",
      durationMs: 3600,
    });
  } finally {
    feedbackSubmitting.value = false;
  }
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
          :feedback-disabled="feedbackSubmitting && activeFeedbackVideo?.url === video.url"
          @feedback="openFeedback"
        />
      </div>
    </div>
    <VideoFeedbackOverlay
      :open="activeFeedbackVideo !== null"
      :submitting="feedbackSubmitting"
      :video="activeFeedbackVideo"
      :templates="feedbackTemplates"
      :overall-intent="overallIntent"
      :overall-weight="overallWeight"
      :overall-weight-input="overallWeightInput"
      :favorite="favorite"
      :dislike="dislike"
      :hide-once="hideOnce"
      :author-intent="authorIntent"
      :author-weight-input="authorWeightInput"
      :tag-weight-input="tagWeightInput"
      :available-tags="availableTags"
      :selected-more-tags="selectedMoreTags"
      :selected-less-tags="selectedLessTags"
      @close="closeFeedback"
      @apply-template="applyFeedbackTemplate"
      @reset="resetFeedbackDraft"
      @update:overall-intent="overallIntent = $event"
      @update:overall-weight-input="overallWeightInput = $event"
      @toggle:favorite="favorite = !favorite"
      @toggle:dislike="dislike = !dislike"
      @toggle:hide-once="hideOnce = !hideOnce"
      @update:author-intent="authorIntent = $event"
      @update:author-weight-input="authorWeightInput = $event"
      @update:tag-weight-input="tagWeightInput = $event"
      @toggle-tag="toggleTag($event.direction, $event.value)"
      @submit="submitFeedback"
    />
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

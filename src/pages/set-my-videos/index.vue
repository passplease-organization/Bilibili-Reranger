<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import router from "@/router";
import { fetchBackend, getValid, setPlatform } from "@/pages/settings/backendSetup.ts";
import { inputPlatform, nowPlatform, supportPlatform } from "@/component/settings/settings.ts";
import { showPopup } from "@/component/utils/screen.ts";

interface PreferenceTemplate {
  name: string;
  description: string;
  platform: string;
}

const templates = ref<PreferenceTemplate[]>([]);
const templatesLoading = ref(false);
const templatesLoaded = ref(false);
const templatesError = ref("");
const selectedTemplateName = ref("");
const runningTemplateName = ref("");

const selectedTemplate = computed(() =>
  templates.value.find((item) => item.name === selectedTemplateName.value) ?? null
);

const groupedTemplates = computed(() => {
  const groups = new Map<string, PreferenceTemplate[]>();
  for (const item of templates.value) {
    const key = item.platform || "未标注平台";
    const current = groups.get(key) ?? [];
    current.push(item);
    groups.set(key, current);
  }
  return Array.from(groups.entries()).map(([platform, items]) => ({
    platform,
    items,
  }));
});

async function loadTemplates(): Promise<void> {
  templatesLoading.value = true;
  templatesError.value = "";
  templatesLoaded.value = false;
  try {
    const response = await fetchBackend("/init-my-account", {
      loading: {
        title: "正在读取偏好模板",
        detail: "后端正在整理可用模板列表，请稍等。",
      },
    });
    if (!response.ok) {
      templatesError.value = `模板列表请求失败：${response.status}`;
      templates.value = [];
      return;
    }

    const json = await response.json() as PreferenceTemplate[];
    templates.value = Array.isArray(json) ? json : [];
    selectedTemplateName.value = templates.value[0]?.name ?? "";
    templatesLoaded.value = true;
  } catch (error) {
    templatesError.value = error instanceof Error ? error.message : "模板列表请求异常";
    templates.value = [];
  } finally {
    templatesLoading.value = false;
  }
}

async function runTemplate(): Promise<void> {
  const template = selectedTemplate.value;
  if (!template || runningTemplateName.value) {
    return;
  }

  if (!window.confirm(`确认执行模板「${template.name}」吗？这会先把当前工作平台切换为 ${template.platform}。`)) {
    return;
  }

  runningTemplateName.value = template.name;
  try {
    inputPlatform.value = template.platform;
    const platformReady = await setPlatform();
    if (!platformReady) {
      showPopup(`切换平台到 ${template.platform} 失败`, {
        type: "error",
        durationMs: 3600,
      });
      return;
    }

    const response = await fetchBackend(
      `/init-my-account?platform=${encodeURIComponent(template.platform)}&name=${encodeURIComponent(template.name)}`,
      {
        loading: {
          title: "正在提交偏好任务",
          detail: "后端已开始执行你的账号偏好调整任务。工作模式通常不会返回正文。",
        },
      }
    );
    if (!response.ok) {
      const message = await response.text();
      showPopup(message || `模板「${template.name}」执行失败`, {
        type: "error",
        durationMs: 4200,
      });
      return;
    }

    showPopup(`模板「${template.name}」已提交，当前工作平台为 ${template.platform}`, {
      type: "success",
      durationMs: 3200,
    });
  } catch (error) {
    showPopup(error instanceof Error ? error.message : `模板「${template.name}」执行异常`, {
      type: "error",
      durationMs: 4200,
    });
  } finally {
    runningTemplateName.value = "";
  }
}

onMounted(async () => {
  if (!getValid()) {
    await router.push("/login");
    return;
  }
  await loadTemplates();
});
</script>

<template>
  <div class="set-my-videos-page">
    <section class="page-hero page-panel">
      <span class="page-kicker">Preference Runner</span>
      <h1 class="page-title">账号偏好设置</h1>
      <p class="page-desc">
        这里展示后端提供的全部偏好模板。选中一个模板后，前端会先把当前工作平台同步到模板对应平台，再发起账号偏好任务。
      </p>
      <div class="hero-meta">
        <span class="hero-chip">当前平台：{{ nowPlatform || "未设置" }}</span>
        <span class="hero-chip">已知平台：{{ supportPlatform.length ? supportPlatform.join(" / ") : "等待后端同步" }}</span>
      </div>
    </section>

    <div class="page-layout">
      <section class="template-panel page-panel">
        <div class="section-head">
          <div>
            <span class="page-kicker">Templates</span>
            <h2 class="section-title">可用模板</h2>
          </div>
          <button class="ghost-button" :disabled="templatesLoading" @click="loadTemplates">
            {{ templatesLoading ? "刷新中..." : "刷新模板" }}
          </button>
        </div>

        <p v-if="templatesError" class="state-message state-error">{{ templatesError }}</p>
        <p v-else-if="templatesLoading && !templatesLoaded" class="state-message">正在读取模板列表...</p>
        <p v-else-if="!templates.length" class="state-message">后端当前没有返回可用模板。</p>

        <div v-else class="template-groups">
          <section v-for="group in groupedTemplates" :key="group.platform" class="template-group">
            <div class="template-group-head">
              <h3 class="template-group-title">{{ group.platform }}</h3>
              <span class="template-group-count">{{ group.items.length }} 个模板</span>
            </div>

            <div class="template-grid">
              <button
                v-for="item in group.items"
                :key="`${item.platform}-${item.name}`"
                type="button"
                class="template-card"
                :class="{ active: selectedTemplateName === item.name }"
                @click="selectedTemplateName = item.name"
              >
                <div class="template-card-top">
                  <span class="template-platform">{{ item.platform }}</span>
                  <span v-if="selectedTemplateName === item.name" class="template-selected">已选中</span>
                </div>
                <h4 class="template-name">{{ item.name }}</h4>
                <p class="template-description">{{ item.description || "这个模板没有额外说明。" }}</p>
              </button>
            </div>
          </section>
        </div>
      </section>

      <aside class="summary-panel page-panel">
        <span class="page-kicker">Execution</span>
        <h2 class="section-title">执行确认</h2>

        <template v-if="selectedTemplate">
          <dl class="summary-list">
            <div class="summary-row">
              <dt>模板名称</dt>
              <dd>{{ selectedTemplate.name }}</dd>
            </div>
            <div class="summary-row">
              <dt>目标平台</dt>
              <dd>{{ selectedTemplate.platform }}</dd>
            </div>
            <div class="summary-row">
              <dt>当前平台</dt>
              <dd>{{ nowPlatform || "未设置" }}</dd>
            </div>
          </dl>

          <p class="summary-description">
            {{ selectedTemplate.description || "这个模板没有额外说明。" }}
          </p>

          <p class="summary-tip">
            执行时会先请求 `/set?platform={{ selectedTemplate.platform }}`，成功后再请求
            `/init-my-account?platform={{ selectedTemplate.platform }}&name={{ selectedTemplate.name }}`。
          </p>

          <button
            class="run-button"
            :disabled="Boolean(runningTemplateName)"
            @click="runTemplate"
          >
            {{ runningTemplateName === selectedTemplate.name ? "执行中..." : "确认执行这个模板" }}
          </button>
        </template>

        <p v-else class="state-message">请先从左侧选择一个模板。</p>
      </aside>
    </div>
  </div>
</template>

<style scoped>
.set-my-videos-page {
  display: flex;
  flex-direction: column;
  gap: 20px;
  min-height: calc(100vh - var(--header-height) - 40px);
  background-image:
    radial-gradient(circle at 12% 12%, rgba(0, 114, 245, 0.10), transparent 24%),
    radial-gradient(circle at 92% 8%, rgba(251, 114, 153, 0.12), transparent 20%),
    radial-gradient(circle at 78% 84%, rgba(32, 201, 151, 0.10), transparent 28%);
}

.page-panel {
  border-radius: 28px;
  background: color-mix(in srgb, var(--surface-color) 94%, transparent);
  box-shadow: var(--surface-shadow);
  backdrop-filter: blur(18px);
}

.page-hero {
  padding: 28px;
}

.page-kicker {
  display: inline-block;
  margin-bottom: 8px;
  color: var(--dark-font-color);
  font-family: var(--mono-font), monospace;
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.page-title,
.section-title,
.template-group-title {
  margin: 0;
  letter-spacing: -0.05em;
}

.page-title {
  font-size: clamp(32px, 3vw, 44px);
  line-height: 1.02;
}

.page-desc,
.summary-description,
.summary-tip,
.state-message,
.template-description {
  color: var(--dark-font-color);
  line-height: 1.65;
}

.page-desc {
  max-width: 760px;
  margin: 12px 0 0;
}

.hero-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 18px;
}

.hero-chip,
.template-platform,
.template-selected,
.template-group-count {
  display: inline-flex;
  align-items: center;
  border-radius: 999px;
  font-size: 12px;
  line-height: 1;
}

.hero-chip {
  padding: 10px 14px;
  background: color-mix(in srgb, var(--font-color) 5%, transparent);
}

.page-layout {
  display: grid;
  grid-template-columns: minmax(0, 1.6fr) minmax(280px, 0.84fr);
  gap: 20px;
  align-items: start;
}

.template-panel,
.summary-panel {
  padding: 24px;
}

.summary-panel {
  position: sticky;
  top: calc(var(--header-height) + 20px);
}

.section-head,
.template-group-head,
.summary-row {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 12px;
}

.section-title {
  font-size: clamp(24px, 2vw, 30px);
}

.ghost-button,
.run-button {
  border: none;
  cursor: pointer;
  font: inherit;
  transition:
    transform 180ms ease,
    box-shadow 180ms ease,
    background-color 180ms ease,
    color 180ms ease;
}

.ghost-button {
  padding: 11px 16px;
  border-radius: 12px;
  background: color-mix(in srgb, var(--font-color) 5%, transparent);
  color: var(--font-color);
}

.ghost-button:hover,
.run-button:hover,
.template-card:hover {
  transform: translateY(-1px);
}

.ghost-button:disabled,
.run-button:disabled {
  cursor: wait;
  opacity: 0.7;
  transform: none;
}

.template-groups {
  display: flex;
  flex-direction: column;
  gap: 18px;
  margin-top: 20px;
}

.template-group {
  padding-top: 6px;
}

.template-group-head {
  margin-bottom: 12px;
}

.template-group-title {
  font-size: 20px;
}

.template-group-count {
  padding: 7px 10px;
  background: color-mix(in srgb, var(--focus-color) 10%, transparent);
  color: var(--focus-color);
}

.template-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 14px;
}

.template-card {
  padding: 18px;
  border: 1px solid color-mix(in srgb, var(--font-color) 8%, transparent);
  border-radius: 20px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--surface-color) 96%, transparent), transparent),
    color-mix(in srgb, var(--background-color) 92%, transparent);
  text-align: left;
  color: inherit;
  cursor: pointer;
  transition:
    transform 180ms ease,
    border-color 180ms ease,
    box-shadow 180ms ease,
    background-color 180ms ease;
}

.template-card.active {
  border-color: color-mix(in srgb, var(--focus-color) 40%, transparent);
  box-shadow: var(--surface-shadow-soft), 0 0 0 1px color-mix(in srgb, var(--focus-color) 18%, transparent);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--focus-color) 10%, transparent), transparent),
    color-mix(in srgb, var(--surface-color) 96%, transparent);
}

.template-card-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.template-platform,
.template-selected {
  padding: 7px 10px;
}

.template-platform {
  background: color-mix(in srgb, var(--font-color) 6%, transparent);
}

.template-selected {
  background: color-mix(in srgb, var(--focus-color) 12%, transparent);
  color: var(--focus-color);
}

.template-name {
  margin: 14px 0 10px;
  font-size: 20px;
  line-height: 1.15;
}

.template-description {
  margin: 0;
  font-size: 14px;
}

.summary-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin: 20px 0 0;
}

.summary-row {
  padding: 12px 14px;
  border-radius: 14px;
  background: color-mix(in srgb, var(--font-color) 4%, transparent);
}

.summary-row dt {
  color: var(--dark-font-color);
}

.summary-row dd {
  margin: 0;
  text-align: right;
  font-weight: 600;
}

.summary-description {
  margin: 16px 0 0;
}

.summary-tip {
  margin: 16px 0 0;
  font-size: 14px;
}

.run-button {
  width: 100%;
  margin-top: 20px;
  padding: 14px 18px;
  border-radius: 14px;
  background: linear-gradient(135deg, #0f7cff, #19a0ff);
  color: white;
  box-shadow: 0 18px 36px -24px rgba(15, 124, 255, 0.9);
}

.state-message {
  margin: 18px 0 0;
}

.state-error {
  color: #dc3d43;
}

html.dark .template-card {
  background:
    linear-gradient(180deg, rgba(255,255,255,0.05), transparent),
    color-mix(in srgb, var(--surface-elevated) 92%, transparent);
}

html.dark .template-card.active {
  background:
    linear-gradient(180deg, rgba(91, 167, 255, 0.14), transparent),
    color-mix(in srgb, var(--surface-elevated) 92%, transparent);
}

@media (max-width: 1024px) {
  .page-layout {
    grid-template-columns: 1fr;
  }

  .summary-panel {
    position: static;
  }
}

@media (max-width: 640px) {
  .page-hero,
  .template-panel,
  .summary-panel {
    padding: 18px;
  }

  .section-head,
  .template-group-head,
  .summary-row {
    flex-direction: column;
  }

  .summary-row dd {
    text-align: left;
  }
}
</style>

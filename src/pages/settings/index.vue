<script setup lang="ts">
import settings from "@/component/settings/settings.ts";
import SettingCategory from "@/component/settings/SettingCategory.vue";
import type {zoom_change} from "@/component/Card.vue";
import PluginSettingsCard from "@/component/settings/PluginSettingsCard.vue";
import {showPopup} from "@/component/utils/screen.ts";
import {
  fetchPluginNames,
  inspectPluginDescribeResponse,
  requestPluginDescribe,
  requestPluginSave,
  type PluginFieldDefinition,
  type PluginScalarValue,
  type PluginStatusTone
} from "@/component/utils/pluginProtocol.js";
import {onMounted, reactive, ref} from "vue";

type PluginCardState = "loading" | "empty" | "unsupported" | "ready" | "error";

interface PluginEntry {
  plugin: string;
  title: string;
  description: string;
  state: PluginCardState;
  statusTone: PluginStatusTone;
  statusText: string;
  fields: PluginFieldDefinition[];
  values: Record<string, PluginScalarValue>;
  submitLabel: string;
  rawResponse: string;
  saving: boolean;
}

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

const pluginEntries = ref<PluginEntry[]>([]);
const pluginLoading = ref(false);
const pluginError = ref("");

function pluginCardId(plugin: string): string {
  return `plugin-${plugin}`;
}

function ensureCard(id: string): void {
  if (!(id in cards)) {
    cards[id] = false;
  }
}

function createPluginEntry(plugin: string): PluginEntry {
  return {
    plugin,
    title: plugin,
    description: "",
    state: "loading",
    statusTone: "info",
    statusText: "正在读取插件配置",
    fields: [],
    values: {},
    submitLabel: "保存插件配置",
    rawResponse: "",
    saving: false,
  };
}

function getPluginEntry(plugin: string): PluginEntry | undefined {
  return pluginEntries.value.find((entry) => entry.plugin === plugin);
}

function updatePluginField(payload: { plugin: string; key: string; value: PluginScalarValue }): void {
  const entry = getPluginEntry(payload.plugin);
  if (!entry) {
    return;
  }
  entry.values = {
    ...entry.values,
    [payload.key]: payload.value,
  };
}

async function loadPluginDetail(plugin: string): Promise<void> {
  const entry = getPluginEntry(plugin);
  if (!entry) {
    return;
  }

  entry.state = "loading";
  entry.statusTone = "info";
  entry.statusText = "正在读取插件配置";
  entry.rawResponse = "";

  try {
    const response = await requestPluginDescribe(plugin);
    const text = await response.text();
    if (!response.ok) {
      entry.state = "error";
      entry.statusTone = "error";
      entry.statusText = text || `插件请求失败：${response.status}`;
      entry.rawResponse = text;
      return;
    }

    const result = inspectPluginDescribeResponse(plugin, text);
    if (result.kind === "empty") {
      entry.title = plugin;
      entry.description = "";
      entry.state = "empty";
      entry.statusTone = "info";
      entry.statusText = "插件没有配置项";
      entry.fields = [];
      entry.values = {};
      entry.submitLabel = "保存插件配置";
      return;
    }

    if (result.kind === "unsupported") {
      entry.title = plugin;
      entry.description = "插件返回了内容，但没有使用前端通用配置协议。";
      entry.state = "unsupported";
      entry.statusTone = "warning";
      entry.statusText = "无法自动渲染插件配置";
      entry.fields = [];
      entry.values = {};
      entry.submitLabel = "保存插件配置";
      entry.rawResponse = text;
      return;
    }

    entry.title = result.descriptor.name;
    entry.description = result.descriptor.description;
    entry.state = "ready";
    entry.statusTone = result.descriptor.status.type;
    entry.statusText = result.descriptor.status.text;
    entry.fields = result.descriptor.fields;
    entry.values = result.descriptor.values;
    entry.submitLabel = result.descriptor.submitLabel;
    entry.rawResponse = "";
  } catch (error) {
    entry.state = "error";
    entry.statusTone = "error";
    entry.statusText = error instanceof Error ? error.message : "插件配置请求异常";
  }
}

async function loadPlugins(): Promise<void> {
  pluginLoading.value = true;
  pluginError.value = "";

  try {
    const plugins = await fetchPluginNames();
    pluginEntries.value = plugins.map((plugin) => createPluginEntry(plugin));
    pluginEntries.value.forEach((entry) => ensureCard(pluginCardId(entry.plugin)));

    await Promise.all(pluginEntries.value.map((entry) => loadPluginDetail(entry.plugin)));
  } catch (error) {
    pluginEntries.value = [];
    pluginError.value = error instanceof Error ? error.message : "插件列表读取失败";
  } finally {
    pluginLoading.value = false;
  }
}

async function savePlugin(plugin: string): Promise<void> {
  const entry = getPluginEntry(plugin);
  if (!entry || entry.saving) {
    return;
  }

  entry.saving = true;
  try {
    const response = await requestPluginSave(plugin, entry.values);
    const text = await response.text();
    if (!response.ok) {
      showPopup(text || `${plugin} 配置保存失败`, {
        type: "error",
        durationMs: 3600,
      });
      return;
    }
    showPopup(`${plugin} 配置已提交，正在刷新插件状态`, {
      type: "success",
      durationMs: 2200,
    });
    await loadPluginDetail(plugin);
  } catch (error) {
    showPopup(error instanceof Error ? error.message : `${plugin} 配置保存异常`, {
      type: "error",
      durationMs: 3600,
    });
  } finally {
    entry.saving = false;
  }
}

onMounted(async () => {
  await loadPlugins();
});
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

    <section class="plugin-section">
      <div class="plugin-section-head settings-hero-panel">
        <div class="plugin-section-copy">
          <span class="settings-kicker">Plugins</span>
          <h2 class="plugin-section-title">插件设置</h2>
          <p class="plugin-section-desc">
            后端先返回插件名列表，前端再逐个按通用协议请求插件描述。没有回信代表没有配置项，有回信但不遵循协议则不会自动渲染表单。
          </p>
        </div>
        <button class="plugin-refresh" :disabled="pluginLoading" @click="loadPlugins">
          {{ pluginLoading ? "刷新中..." : "刷新插件列表" }}
        </button>
      </div>

      <p v-if="pluginError" class="plugin-page-message plugin-page-error">{{ pluginError }}</p>
      <p v-else-if="pluginLoading && !pluginEntries.length" class="plugin-page-message">
        正在读取插件列表与插件配置。
      </p>
      <p v-else-if="!pluginEntries.length" class="plugin-page-message">
        当前后端没有返回任何插件。
      </p>

      <div v-else class="plugin-categories">
        <PluginSettingsCard
          v-for="entry in pluginEntries"
          :key="entry.plugin"
          :entry="entry"
          :card-id="pluginCardId(entry.plugin)"
          class="category"
          :class="{zoom: zoomed(pluginCardId(entry.plugin))}"
          @zoom-change="handleZoomChange"
          @reload="loadPluginDetail"
          @save="savePlugin"
          @update-field="updatePluginField"
        />
      </div>
    </section>
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

.plugin-section {
  display: flex;
  flex-direction: column;
  gap: 20px;
  width: min(1320px, 100%);
}

.plugin-section-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 24px;
  box-sizing: border-box;
  margin: 0;
}

.plugin-section-copy {
  max-width: 760px;
}

.plugin-section-title {
  margin: 0;
  font-size: clamp(26px, 3vw, 34px);
  line-height: 1.08;
  letter-spacing: -0.06em;
  color: #ffffff;
}

.plugin-section-desc {
  margin: 12px 0 0;
  color: rgba(255,255,255,0.84);
  font-size: 15px;
  line-height: 1.75;
}

.plugin-refresh {
  min-height: 42px;
  padding: 0 16px;
  border: none;
  border-radius: 12px;
  background: rgba(255,255,255,0.12);
  color: #ffffff;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition:
    transform 180ms ease,
    background-color 180ms ease;
}

.plugin-refresh:hover:not(:disabled) {
  transform: translateY(-1px);
  background: rgba(255,255,255,0.18);
}

.plugin-refresh:disabled {
  cursor: not-allowed;
  opacity: 0.72;
  transform: none;
}

.plugin-page-message {
  margin: 0;
  padding: 16px 18px;
  border: 1px solid var(--surface-border);
  border-radius: 16px;
  background: var(--surface-color);
  box-shadow: var(--surface-shadow-soft);
  color: var(--dark-font-color);
  font-size: 14px;
  line-height: 1.7;
}

.plugin-page-error {
  color: #d63e3e;
}

.plugin-categories {
  display: flex;
  flex-direction: column;
  gap: 24px;
  width: min(960px, 100%);
  align-self: center;
}

.plugin-categories .card {
  max-height: none;
  overflow-y: visible;
  padding: 26px 28px;
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

  .plugin-section-head {
    flex-direction: column;
  }

  .plugin-categories .card {
    padding: 22px;
  }
}
</style>

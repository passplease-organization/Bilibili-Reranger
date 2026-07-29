<script setup lang="ts">
import Card, {type zoom_change} from "@/component/Card.vue";
import PluginField from "@/component/settings/PluginField.vue";
import type {
  PluginFieldDefinition,
  PluginScalarValue,
  PluginStatusTone
} from "@/component/utils/pluginProtocol.js";

type PluginCardState = "loading" | "empty" | "unsupported" | "ready" | "error";

interface PluginCardEntry {
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

const props = defineProps<{
  entry: PluginCardEntry;
  cardId?: string;
}>();

const emit = defineEmits<{
  (e: "zoom-change", payload: zoom_change): void;
  (e: "reload", plugin: string): void;
  (e: "save", plugin: string): void;
  (e: "update-field", payload: { plugin: string; key: string; value: PluginScalarValue }): void;
}>();

function updateField(key: string, value: PluginScalarValue): void {
  emit("update-field", {
    plugin: props.entry.plugin,
    key,
    value,
  });
}

function fieldInputId(plugin: string, fieldKey: string): string {
  return `plugin-field-${encodeURIComponent(plugin)}-${encodeURIComponent(fieldKey)}`;
}

function statusLabel(entry: PluginCardEntry): string {
  if (entry.state === "loading") {
    return "正在读取插件配置";
  }
  if (entry.state === "empty") {
    return "插件未声明配置项";
  }
  if (entry.state === "unsupported") {
    return "插件未遵循前端通用配置协议";
  }
  if (entry.state === "error") {
    return "插件配置读取失败";
  }
  return entry.statusText;
}
</script>

<template>
  <Card
    :cardId="cardId"
    :title="entry.title"
    @zoom-change="emit('zoom-change', $event)"
  >
    <div class="plugin-card">
      <div class="plugin-card-head">
        <span class="plugin-badge" :class="`tone-${entry.statusTone}`">{{ statusLabel(entry) }}</span>
        <code class="plugin-id">{{ entry.plugin }}</code>
      </div>

      <p v-if="entry.description" class="plugin-description">{{ entry.description }}</p>

      <p v-if="entry.state === 'loading'" class="plugin-message">
        正在向插件请求前端配置描述。
      </p>

      <p v-else-if="entry.state === 'empty'" class="plugin-message">
        这个插件能接收请求，但当前没有返回任何配置项。
      </p>

      <div v-else-if="entry.state === 'unsupported'" class="plugin-unsupported">
        <p class="plugin-message">
          插件有回信，但没有使用前端通用配置协议，无法自动渲染配置表单。
        </p>
        <pre v-if="entry.rawResponse" class="plugin-raw">{{ entry.rawResponse }}</pre>
      </div>

      <p v-else-if="entry.state === 'error'" class="plugin-message plugin-message-error">
        {{ entry.statusText }}
      </p>

      <div v-else class="plugin-ready">
        <div v-if="entry.fields.length" class="plugin-fields">
          <PluginField
            v-for="field in entry.fields"
            :key="`${entry.plugin}-${field.key}`"
            :input-id="fieldInputId(entry.plugin, field.key)"
            :field="field"
            :model-value="entry.values[field.key] ?? null"
            @update:model-value="updateField(field.key, $event)"
          />
        </div>
        <p v-else class="plugin-message">
          插件已接入前端协议，但没有声明可编辑字段。
        </p>

        <div class="plugin-actions">
          <button
            class="plugin-action plugin-action-primary"
            :disabled="entry.saving || !entry.fields.length"
            @click="emit('save', entry.plugin)"
          >
            {{ entry.saving ? "保存中..." : entry.submitLabel }}
          </button>
          <button
            class="plugin-action plugin-action-ghost"
            :disabled="entry.saving"
            @click="emit('reload', entry.plugin)"
          >
            重新读取
          </button>
        </div>
      </div>
    </div>
  </Card>
</template>

<style scoped>
.plugin-card {
  display: flex;
  flex-direction: column;
  min-height: 100%;
  gap: 18px;
}

.plugin-card-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  padding-bottom: 16px;
  border-bottom: 1px solid var(--surface-border);
}

.plugin-badge {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 12px;
  border-radius: 999px;
  font-size: 13px;
  font-weight: 600;
}

.plugin-badge.tone-ready {
  background: rgba(12, 166, 120, 0.14);
  color: #0b8f68;
}

.plugin-badge.tone-info {
  background: rgba(0, 114, 245, 0.12);
  color: var(--focus-color);
}

.plugin-badge.tone-warning {
  background: rgba(201, 132, 10, 0.14);
  color: #b36f00;
}

.plugin-badge.tone-error {
  background: rgba(214, 62, 62, 0.14);
  color: #c53030;
}

.plugin-id {
  color: var(--dark-font-color);
  font-size: 12px;
}

.plugin-description,
.plugin-message {
  margin: 0;
  color: var(--dark-font-color);
  line-height: 1.7;
  font-size: 14px;
}

.plugin-message-error {
  color: #c53030;
}

.plugin-fields {
  display: flex;
  flex-direction: column;
  gap: 20px;
  padding: 4px 0;
}

.plugin-actions {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  margin-top: auto;
  padding-top: 18px;
  border-top: 1px solid var(--surface-border);
}

.plugin-action {
  min-height: 42px;
  padding: 0 16px;
  border: none;
  border-radius: 12px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition:
    transform 180ms ease,
    box-shadow 180ms ease,
    background-color 180ms ease,
    color 180ms ease;
}

.plugin-action:hover:not(:disabled) {
  transform: translateY(-1px);
}

.plugin-action:disabled {
  cursor: not-allowed;
  opacity: 0.65;
  transform: none;
}

.plugin-action-primary {
  background: var(--focus-color);
  color: #ffffff;
  box-shadow: 0 12px 24px -18px color-mix(in srgb, var(--focus-color) 75%, transparent);
}

.plugin-action-ghost {
  background: var(--surface-muted);
  color: var(--font-color);
  box-shadow: var(--surface-shadow-soft);
}

.plugin-raw {
  margin: 0;
  padding: 14px;
  border-radius: 14px;
  background: var(--surface-muted);
  color: var(--font-color);
  font-size: 12px;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

html.dark .plugin-badge.tone-ready {
  background: rgba(80, 227, 163, 0.16);
  color: #68e3b1;
}

html.dark .plugin-badge.tone-warning {
  background: rgba(255, 190, 92, 0.16);
  color: #ffbe5c;
}

html.dark .plugin-badge.tone-error,
html.dark .plugin-message-error {
  background: rgba(255, 117, 117, 0.14);
  color: #ff9191;
}
</style>

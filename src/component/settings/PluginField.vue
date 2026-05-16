<script setup lang="ts">
import {computed} from "vue";
import type {PluginFieldDefinition, PluginScalarValue} from "@/pages/settings/pluginProtocol.ts";

const props = defineProps<{
  field: PluginFieldDefinition;
  modelValue: PluginScalarValue;
}>();

const emit = defineEmits<{
  (e: "update:modelValue", value: PluginScalarValue): void;
}>();

const stringValue = computed(() => {
  const value = props.modelValue;
  if (typeof value === "string") {
    return value;
  }
  if (typeof value === "number" || typeof value === "boolean") {
    return String(value);
  }
  return "";
});

const booleanValue = computed(() => props.modelValue === true);

const selectValue = computed(() => {
  return typeof props.modelValue === "string" ? props.modelValue : "";
});

function updateStringValue(event: Event): void {
  const target = event.currentTarget as HTMLInputElement | null;
  emit("update:modelValue", target?.value ?? "");
}

function updateNumberValue(event: Event): void {
  const target = event.currentTarget as HTMLInputElement | null;
  const value = target?.value ?? "";
  if (value.trim() === "") {
    emit("update:modelValue", null);
    return;
  }
  const parsed = Number(value);
  emit("update:modelValue", Number.isFinite(parsed) ? parsed : null);
}

function updateBooleanValue(event: Event): void {
  const target = event.currentTarget as HTMLInputElement | null;
  emit("update:modelValue", target?.checked ?? false);
}

function updateSelectValue(event: Event): void {
  const target = event.currentTarget as HTMLSelectElement | null;
  emit("update:modelValue", target?.value ?? "");
}
</script>

<template>
  <div class="plugin-field">
    <label class="plugin-field-label" :for="field.key">{{ field.label }}</label>
    <p v-if="field.description" class="plugin-field-description">{{ field.description }}</p>

    <span v-if="field.type === 'string'" class="plugin-field-shell">
      <input
        :id="field.key"
        class="plugin-field-input"
        :type="field.input ?? 'text'"
        :placeholder="field.placeholder"
        :value="stringValue"
        @input="updateStringValue"
      />
    </span>

    <span v-else-if="field.type === 'number'" class="plugin-field-shell">
      <input
        :id="field.key"
        class="plugin-field-input"
        type="number"
        :placeholder="field.placeholder"
        :value="stringValue"
        @input="updateNumberValue"
      />
    </span>

    <label v-else-if="field.type === 'boolean'" class="plugin-boolean-shell" :for="field.key">
      <input
        :id="field.key"
        class="plugin-checkbox"
        type="checkbox"
        :checked="booleanValue"
        @change="updateBooleanValue"
      />
      <span class="plugin-boolean-text">{{ booleanValue ? "已启用" : "已关闭" }}</span>
    </label>

    <span v-else-if="field.type === 'select'" class="plugin-field-shell">
      <select
        :id="field.key"
        class="plugin-field-input plugin-field-select"
        :value="selectValue"
        @change="updateSelectValue"
      >
        <option
          v-for="option in field.options"
          :key="option.value"
          :value="option.value"
        >
          {{ option.label }}
        </option>
      </select>
    </span>
  </div>
</template>

<style scoped>
.plugin-field {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.plugin-field-label {
  font-size: 14px;
  font-weight: 600;
  color: var(--font-color);
}

.plugin-field-description {
  margin: 0;
  color: var(--dark-font-color);
  font-size: 13px;
  line-height: 1.6;
}

.plugin-field-shell {
  display: block;
  width: 100%;
}

.plugin-field-input {
  box-sizing: border-box;
  width: 100%;
  padding: 12px 14px;
  border: none;
  border-radius: 12px;
  background: var(--surface-color);
  box-shadow: var(--surface-shadow-soft);
  color: var(--font-color);
  font-size: 14px;
  line-height: 1.4;
  outline: none;
  transition:
    box-shadow 180ms ease,
    transform 180ms ease,
    background-color 180ms ease;
}

.plugin-field-input:hover {
  transform: translateY(-1px);
}

.plugin-field-input:focus {
  box-shadow: var(--surface-shadow-soft), var(--focus-ring);
}

.plugin-field-select {
  appearance: none;
}

.plugin-boolean-shell {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  width: fit-content;
  cursor: pointer;
}

.plugin-checkbox {
  width: 18px;
  height: 18px;
  accent-color: var(--focus-color);
}

.plugin-boolean-text {
  color: var(--font-color);
  font-size: 14px;
  font-weight: 500;
}

html.dark .plugin-field-input {
  background: var(--surface-elevated);
}
</style>

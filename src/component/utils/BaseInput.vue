<script setup lang="ts">
import {computed, isRef, type Ref, unref} from "vue";
import type {handler} from "@/component/utils/screen.ts";

export interface InputSetting {
  type?: string;
  input_mode?: 'none' | 'text' | 'tel' | 'url' | 'email' | 'numeric' | 'decimal' | 'search' | undefined;
  auto_complete?: string | boolean;
  placeholder?: string | Ref<string>;
  v_model?: string | Ref<string>;
  click?: handler;
  focusin?: handler;
  focusout?: handler;
}

const prop = defineProps<{ input: InputSetting | undefined }>();

const input = computed(() => prop.input);

const normalizedAutocomplete = computed<string | undefined>(() => {
  const value = input.value?.auto_complete;

  if (typeof value === "boolean") {
    return value ? "on" : "off";
  }

  return value;
});

const normalizedPlaceholder = computed<string | undefined>(() => {
  const value = input.value?.placeholder;
  return value === undefined ? undefined : unref(value);
});

const model = computed<string>({
  get() {
    const value = input.value?.v_model;
    if (value === undefined) {
      return "";
    }
    return isRef(value) ? value.value : value;
  },
  set(value) {
    const currentModel = input.value?.v_model;

    if (currentModel !== undefined && isRef(currentModel)) {
      currentModel.value = value;
    }
  },
});
</script>

<template>
  <span v-if="input" class="base-input-shell">
    <input
      :inputmode="input.input_mode"
      :autocomplete="normalizedAutocomplete"
      :type="input.type"
      v-model="model"
      :placeholder="normalizedPlaceholder"
      class="base-input"
      @click="input.click"
      @focusin="input.focusin"
      @focusout="input.focusout"
    />
  </span>
</template>

<style scoped>
.base-input-shell {
  --base-input-size: 16px;
  --base-input-min-font-size: 14px;
  --base-input-max-font-size: 18px;
  --base-input-resolved-size: clamp(
    var(--base-input-min-font-size),
    var(--base-input-size),
    var(--base-input-max-font-size)
  );
  display: inline-block;
  width: min(512px, 100%);
  max-width: 100%;
  font-size: var(--base-input-resolved-size);
}

.base-input {
  --input-bg: var(--surface-color);
  --input-shadow: var(--surface-shadow-soft);
  --input-shadow-focus: var(--focus-ring);
  --input-placeholder: color-mix(in srgb, var(--dark-font-color) 80%, transparent);
  box-sizing: border-box;
  width: 100%;
  padding:
    clamp(11px, calc(var(--base-input-resolved-size) * 0.76), 14px)
    clamp(14px, calc(var(--base-input-resolved-size) * 1), 18px);
  border: none;
  border-radius: 12px;
  background: var(--input-bg);
  box-shadow: var(--input-shadow);
  color: var(--font-color);
  font-size: var(--base-input-resolved-size);
  line-height: 1.4;
  outline: none;
  transition:
    box-shadow 180ms ease,
    background-color 180ms ease,
    transform 180ms ease;
}

.base-input:hover {
  transform: translateY(-1px);
}

.base-input:focus {
  box-shadow: var(--input-shadow), var(--input-shadow-focus);
}

.base-input::placeholder {
  color: var(--input-placeholder);
}

.base-input:disabled {
  cursor: not-allowed;
  opacity: 0.7;
  transform: none;
}

html.dark .base-input {
  --input-bg: var(--surface-elevated);
  --input-shadow: var(--surface-shadow-soft);
  --input-placeholder: rgba(255, 255, 255, 0.42);
}

@media (max-width: 640px) {
  .base-input-shell {
    width: 100%;
  }

  .base-input {
    padding:
      clamp(11px, calc(var(--base-input-resolved-size) * 0.75), 13px)
      clamp(13px, calc(var(--base-input-resolved-size) * 0.9), 16px);
    border-radius: clamp(10px, calc(var(--base-input-resolved-size) * 0.75), 14px);
  }
}
</style>

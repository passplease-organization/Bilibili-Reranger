<script setup lang="ts">
import { computed, getCurrentInstance, nextTick, onBeforeUnmount, onMounted, ref, unref, type Ref } from "vue";

type option = { label: string, value: string };

export interface SelectSetting {
  v_model: Ref<string>,
  description: string,
  values: option[] | Ref<option[]>
}

const props = defineProps<{ setting: SelectSetting }>();

const values = computed(() => unref(props.setting.values));
const selectedValue = computed({
  get: () => props.setting.v_model.value,
  set: (value: string) => {
    props.setting.v_model.value = value;
  },
});
const selectedOption = computed(() =>
  values.value.find((item) => item.value === selectedValue.value)
);
const displayLabel = computed(() => selectedOption.value?.label ?? props.setting.description);
const selectedIndex = computed(() =>
  values.value.findIndex((item) => item.value === selectedValue.value)
);

const isOpen = ref(false);
const activeIndex = ref(-1);
const shellRef = ref<HTMLElement | null>(null);
const listRef = ref<HTMLElement | null>(null);
const uid = getCurrentInstance()?.uid ?? "selector";
const buttonId = `base-selector-button-${uid}`;
const listboxId = `base-selector-listbox-${uid}`;

function syncActiveIndex(): void {
  activeIndex.value = selectedIndex.value >= 0 ? selectedIndex.value : 0;
}

function closePanel(): void {
  isOpen.value = false;
}

function focusActiveOption(): void {
  nextTick(() => {
    const active = listRef.value?.querySelector<HTMLElement>("[data-active='true']");
    active?.focus();
    active?.scrollIntoView({ block: "nearest" });
  });
}

function openPanel(): void {
  if (!values.value.length) {
    return;
  }

  syncActiveIndex();
  isOpen.value = true;
  focusActiveOption();
}

function togglePanel(): void {
  if (isOpen.value) {
    closePanel();
    return;
  }

  openPanel();
}

function selectOption(item: option): void {
  selectedValue.value = item.value;
  closePanel();
}

function moveActive(step: number): void {
  if (!values.value.length) {
    return;
  }

  if (!isOpen.value) {
    openPanel();
    return;
  }

  const maxIndex = values.value.length - 1;
  const nextIndex = activeIndex.value < 0
    ? 0
    : Math.min(maxIndex, Math.max(0, activeIndex.value + step));

  activeIndex.value = nextIndex;
  focusActiveOption();
}

function handleTriggerKeydown(event: KeyboardEvent): void {
  switch (event.key) {
    case "ArrowDown":
      event.preventDefault();
      moveActive(1);
      break;
    case "ArrowUp":
      event.preventDefault();
      moveActive(-1);
      break;
    case "Enter":
    case " ":
      event.preventDefault();
      togglePanel();
      break;
    case "Escape":
      closePanel();
      break;
    default:
      break;
  }
}

function handleListKeydown(event: KeyboardEvent): void {
  switch (event.key) {
    case "ArrowDown":
      event.preventDefault();
      moveActive(1);
      break;
    case "ArrowUp":
      event.preventDefault();
      moveActive(-1);
      break;
    case "Home":
      event.preventDefault();
      activeIndex.value = 0;
      focusActiveOption();
      break;
    case "End":
      event.preventDefault();
      activeIndex.value = values.value.length - 1;
      focusActiveOption();
      break;
    case "Enter":
    case " ":
      event.preventDefault();
      if (activeIndex.value >= 0) {
        const current = values.value[activeIndex.value];
        if (current) {
          selectOption(current);
        }
      }
      break;
    case "Escape":
      event.preventDefault();
      closePanel();
      break;
    case "Tab":
      closePanel();
      break;
    default:
      break;
  }
}

function handlePointerDown(event: PointerEvent): void {
  const target = event.target;
  if (!(target instanceof Node)) {
    return;
  }

  if (!shellRef.value?.contains(target)) {
    closePanel();
  }
}

onMounted(() => {
  document.addEventListener("pointerdown", handlePointerDown);
});

onBeforeUnmount(() => {
  document.removeEventListener("pointerdown", handlePointerDown);
});
</script>

<template>
  <span ref="shellRef" class="base-selector-shell">
    <button
      :id="buttonId"
      type="button"
      class="base-selector-trigger"
      :class="{ 'is-open': isOpen, 'is-placeholder': !selectedOption }"
      :aria-expanded="isOpen"
      :aria-controls="listboxId"
      aria-haspopup="listbox"
      @click="togglePanel"
      @keydown="handleTriggerKeydown"
    >
      <span class="base-selector-value">{{ displayLabel }}</span>
      <span class="base-selector-arrow" aria-hidden="true"></span>
    </button>

    <Transition name="base-selector-panel">
      <div
        v-if="isOpen"
        :id="listboxId"
        ref="listRef"
        class="base-selector-panel"
        role="listbox"
        :aria-labelledby="buttonId"
        tabindex="-1"
        @keydown="handleListKeydown"
      >
        <button
          v-for="(item, index) in values"
          :key="item.value"
          type="button"
          class="base-selector-option"
          :class="{
            'is-selected': item.value === selectedValue,
            'is-active': index === activeIndex
          }"
          :data-active="index === activeIndex"
          role="option"
          :aria-selected="item.value === selectedValue"
          @click="selectOption(item)"
          @mouseenter="activeIndex = index"
        >
          <span class="base-selector-option-label">{{ item.label }}</span>
          <span v-if="item.value === selectedValue" class="base-selector-option-check" aria-hidden="true">✓</span>
        </button>
      </div>
    </Transition>
  </span>
</template>

<style scoped>
.base-selector-shell {
  --base-input-size: 16px;
  --base-input-min-font-size: 14px;
  --base-input-max-font-size: 18px;
  --base-input-resolved-size: clamp(
    var(--base-input-min-font-size),
    var(--base-input-size),
    var(--base-input-max-font-size)
  );
  display: inline-block;
  position: relative;
  width: min(512px, 100%);
  max-width: 100%;
  font-size: var(--base-input-resolved-size);
}

.base-selector-trigger {
  --selector-bg: var(--surface-color);
  --selector-shadow: var(--surface-shadow-soft);
  --selector-shadow-focus: var(--focus-ring);
  --selector-arrow-color: color-mix(in srgb, var(--font-color) 72%, transparent);
  box-sizing: border-box;
  display: inline-flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  width: 100%;
  min-height: 48px;
  padding: 12px 16px;
  border: none;
  border-radius: 12px;
  background: var(--selector-bg);
  box-shadow: var(--selector-shadow);
  color: var(--font-color);
  font-size: var(--base-input-resolved-size);
  line-height: 1.4;
  text-align: left;
  cursor: pointer;
  outline: none;
  transition:
    border-color 180ms ease,
    box-shadow 180ms ease,
    background-color 180ms ease,
    transform 180ms ease;
}

.base-selector-trigger:hover {
  transform: translateY(-1px);
}

.base-selector-trigger:focus-visible,
.base-selector-trigger.is-open {
  box-shadow: var(--selector-shadow), var(--selector-shadow-focus);
}

.base-selector-trigger.is-placeholder .base-selector-value {
  color: rgba(15, 23, 42, 0.42);
}

.base-selector-value {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.base-selector-arrow {
  flex: 0 0 auto;
  width: 10px;
  height: 10px;
  border-right: 2px solid var(--selector-arrow-color);
  border-bottom: 2px solid var(--selector-arrow-color);
  transform: rotate(45deg) translateY(-1px);
  transform-origin: center;
  transition: transform 180ms ease, border-color 180ms ease;
}

.base-selector-trigger.is-open .base-selector-arrow {
  transform: rotate(-135deg) translateY(-1px);
}

.base-selector-panel {
  --panel-bg: var(--surface-elevated);
  --panel-shadow: var(--surface-shadow);
  --panel-option-hover: color-mix(in srgb, var(--font-color) 4%, transparent);
  --panel-option-active: color-mix(in srgb, var(--focus-color) 12%, transparent);
  --panel-option-selected: color-mix(in srgb, var(--focus-color) 16%, transparent);
  position: absolute;
  z-index: 30;
  top: calc(100% + 10px);
  left: 0;
  right: 0;
  display: flex;
  flex-direction: column;
  gap: 6px;
  max-height: min(320px, 50vh);
  padding: 10px;
  border: none;
  border-radius: 16px;
  background: var(--panel-bg);
  backdrop-filter: blur(18px);
  box-shadow: var(--panel-shadow);
  overflow-y: auto;
}

.base-selector-option {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  width: 100%;
  min-height: 42px;
  padding: 11px 14px;
  border: none;
  border-radius: 12px;
  background: transparent;
  color: var(--font-color);
  font-size: var(--base-input-resolved-size);
  line-height: 1.35;
  text-align: left;
  cursor: pointer;
  outline: none;
  transition:
    background-color 160ms ease,
    border-color 160ms ease,
    transform 160ms ease;
}

.base-selector-option:hover,
.base-selector-option.is-active {
  background: var(--panel-option-hover);
  transform: translateX(2px);
}

.base-selector-option:focus-visible {
  background: var(--panel-option-active);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--focus-color) 35%, transparent);
}

.base-selector-option.is-selected {
  background: var(--panel-option-selected);
  font-weight: 600;
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--focus-color) 22%, transparent);
}

.base-selector-option-check {
  flex: 0 0 auto;
  color: var(--focus-color);
  font-size: 14px;
}

.base-selector-panel-enter-active,
.base-selector-panel-leave-active {
  transition:
    opacity 180ms ease,
    transform 180ms ease;
}

.base-selector-panel-enter-from,
.base-selector-panel-leave-to {
  opacity: 0;
  transform: translateY(-6px) scale(0.98);
}

html.dark .base-selector-trigger {
  --selector-bg: var(--surface-elevated);
  --selector-shadow: var(--surface-shadow-soft);
  --selector-arrow-color: rgba(255, 255, 255, 0.78);
}

html.dark .base-selector-trigger.is-placeholder .base-selector-value {
  color: rgba(255, 255, 255, 0.42);
}

html.dark .base-selector-panel {
  --panel-bg: var(--surface-elevated);
  --panel-shadow: var(--surface-shadow);
  --panel-option-hover: rgba(255, 255, 255, 0.07);
  --panel-option-active: color-mix(in srgb, var(--focus-color) 20%, rgba(255, 255, 255, 0.06));
  --panel-option-selected: color-mix(in srgb, var(--focus-color) 24%, rgba(255, 255, 255, 0.08));
}

@media (max-width: 640px) {
  .base-selector-shell {
    width: 100%;
  }

  .base-selector-trigger {
    min-height: 44px;
    padding: 11px 14px;
    border-radius: 12px;
  }

  .base-selector-panel {
    top: calc(100% + 8px);
    padding: 8px;
    border-radius: 14px;
  }

  .base-selector-option {
    min-height: 40px;
    padding: 10px 12px;
  }
}
</style>

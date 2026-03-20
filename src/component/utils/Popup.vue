<script setup lang="ts">
import { computed } from "vue";
import {
  toastMessage,
  toastType,
  toastVisible,
} from "@/component/utils/screen";

const popupClass = computed(() => [
  "popup",
  `popup-${toastType.value}`,
  { "popup-visible": toastVisible.value },
]);
</script>

<template>
  <Teleport to="body">
    <div
      :class="popupClass"
      :aria-hidden="!toastVisible"
      role="status"
      aria-live="polite"
    >
      <span class="popup-indicator"></span>
      <span class="popup-message">{{ toastMessage }}</span>
    </div>
  </Teleport>
</template>

<style scoped>
.popup {
  --popup-bg: rgba(255, 255, 255, 0.78);
  --popup-border: rgba(15, 23, 42, 0.12);
  --popup-shadow: 0 20px 45px rgba(15, 23, 42, 0.16);
  --popup-text: var(--font-color);
  --popup-accent: var(--focus-color);
  position: fixed;
  left: 50%;
  bottom: 24px;
  z-index: 9999;
  display: inline-flex;
  align-items: center;
  gap: 12px;
  width: min(420px, calc(100vw - 32px));
  padding: 14px 18px;
  border: 1px solid var(--popup-border);
  border-radius: 18px;
  background:
    linear-gradient(135deg, rgba(255, 255, 255, 0.72), rgba(255, 255, 255, 0.2)),
    var(--popup-bg);
  box-shadow: var(--popup-shadow);
  backdrop-filter: blur(16px);
  color: var(--popup-text);
  opacity: 0;
  pointer-events: none;
  transform: translateX(-50%) translateY(24px) scale(0.96);
  transform-origin: center bottom;
  transition:
    opacity 220ms ease,
    transform 260ms cubic-bezier(0.22, 1, 0.36, 1),
    border-color 180ms ease,
    box-shadow 180ms ease;
}

.popup-visible {
  opacity: 1;
  transform: translateX(-50%) translateY(0) scale(1);
}

.popup-indicator {
  flex: none;
  width: 10px;
  height: 10px;
  border-radius: 999px;
  background: var(--popup-accent);
  box-shadow: 0 0 0 6px color-mix(in srgb, var(--popup-accent) 18%, transparent);
}

.popup-message {
  flex: 1;
  min-width: 0;
  font-size: 15px;
  line-height: 1.45;
  overflow-wrap: anywhere;
}

.popup-info {
  --popup-accent: #3b82f6;
}

.popup-success {
  --popup-accent: #16a34a;
}

.popup-error {
  --popup-accent: #ef4444;
}

html.dark .popup {
  --popup-bg: rgba(17, 24, 39, 0.84);
  --popup-border: rgba(255, 255, 255, 0.14);
  --popup-shadow: 0 22px 48px rgba(0, 0, 0, 0.34);
  background:
    linear-gradient(135deg, rgba(255, 255, 255, 0.08), rgba(255, 255, 255, 0.02)),
    var(--popup-bg);
}

@media (max-width: 640px) {
  .popup {
    left: 16px;
    right: 16px;
    bottom: 16px;
    width: auto;
    padding: 13px 15px;
    border-radius: 16px;
    transform: translateY(24px) scale(0.96);
  }

  .popup-visible {
    transform: translateY(0) scale(1);
  }
}
</style>

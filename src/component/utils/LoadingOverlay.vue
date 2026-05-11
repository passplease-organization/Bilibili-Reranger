<script setup lang="ts">
import {
  cancelLatestLoading,
  loadingCancelable,
  loadingCancelLabel,
  loadingDetail,
  loadingTitle,
  loadingVisible,
} from "@/component/utils/screen";
</script>

<template>
  <Teleport to="body">
    <Transition name="loading-fade">
      <div v-if="loadingVisible" class="loading-shell" role="status" aria-live="polite">
        <div class="loading-panel">
          <div class="loading-spinner" aria-hidden="true">
            <span></span>
            <span></span>
            <span></span>
          </div>
          <div class="loading-copy">
            <p class="loading-title">{{ loadingTitle }}</p>
            <p v-if="loadingDetail" class="loading-detail">{{ loadingDetail }}</p>
            <button
              v-if="loadingCancelable"
              type="button"
              class="loading-cancel"
              @click="cancelLatestLoading"
            >
              {{ loadingCancelLabel }}
            </button>
          </div>
        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<style scoped>
.loading-shell {
  position: fixed;
  inset: 0;
  z-index: 9998;
  display: grid;
  place-items: center;
  padding: 24px;
  background:
    radial-gradient(circle at top, rgba(91, 167, 255, 0.12), transparent 28%),
    rgba(7, 10, 18, 0.24);
  backdrop-filter: blur(14px);
}

.loading-panel {
  display: flex;
  align-items: center;
  gap: 18px;
  width: min(460px, calc(100vw - 32px));
  padding: 22px 24px;
  border: 1px solid rgba(255, 255, 255, 0.22);
  border-radius: 24px;
  background:
    linear-gradient(145deg, rgba(255, 255, 255, 0.95), rgba(245, 248, 255, 0.86));
  box-shadow:
    0 26px 70px rgba(15, 23, 42, 0.18),
    inset 0 1px 0 rgba(255, 255, 255, 0.74);
}

.loading-spinner {
  position: relative;
  width: 56px;
  height: 56px;
  flex: 0 0 56px;
}

.loading-spinner span {
  position: absolute;
  inset: 0;
  border-radius: 50%;
  border: 3px solid transparent;
  border-top-color: var(--focus-color);
  animation: loading-spin 1.1s linear infinite;
}

.loading-spinner span:nth-child(2) {
  inset: 8px;
  border-top-color: rgba(0, 114, 245, 0.5);
  animation-duration: 1.5s;
  animation-direction: reverse;
}

.loading-spinner span:nth-child(3) {
  inset: 17px;
  border-top-color: rgba(251, 114, 153, 0.9);
  animation-duration: 1.8s;
}

.loading-copy {
  min-width: 0;
}

.loading-title,
.loading-detail {
  margin: 0;
}

.loading-title {
  color: #0f172a;
  font-size: 18px;
  font-weight: 650;
  line-height: 1.25;
  letter-spacing: -0.03em;
}

.loading-detail {
  margin-top: 6px;
  color: rgba(15, 23, 42, 0.68);
  font-size: 14px;
  line-height: 1.55;
}

.loading-cancel {
  width: fit-content;
  margin-top: 14px;
  padding: 9px 14px;
  border: 1px solid color-mix(in srgb, var(--focus-color) 18%, transparent);
  border-radius: 999px;
  background: color-mix(in srgb, var(--focus-color) 8%, white);
  color: var(--focus-color);
  font: inherit;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: transform 160ms ease, background-color 160ms ease, border-color 160ms ease;
}

.loading-cancel:hover {
  transform: translateY(-1px);
  background: color-mix(in srgb, var(--focus-color) 12%, white);
}

html.dark .loading-shell {
  background:
    radial-gradient(circle at top, rgba(91, 167, 255, 0.14), transparent 30%),
    rgba(2, 6, 14, 0.42);
}

html.dark .loading-panel {
  border-color: rgba(255, 255, 255, 0.12);
  background:
    linear-gradient(145deg, rgba(12, 18, 30, 0.94), rgba(17, 24, 39, 0.88));
  box-shadow:
    0 28px 78px rgba(0, 0, 0, 0.36),
    inset 0 1px 0 rgba(255, 255, 255, 0.04);
}

html.dark .loading-title {
  color: rgba(255, 255, 255, 0.94);
}

html.dark .loading-detail {
  color: rgba(255, 255, 255, 0.62);
}

html.dark .loading-cancel {
  background: color-mix(in srgb, var(--focus-color) 14%, rgba(255, 255, 255, 0.04));
  border-color: color-mix(in srgb, var(--focus-color) 22%, rgba(255, 255, 255, 0.08));
}

.loading-fade-enter-active,
.loading-fade-leave-active {
  transition: opacity 180ms ease;
}

.loading-fade-enter-from,
.loading-fade-leave-to {
  opacity: 0;
}

@keyframes loading-spin {
  to {
    transform: rotate(360deg);
  }
}

@media (max-width: 640px) {
  .loading-panel {
    align-items: flex-start;
    gap: 14px;
    padding: 18px 18px 17px;
    border-radius: 20px;
  }

  .loading-spinner {
    width: 48px;
    height: 48px;
    flex-basis: 48px;
  }

  .loading-title {
    font-size: 16px;
  }

  .loading-detail {
    font-size: 13px;
  }
}
</style>

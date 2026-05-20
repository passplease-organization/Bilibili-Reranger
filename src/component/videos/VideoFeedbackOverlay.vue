<script setup lang="ts">
import {computed, onBeforeUnmount, onMounted, watch} from "vue";
import type {Video} from "@/component/videos/interfaces.ts";

interface FeedbackTemplate {
  label: string;
  score: number;
  tone: string;
}

const props = defineProps<{
  open: boolean;
  submitting: boolean;
  video: Video | null;
  score: number;
  scoreInput: string;
  templates: FeedbackTemplate[];
}>();

const emit = defineEmits<{
  (e: "close"): void;
  (e: "update:scoreInput", value: string): void;
  (e: "apply-template", value: number): void;
  (e: "submit"): void;
}>();

const selectedTemplateLabel = computed(() => {
  const current = props.templates.find((item) => item.score === props.score);
  return current?.label ?? "自定义";
});

function handleBackdropClick(event: MouseEvent): void {
  if (event.target === event.currentTarget) {
    emit("close");
  }
}

function handleEscape(event: KeyboardEvent): void {
  if (event.key === "Escape" && props.open && !props.submitting) {
    emit("close");
  }
}

watch(() => props.open, (open) => {
  document.body.style.overflow = open ? "hidden" : "";
}, { immediate: true });

onBeforeUnmount(() => {
  document.body.style.overflow = "";
});

onMounted(() => {
  window.addEventListener("keydown", handleEscape);
});

onBeforeUnmount(() => {
  window.removeEventListener("keydown", handleEscape);
});
</script>

<template>
  <Teleport to="body">
    <Transition name="feedback-overlay">
      <div
        v-if="open && video"
        class="feedback-overlay"
        role="dialog"
        aria-modal="true"
        aria-labelledby="feedback-title"
        @click="handleBackdropClick"
      >
        <div class="feedback-backdrop"></div>
        <section class="feedback-panel">
          <div class="feedback-topline">
            <span class="feedback-kicker">Feedback</span>
            <button
              type="button"
              class="feedback-close"
              :disabled="submitting"
              aria-label="关闭反馈面板"
              @click="emit('close')"
            >
              关闭
            </button>
          </div>

          <div class="feedback-hero">
            <div class="feedback-copy">
              <h2 id="feedback-title" class="feedback-title">对这条视频做反馈</h2>
              <p class="feedback-desc">
                这次评分会直接发送给后端，平台名和视频原始数据会一并带回，用于后续推送调整。
              </p>
            </div>
            <div class="feedback-score-badge">
              <span>当前档位</span>
              <strong>{{ selectedTemplateLabel }}</strong>
              <em>{{ score > 0 ? "+" : "" }}{{ score }}</em>
            </div>
          </div>

          <div class="feedback-video-summary">
            <h3>{{ video.title }}</h3>
            <p>{{ video.author }}</p>
            <span>{{ video.publishTime }}</span>
          </div>

          <div class="feedback-template-grid">
            <button
              v-for="item in templates"
              :key="item.label"
              type="button"
              class="feedback-template"
              :class="{ active: score === item.score }"
              :disabled="submitting"
              @click="emit('apply-template', item.score)"
            >
              <span class="feedback-template-label">{{ item.label }}</span>
              <strong>{{ item.score > 0 ? "+" : "" }}{{ item.score }}</strong>
              <small>{{ item.tone }}</small>
            </button>
          </div>

          <div class="feedback-custom">
            <div class="feedback-custom-head">
              <h3>自定义分值</h3>
              <span>允许范围 -16 到 16</span>
            </div>
            <div class="feedback-custom-controls">
              <input
                class="feedback-range"
                type="range"
                min="-16"
                max="16"
                step="1"
                :value="score"
                :disabled="submitting"
                @input="emit('update:scoreInput', ($event.target as HTMLInputElement).value)"
              />
              <input
                class="feedback-number"
                type="number"
                min="-16"
                max="16"
                step="1"
                :value="scoreInput"
                :disabled="submitting"
                @input="emit('update:scoreInput', ($event.target as HTMLInputElement).value)"
              />
            </div>
          </div>

          <div class="feedback-actions">
            <button
              type="button"
              class="feedback-action feedback-action-secondary"
              :disabled="submitting"
              @click="emit('close')"
            >
              取消
            </button>
            <button
              type="button"
              class="feedback-action feedback-action-primary"
              :disabled="submitting"
              @click="emit('submit')"
            >
              {{ submitting ? "提交中..." : "提交反馈" }}
            </button>
          </div>
        </section>
      </div>
    </Transition>
  </Teleport>
</template>

<style scoped>
.feedback-overlay {
  position: fixed;
  inset: 0;
  z-index: 120;
  display: grid;
  place-items: center;
  padding: 24px;
}

.feedback-backdrop {
  position: absolute;
  inset: 0;
  background:
    radial-gradient(circle at 22% 18%, rgba(0, 114, 245, 0.22), transparent 26%),
    radial-gradient(circle at 78% 22%, rgba(251, 114, 153, 0.18), transparent 28%),
    rgba(7, 10, 18, 0.58);
  backdrop-filter: blur(22px);
}

.feedback-panel {
  position: relative;
  z-index: 1;
  width: min(920px, calc(100vw - 32px));
  max-height: calc(100vh - 32px);
  overflow-y: auto;
  padding: 26px;
  border: 1px solid color-mix(in srgb, var(--surface-border) 94%, transparent);
  border-radius: 28px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--surface-elevated) 92%, transparent), color-mix(in srgb, var(--surface-color) 96%, transparent)),
    radial-gradient(circle at top right, rgba(255, 255, 255, 0.16), transparent 38%);
  box-shadow:
    0 28px 80px rgba(6, 10, 18, 0.32),
    inset 0 1px 0 rgba(255, 255, 255, 0.16);
}

.feedback-topline,
.feedback-hero,
.feedback-custom-head,
.feedback-actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.feedback-topline {
  margin-bottom: 18px;
}

.feedback-kicker {
  font-family: var(--mono-font), monospace;
  font-size: 11px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--dark-font-color);
}

.feedback-close,
.feedback-action,
.feedback-template {
  border: none;
  font: inherit;
  cursor: pointer;
}

.feedback-close,
.feedback-action-secondary {
  background: color-mix(in srgb, var(--font-color) 5%, transparent);
  color: var(--font-color);
}

.feedback-close {
  padding: 10px 14px;
  border-radius: 999px;
}

.feedback-hero {
  align-items: end;
  margin-bottom: 20px;
}

.feedback-title {
  margin: 0;
  font-size: clamp(30px, 4vw, 42px);
  line-height: 1.02;
  letter-spacing: -0.06em;
}

.feedback-desc {
  margin: 10px 0 0;
  max-width: 620px;
  color: var(--dark-font-color);
  line-height: 1.7;
}

.feedback-score-badge {
  min-width: 148px;
  padding: 16px 18px;
  border-radius: 20px;
  background: linear-gradient(135deg, color-mix(in srgb, var(--focus-color) 18%, transparent), color-mix(in srgb, #fb7299 16%, transparent));
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--focus-color) 16%, transparent);
}

.feedback-score-badge span,
.feedback-score-badge em,
.feedback-video-summary p,
.feedback-video-summary span,
.feedback-template small,
.feedback-custom-head span {
  color: var(--dark-font-color);
}

.feedback-score-badge strong,
.feedback-score-badge em {
  display: block;
}

.feedback-score-badge strong {
  margin-top: 8px;
  font-size: 20px;
}

.feedback-score-badge em {
  margin-top: 6px;
  font-style: normal;
  font-family: var(--mono-font), monospace;
  font-size: 14px;
}

.feedback-video-summary {
  margin-bottom: 24px;
  padding: 20px 22px;
  border-radius: 22px;
  background: color-mix(in srgb, var(--surface-color) 78%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--surface-border) 92%, transparent);
}

.feedback-video-summary h3,
.feedback-custom h3 {
  margin: 0;
  font-size: 20px;
  letter-spacing: -0.03em;
}

.feedback-video-summary p {
  margin: 10px 0 6px;
}

.feedback-template-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
  margin-bottom: 24px;
}

.feedback-template {
  display: grid;
  gap: 10px;
  padding: 18px;
  border-radius: 22px;
  text-align: left;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--surface-color) 96%, transparent), color-mix(in srgb, var(--surface-muted) 96%, transparent));
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--surface-border) 90%, transparent),
    var(--surface-shadow-soft);
  transition: transform 0.2s ease, box-shadow 0.2s ease, background-color 0.2s ease;
}

.feedback-template:hover {
  transform: translateY(-2px);
}

.feedback-template.active {
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--focus-color) 15%, transparent), color-mix(in srgb, #fb7299 10%, transparent));
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--focus-color) 24%, transparent),
    0 16px 28px -22px rgba(0, 114, 245, 0.38);
}

.feedback-template-label {
  font-size: 17px;
  font-weight: 600;
}

.feedback-template strong {
  font-size: 26px;
  letter-spacing: -0.04em;
}

.feedback-custom {
  padding: 22px;
  border-radius: 24px;
  background: color-mix(in srgb, var(--surface-color) 74%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--surface-border) 90%, transparent);
}

.feedback-custom-controls {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 112px;
  gap: 16px;
  margin-top: 18px;
}

.feedback-range,
.feedback-number {
  width: 100%;
}

.feedback-range {
  accent-color: var(--focus-color);
}

.feedback-number {
  box-sizing: border-box;
  padding: 12px 14px;
  border: none;
  border-radius: 16px;
  background: var(--surface-elevated);
  box-shadow: var(--surface-shadow-soft);
  color: var(--font-color);
  font: inherit;
}

.feedback-actions {
  margin-top: 24px;
  justify-content: end;
}

.feedback-action {
  min-width: 118px;
  padding: 13px 18px;
  border-radius: 16px;
}

.feedback-action-primary {
  background: linear-gradient(135deg, var(--focus-color), #fb7299);
  color: #fff;
  box-shadow: 0 18px 28px -22px rgba(0, 114, 245, 0.62);
}

.feedback-close:disabled,
.feedback-action:disabled,
.feedback-template:disabled,
.feedback-range:disabled,
.feedback-number:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.feedback-overlay-enter-active,
.feedback-overlay-leave-active {
  transition: opacity 0.22s ease;
}

.feedback-overlay-enter-active .feedback-panel,
.feedback-overlay-leave-active .feedback-panel {
  transition: transform 0.28s cubic-bezier(0.22, 1, 0.36, 1), opacity 0.22s ease;
}

.feedback-overlay-enter-from,
.feedback-overlay-leave-to {
  opacity: 0;
}

.feedback-overlay-enter-from .feedback-panel,
.feedback-overlay-leave-to .feedback-panel {
  opacity: 0;
  transform: translateY(18px) scale(0.97);
}

@media (max-width: 900px) {
  .feedback-hero,
  .feedback-topline,
  .feedback-custom-head {
    align-items: start;
    flex-direction: column;
  }

  .feedback-score-badge {
    width: 100%;
    box-sizing: border-box;
  }

  .feedback-template-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .feedback-overlay {
    padding: 12px;
  }

  .feedback-panel {
    width: min(100vw - 24px, 920px);
    max-height: calc(100vh - 24px);
    padding: 20px;
    border-radius: 24px;
  }

  .feedback-template-grid,
  .feedback-custom-controls {
    grid-template-columns: 1fr;
  }

  .feedback-actions {
    flex-direction: column-reverse;
    align-items: stretch;
  }
}
</style>

<script setup lang="ts">
import {computed, onBeforeUnmount, onMounted, watch} from "vue";
import type {Video} from "@/component/videos/interfaces.ts";

type Direction = "more" | "less";
type Intent = Direction | "none";

interface FeedbackTemplate {
  id: string;
  label: string;
  description: string;
}

const props = defineProps<{
  open: boolean;
  submitting: boolean;
  video: Video | null;
  templates: FeedbackTemplate[];
  overallIntent: Intent;
  overallWeight: number;
  overallWeightInput: string;
  favorite: boolean;
  dislike: boolean;
  hideOnce: boolean;
  authorIntent: Intent;
  authorWeightInput: string;
  tagWeightInput: string;
  availableTags: string[];
  selectedMoreTags: string[];
  selectedLessTags: string[];
}>();

const emit = defineEmits<{
  (e: "close"): void;
  (e: "reset"): void;
  (e: "apply-template", value: string): void;
  (e: "update:overallIntent", value: Intent): void;
  (e: "update:overallWeightInput", value: string): void;
  (e: "toggle:favorite"): void;
  (e: "toggle:dislike"): void;
  (e: "toggle:hideOnce"): void;
  (e: "update:authorIntent", value: Intent): void;
  (e: "update:authorWeightInput", value: string): void;
  (e: "update:tagWeightInput", value: string): void;
  (e: "toggle-tag", payload: { direction: Direction; value: string }): void;
  (e: "submit"): void;
}>();

const activeSummaryLabel = computed(() => {
  if (props.favorite && props.overallIntent === "more") {
    return "强烈喜欢";
  }
  if (props.dislike && props.overallIntent === "less") {
    return "强烈厌恶";
  }
  if (props.hideOnce) {
    return "仅隐藏本条";
  }
  if (props.overallIntent === "more") {
    return "想看更多";
  }
  if (props.overallIntent === "less") {
    return "想看更少";
  }
  return "待选择";
});

const selectedTagCount = computed(() =>
  props.selectedMoreTags.length + props.selectedLessTags.length
);

const hasAuthorControl = computed(() => Boolean(props.video?.author));

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
              <p class="feedback-desc">这次提交会把当前视频原始对象、总分、作者倾向、整体总结和标签名单一起写进同一个 body，方便后端插件直接判断。</p>
            </div>
            <div class="feedback-score-badge">
              <span>当前概览</span>
              <strong>{{ activeSummaryLabel }}</strong>
              <em>整体强度 {{ overallWeight }}</em>
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
              :key="item.id"
              type="button"
              class="feedback-template"
              :disabled="submitting"
              @click="emit('apply-template', item.id)"
            >
              <span class="feedback-template-label">{{ item.label }}</span>
              <small>{{ item.description }}</small>
            </button>
          </div>

          <div class="feedback-custom">
            <div class="feedback-custom-head">
              <h3>整体倾向</h3>
              <span>主表达：想看更多还是更少</span>
            </div>
            <div class="feedback-segmented">
              <button type="button" class="feedback-chip" :class="{ active: overallIntent === 'more' }" :disabled="submitting" @click="emit('update:overallIntent', overallIntent === 'more' ? 'none' : 'more')">更多这种视频</button>
              <button type="button" class="feedback-chip" :class="{ active: overallIntent === 'less' }" :disabled="submitting" @click="emit('update:overallIntent', overallIntent === 'less' ? 'none' : 'less')">减少这种视频</button>
            </div>
            <p class="feedback-help">
              数值越高，代表这次整体倾向越强。`1-4` 偏轻微，`5-10` 偏明确，`11-16` 偏强烈。
            </p>
            <div class="feedback-custom-controls">
              <input class="feedback-range" type="range" min="1" max="16" step="1" :value="overallWeight" :disabled="submitting" @input="emit('update:overallWeightInput', ($event.target as HTMLInputElement).value)" />
              <input class="feedback-number" type="number" min="1" max="16" step="1" :value="overallWeightInput" :disabled="submitting" @input="emit('update:overallWeightInput', ($event.target as HTMLInputElement).value)" />
            </div>
            <div class="feedback-strength-legend">
              <span>轻微</span>
              <span>明确</span>
              <span>强烈</span>
            </div>
          </div>

          <div class="feedback-custom">
            <div class="feedback-custom-head">
              <h3>附加信号</h3>
              <span>这些字段会和整体倾向一起提交</span>
            </div>
            <div class="feedback-segmented">
              <button type="button" class="feedback-chip" :class="{ active: favorite }" :disabled="submitting" @click="emit('toggle:favorite')">标记喜爱</button>
              <button type="button" class="feedback-chip" :class="{ active: dislike }" :disabled="submitting" @click="emit('toggle:dislike')">标记厌恶</button>
              <button type="button" class="feedback-chip" :class="{ active: hideOnce }" :disabled="submitting" @click="emit('toggle:hideOnce')">仅隐藏这条</button>
            </div>
          </div>

          <div v-if="hasAuthorControl" class="feedback-custom">
            <div class="feedback-custom-head">
              <h3>作者倾向</h3>
              <span>{{ video?.author }}</span>
            </div>
            <div class="feedback-segmented">
              <button type="button" class="feedback-chip" :class="{ active: authorIntent === 'more' }" :disabled="submitting" @click="emit('update:authorIntent', authorIntent === 'more' ? 'none' : 'more')">多推这个作者</button>
              <button type="button" class="feedback-chip" :class="{ active: authorIntent === 'less' }" :disabled="submitting" @click="emit('update:authorIntent', authorIntent === 'less' ? 'none' : 'less')">减少这个作者</button>
            </div>
            <p class="feedback-help">
              这里控制的是“以后遇到这个作者时”的倾向强弱，不只影响当前这条。数值越高，作者偏好修正越明显。
            </p>
            <div class="feedback-custom-controls">
              <input class="feedback-range" type="range" min="1" max="16" step="1" :value="authorWeightInput" :disabled="submitting" @input="emit('update:authorWeightInput', ($event.target as HTMLInputElement).value)" />
              <input class="feedback-number" type="number" min="1" max="16" step="1" :value="authorWeightInput" :disabled="submitting" @input="emit('update:authorWeightInput', ($event.target as HTMLInputElement).value)" />
            </div>
            <div class="feedback-strength-legend">
              <span>轻微</span>
              <span>明确</span>
              <span>强烈</span>
            </div>
          </div>

          <div class="feedback-custom">
            <div class="feedback-custom-head">
              <h3>标签倾向</h3>
              <span>{{ availableTags.length ? `已选 ${selectedTagCount} 个标签` : "当前视频没有可用标签" }}</span>
            </div>
            <div v-if="availableTags.length" class="feedback-tag-layout">
              <div>
                <div class="feedback-tag-head">增加这些标签</div>
                <div class="feedback-chip-grid">
                  <button
                    v-for="tag in availableTags"
                    :key="`more-${tag}`"
                    type="button"
                    class="feedback-chip"
                    :class="{ active: selectedMoreTags.includes(tag) }"
                    :disabled="submitting || selectedLessTags.includes(tag)"
                    @click="emit('toggle-tag', { direction: 'more', value: tag })"
                  >
                    {{ tag }}
                  </button>
                </div>
              </div>
              <div>
                <div class="feedback-tag-head">减少这些标签</div>
                <div class="feedback-chip-grid">
                  <button
                    v-for="tag in availableTags"
                    :key="`less-${tag}`"
                    type="button"
                    class="feedback-chip"
                    :class="{ active: selectedLessTags.includes(tag) }"
                    :disabled="submitting || selectedMoreTags.includes(tag)"
                    @click="emit('toggle-tag', { direction: 'less', value: tag })"
                  >
                    {{ tag }}
                  </button>
                </div>
              </div>
              <p class="feedback-help">
                当前后端会直接接收标签字符串数组。这里的“增加/减少”选择会保留在标签文本里一起发出，标签强度则主要体现在总分和整体倾向上。
              </p>
              <div class="feedback-custom-controls">
                <input class="feedback-range" type="range" min="1" max="16" step="1" :value="tagWeightInput" :disabled="submitting" @input="emit('update:tagWeightInput', ($event.target as HTMLInputElement).value)" />
                <input class="feedback-number" type="number" min="1" max="16" step="1" :value="tagWeightInput" :disabled="submitting" @input="emit('update:tagWeightInput', ($event.target as HTMLInputElement).value)" />
              </div>
              <div class="feedback-strength-legend">
                <span>轻微</span>
                <span>明确</span>
                <span>强烈</span>
              </div>
              <div class="feedback-inline-field">
                <span class="feedback-tag-head">标签强度</span>
                <span class="feedback-inline-value">{{ tagWeightInput }}</span>
              </div>
            </div>
            <p v-else class="feedback-empty">后端若后续补充 `tags`、`keywords` 或 `category`，这里会自动出现更多可选标签。</p>
          </div>

          <div class="feedback-actions">
            <button type="button" class="feedback-action feedback-action-secondary" :disabled="submitting" @click="emit('reset')">重置选择</button>
            <button type="button" class="feedback-action feedback-action-secondary" :disabled="submitting" @click="emit('close')">取消</button>
            <button type="button" class="feedback-action feedback-action-primary" :disabled="submitting" @click="emit('submit')">{{ submitting ? "提交中..." : "提交反馈" }}</button>
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
.feedback-template,
.feedback-chip {
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
  color: var(--font-color);
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
  color: var(--font-color);
}

.feedback-template small {
  color: color-mix(in srgb, var(--font-color) 78%, var(--dark-font-color));
  line-height: 1.6;
}

.feedback-custom {
  padding: 22px;
  border-radius: 24px;
  background: color-mix(in srgb, var(--surface-color) 74%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--surface-border) 90%, transparent);
}

.feedback-custom + .feedback-custom {
  margin-top: 18px;
}

.feedback-segmented,
.feedback-chip-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.feedback-segmented {
  margin-top: 18px;
}

.feedback-chip-grid {
  margin-top: 12px;
}

.feedback-chip {
  padding: 11px 14px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--font-color) 4%, transparent);
  color: var(--font-color);
  transition: transform 0.18s ease, background-color 0.18s ease, color 0.18s ease;
}

.feedback-chip.active {
  color: #fff;
  background: linear-gradient(135deg, var(--focus-color), #fb7299);
}

.feedback-inline-field {
  margin-top: 18px;
  max-width: 160px;
}

.feedback-inline-value {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 44px;
  min-width: 76px;
  margin-top: 10px;
  padding: 0 14px;
  border-radius: 14px;
  background: var(--surface-elevated);
  box-shadow: var(--surface-shadow-soft);
  color: var(--font-color);
  font-family: var(--mono-font), monospace;
}

.feedback-custom-controls {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 112px;
  gap: 16px;
  margin-top: 18px;
}

.feedback-help {
  margin: 16px 0 0;
  color: var(--dark-font-color);
  font-size: 14px;
  line-height: 1.7;
}

.feedback-strength-legend {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  margin-top: 10px;
  color: var(--dark-font-color);
  font-size: 12px;
  font-family: var(--mono-font), monospace;
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
  appearance: textfield;
}

.feedback-number::-webkit-outer-spin-button,
.feedback-number::-webkit-inner-spin-button {
  appearance: none;
  margin: 0;
}

.feedback-tag-layout {
  display: grid;
  gap: 18px;
  margin-top: 18px;
}

.feedback-tag-head {
  color: var(--dark-font-color);
  font-size: 13px;
}

.feedback-empty {
  margin: 18px 0 0;
  color: var(--dark-font-color);
  line-height: 1.7;
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

<script setup lang="ts">
import {computed} from "vue";
import type {Video} from "@/component/videos/interfaces.ts";

type Props = {
  video: Video;
  feedbackDisabled?: boolean;
};

const {video, feedbackDisabled = false} = defineProps<Props>();

const emit = defineEmits<{
  (e: "feedback", video: Video): void;
}>();

const coverUrl = computed(() => {
  if(!video.videoURL)
    return "";
  return `/image?url=${video.videoURL}`;
});

function openFeedback(event: MouseEvent): void {
  event.preventDefault();
  event.stopPropagation();
  emit("feedback", video);
}
</script>

<template>
  <article class="video-card">
    <a
      class="video-card-link"
      :href="video.url"
      target="_blank"
      rel="noopener noreferrer"
    >
      <div class="video-cover-wrapper">
        <img
          class="video-cover"
          :src="coverUrl"
          :alt="video.title"
          loading="lazy"
        />
        <span class="video-duration">{{ video.videoTime }}</span>
      </div>

      <div class="video-info">
        <div class="video-title">{{ video.title }}</div>
        <div class="video-meta">{{ video.author }}</div>
        <div class="video-meta">
          <span>{{ video.views }} 播放</span>
          <span> · </span>
          <span>{{ video.popups }} 弹幕</span>
        </div>
        <div class="video-meta">{{ video.publishTime }}</div>
      </div>
    </a>

    <div class="video-actions">
      <a
        class="video-action-link"
        :href="video.url"
        target="_blank"
        rel="noopener noreferrer"
      >
        立即观看
      </a>
      <button
        type="button"
        class="video-feedback-button"
        :disabled="feedbackDisabled"
        @click="openFeedback"
      >
        {{ feedbackDisabled ? "提交中..." : "反馈" }}
      </button>
    </div>
  </article>
</template>

<style scoped src="@/component/videos/video-card.css"></style>

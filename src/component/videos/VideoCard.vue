<script setup lang="ts">
import {computed} from "vue";
import type {Video} from "@/component/videos/interfaces.ts";

type Props = {
  video: Video;
};

const {video} = defineProps<Props>();

const coverUrl = computed(() => {
  if(!video.videoURL)
    return "";
  return `/image?url=${video.videoURL}`;
});
</script>

<template>
  <a
    class="video-card"
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
</template>

<style scoped src="@/component/videos/video-card.css"></style>

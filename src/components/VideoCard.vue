<script lang="ts">
import {defineComponent} from 'vue'

export default defineComponent({
name: "VideoCard"
})

export interface Video{
  url: string
  title: string
  publishTime: string
  videoTime: string
  videoImage: {
    url: string
    width?: string
    height?: string
  }
  views: string
  popups: string
  author: string
}

const getProxiedImageUrl = (originalUrl: string) => {
  if (!originalUrl) return '';
  // 使用 encodeURIComponent 确保原始URL中的特殊字符（如&）不会破坏我们的API调用
  const encodedUrl = encodeURIComponent(originalUrl);
  return `/api/image-proxy?url=${encodedUrl}`;
};
</script>
<script setup lang="ts">
const _video = defineProps<{video:Video}>()
const video: Video = _video.video
// TODO 视频样式还需调整，以适应不同种类视频封面和主题
</script>
<template>
  <a :href="video.url" target="_blank" rel="noopener noreferrer" class="block w-72">
    <div class="card card-sm bg-base-100 shadow-md rounded-lg overflow-hidden hover:-translate-y-1 hover:shadow-xl transition-transform duration-300">

      <!-- 顶部的视频封面区域 -->
      <figure class="relative">
        <!-- 封面图片 -->
        <img :src="getProxiedImageUrl(video.videoImage.url)"  alt="" class="w-full aspect-video object-cover" />

        <!-- 视频时长和统计信息 -->
        <div class="absolute bottom-0 left-0 right-0 p-2 bg-gradient-to-t to-transparent">
          <div class="flex justify-between items-center text-primary-700 text-xs font-semibold">
            <!-- 左侧：播放量和弹幕数 -->
            <div class="flex items-center gap-3">
                    <span class="flex items-center gap-1">
                        <!-- 播放量图标 -->
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" viewBox="0 0 20 20" fill="currentColor"><path d="M10 12a2 2 0 100-4 2 2 0 000 4z" /><path fill-rule="evenodd" d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.022 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z" clip-rule="evenodd" /></svg>
                        <span class="text-base">{{video.views}}</span>
                    </span>
              <span class="flex items-center gap-1">
                        <!-- 弹幕图标 -->
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6" viewBox="0 -1 20 19" fill="currentColor"><path d="M2 5a2 2 0 012-2h12a2 2 0 012 2v6a2 2 0 01-2 2H4a2 2 0 01-2-2V5zm14 1a1 1 0 00-1-1H5a1 1 0 00-1 1v5h12V6zM2 15a1 1 0 011-1h1v-2H3a3 3 0 00-3 3v1h16v-1a3 3 0 00-3-3h-1v2h1a1 1 0 011 1v1H2v-1z" /></svg>
                        <span class="text-base">{{video.popups}}</span>
                    </span>
            </div>
            <!-- 右侧：视频时长 -->
            <span>{{video.videoTime}}</span>
          </div>
        </div>
      </figure>

      <!-- 下方的视频信息区域 -->
      <div class="card-body p-3 text-base-content/90">
        <!-- 视频标题，最多显示两行 -->
        <h2 class="card-title text-sm font-bold line-clamp-2 h-10" :title="video.title">
          {{video.title}}
        </h2>
        <span></span>
        <!-- UP主信息和发布时间 -->
        <div class="flex items-center text-xs text-base-content/70 mt-1">
          <span>{{ video.author }}</span>
          <span class="mx-2">·</span>
          <span>{{ video.publishTime }}</span>
        </div>
      </div>
    </div>
  </a>
</template>

<style></style>

<script setup lang="ts">
import VideoCard, { type Category, type Video } from '@/components/VideoCard.vue'
import MainContainer from '@/components/MainContainer.vue'
import { onMounted } from 'vue'

const categories: Category[];
onMounted(() => {
  categories.value = [];
})

const video: Video[] = [
  {
    url: 'https://www.bilibili.com/video/BV1Q4NWzSEGm/',
    title: '以色列上空的焰火英文版Fireworks Over Israel',
    publishTime: '06-19',
    videoTime: '03:18',
    videoImage: {
      url: 'https://i0.hdslb.com/bfs/archive/5381c145288378d74a96e591a4f65f8e470732db.jpg@672w_378h_1c.webp',
    },
    views: '105.8万',
    popups: '3246',
    author: 'Innokids英之子乐队',
  },
  {
    url: 'https://www.bilibili.com/video/BV1Q4NWzSEGm/',
    title: '以色列上空的焰火英文版Fireworks Over Israel',
    publishTime: '06-19',
    videoTime: '03:18',
    videoImage: {
      url: 'https://i0.hdslb.com/bfs/archive/5381c145288378d74a96e591a4f65f8e470732db.jpg@672w_378h_1c.webp',
    },
    views: '105.8万',
    popups: '3246',
    author: 'Innokids英之子乐队',
  },
]
</script>

<template>
  <MainContainer>
    <h1 class="text-4xl font-bold text-accent items-center px-4 py-2 rounded-full shadow">
      为您精选视频💕
    </h1>
    <ul class="flex flex-wrap space-x-4.5 space-y-2">
      <li v-for="v in video" :key="v.url">
        <VideoCard :video="v"></VideoCard>
      </li>
    </ul>
    <ul class="flex flex-wrap space-x-4.5 space-y-2" v-for=""></ul>
  </MainContainer>
</template>

<style scoped></style>

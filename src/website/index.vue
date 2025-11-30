<script setup lang="ts">
import VideoCard, { type Category } from '@/components/VideoCard.vue'
import MainContainer from '@/components/MainContainer.vue'
import { onMounted, ref } from 'vue'
import { refreshCategories, requestCategories } from '@/backend.ts'

const categories = ref<Category[]>([])

onMounted(async () => {
  initBackendUrl()
  try{
    categories.value = await Promise.all((await requestCategories()).map(async name => {
      return {
        name: name,
        videos : await refreshCategories(name)
      }
    }))
  }catch (reason){
    console.log('Init categories failed !\n' + reason)
    categories.value = []
  }
})
</script>
<script lang="ts">
export const Docker: boolean = import.meta.env.VITE_DOCKER == 'true'
export const backendURL: string = 'backend_url'

export function getBackendUrl(): string {
  let url = localStorage.getItem(backendURL)
  if (url)
    return url
  else {
    initBackendUrl()
    url = localStorage.getItem(backendURL)
    return url ? url : ''
  }
}

export function setBackendUrl(url: string | null): void {
  if (url) localStorage.setItem(backendURL, url)
  else localStorage.removeItem(backendURL)
}

export function initBackendUrl() {
  if (Docker && !localStorage.getItem(backendURL))
    setBackendUrl('/backend')
}
</script>

<template>
  <MainContainer>
    <h1 class="text-4xl font-bold text-accent items-center px-4 py-2 rounded-full shadow">
      为您精选视频💕
    </h1>
    <ul
      class="flex flex-wrap space-x-4.5 space-y-2"
      v-for="category in categories"
      :key="category.name"
    >
      <li v-for="video in category.videos" :key="video.url">
        <VideoCard :video="video"></VideoCard>
      </li>
    </ul>
  </MainContainer>
</template>

<style scoped></style>

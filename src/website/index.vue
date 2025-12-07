<script setup lang="ts">
import { type Category } from '@/components/VideoCard.vue'
import MainContainer from '@/components/MainContainer.vue'
import { onMounted, type Ref, ref } from 'vue'
import { refreshCategories, requestCategories } from '@/backend.ts'
import CategoriesContainer from '@/components/CategoriesContainer.vue'

const categories: Ref<Category[]> = ref<Category[]>([])
const categoriesInitError : Ref<boolean> = ref<boolean>(false)

onMounted(async () => {
  initBackendUrl()
  try {
    categories.value = await Promise.all(
      (await requestCategories()).map(async (name) => {
        return {
          name: name,
          videos: [],
        }
      }),
    )
    categoriesInitError.value = false
  } catch (reason) {
    console.log('Init categories failed !\n' + reason)
    categories.value = []
    categoriesInitError.value = true
  }
})

async function refreshVideos(category : Category){
  category.videos = await refreshCategories(category.name)
}
</script>
<script lang="ts">
export const Docker: boolean = import.meta.env.VITE_DOCKER == 'true'
export const backendURL: string = 'backend_url'

export function getBackendUrl(): string {
  let url = localStorage.getItem(backendURL)
  if (url) return url
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
  if (Docker && !localStorage.getItem(backendURL)) setBackendUrl('/backend')
}
</script>

<template>
  <MainContainer>
    <h1 class="text-4xl font-bold text-accent items-center px-3 py-2">
      为您精选视频💕
    </h1>
    <div class="@container-normal py-2 px-8">
      <CategoriesContainer
        :categories="categories"
        :error="categoriesInitError"
        @refresh="refreshVideos"
      ></CategoriesContainer>
    </div>
  </MainContainer>
</template>

<style>
@import "@/main.css";
h1,h2 {
  @apply text-base-content
}
p,span{
  @apply text-base-content/70
}
</style>

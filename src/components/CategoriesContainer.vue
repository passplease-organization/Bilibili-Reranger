<script setup lang="ts">
import VideoCard, { type Category } from '@/components/VideoCard.vue'
import { defineComponent, reactive } from 'vue'
import { ChevronDoubleRightIcon } from '@heroicons/vue/24/solid'

interface props {
  categories: Category[]
  error: boolean
}
const props = defineProps<props>()
const emits = defineEmits(['refresh'])

defineComponent({
  name: 'CategoriesContainer',
})

function refreshVideos(category: Category) {
  emits('refresh', category)
  rotate[category.name] = false
}

const rotate = reactive<Record<string, boolean>>({})
function toggleCategory(name : string) {
  rotate[name] = !rotate[name]
}
</script>

<template>
  <p v-if="props.error" class="text-3xl font-bold text-base-200">视频加载错误，请稍后刷新重试...</p>
  <p v-else-if="props.categories.length <= 0" class="text-5xl font-bold text-accent">
    正在加载视频数据，请稍等...
  </p>
  <ul v-else class="flex flex-col align-bottom justify-start gap-4">
    <li v-for="category in props.categories" :key="category.name" class="min-h-3">
      <div class="flex content-start align-middle items-center">
        <div
          @click="toggleCategory(category.name)"
          :class="{
            'rotate-90': rotate[category.name],
            'transition-transform duration-200': true
          }"
          class="min-w-7 cursor-pointer top-1.5"
        >
          <ChevronDoubleRightIcon />
        </div>
        <p @click="toggleCategory(category.name)" class="cursor-pointer text-2xl font-500 text-primary-content/90" id="category">
          {{ category.name }}
        </p>
        <button @click="refreshVideos(category)" class="btn btn-primary btn-soft shrink-0">
          获取最新
        </button>
      </div>
      <div class="py-2"></div>
      <ul
        class="flex align-top gap-2 content-start gap-y-1 flex-wrap justify-evenly"
        v-if="rotate[category.name] && category.videos.length >= 0"
      >
        <li v-for="video in category.videos" :key="video.url">
          <VideoCard :video="video"></VideoCard>
        </li>
      </ul>
    </li>
  </ul>
</template>

<style scoped>
#category {
  margin-right: 5%;
}
</style>

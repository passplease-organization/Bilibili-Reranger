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
  rotate[category.name] = true
}

const rotate = reactive<Record<string, boolean>>({})
function toggleCategory(category : Category) {
  rotate[category.name] = !rotate[category.name]
  if(rotate[category.name] && category.videos.length <= 0)
    refreshVideos(category)
}

const load : string[] = Array.from("加载中...")
</script>

<template>
  <p v-if="props.error" class="text-3xl font-bold text-base-200">视频加载错误，请稍后刷新重试...</p>
  <p v-else-if="props.categories.length <= 0" class="text-5xl font-bold text-accent">
    正在加载视频数据，请稍等...
  </p>
  <ul v-else class="flex flex-col align-bottom justify-start gap-6">
    <li v-for="category in props.categories" :key="category.name" class="min-h-3">
      <div class="flex content-start align-middle items-center gap-5">
        <div
          @click="toggleCategory(category)"
          class="hover:scale-105 transition-transform flex content-center cursor-pointer"
        >
          <div
            :class="{
            'rotate-90': rotate[category.name],
            'transition-transform duration-200': true
          }"
            class="min-w-7 top-1.5"
          >
            <ChevronDoubleRightIcon />
          </div>
          <p class="text-2xl font-500 text-base-content" id="category">
            {{ category.name }}
          </p>
        </div>
        <button @click="refreshVideos(category)" class="btn btn-primary btn-soft shrink-0">
          获取最新
        </button>
      </div>
      <div class="py-2"></div>
      <div v-if="rotate[category.name]">
        <ul
          class="flex align-top content-start gap-4 gap-y-1 flex-wrap justify-evenly"
          v-if="category.videos.length > 0"
        >
          <li v-for="video in category.videos" :key="video.url">
            <VideoCard :video="video"></VideoCard>
          </li>
        </ul>
        <div v-else class="max-w-70">
          <div class="items-center flex justify-center gap-1">
            <span v-for="(character,index) in load"
                  :key="index"
                  class="loadText"
                  :style="{'--i': index, '--count': load.length}"
            >{{ character }}</span>
          </div>
          <div id="track" class="bg-base-300 shadow-neutral border-secondary/50">
            <div id="waiting" class="bg-primary shadow-neutral"></div>
          </div>
        </div>
      </div>
    </li>
  </ul>
</template>

<style scoped>
:root{
  --i: 0;
  --count: 0;
}

#category {
  margin-right: 5%;
}

.loadText{
  font-size: larger;
  animation-name: jump;
  animation-delay: calc(var(--i) * 0.15s);
  animation-timing-function: ease-in-out;
  animation-duration: calc(var(--count) * 0.18s);
  animation-iteration-count: infinite;
}

#track{
  height: 15px;
  border-width: 2px;
  border-style: inset;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  overflow: hidden;
  position: relative;
  border-radius: 10px;
  user-select: none;
}

#waiting{
  width: 30px;
  height: 30px;
  animation-name: wait;
  animation-duration: 2s;
  animation-iteration-count: infinite;
  animation-timing-function: ease;
  z-index: 10;
  position: absolute;
  border-radius: 50%;
  filter: blur(8px);
}

@keyframes jump {
  0%,50%,100%{
    transform: translateY(0%);
  }
  25%{
    transform: translateY(-50%);
  }
}

@keyframes wait {
  0%{
    left: -30px;
  }
  100%{
    left: calc(100% + 30px);
  }
}

@media (max-width: 768px){
  #waiting{
    width: 24px;
    height: 24px;
  }
  #track {
    height: 8px;
  }
}
</style>

<script lang="ts">
import {defineComponent} from 'vue'

export default defineComponent({
name: "ThemeCard"
})
</script>
<script setup lang="ts">
const theme = defineProps<{
  theme: string
}>()
</script>

<template>
  <div class="bg-base-100 text-base-content w-full font-sans hover-scale" :data-theme="theme.theme">
    <div class="grid grid-cols-5 grid-rows-3">
      <!-- Sidebar preview -->
      <div class="bg-base-200 col-start-1 row-span-2 row-start-1"></div>
      <div class="bg-base-300 col-start-1 row-start-3"></div>

      <!-- Main content preview -->
      <div class="bg-base-100 col-span-4 col-start-2 row-span-3 row-start-1 flex flex-col gap-1 p-2">
        <div class="font-bold">{{ theme.theme }}</div>
        <div class="flex flex-wrap gap-1">
          <!-- Color swatches -->
          <div class="bg-primary flex aspect-square w-5 items-center justify-center rounded lg:w-6">
            <div class="text-primary-content text-sm font-bold">A</div>
          </div>
          <div class="bg-secondary flex aspect-square w-5 items-center justify-center rounded lg:w-6">
            <div class="text-secondary-content text-sm font-bold">A</div>
          </div>
          <div class="bg-accent flex aspect-square w-5 items-center justify-center rounded lg:w-6">
            <div class="text-accent-content text-sm font-bold">A</div>
          </div>
          <div class="bg-neutral flex aspect-square w-5 items-center justify-center rounded lg:w-6">
            <div class="text-neutral-content text-sm font-bold">A</div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style></style>

<script setup lang="ts">
import { defineComponent } from "vue";
import SettingsComponent from "@/components/SettingsComponent.vue";
import type { setCategory } from "@/components/settingsInterface.ts";

defineComponent({
  name: "SettingsCategory",
});

const { categories: props } = defineProps<{ categories: setCategory }>();
</script>

<template>
  <h2 class="text-4xl font-bold rounded-full">{{ props.name }}</h2>
  <div class="h-3"></div>
  <div class="grid gap-0.5" id="grid">
    <div v-for="setting in props.settings" :key="setting.title">
      <SettingsComponent :props="setting" />
      <div class="gap-y-2"></div>
    </div>
  </div>
</template>

<style scoped>
#grid {
  grid-template-columns: 1fr 1fr;
  gap: 2em 4em;
  justify-content: center;
}

@media (max-width: 1000px) {
  #grid {
    grid-template-columns: 1fr;
  }
}
</style>

<script setup lang="ts">
import { type Category } from "@/components/VideoCard.vue";
import MainContainer from "@/components/MainContainer.vue";
import { onMounted, type Ref, ref } from "vue";
import { refreshCategories, requestCategories } from "@/backend.ts";
import CategoriesContainer from "@/components/CategoriesContainer.vue";
import { setupValid } from "@/website/backendSetup.ts";
import router from "@/router";

const categories: Ref<Category[]> = ref<Category[]>([]);
const categoriesInitError: Ref<boolean> = ref<boolean>(false);
const loadFinished = ref<boolean>(false);

onMounted(async () => {
  initBackendUrl();
  if (!setupValid()) {
    await router.push({
      path: "/login",
    });
    return false;
  }
  try {
    categories.value = await Promise.all(
      (await requestCategories()).map(async (name) => {
        return {
          name: name,
          videos: [],
        };
      }),
    );
    categoriesInitError.value = false;
    loadFinished.value = true;
  } catch (reason) {
    console.log("Init categories failed !\n" + reason);
    categories.value = [];
    categoriesInitError.value = true;
    loadFinished.value = true;
  }
});

async function refreshVideos(category: Category) {
  category.videos = await refreshCategories(category.name);
}
import { initBackendUrl } from "@/website/backendSetup.ts";
</script>

<template>
  <MainContainer>
    <h1 class="text-4xl font-bold text-accent items-center px-3 py-2">为您精选视频💕</h1>
    <div class="@container-normal py-2 px-8">
      <CategoriesContainer
        :categories="categories"
        :error="categoriesInitError"
        :load-finished="loadFinished"
        @refresh="refreshVideos"
      ></CategoriesContainer>
    </div>
  </MainContainer>
</template>

<style>
@import "@/main.css";
h1,
h2 {
  @apply text-base-content;
}
p,
span {
  @apply text-base-content/70;
}
</style>

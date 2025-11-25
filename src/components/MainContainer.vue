<script lang="ts">
import {defineComponent} from 'vue'

export default defineComponent({
name: "MainContainer"
})
</script>

<template>
  <div class="@container px-8 py-4">
    <slot />
  </div>
</template>

<style></style>

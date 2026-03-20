<script setup lang="ts">
import type setCategory from "@/component/utils/settingsInterface.ts";
import SettingProp from "@/component/settings/SettingProp.vue";
import Card from "@/component/Card.vue";

type props = {
  setting: setCategory,
  size?: string,
  cardId?: string
};

const p = defineProps<props>();
const setting: setCategory = p.setting;
const size: string = p.size || "14px";
</script>

<template>
  <Card :cardId="p.cardId" :size="size" :title="setting.name">
    <div class="set">
      <SettingProp :style="`--max-set-size: ${size}`" v-for="set in setting.settings" :setting="set" />
    </div>
  </Card>
</template>

<style>
.set{
  --max-set-size: 14px;
  margin: clamp(10px,calc(var(--max-set-size) / 2),20px) 5px;
  overflow-wrap: anywhere;
}
</style>

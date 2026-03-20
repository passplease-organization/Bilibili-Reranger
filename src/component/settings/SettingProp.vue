<script setup lang="ts">
import type {button, settingsProps} from "@/component/utils/settingsInterface.ts";
import BaseInput from "@/component/utils/BaseInput.vue";
import {computed} from "vue";
import BaseSelector from "@/component/utils/BaseSelector.vue";

type Props = {
  setting: settingsProps;
};

const props = defineProps<Props>();
const button = computed(() => props.setting.button);
const input = computed(() => props.setting.input);
const select = computed(() => props.setting.select);

async function enter(button: button | undefined, target: EventTarget | null,event: Event) {
  if (button?.enter != false) {
    if(target instanceof HTMLButtonElement)
      target.click();
    else await button?.click(event);
    if(target instanceof HTMLElement)
      target.blur();
    else if(document.activeElement instanceof HTMLElement)
      document.activeElement.blur();
  }
}
</script>

<template>
  <h2 class="base-title">{{ props.setting.title }}</h2>
  <div class="setting-content">
    <a class="base-desc">{{ props.setting.description }}</a>

    <div class="input-group" style="--base-input-size: var(--max-set-size)">
      <BaseInput v-if="input" :input="input" @keydown.enter="enter(button?.save, $event.target,$event)" />
      <BaseSelector v-else-if="select" :setting="select" />
      <button
          v-if="button?.save"
          @click="button.save.click"
          class="btn btn-primary"
      >
        保存
      </button>
      <button
          v-if="button?.reset"
          @click="button.reset.click"
          class="btn btn-ghost"
      >
        重置
      </button>
    </div>
  </div>
</template>

<style scoped src="../utils/base-text.css"></style>
<style scoped src="../utils/base-button.css"></style>

<style scoped>
.input-group {
  --setting-prop-min-font-size: 14px;
  --setting-prop-max-font-size: 18px;
  --setting-prop-resolved-size: clamp(
    var(--setting-prop-min-font-size),
    var(--base-input-size, 16px),
    var(--setting-prop-max-font-size)
  );
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.setting-content {
  padding: 0 4px 4px;
}
</style>

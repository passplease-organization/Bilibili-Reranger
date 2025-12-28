<script lang="ts">
import type { button } from "@/components/settingsInterface.ts";

function enter(button: button, target: EventTarget | null) {
  if (button.enter != false && target) target.click();
}
</script>

<script setup lang="ts">
import { computed, defineComponent, isRef, Ref, unref } from "vue";
import type { settingsProps } from "@/components/settingsInterface.ts";
defineComponent({ name: "SettingsComponent" });
const { props: props } = defineProps<{ props: settingsProps }>();
const input = props.input;

const placeholder = computed({
  get() {
    return unref(input?.placeholder);
  },
  set(val: string) {
    if (input && input.placeholder && isRef(input.placeholder)) {
      input.placeholder.value = val;
    }
  },
});

const model = computed({
  get() {
    return unref(input?.v_model);
  },
  set(val: string) {
    if (input && input.v_model && isRef(input.v_model)) {
      input.v_model.value = val;
    }
  },
});
</script>

<template>
  <h3>{{ props.title }}</h3>
  <p>{{ props.description }}</p>
  <input
    v-if="input"
    :type="input.type"
    v-model="model"
    :placeholder="placeholder"
    class="input input-bordered w-1/2"
    @click="input.click"
    @focusin="input.focusin"
    @focusout="input.focusout"
  />
  <button
    v-if="input"
    @click="input.save.click"
    @keydown.enter="enter(input.save, $event.currentTarget)"
    class="btn btn-primary"
  >
    保存
  </button>
  <button
    v-if="input"
    @click="input.reset.click"
    @keydown.enter="enter(input.reset, $event.currentTarget)"
    class="btn btn-ghost"
  >
    重置
  </button>
</template>

<style scoped>
h3 {
  @apply text-3xl font-bold py-2 px-4 rounded-full;
}
p {
  @apply px-4 mb-4;
  font-size: 1.25rem;
}
</style>

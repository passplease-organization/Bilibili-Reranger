<script setup lang="ts">
import {getCurrentInstance, ref, type Ref} from "vue";
import {IconArrowBadgeDown,IconZoomInArea,IconZoomOutArea} from "@tabler/icons-vue";

type props = {
  size?: string,
  fold?: boolean,
  title?: string,
  cardId?: string,
  hasTools?: boolean,
};

const p = withDefaults(defineProps<props>(),{
  hasTools: true,
});
const style: string = `--size: ${p.size ?? '16px'};`;
const zoom: Ref<boolean> = ref(false);
const fold: Ref<boolean> = ref(p.fold || false);

const uid = getCurrentInstance()?.uid;
function changeZoom(z?: boolean | unknown | undefined): void {
  if(!p.hasTools)
    return;
  fold.value = false;
  zoom.value = typeof z === 'boolean' ? z : !zoom.value;
  emit('zoom-change',{
    id: uid,
    zoom: zoom.value,
    cardId: p.cardId
  });
}
function changeFold(): void{
  if(!p.hasTools)
    return;
  zoom.value = false;
  fold.value = !fold.value;
  emit('zoom-change',{
    id: uid,
    zoom: zoom.value,
    cardId: p.cardId
  });
}

export type zoom_change = {
  id: number | undefined;
  cardId?: string;
  zoom: boolean;
}
const emit = defineEmits<{
  (e: "zoom-change",payload: zoom_change): void;
}>();
</script>

<template>
  <div class="card" :style="style" :class="{zoom,fold}" @dblclick="changeZoom(true)">
    <div class="tools" v-if="p.hasTools" style="margin-bottom: 5px" @click="changeFold">
      <IconArrowBadgeDown class="fold-botton" :class="{fold}" />
      <div class="title" :class="{fold}">{{p.title}}</div>
      <div @click.stop="changeZoom"
           style="min-width: min(40px,calc(7 * var(--size)));display: flex;justify-content: flex-end;"
           title="点击按钮或双击任意区域放大"
      >
        <Transition name="card-icon" mode="out-in">
          <IconZoomOutArea v-if="zoom" key="zoom-out" />
          <IconZoomInArea v-else key="zoom-in" />
        </Transition>
      </div>
    </div>
    <Transition name="card-body">
      <div v-show="!fold" class="card-body">
        <slot style="margin-top: var(--card-size);" />
      </div>
    </Transition>
  </div>
</template>

<style scoped>
html .card{
  --bg-color: var(--surface-color);
}
html.dark .card{
  --bg-color: var(--surface-elevated);
}

.card{
  position: relative;
  --transition-time: 0.22s;
  --size: 16px;
  --card-size: var(--size);
  min-width: calc(20 * var(--card-size));
  max-width: calc(42 * var(--card-size));
  min-height: calc(22 * var(--card-size));
  max-height: calc(54 * var(--card-size));
  margin: clamp(8px,calc(2 * var(--card-size)),12px);
  padding: calc(1.5 * var(--card-size));
  background-color: var(--bg-color);
  border-radius: clamp(18px, var(--card-size), 24px);
  box-shadow: var(--surface-shadow);
  transform-origin: top center;
  transition:
    transform var(--transition-time) ease,
    box-shadow var(--transition-time) ease,
    background-color var(--transition-time) ease,
    max-width calc(var(--transition-time) * 1.3) cubic-bezier(0.22, 1, 0.36, 1),
    min-height calc(var(--transition-time) * 1.3) cubic-bezier(0.22, 1, 0.36, 1),
    max-height calc(var(--transition-time) * 1.3) cubic-bezier(0.22, 1, 0.36, 1),
    padding calc(var(--transition-time) * 1.3) cubic-bezier(0.22, 1, 0.36, 1),
    border-radius calc(var(--transition-time) * 1.3) cubic-bezier(0.22, 1, 0.36, 1);
  overflow-wrap: anywhere;
  overflow-y: auto;
  scrollbar-width: thin;
  scrollbar-color: var(--dark-font-color) transparent;
}
html.dark .card{
  scrollbar-color: var(--dark-font-color) transparent;
}

.card:hover{
  transform: translateY(-2px);
  box-shadow:
    0 0 0 1px var(--surface-border),
    rgba(0,0,0,0.06) 0 16px 24px -18px,
    rgba(255,255,255,0.04) 0 1px 0 inset;
}

.card.zoom{
  --card-size: calc(1.35 * var(--size));
  max-width: calc(54 * var(--card-size));
  min-height: calc(28 * var(--card-size));
  max-height: 100vh;
}

.card.fold{
  min-height: 0;
  height: 54px;
  overflow: hidden;
}

.card-body {
  overflow: hidden;
  transform-origin: top center;
}

html.dark .tools{
  --border-color: var(--surface-border);
}

html .tools{
  --border-color: var(--surface-border);
}

.tools{
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid var(--border-color);
  margin-bottom: 8px;
  padding-bottom: 10px;
  user-select: none;
  max-height: 42px;
}

.fold-botton{
  transition: transform,color var(--transition-time) ease-in-out;
  color: var(--dark-font-color);
}

.fold-botton.fold{
  transform: rotateZ(-90deg);
  color: var(--focus-color);
}

.title{
  text-align: center;
  font-family: var(--important-font),sans-serif;
  font-size: clamp(18px,calc(var(--card-size) * 1.15),24px);
  font-weight: 600;
  letter-spacing: -0.04em;
  color: var(--font-color);
  pointer-events: none;
}

.title.fold{
  color: var(--dark-font-color);
}

.card-body-enter-active,
.card-body-leave-active {
  transition:
    opacity calc(var(--transition-time) * 1.2) ease,
    transform calc(var(--transition-time) * 1.2) cubic-bezier(0.22, 1, 0.36, 1),
    max-height calc(var(--transition-time) * 1.4) cubic-bezier(0.22, 1, 0.36, 1);
}

.card-body-enter-from,
.card-body-leave-to {
  opacity: 0;
  transform: translateY(-8px) scaleY(0.96);
  max-height: 0;
}

.card-body-enter-to,
.card-body-leave-from {
  opacity: 1;
  transform: translateY(0) scaleY(1);
  max-height: 1200px;
}

.card-icon-enter-active,
.card-icon-leave-active {
  transition:
    opacity 160ms ease,
    transform 180ms ease;
}

.card-icon-enter-from,
.card-icon-leave-to {
  opacity: 0;
  transform: scale(0.8) rotate(-8deg);
}

.card-icon-enter-to,
.card-icon-leave-from {
  opacity: 1;
  transform: scale(1) rotate(0deg);
}
</style>

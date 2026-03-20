<script setup lang="ts">
import Card from "@/component/Card.vue";
import {getBrowserUrl, inputPlatform, nowPlatform, supportPlatform} from "@/component/settings/settings.ts";
import {fetchBackend, setPlatform} from "@/pages/settings/backendSetup.ts";
import {showPopup} from "@/component/utils/screen.ts";

interface BackendScreen{
  width: number;
  height: number;
  depth: number;
}

let token: string | null;
async function login(platform: string){
  type BackendResponse = {
    url: string;
  }
  const response = await fetchBackend('/login',{
    body: JSON.stringify({
      platform: platform,
      screen: {
        width: window.screen.width,
        height: window.screen.height,
        depth: window.screen.colorDepth
      } as BackendScreen
    })
  });
  if(response.ok){
    const r: BackendResponse = await response.json() as BackendResponse;
    const url = new URL(getBrowserUrl() + r.url);
    window.open(url);
    token = url.searchParams.get("token");
    showPopup(`打开登录窗口成功`);
  }else showPopup(`获取登录链接失败`);
}

async function initBackend(){
  const response = await fetchBackend('/init',{
    body: JSON.stringify({
      token
    })
  });
  showPopup(`初始化${response.ok ? '成功' : '失败'}`);
}

async function _setPlatform(platform: string){
  inputPlatform.value = platform;
  showPopup(`设置平台${await setPlatform() ? '成功' : '失败'}`);
}
</script>

<template>
  <div class="page-shell">
    <section class="page-hero">
      <span class="page-kicker">Access</span>
      <h1 class="page-title">登录平台并同步浏览器状态</h1>
      <p class="page-desc">每个平台都会打开独立登录窗口。完成授权后，再初始化后台并把当前平台设为观看来源。</p>
    </section>
    <div class="cards">
      <Card :size="`16px`" v-for="platform in supportPlatform" :title="`${platform} 登录配置`">
        <a class="base-content" style="--base-content-size: 16px;margin-bottom: 8px">配置您的登录状态</a>
        <div class="action-row">
          <button class="btn btn-primary" @click="login(platform)">登录</button>
          <button class="btn btn-ghost" @click="initBackend">初始化后台</button>
        </div>
        <button style="display: block; margin-top: 10px" class="btn btn-primary" @click="_setPlatform(platform)">设为观看</button>
      </Card>
    </div>
  </div>
</template>

<style scoped src="@/component/utils/base-text.css"></style>
<style scoped src="@/component/utils/base-button.css"></style>
<style>
.page-shell{
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.page-hero{
  width: min(720px, 100%);
  padding: 28px 28px 26px;
  border-radius: 28px;
  background:
    linear-gradient(180deg, rgba(11,14,22,0.58), rgba(11,14,22,0.42)),
    url("/signal-ribbon.svg") center/cover no-repeat;
  box-shadow: var(--surface-shadow);
  background-blend-mode: multiply, normal;
}

.page-kicker{
  display: inline-block;
  margin-bottom: 8px;
  color: rgba(255,255,255,0.74);
  font-family: var(--mono-font), monospace;
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.page-title{
  margin: 0;
  font-size: clamp(34px, 4vw, 48px);
  line-height: 1.02;
  letter-spacing: -0.08em;
  color: #ffffff;
}

.page-desc{
  margin: 12px 0 0;
  color: rgba(255,255,255,0.86);
  font-size: 16px;
  line-height: 1.7;
}

html.dark .page-hero {
  background:
    linear-gradient(180deg, rgba(4,6,10,0.76), rgba(4,6,10,0.62)),
    url("/signal-ribbon.svg") center/cover no-repeat;
  background-blend-mode: multiply, normal;
}

.cards{
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  align-items: start;
  gap: 16px;
}

.cards :deep(.card) {
  min-width: 0;
  max-width: none;
  width: 100%;
  margin: 0;
}

.action-row{
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

@media (max-width: 640px) {
  .cards {
    grid-template-columns: 1fr;
  }
}
</style>

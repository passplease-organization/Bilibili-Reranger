import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'
import tailwindcss from '@tailwindcss/vite'
import VueRouter from 'unplugin-vue-router/vite';
import themes from './node_modules/daisyui/theme/object'
const themeNames = Object.keys(themes)

import path from 'path'
/**
 * 在全部<style scoped>块后添加全局配置：@reference "@/main.css";（否则使用Tailwindcss会无法预处理）除非有特定注释
 * */
export const autoReference = () => {
  const projectRoot = process.cwd();
  const srcRoot = path.resolve(projectRoot, 'src');
  return {
    name: 'auto-inject-main-reference',
    transform(code : string, id : string) {
      if(!id.startsWith(srcRoot))
        return null;

      if (!id.endsWith('.vue'))
        return null;

      const hasReference = /<style.*scoped.*>(?!<\/style>)@reference "@\/main\.css";/
      if (code.includes('no-reference') || hasReference.test(code))
        return null;

      const scopedStyleRegex = /<style.*scoped.*>/;
      if (scopedStyleRegex.test(code))
        return code.replace(
          scopedStyleRegex,
          (match) => `${match}\n@reference "@/main.css";`
        );
      return null;
    }
  }
}

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    autoReference(),
    vue(),
    vueDevTools(),
    tailwindcss(),
    VueRouter({
      routesFolder: ['src/website'],
      dts: 'types/typed-router.d.ts',
      extensions: ['.page.vue', '.vue'],
    })
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  server: {
    host: '0.0.0.0',
    hmr: {
      host: 'localhost',
      clientPort: 3000
    },
    watch: {
      usePolling: true
    },
    proxy: {// TODO 真的还有用吗
      '/bilibili-image': {
        target: 'https://i0.hdslb.com',
        changeOrigin: true, // 必须设置为 true，这样目标服务器才会认为请求来自同源
        rewrite: (path) => path.replace(/^\/bili-image/, ''), // 重写请求路径，去掉我们自定义的前缀
        // B站服务器可能还需要正确的 Referer
        secure: false,
        headers: {
          Referer: 'https://player.bilibili.com/'
        }
      }
    }
  },
  define: {
    'THEMES': themeNames
  }
})

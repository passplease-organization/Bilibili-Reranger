# Repository Guidelines

## Project Structure & Module Organization
This repository is a Vue 3 + TypeScript frontend built with Vite. Application code lives in `src/`, with the app entry in `src/main.ts`, the root component in `src/App.vue`, and routing in `src/router/index.ts`. Unit tests live under `src/__tests__/`. Static assets belong in `public/`. CI and code-quality automation are defined in `.github/workflows/` and `qodana.yaml`.

## Build, Test, and Development Commands
- `npm install` — install project dependencies.
- `npm run dev` — start the Vite development server.
- `npm run build` — run type checking and produce a production build.
- `npm run build-only` — build without the extra type-check step.
- `npm run type-check` — validate TypeScript and Vue types with `vue-tsc`.
- `npm run test:unit` — run unit tests with Vitest.
- `npm run preview` — serve the built app locally for a final check.

Use Node.js versions compatible with `package.json` (`^20.19.0 || >=22.12.0`).

## Coding Style & Naming Conventions
Follow the existing style: TypeScript-first, Vue Single-File Components, and 2-space indentation. Use `PascalCase` for Vue component filenames, `camelCase` for variables/functions, and descriptive names for routes and modules. Keep components small and focused. Prefer explicit imports over wildcard patterns. No formatter or linter is currently configured, so match the surrounding code carefully before submitting changes.

## Testing Guidelines
Tests use Vitest together with `@vue/test-utils` and `jsdom`. Place tests in `src/__tests__/` and name them `*.spec.ts`. Focus on user-visible behavior and component output instead of internal implementation. Run `npm run test:unit` before opening a PR; run `npm run build` as a basic integration check when changing app structure, routing, or TypeScript types.

## Security & Configuration Tips
Do not commit secrets or deployment tokens. Keep environment-specific settings out of source files, and review `qodana.yaml` and workflow changes carefully because they affect CI behavior.
## 关于项目
项目全部分支请看远端仓库，此部分只是前端部分，需要和`backend`分支一起工作，`master`分支是部署使用的，不涉及代码。项目目的是为我刷视频时从各大视频平台筛选视频，并通过这部分展现在我面前。项目面向个人使用，这个部分可能单独部署在`Vercel`或者`EdgeOne`等静态网站平台，也可能与整个docker-compose一同部署，网站为静态网站，docker-compose中会使用nginx封装向外提供服务（只暴露nginx的端口）。

当前环境是在docker开发容器中，其他辅助服务在其他容器，你看不到，配置文件在`.devcontainer/`下。
## Prompts
- 使用中文与我交流
- 除非得到我的同意，你不得直接修改我的代码，默认只是分析各种报错成因和解决方法并告诉我去操作

## Frontend Experience Notes
- 这个项目的开发态可能直接走 `Vite 3000`，不会天然经过 `.devcontainer/nginx.conf`。如果前端依赖 `/image` 这类代理路径，除了改 `nginx` 外，还要同步检查 `vite.config.ts` 的 `server.proxy`，否则会出现“生产入口可用、开发入口失效”的假象。
- 修改 `.devcontainer/nginx.conf` 时，`proxy_set_header` 里带空格的值必须加引号，例如 `User-Agent` 和 `Accept`。否则 `nginx` 会报 `invalid number of arguments in "proxy_set_header" directive`。
- Vue SFC 里只有 `scoped` 样式才应该用 `:deep(...)`。如果 `<style>` 不是 `scoped`，就直接写普通选择器；把 `:deep` 写进非 `scoped` 样式里，通常等于规则没有按预期生效。
- `Card` 组件本身带有 `min-width`、`max-width`、`margin` 和 `size` 驱动的尺寸体系。页面若要把 `Card` 放进 grid 中，必须显式覆盖这些约束，否则很容易出现卡片挤在一侧、列宽失效、看起来像“重复块”的问题。
- `src/component/settings/SettingCategory.vue` 默认 `size` 会直接影响设置页卡片的整体尺度。设置页如果看起来过小，不要只调页面 grid，先检查这里是否仍在用过小的默认值。
- SVG 图标不要把旋转 hover 直接挂在图标本体上，尤其是齿轮这种不规则形状。应把 hover 触发和 `transform` 放在固定尺寸的外层容器上，否则鼠标命中区域会随着旋转变化，表现为“抖动”或“乱动”。
- 桌面端固定侧栏时，优先让侧栏贴边，再通过内容区 `margin-left` 让出空间；不要把侧栏悬在页面中间，否则容易遮挡右侧主内容。

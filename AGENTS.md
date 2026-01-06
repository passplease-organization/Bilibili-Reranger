# Repository Guidelines

## Project Structure & Module Organization
- `src/` contains the Vue 3 application code; main UI lives under `src/website/`.
- `src/__tests__/` holds unit tests (Vitest + Vue Test Utils).
- `public/` stores static assets served by Vite.
- `api/` and `express/` contain backend or server-related code used for deployment.
- `types/` provides shared TypeScript type definitions.
- GitHub Actions workflows live in `.github/workflows/`.

## Build, Test, and Development Commands
- `npm install` installs dependencies for local development.
- `npm run dev` starts the Vite dev server.
- `npm run build` creates a production build.
- `npm run build:vercel` / `npm run build:docker` build with environment-specific modes.
- `npm exec vitest run` runs unit tests once (CI-friendly).

## Coding Style & Naming Conventions
- Language: TypeScript + Vue SFCs; prefer clear component names in `PascalCase`.
- Indentation: 2 spaces for TS/JS/JSON and Vue SFC blocks.
- Formatting: use Prettier and ESLint (configs in `eslint.config.ts`).
- Files: test files use `*.spec.ts` (example: `src/__tests__/App.spec.ts`).

## Testing Guidelines
- Frameworks: Vitest with `jsdom` environment; Vue Test Utils for component mounting.
- Naming: keep unit tests in `src/__tests__/` with `*.spec.ts` naming.
- Run tests: `npm exec vitest run`.

## Commit & Pull Request Guidelines
- No formal commit convention detected; use concise, imperative messages (e.g., "Add PR test workflow").
- PRs should include a short description, test evidence (command output or note), and screenshots for UI changes.
- Keep changes scoped and update related documentation when behavior changes.

## Security & Configuration Tips
- Node version targets: `^20.19.0 || >=22.12.0` (see `package.json`).
- Avoid committing secrets; prefer environment variables and CI secrets for deploy configs.
## Prompts
总是用中文回答我。
修改代码错误时随意阅读程序代码，但是默认解释问题而不修改。首先告诉我原因，我同意后再进行修改
# BilibiliReranger Frontend Project

## Project Overview

This is the frontend for the BilibiliReranger project, a Vue 3 application built with Vite. It appears to be a custom Bilibili interface or enhancement tool that allows users to customize their Bilibili experience. The project uses modern web technologies including Vue 3, TypeScript, Tailwind CSS, and DaisyUI for styling.

### Key Technologies & Features

- **Vue 3** with Composition API
- **TypeScript** for type safety
- **Vite** as build tool and development server
- **Tailwind CSS** for styling with **DaisyUI** for component styling
- **Pinia** for state management
- **Vue Router** with auto-routing via `unplugin-vue-router`
- **Vitest** for unit testing with JSDOM environment
- **ESLint** and **Prettier** for code quality and formatting

### Architecture

The project follows a modern frontend architecture:
- Components are organized in the `src/website` directory with auto-routing enabled
- State management is handled by Pinia
- Styles are managed with Tailwind CSS and DaisyUI
- Routing is automatically generated based on file structure
- Proxy configuration is set up to handle Bilibili image requests

## Building and Running

### Prerequisites
- Node.js version ^20.19.0 or >=22.12.0

### Setup Commands

```bash
# Install dependencies
npm install

# Compile and hot-reload for development
npm run dev

# Type-check, compile and minify for production
npm run build

# Build for Vercel deployment
npm run build:vercel

# Build for Docker deployment
npm run build:docker

# Preview production build
npm run preview

# Run unit tests with Vitest
npm run test:unit

# Lint with ESLint
npm run lint

# Debug mode with Node inspector
npm run debug
```

### Development Server Configuration

The development server is configured to:
- Host on `0.0.0.0` to allow external connections
- Use `localhost` for Hot Module Replacement (HMR)
- Include file watching with polling
- Proxy `/bilibili-image` requests to `https://i0.hdslb.com` to handle image requests with proper headers and origin settings

## Development Conventions

### File Structure
- `src/website/` - Contains Vue components with auto-routing enabled
- `src/main.ts` - Main application entry point
- `src/router/` - Router configuration (auto-generated)
- `src/main.css` - Main styling file with Tailwind CSS and DaisyUI
- `types/` - Auto-generated TypeScript type definitions for routes

### Code Quality
- TypeScript is used throughout the project for type safety
- ESLint with Vue and TypeScript configurations for linting
- Prettier for code formatting consistency
- Vitest for unit testing

### Styling
- Tailwind CSS with DaisyUI for component styling
- Themes are automatically processed and made available through the Vite configuration
- The `autoReference` plugin in `vite.config.ts` automatically injects the main CSS file into all scoped style blocks in Vue components

### Routing
- Auto-routing is enabled via `unplugin-vue-router`
- Routes are generated based on the file structure in `src/website`
- Files with `.page.vue` or `.vue` extensions in the `src/website` directory become routes

### Environment Configuration
- The project supports different build modes for Docker and Vercel deployment
- Backend inclusion can be toggled via environment variables
- Image caching behavior can be configured for browser and CDN

## Special Features

The project includes a custom Vite plugin `autoReference` that automatically injects the main CSS reference (`@reference "@/main.css";`) into all `<style scoped>` blocks in Vue components to ensure Tailwind CSS preprocessing works correctly.

The proxy configuration is specifically designed to handle Bilibili image requests, rewriting paths and including proper headers to avoid CORS issues.

## Additional Notes

- The project uses Vue DevTools plugin for enhanced development experience
- TypeScript path aliases are configured to map `@/*` to `./src/*`
- The project defines a `THEMES` global constant that includes all available DaisyUI themes
## Prompts
使用中文回答我的问题
任何情况下都不要直接更改我的代码，对于我的问题都是探讨解决方案，而不是让你修改，除非我明确告知你需要你帮我修改代码才能修改我的代码
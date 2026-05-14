# Service Frontend
[中文](README.md)

***Now this branch is almost done by Codex***

## Architecture
Built with Vue and mainly written in TypeScript. I am still learning while building it.

The site is static. Serverless functions or an auxiliary Docker container are used to handle the `Referer` header when loading images from Bilibili; otherwise those images cannot be loaded. No other server-side logic is currently required.

## Deployment
### Deploy with Docker (Recommended)
Pull the main branch and follow the `docker-compose.yml` in it.

Advantage: visitors do not need to deploy an extra backend, and no default configuration changes are needed.

### Deploy with Vercel (Deprecated)
[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new/clone?repository-url=https://github.com/passplease/Bilibili-Reranger/tree/frontend&repository-name=my-bilibili)

Uses Vercel serverless functions to load Bilibili images.

Drawback: visitors still need to deploy their own backend.

### Deploy with EdgeOne (Deprecated)
Fork the repository, then import it via the [EdgeOne deployment page](https://console.tencentcloud.com/edgeone/pages/create/git). Keep the remaining settings at their defaults.

Drawback: visitors still need to deploy their own backend.

## Usage
After opening the page, log in first, initialize the backend after logging in, and wait until initialization succeeds. Then you can enter the home page and start looking for videos. Avoid changing settings unless necessary.

For first-time use, configure all backend service URLs on the settings page (otherwise the app will not work). This is not required for docker-compose deployment.

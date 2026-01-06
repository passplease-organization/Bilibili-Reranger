# Service Frontend
[中文](README.md)

## Architecture
Built with Vue and mainly written in TypeScript.

The site is static. Serverless functions or an auxiliary Docker container are used to handle the `Referer` header when loading images from Bilibili; otherwise those images cannot be loaded. No other server-side logic is currently required.

## Deployment
### Deploy with Vercel
[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new/clone?repository-url=https://github.com/passplease/Bilibili-Reranger/tree/frontend&repository-name=my-bilibili)
Uses Vercel serverless functions to load Bilibili images.

Drawback: visitors still need to deploy their own backend.

### Deploy with EdgeOne
Fork the repository, then import it via the [EdgeOne deployment page](https://console.tencentcloud.com/edgeone/pages/create/git). Keep the remaining settings at their defaults.

Drawback: visitors still need to deploy their own backend.

### Deploy with Docker (Recommended)
Pull the main branch and follow the `docker-compose.yml` in it.

Advantage: no extra backend deployment required for visitors and no default configuration changes needed.

## Usage
For first-time use, configure all backend service URLs on the settings page (otherwise the app will not work).

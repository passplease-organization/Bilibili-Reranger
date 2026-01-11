FROM mcr.microsoft.com/playwright:v1.58.2-jammy AS base

WORKDIR /crawler

ENV NODE_ENV=production
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    tzdata \
    xvfb \
    x11vnc \
    novnc \
    websockify \
  && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
  && echo $TZ > /etc/timezone \
  && rm -rf /var/lib/apt/lists/*

FROM base AS compiler

ENV NODE_ENV=development
COPY package.json package-lock.json ./
RUN npm ci
COPY tsconfig.json ./
COPY src ./src
RUN npm run build
RUN npm prune --omit=dev

FROM base AS runtime

ENV NODE_ENV=production

WORKDIR /crawler

COPY --from=compiler /crawler/package.json ./package.json
COPY --from=compiler /crawler/package-lock.json ./package-lock.json
COPY --from=compiler /crawler/dist ./
RUN npm install

EXPOSE 3000 6080

CMD ["node","/crawler/server.js"]

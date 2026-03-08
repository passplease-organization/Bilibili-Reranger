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

COPY . .
RUN npm install && npm install -g typescript @types/node
RUN npm run build

FROM base

EXPOSE 3000 6080

COPY --from compiler /crawler/dist ./

CMD ["node", "/crawler/dist/server.js"]

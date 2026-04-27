FROM mcr.microsoft.com/playwright:v1.58.2-jammy

WORKDIR /crawler

ENV NODE_ENV=development
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai
ENV BROWSER_DEBUG=1
ENV BROWSER_DEBUG_HEADLESS=0
ENV BROWSER_DEBUG_KEEP_OPEN=1
ENV BROWSER_DEBUG_ARTIFACTS_DIR=./logs

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

RUN npm install -g @playwright/cli
RUN npm install -g typescript @types/node @google/gemini-cli @openai/codex @qwen-code/qwen-code@latest

RUN mkdir -p /crawler/logs && chmod 777 /crawler/logs

EXPOSE 3000 6080

CMD ["sh", "-c", "npm install && xvfb-run -a npm run dev"]

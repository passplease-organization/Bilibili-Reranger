FROM mcr.microsoft.com/playwright:v1.42.1-jammy

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

RUN npm install -g typescript @types/node @google/gemini-cli @openai/codex @qwen-code/qwen-code@latest

EXPOSE 3000 6080

CMD ["bash", "./start.sh"]

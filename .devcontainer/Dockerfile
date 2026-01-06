FROM node:24

WORKDIR /bilibili

EXPOSE 5173
EXPOSE 9229
EXPOSE 3000

RUN npm install -g @openai/codex
RUN npm install -g @qwen-code/qwen-code@latest
RUN npm install -g npm-check-updates

RUN apt update && apt install curl
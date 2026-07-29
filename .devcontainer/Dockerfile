FROM node:24

WORKDIR /bilibili

EXPOSE 5173
EXPOSE 9229
EXPOSE 3000

RUN npm install -g @openai/codex @google/gemini-cli @anthropic-ai/claude-code
RUN npm install -g @playwright/cli && npx playwright install chrome && npx playwright install-deps
RUN npm install -g npm-check-updates

RUN apt update && apt install -y curl
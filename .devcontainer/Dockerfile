FROM ghcr.io/passplease-organization/bilibili_reranger:plugin_dev

RUN apt-get update && apt-get install -y curl \
    && curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && npm install -g @google/gemini-cli \
    && npm install -g @openai/codex \
    && npm install -g @anthropic-ai/claude-code \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN npm install -g @playwright/cli && npx playwright install chrome && npx playwright install-deps

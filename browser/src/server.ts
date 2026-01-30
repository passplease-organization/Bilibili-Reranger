import Fastify from 'fastify';
import { chromium, Page, BrowserContext } from 'playwright';
import type { StorageState } from 'playwright';

const fastify = Fastify({ logger: true });

type TaskStep = {
  op: string;
  url?: string;
  waitUntil?: 'load' | 'domcontentloaded' | 'networkidle' | 'commit';
  selector?: string;
  timeout?: number;
  text?: string;
  key?: string;
  name?: string;
  script?: string;
  saveAs?: string;
};

type ContextEntry = {
  context: BrowserContext;
  page: Page;
};

const contexts = new Map<string, ContextEntry>();
let browserPromise: ReturnType<typeof chromium.launch> | null = null;

const API_PORT = Number.parseInt(process.env.BM_API_PORT || '3001', 10);
const VIEWER_PATH = process.env.BM_VIEWER_PATH || '/browser/vnc.html';

async function ensureBrowser() {
  if (!browserPromise) {
    browserPromise = chromium.launch({
      headless: false,
      args: ['--no-sandbox', '--disable-dev-shm-usage', '--disable-gpu']
    });
  }
  return browserPromise;
}

async function getContext(clientId: string, storageState: StorageState | string | undefined, force: boolean) {
  const existing = contexts.get(clientId);
  if (existing && force) {
    try {
      await existing.context.close();
    } catch (err) {
      fastify.log.warn({ err, clientId }, 'failed to close old context');
    }
    contexts.delete(clientId);
  }

  const cached = contexts.get(clientId);
  if (cached) {
    return cached;
  }

  const browser = await ensureBrowser();
  const normalizedState = storageState as StorageState | string | undefined;
  const context = normalizedState
    ? await browser.newContext({ storageState: normalizedState })
    : await browser.newContext();
  const page = await context.newPage();
  const entry = { context, page };
  contexts.set(clientId, entry);
  return entry;
}

async function runTask(page: Page, task: TaskStep[]) {
  const result: Record<string, unknown> = {};
  let lastValue: unknown = null;

  for (const step of task || []) {
    switch (step.op) {
      case 'goto':
        lastValue = await page.goto(step.url || '', {
          waitUntil: step.waitUntil || 'domcontentloaded'
        });
        break;
      case 'wait':
        await page.waitForSelector(step.selector || '', {
          timeout: step.timeout || 30000
        });
        lastValue = true;
        break;
      case 'wait_timeout':
        await page.waitForTimeout(step.timeout || 1000);
        lastValue = true;
        break;
      case 'click':
        await page.click(step.selector || '');
        lastValue = true;
        break;
      case 'type':
        await page.fill(step.selector || '', step.text || '');
        lastValue = true;
        break;
      case 'press':
        await page.press(step.selector || '', step.key || 'Enter');
        lastValue = true;
        break;
      case 'text':
        lastValue = await page.textContent(step.selector || '');
        break;
      case 'attr':
        lastValue = await page.getAttribute(step.selector || '', step.name || '');
        break;
      case 'eval':
        lastValue = await page.evaluate((script) => {
          // eslint-disable-next-line no-eval
          return eval(script);
        }, step.script || 'null');
        break;
      default:
        throw new Error(`unsupported op: ${step.op}`);
    }

    if (step.saveAs) {
      result[step.saveAs] = lastValue;
    }
  }

  return { result, lastValue };
}

fastify.get('/health', async () => ({ ok: true }));

fastify.post('/context/restore', async (request, reply) => {
  const body = (request.body || {}) as Record<string, unknown>;
  const clientId = body.client_id as string | undefined;
  if (!clientId) {
    reply.code(400);
    return { ok: false, error: 'client_id required' };
  }

  await getContext(clientId, body.storage_state as StorageState | string | undefined, true);
  return { ok: true, context_id: clientId };
});

fastify.post('/context/save', async (request, reply) => {
  const body = (request.body || {}) as Record<string, unknown>;
  const clientId = body.client_id as string | undefined;
  const entry = clientId ? contexts.get(clientId) : undefined;
  if (!entry) {
    reply.code(404);
    return { ok: false, error: 'context not found' };
  }

  const storageState = await entry.context.storageState();
  return { ok: true, storage_state: storageState };
});

fastify.post('/task/run', async (request, reply) => {
  const body = (request.body || {}) as Record<string, unknown>;
  const clientId = body.client_id as string | undefined;
  const task = (body.task || []) as TaskStep[];

  if (!clientId) {
    reply.code(400);
    return { ok: false, error: 'client_id required' };
  }

  const entry = await getContext(clientId, body.storage_state as StorageState | string | undefined, false);
  try {
    const output = await runTask(entry.page, task);
    return { ok: true, result: output.result, last_value: output.lastValue };
  } catch (err) {
    fastify.log.error({ err, clientId }, 'task failed');
    reply.code(500);
    return { ok: false, error: (err as Error).message };
  }
});

fastify.post('/captcha/open', async () => {
  return { ok: true, viewer_path: VIEWER_PATH };
});

fastify.post('/captcha/continue', async () => {
  return { ok: true };
});

fastify.listen({ port: API_PORT, host: '0.0.0.0' })
  .catch((err) => {
    fastify.log.error(err);
    process.exit(1);
  });
console.log("运行")
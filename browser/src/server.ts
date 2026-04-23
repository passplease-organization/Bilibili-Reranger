import fs from "node:fs";
import path from "node:path";
import {randomUUID} from "node:crypto";
import {Browser, BrowserContext, chromium, Page} from 'playwright';
import {closeServerPlugins, fastify, initServerPlugins, Worker} from "./PluginAPI";
import serverInit from "./init";
import {login} from "./login";
import LoginRequest = login.LoginRequest;
import LoginResponse = login.LoginResponse;
import {
    API_PORT,
    BROWSER_DEBUG_ARTIFACTS_DIR,
    BROWSER_DEBUG_ENABLED,
    BROWSER_DEBUG_HEADLESS,
    BROWSER_DEBUG_HTML_LIMIT,
    BROWSER_DEBUG_KEEP_OPEN
} from "./env";
import {DisplaySession, ensureDisplayForHeadedLaunch} from "./debug/xvfb";
import buildNginxURL = login.buildNginxURL;
import collectContext = login.collectContext;
import getSession = login.getSession;
import CollectRequest = login.CollectRequest;
import CollectResponse = login.CollectResponse;
import closeAll = login.closeAll;
import {ChineseLogger, getCurrentLogger, logger, runWithLogger} from "./logger";

export interface WorkContext {
    cookie: {
        value: string,
        domain: string,
        path?: string
    };
}

export namespace WorkContext{
    export function toCookie(context: WorkContext){
        return context.cookie.value
            .split(";")
            .map(s => s.trim())
            .filter(Boolean)
            .map(pair => {
                const eq = pair.indexOf("=");
                const name = eq >= 0 ? pair.slice(0, eq).trim() : pair.trim();
                const value = eq >= 0 ? pair.slice(eq + 1).trim() : "";
                return {
                    name,
                    value,
                    domain: context.cookie.domain,
                    path: context.cookie.path || "/"
                };
            });
    }
}
export interface WorkResult{}
export interface WorkerDescription{
    type: string;
    info: unknown;
}
export interface DebugSnapshot {
    url?: string;
    title?: string;
    screenshot?: string;
    html?: string;
    htmlPreview?: string;
    records?: {url: string, handled: boolean}[];
    workerType?: string;
    workerIndex?: number;
    tag?: string;
    context: BrowserContext;
}
type NetRecord = {
    url: string;
    body: string;
    headers: Record<string, string>;
};
export class Handler{
    protected browse: Browser;
    private readonly displaySession: DisplaySession;
    private readonly clientID: string;
    private readonly platform: string;
    public context: BrowserContext;
    public page: Page;
    public records: Map<{ url:string, handled: boolean }, NetRecord> = new Map();
    private currentWorkerType?: string;
    private currentWorkerIndex?: number;

    constructor(browser: Browser, displaySession: DisplaySession, clientID: string, platform: string) {
        this.browse = browser;
        this.displaySession = displaySession;
        this.clientID = clientID;
        this.platform = platform;
        this.record = this.record.bind(this);
    }

    public async init(context: WorkContext) : Promise<boolean>{
        this.records.clear();
        this.logger.info({cookieCount: WorkContext.toCookie(context).length}, "初始化浏览器上下文");
        this.context = await this.browse.newContext();
        await this.context.addCookies(WorkContext.toCookie(context));
        return true;
    }

    public get logger(): ChineseLogger {
        return getCurrentLogger();
    }

    protected async record(response: import("playwright").Response): Promise<void>{
        if(response.ok()){
            let body: string;
            try{
                body = await response.text();
            }catch(e){
                return;
            }
            this.records.set({url: response.url(), handled: false},{
                url: response.url(),
                body,
                headers: response.headers()
            });
            this.logger.debug({url: response.url(), statusCode: response.status()}, "记录页面响应");
        }
    }

    public async newPage(): Promise<Page> {
        this.records.clear();
        if(this.page) {
            this.logger.debug({url: this.page.url()}, "关闭旧页面");
            await this.page.close();
        }
        this.page = await this.context.newPage();
        this.page.on("close", () => this.records.clear());
        this.page.on("response",this.record);
        this.logger.debug("创建新页面并开始记录响应");
        return this.page;
    }

    public stopRecord(): void{
        this.page.off("response",this.record);
        this.logger.debug("停止记录页面响应");
    }

    public setCurrentWorker(type: string, index: number): void {
        this.currentWorkerType = type;
        this.currentWorkerIndex = index;
    }

    public async collectDebugArtifacts(tag: string): Promise<DebugSnapshot | undefined> {
        if (!BROWSER_DEBUG_ENABLED || !this.page) {
            return undefined;
        }

        fs.mkdirSync(BROWSER_DEBUG_ARTIFACTS_DIR, {recursive: true});
        const stamp = new Date().toISOString().replace(/[:.]/g, "-");
        const requestId = String(this.logger.getBinding("requestId") || "no-request-id");
        const workerType = this.currentWorkerType || "UnknownWorker";
        const workerIndex = this.currentWorkerIndex ?? -1;
        const workerOrder = workerIndex >= 0 ? String(workerIndex + 1).padStart(2, "0") : "00";
        const prefix = `${stamp}-${this.safePart(requestId)}-${this.safePart(this.platform)}-${this.safePart(workerType)}-${workerOrder}-${this.safePart(tag)}`;
        const screenshot = path.join(BROWSER_DEBUG_ARTIFACTS_DIR, `${prefix}.png`);
        const html = path.join(BROWSER_DEBUG_ARTIFACTS_DIR, `${prefix}.html`);
        const back: DebugSnapshot = {
            records: Array.from(this.records.keys()).slice(-10),
            workerType,
            workerIndex,
            tag,
            context: this.context
        };

        try {
            back.url = this.page.url();
        } catch {}
        try {
            const cookies = await this.context.cookies(back.url);
            this.logger.warn({url: back.url, cookieCount: cookies.length}, "收集浏览器调试信息");
        } catch {
            this.logger.warn({url: back.url}, "收集浏览器调试信息");
        }
        try {
            back.title = await this.page.title();
        } catch {}
        try {
            await this.page.screenshot({path: screenshot, fullPage: true, timeout: 5000});
            back.screenshot = screenshot;
        } catch {}
        try {
            const content = await this.page.content();
            fs.writeFileSync(html, content, "utf8");
            back.html = html;
            back.htmlPreview = content.slice(0, BROWSER_DEBUG_HTML_LIMIT);
        } catch {}
        this.logger.warn({screenshot: back.screenshot, html: back.html}, "浏览器调试文件已保存");

        return back;
    }

    public async close(): Promise<void> {
        this.logger.info({clientID: this.clientID, platform: this.platform}, "关闭浏览器处理器");
        await this.browse.close().catch(() => undefined);
        await this.displaySession.close().catch(() => undefined);
    }

    private safePart(value: string): string {
        return value.replace(/[^a-zA-Z0-9_-]+/g, "_").slice(0, 40) || "unknown";
    }
}

export interface RequestBody {
    clientID: string;
    platform: string;
    context: WorkContext;
    workers?: WorkerDescription[];
    mode?: string;
    requestID?: string;
    timeout?: number;
}
export interface BackBody {
    ok: boolean;
    requestID?: string;
    error?: Error;
    debug?: DebugSnapshot;
    back: (WorkResult | WorkResult[])[]
}

type HandlerMap = Record<string, Record<string, Handler>>;
async function closeHandler(clientID: string,platform: string): Promise<void>{
    if(Boolean(handlers[clientID]?.[platform])){
        await handlers[clientID][platform].close();
        delete handlers[clientID][platform];
        if (Object.keys(handlers[clientID]).length === 0) {
            delete handlers[clientID];
        }
    }
}

async function getHandler(clientID: string,platform: string): Promise<Handler>{
    if(Boolean(handlers[clientID]?.[platform])){
        return handlers[clientID][platform];
    }
    if(!Boolean(handlers[clientID]))
        handlers[clientID] = {};
    const displaySession = await ensureDisplayForHeadedLaunch(BROWSER_DEBUG_HEADLESS);
    const handler : Handler = new Handler(await chromium.launch({
        headless: BROWSER_DEBUG_HEADLESS,
        env: displaySession.env
    }), displaySession, clientID, platform)
    handlers[clientID][platform] = handler;
    return handler;
}

const handlers: HandlerMap = {};

function getRequestID(request: {headers: Record<string, unknown>}, body?: RequestBody): string {
    const header = request.headers["x-request-id"];
    if (typeof header === "string" && header.trim()) return header.trim();
    if (Array.isArray(header) && typeof header[0] === "string" && header[0].trim()) return header[0].trim();
    if (typeof body?.requestID === "string" && body.requestID.trim()) return body.requestID.trim();
    return randomUUID();
}

function errorBody(error: unknown): {name: string; message: string; stack?: string} {
    if (error instanceof Error) {
        return {
            name: error.name,
            message: error.message,
            stack: error.stack
        };
    }
    return {
        name: "Unknown Error",
        message: String(error)
    };
}

function getRequestTimeoutSeconds(body?: RequestBody): number | undefined {
    if (typeof body?.timeout !== "number" || !Number.isFinite(body.timeout) || body.timeout <= 0) {
        return undefined;
    }
    return body.timeout;
}

const timeoutResult = Symbol("timeoutResult");

async function withDeadline<T>(promise: Promise<T>, deadlineMs: number | undefined): Promise<T | typeof timeoutResult> {
    if (deadlineMs === undefined) return promise;
    const remainingMs = deadlineMs - Date.now();
    if (remainingMs <= 0) {
        promise.catch(() => undefined);
        return timeoutResult;
    }
    let timer: NodeJS.Timeout | undefined;
    let timedOut = false;
    try {
        const result = await Promise.race([
            promise,
            new Promise<typeof timeoutResult>((resolve) => {
                timer = setTimeout(() => {
                    timedOut = true;
                    resolve(timeoutResult);
                }, remainingMs);
            })
        ]);
        if (result === timeoutResult) {
            promise.catch(() => undefined);
        }
        return result;
    } finally {
        if (timer && !timedOut) clearTimeout(timer);
    }
}

fastify.addHook("onClose", async () => {
    await Promise.all([
        closeAll(),
        ...Object.values(handlers).flatMap((platformHandlers) =>
            Object.values(platformHandlers).map((handler) => handler.close())
        )
    ]);
});

serverInit()
initServerPlugins();

fastify.get('/test',() => ({ok: true}));
fastify.post('/',async (request) => {
    let body: RequestBody | undefined;
    let requestID: string = randomUUID();
    const startedAt = Date.now();
    let requestLogger = logger.child({requestId: requestID});
    try{
        body = request.body as RequestBody;
        requestID = getRequestID(request, body);
        requestLogger = logger.child({
            requestId: requestID,
            clientID: body.clientID,
            platform: body.platform
        });
        const timeoutSeconds = getRequestTimeoutSeconds(body);
        const deadlineMs = timeoutSeconds === undefined ? undefined : startedAt + timeoutSeconds * 1000;
        return await runWithLogger(requestLogger, async () => {
            requestLogger.info({workerCount: body.workers?.length || 0, timeoutSeconds}, "爬取请求开始");
            if(!body.workers) {
                requestLogger.warn("爬取请求缺少工作项");
                return {
                    ok: false,
                    requestID,
                    error: {
                        name: "Invalid Request",
                        message: "workers are empty or invalid"
                    },
                    back: []
                } as BackBody;
            }
            const handler = await getHandler(body.clientID,body.platform);
            const back: BackBody = {back: [], ok: true, requestID};
            const initResult = await withDeadline(handler.init(body.context), deadlineMs);
            if (initResult === timeoutResult) {
                requestLogger.warn({timeoutSeconds, durationMs: Date.now() - startedAt}, "爬取请求超过限定时长，直接返回已有结果");
                void closeHandler(body.clientID, body.platform);
                return back;
            }
            if(!initResult) {
                requestLogger.warn("初始化浏览器上下文失败");
                return {
                    ok: false,
                    requestID,
                    error: {
                        name: "Invalid Request",
                        message: "context is invalid"
                    },
                    back: []
                } as BackBody;
            }
            for(let index = 0; index < body.workers.length; index++) {
                if (deadlineMs !== undefined && Date.now() >= deadlineMs) {
                    requestLogger.warn({timeoutSeconds, durationMs: Date.now() - startedAt, completedWorkerCount: back.back.length}, "爬取请求超过限定时长，直接返回已有结果");
                    return back;
                }
                const description = body.workers[index];
                const workerStartedAt = Date.now();
                handler.setCurrentWorker(description.type, index);
                requestLogger.info({index: index + 1, type: description.type}, "工作项开始执行");
                try {
                    const workerResult = await withDeadline(Worker.fromDescription(description).work(handler), deadlineMs);
                    if (workerResult === timeoutResult) {
                        requestLogger.warn({timeoutSeconds, durationMs: Date.now() - startedAt, completedWorkerCount: back.back.length}, "爬取请求超过限定时长，直接返回已有结果");
                        void closeHandler(body.clientID, body.platform);
                        return back;
                    }
                    back.back.push(workerResult);
                    requestLogger.info({index: index + 1, type: description.type, durationMs: Date.now() - workerStartedAt}, "工作项执行完成");
                } catch (error) {
                    requestLogger.error({index: index + 1, type: description.type, error}, "工作项执行失败");
                    throw error;
                }
            }
            requestLogger.info({durationMs: Date.now() - startedAt}, "爬取请求完成");
            return back;
        });
    }catch(e){
        return await runWithLogger(requestLogger, async () => {
            requestLogger.error({error: e}, "爬取请求失败");
            let debug: DebugSnapshot | undefined;
            if (body?.clientID && body?.platform && handlers[body.clientID]?.[body.platform]) {
                debug = await handlers[body.clientID][body.platform].collectDebugArtifacts("request-failed");
            }
            if (!(BROWSER_DEBUG_ENABLED && BROWSER_DEBUG_KEEP_OPEN) && body?.clientID && body?.platform) {
                await closeHandler(body.clientID, body.platform);
            }
            return {
                ok: false,
                requestID,
                error: errorBody(e),
                debug,
                back: []
            } as BackBody;
        });
    }
});
fastify.post('/other/closeWorker', async (request) => {
    try{
        const body = request.body as RequestBody;
        logger.info({clientID: body.clientID, platform: body.platform}, "收到关闭工作项请求");
        if(body.mode && body.mode != 'closeWorker')
            return {
                ok:false,
                error:{
                    name: "Wrong request mode",
                    message: `request mode is closeWorker, but received ${body.mode}`
                },
                back:[]
            } as BackBody;
        await closeHandler(body.clientID,body.platform);
        return {
            ok: !Boolean(handlers[body.clientID]?.[body.platform]),
            back: []
        } as BackBody
    }catch (e) {
        logger.error({error: e}, "关闭工作项请求失败");
        return {
            ok: false,
            error: errorBody(e),
            back: []
        } as BackBody;
    }
});
fastify.post('/other/testContext',(request,reply) => {
    reply.redirect('/');
    const body = request.body as RequestBody;
    logger.info({clientID: body.clientID, platform: body.platform}, "检测登录凭据");
});
fastify.post('/login',(request) => {
    try {
        logger.info("收到登录请求");
        return login.login(request.body as LoginRequest);
    }catch(e){
        logger.error({error: e}, "登录请求失败");
        return {
            ok: false,
            error: errorBody(e),
            back: [],
            login: ''
        }as LoginResponse;
    }
});
fastify.get('/screen',(request,reply) => {
    try{
        const url = new URL(request.url,`http://${request.host}`);
        reply.header("X-Accel-Redirect",encodeURI(buildNginxURL(url.searchParams.get('token'),url.searchParams.get('session')))).send();
    }catch(e){
        logger.error({error: e}, "访问登录屏幕失败");
        reply.code(500).send();
    }
})
fastify.post('/other/login/backend',(request) => {
    try {
        const body = request.body as CollectRequest;
        logger.info({clientID: body.clientID, platform: body.platform}, "后端收集登录信息");
        return collectContext(body.token,getSession(body.clientID,body.platform))
    }catch(e){
        logger.error({error: e}, "后端收集登录信息失败");
        return {
            ok: false,
            error: errorBody(e),
            back: [],
            context: {
                cookie: {
                    value: '',
                    domain: '',
                    path: '/'
                }
            }
        } as CollectResponse
    }
})

if (process.env.BROWSER_SKIP_LISTEN !== "1") {
    fastify.listen({port: API_PORT, host: '0.0.0.0'})
        .catch(async (err) => {
            await closeServerPlugins();
            logger.error({error: err}, "浏览器服务启动失败");
            process.exit(1);
        });
}

logger.info({port: API_PORT}, "浏览器主程序已启动");

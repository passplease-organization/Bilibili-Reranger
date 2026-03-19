import fs from "node:fs";
import path from "node:path";
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

export interface WorkContext {
    cookie: string;
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
    records?: string[];
    workerType?: string;
    workerIndex?: number;
    tag?: string;
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
    public records: Map<string, NetRecord> = new Map();
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
        this.context = await this.browse.newContext();
        const cookies = context.cookie
            .split(";")
            .map(s => s.trim())
            .filter(Boolean)
            .map(pair => {
                const eq = pair.indexOf("=");
                const name = eq >= 0 ? pair.slice(0, eq).trim() : pair.trim();
                const value = eq >= 0 ? pair.slice(eq + 1).trim() : "";
                return { name, value };
            });
        await this.context.addCookies(cookies);
        return true;
    }

    protected async record(response: import("playwright").Response): Promise<void>{
        if(response.ok()){
            this.records.set(response.url(),{
                url: response.url(),
                body: await response.text(),
                headers: response.headers()
            });
        }
    }

    public async newPage(options?: any): Promise<Page> {
        this.records.clear();
        this.page = await this.browse.newPage(options);
        this.page.on("close", () => this.records.clear());
        this.page.on("response",this.record);
        return this.page;
    }

    public stopRecord(): void{
        this.page.off("response",this.record);
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
        const workerType = this.currentWorkerType || "UnknownWorker";
        const workerIndex = this.currentWorkerIndex ?? -1;
        const workerOrder = workerIndex >= 0 ? String(workerIndex + 1).padStart(2, "0") : "00";
        const prefix = `${stamp}-${this.safePart(this.platform)}-${this.safePart(workerType)}-${workerOrder}-${this.safePart(tag)}`;
        const screenshot = path.join(BROWSER_DEBUG_ARTIFACTS_DIR, `${prefix}.png`);
        const html = path.join(BROWSER_DEBUG_ARTIFACTS_DIR, `${prefix}.html`);
        const back: DebugSnapshot = {
            records: Array.from(this.records.keys()).slice(-10),
            workerType,
            workerIndex,
            tag
        };

        try {
            back.url = this.page.url();
        } catch {}
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

        return back;
    }

    public async close(): Promise<void> {
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
}
export interface BackBody {
    ok: boolean;
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
    try{
        body = request.body as RequestBody;
        console.log(`启动爬取，平台：${body.platform}`)
        if(!body.workers) {
            console.warn("无Workers !");
            return {
                ok: false,
                error: {
                    name: "Invalid Request",
                    message: "workers are empty or invalid"
                },
                back: []
            } as BackBody;
        }
        const handler = await getHandler(body.clientID,body.platform);
        if(!await handler.init(body.context)) {
            console.warn("初始化Context失败！");
            return {
                ok: false,
                error: {
                    name: "Invalid Request",
                    message: "context is invalid"
                },
                back: []
            } as BackBody;
        }
        const back: BackBody = {back: [], ok: true};
        for(let index = 0; index < body.workers.length; index++) {
            const description = body.workers[index];
            handler.setCurrentWorker(description.type, index);
            back.back.push(await Worker.fromDescription(description).work(handler));
        }
        return back;
    }catch(e){
        console.error(`Working Error:${e}`);
        let debug: DebugSnapshot | undefined;
        if (body?.clientID && body?.platform && handlers[body.clientID]?.[body.platform]) {
            debug = await handlers[body.clientID][body.platform].collectDebugArtifacts("request-failed");
        }
        if (!(BROWSER_DEBUG_ENABLED && BROWSER_DEBUG_KEEP_OPEN) && body?.clientID && body?.platform) {
            await closeHandler(body.clientID, body.platform);
        }
        return {
            ok: false,
            error: {
                name: e.name,
                message: e.message,
                stack: e.stack,
            },
            debug,
            back: []
        } as BackBody;
    }
});
fastify.post('/other/closeWorker', async (request) => {
    try{
        const body = request.body as RequestBody;
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
        console.error(`CloseWorker Error:${e}`);
        return {
            ok: false,
            error: {
                name: e.name,
                message: e.message,
                stack: e.stack,
            },
            back: []
        } as BackBody;
    }
});
fastify.post('/other/testContext',(request) => ({ok:false}));
fastify.post('/login',(request) => {
    try {
        return login.login(request.body as LoginRequest);
    }catch(e){
        console.error(`Login Error:${e}`);
        return {
            ok: false,
            error: {
                name: e.name,
                message: e.message,
                stack: e.stack,
            },
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
        console.error(`访问登录屏幕错误:${e}`);
        reply.code(500).send();
    }
})
fastify.post('/other/login/backend',(request) => {
    try {
        const body = request.body as CollectRequest;
        return collectContext(getSession(body.clientID,body.platform),body.token)
    }catch(e){
        console.error(`backend收集数据错误:\n${e}`)
        return {
            ok: false,
            error: {
                name: e.name,
                message: e.message,
                stack: e.stack,
            },
            back: [],
            context: {
                cookie: ''
            }
        } as CollectResponse
    }
})

if (process.env.BROWSER_SKIP_LISTEN !== "1") {
    fastify.listen({port: API_PORT, host: '0.0.0.0'})
        .catch(async (err) => {
            await closeServerPlugins();
            fastify.log.error(err);
            process.exit(1);
        });
}

console.log('主程序已启动');

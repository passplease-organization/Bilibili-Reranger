import Fastify from 'fastify';
import {Browser, BrowserContext, chromium, Page} from 'playwright';
import serverInit from "./init";

const fastify = Fastify({ logger: true });

export interface WorkContext {
    cookie: string;
    user_agent: string;
}
export interface WorkResult{}
export interface WorkerDescription{
    type: string;
    info: unknown;
}
export class Handler{
    public browse: Browser;
    public context: BrowserContext;
    public page: Page;

    constructor(browser: Browser) {
        this.browse = browser;
    }

    public async init(context: WorkContext) : Promise<boolean>{
        this.context = await this.browse.newContext({
            userAgent: context.user_agent
        });
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
}
declare interface WorkerFactory{
    getWorker: (info: unknown) => Worker;
}
const registeredWorker : Record<string,WorkerFactory> = {}
export function registerWorker(name: string, workerFactory:WorkerFactory): boolean{
    if(registeredWorker[name])
        return false;
    registeredWorker[name] = workerFactory;
    return true;
}

export abstract class Worker{
    protected constructor(info: unknown){}

    public abstract work(handler : Handler): Promise<WorkResult>;

    public static fromDescription(description: WorkerDescription) :Worker{
        if(!registeredWorker[description.type])
            throw new Error(`${description.type} is not registered`);
        return registeredWorker[description.type].getWorker(description.info);
    }
}

export interface RequestBody {
    clientID: string;
    platform: string;
    context: WorkContext;
    workers?: WorkerDescription[];
    mode?: string;
}
type BackBody = {
    ok: boolean;
    error?: Error;
    back:[
        WorkResult | WorkResult[]
    ] | []
}

type HandlerMap = Record<string, Record<string, Handler>>;
function closeHandler(clientID: string,platform: string): void{
    if(Boolean(handlers[clientID]?.[platform])){
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
    const handler : Handler = new Handler(await chromium.launch())
    handlers[clientID][platform] = handler;
    return handler;
}

const handlers: HandlerMap = {};

serverInit()
fastify.get('/test',() => ({ok: true}));
fastify.post('/',async (request) => {
    const body = request.body as RequestBody;
    if(!body.workers)
        return {
            ok: false,
            error: {
                name: "Invalid Request",
                message: "workers are empty or invalid"
            },
            back: []
        } as BackBody;
    const handler = await getHandler(body.clientID,body.platform);
    if(!await handler.init(body.context))
        return {
            ok: false,
            error: {
                name: "Invalid Request",
                message: "context is invalid"
            },
            back: []
        } as BackBody;
    for(const description of body.workers) {
        Worker.fromDescription(description).work(handler)
    }
});
fastify.post('/other/closeWorker',(request) => {
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
    closeHandler(body.clientID,body.platform);
    return {
        ok: !Boolean(handlers[body.clientID]?.[body.platform]),
        back: []
    } as BackBody
});
fastify.post('/other/testContext',(request) => ({ok:false}));
fastify.post('/other/login',(request) => ({ok:false}));

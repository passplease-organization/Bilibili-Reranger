import {BackBody, RequestBody, WorkContext} from "./server";
import {Browser, chromium} from "playwright";
import {ChildProcessWithoutNullStreams, spawn} from "child_process";
import {LOGIN_IDLE_SECONDS, MAX_LOGIN_PORT, START_SERVICE_WAITING_TIME} from "./env";
import * as net from "node:net";
import fs from "node:fs";

export namespace login{
    type ScreenSize = {
        width: number,
        height: number,
        depth: number
    }
    export interface LoginRequest extends RequestBody{
        platform_url: string,
        screen: ScreenSize,
    }

    export interface LoginResponse extends BackBody{
        login: string
    }

    export function getSession(clientID: string,platform: string): string{
        return `${clientID}:${platform}`
    }

    type xvfb = {
        xvfb: ChildProcessWithoutNullStreams,
        display: number
    }
    type web = {
        websockify: ChildProcessWithoutNullStreams,
        port: number
    }
    type vnc = {
        vnc: ChildProcessWithoutNullStreams,
        port: number
    }
    export interface LoginSession{
        url: string,
        browser: Browser;
        token: string;
        createdAt: number;
        Xvfb: xvfb;
        Websockify: web;
        Vnc: vnc;
    }

    export function randomToken(len: number = 32): string {
        const chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        let out = "";
        for (let i = 0; i < len; i++) {
            out += chars[Math.floor(Math.random() * chars.length)];
        }
        return out;
    }

    const loginSessions: Map<string, LoginSession> = new Map();

    export async function login(body: LoginRequest): Promise<LoginResponse>{
        const session = getSession(body.clientID, body.platform);
        if(loginSessions.has(session))
            return {
                ok: true,
                back: [],
                login: buildVisitURL(loginSessions.get(session).token,session)
            };
        const token = randomToken();
        let Xvfb: xvfb | null = null;
        let Vnc: vnc | null = null;
        let Web: web | null = null;
        let browser: Browser | null = null;
        try{
            Xvfb = await startXvfb(body.screen);
            Vnc = await startVnc(Xvfb.display);
            Web = await startWebsockify(async () => await close(session),Vnc);
            browser = await chromium.launch({
                headless: false,
                env: {
                    DISPLAY: `:${Xvfb.display}`
                }
            });
            const context = await browser.newContext();
            await context.addCookies(WorkContext.toCookie(body.context));
            await context.newPage().then(page => page.goto(body.platform_url,{waitUntil: "domcontentloaded"}));
        }catch (e) {
            if(browser)
                await browser.close();
            if(Xvfb && !Xvfb.xvfb.killed)
                Xvfb.xvfb.kill("SIGKILL");
            if(Vnc && !Vnc.vnc.killed)
                Vnc.vnc.kill("SIGKILL");
            if(Web && !Web.websockify.killed)
                Web.websockify.kill("SIGKILL");
            throw e;
        }
        loginSessions.set(session, {
            url: body.platform_url,
            browser,
            token,
            createdAt: Date.now(),
            Xvfb,
            Vnc,
            Websockify: Web
        });
        return {
            ok: true,
            back: [],
            login: buildVisitURL(token,session)
        }
    }

    function buildVisitURL(token: string,session: string): string {
        return `/screen?token=${token}&session=${session}`;
    }

    export function buildNginxURL(token: string,session: string): string{
        if(loginSessions.has(session)){
            const login = loginSessions.get(session);
            if(login.token == token) {
                return `/nginx/browser_screen/${login.Websockify.port}/vnc.html?autoconnect=1&resize=remote&reconnect=1`
            }
        }
        throw new Error("错误Token和Session参数！")
    }

    function cleanupStaleXLock(display: number): boolean {
        const lockFile = `/tmp/.X${display}-lock`;
        const socketFile = `/tmp/.X11-unix/X${display}`;

        if (!fs.existsSync(lockFile)) {
            return true;
        }

        try {
            const content = fs.readFileSync(lockFile, "utf8").trim();
            const pid = Number.parseInt(content, 10);

            if (!Number.isFinite(pid)) {
                fs.rmSync(lockFile, { force: true });
                fs.rmSync(socketFile, { force: true });
                return true;
            }

            try {
                process.kill(pid, 0);
                return false;
            } catch (err: any) {
                if (err?.code === "ESRCH") {
                    fs.rmSync(lockFile, { force: true });
                    fs.rmSync(socketFile, { force: true });
                    return true;
                }
                throw err;
            }
        } catch {
            fs.rmSync(lockFile, { force: true });
            fs.rmSync(socketFile, { force: true });
            return true;
        }
    }

    function startXvfb(size: ScreenSize): Promise<xvfb>{
        const allDisplay = Array.from(loginSessions.values()).map(session => session.Xvfb.display)
        let display = 0;
        for (; display <= MAX_LOGIN_PORT; display++){
            if(!allDisplay.includes(display) && cleanupStaleXLock(display))
                break;
        }
        if(display == MAX_LOGIN_PORT)
            throw new Error(`无可用登录屏幕端口！已达到上限${MAX_LOGIN_PORT}个！`)
        const xvfb = spawn("Xvfb", [
            `:${display}`,
            "-screen", "0", `${size.width}x${size.height}x${Math.min(size.depth,24)}`,
            "-nolisten", "tcp"
        ],{
            stdio: ["ignore", "pipe", "pipe"]
        });
        return new Promise((resolve, reject) => {
            const timer = setTimeout(()=>{
                clearTimeout(timer);
                resolve({xvfb,display});
            },START_SERVICE_WAITING_TIME);
            xvfb.on("error", (err) => {
                clearTimeout(timer);
                reject(new Error(`启动Xvfb失败，原因如下：\n${err.stack}`));
            });
            xvfb.on("exit", (code: number | null,signal: NodeJS.Signals | null) => {
                clearTimeout(timer);
                reject(`Xvfb退出，退出码：${code}，信号：${signal}`);
            });
            xvfb.stdout.on("data", (chunk) => {
                process.stdout.write(`[xvfb:${display}] ${chunk}`);
            });
            xvfb.stderr.on("data", (chunk) => {
                process.stderr.write(`[xvfb:${display}] ${chunk}`);
            });
        });
    }

    export async function validPort(): Promise<number>{
        return new Promise((resolve, reject) => {
            const s = net.createServer();
            s.on("error", reject);
            s.listen(0, "127.0.0.1", () => {
                const addr = s.address();
                if (!addr || typeof addr === "string") {
                    s.close();
                    reject(new Error("failed to get free port"));
                    return;
                }
                const port = addr.port;
                s.close(err => (err ? reject(err) : resolve(port)));
            });
        });
    }

    async function startVnc(display: number): Promise<vnc>{
        const port = await validPort();
        const vnc = spawn("x11vnc",[
            "-display", `:${display}`,
            "-rfbport", `${port}`,
            "-localhost",
            "-nopw"
        ]);
        return await new Promise((resolve, reject) => {
            const timer = setTimeout(()=>{
                clearTimeout(timer);
                resolve({
                    vnc,
                    port
                });
                vnc.on("error", (err) => {
                    clearTimeout(timer);
                    reject(new Error(`启动VNC失败，原因如下：\n${err.stack}`));
                });
            },START_SERVICE_WAITING_TIME);
        });
    }

    async function startWebsockify(closer: () => void,vnc: vnc): Promise<web>{
        const port = await validPort();
        const web = spawn("websockify",[
            "--web", "/usr/share/novnc",
            "--idle-timeout", `${LOGIN_IDLE_SECONDS}`,
            `${port}`, `localhost:${vnc.port}`
        ]);
        return await new Promise((resolve, reject) => {
            const timer = setTimeout(()=>{
                clearTimeout(timer);
                resolve({
                    websockify: web,
                    port
                });
            },START_SERVICE_WAITING_TIME);
            web.addListener("exit",(code,signal) => {
                const err = `Websockify关闭，状态码：${code}，收到信号：${signal}`;
                console.log(err);
                closer();
                clearTimeout(timer);
                reject(err);
            })
            web.on("error", (err) => {
                clearTimeout(timer);
                reject(new Error(`启动Websockify失败，原因如下：\n${err.stack}`));
            })
            web.addListener("error", (err) => {
                const error = `Websockify遇到错误：${err}`;
                console.log(error);
                closer();
                clearTimeout(timer);
                reject(error);
            })
        });
    }

    async function close(session: string): Promise<void>{
        if(!loginSessions.has(session))
            return;
        const {browser: browser,Xvfb: xvfb,Vnc: vnc,Websockify: web} = loginSessions.get(session);
        await browser.close();
        if(!xvfb.xvfb.killed)
            xvfb.xvfb.kill("SIGQUIT");
        if(!vnc.vnc.killed)
            vnc.vnc.kill("SIGQUIT");
        if(!web.websockify.killed)
            web.websockify.kill("SIGQUIT");
        loginSessions.delete(session);
    }

    export async function closeAll(): Promise<void>{
        return Array.from(loginSessions.keys()).forEach(close)
    }

    export interface CollectRequest extends RequestBody {
        token: string;
    }

    export interface CollectResponse extends BackBody {
        context: WorkContext
    }

    export async function collectContext(token: string,session: string): Promise<CollectResponse>{
        if(loginSessions.has(session)){
            console.log(`收集${session}的登录信息`);
            const login: LoginSession = loginSessions.get(session);
            if(login.token == token) {
                for (const context of login.browser.contexts()) {
                    const cookies = Array.from(await context.cookies(login.url));
                    if(cookies.find(c => c.name == "_uuid") != undefined) {
                        let cookie = "";
                        for (const c of cookies) {
                            cookie += `${c.name}=${c.value}; `;
                        }
                        return {
                            ok: true,
                            back: [],
                            context: {
                                cookie: {
                                    value: cookie,
                                    domain: '',
                                    path: '/'
                                }
                            }
                        }
                    }
                }
                console.error(`收集${session}的登录信息失败`);
                return {
                    ok: false,
                    error: {
                        name: "未找到Context",
                        message: "没有找到合适要求的Context"
                    },
                    back: [],
                    context: {
                        cookie: {
                            value: '',
                            domain: '',
                            path: '/'
                        }
                    }
                }
            }
        }
        throw new Error("错误Token和Session参数！")
    }
}

import {BackBody, RequestBody, WorkContext} from "./server";
import {Browser, chromium} from "playwright";
import {ChildProcessWithoutNullStreams, spawn} from "child_process";
import {LOGIN_IDLE_SECONDS, MAX_LOGIN_PORT, START_SERVICE_WAITING_TIME} from "./env";
import * as net from "node:net";
import fs from "node:fs";
import {logger} from "./logger";

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

    function isProcessRunning(process: ChildProcessWithoutNullStreams): boolean {
        return !process.killed && process.exitCode === null && process.signalCode === null;
    }

    function stopProcess(process: ChildProcessWithoutNullStreams, signal: NodeJS.Signals): void {
        if (!isProcessRunning(process)) {
            return;
        }
        try {
            process.kill(signal);
        } catch (error) {
            logger.warn({error}, "关闭登录子进程失败");
        }
    }

    function pipeProcessOutput(name: string, process: ChildProcessWithoutNullStreams): void {
        process.stdout.on("data", (chunk) => {
            globalThis.process.stdout.write(`[${name}] ${chunk}`);
        });
        process.stderr.on("data", (chunk) => {
            globalThis.process.stderr.write(`[${name}] ${chunk}`);
        });
    }

    function isLoginSessionAlive(session: LoginSession): boolean {
        return session.browser.isConnected()
            && isProcessRunning(session.Xvfb.xvfb)
            && isProcessRunning(session.Vnc.vnc)
            && isProcessRunning(session.Websockify.websockify);
    }

    function platformFromSession(session: string): string {
        const splitAt = session.indexOf(":");
        return splitAt >= 0 ? session.slice(splitAt + 1) : "";
    }

    function loginCookieUrls(platform: string, url: string): string[] {
        if (platform === "Bilibili") {
            return Array.from(new Set([
                url,
                "https://www.bilibili.com/",
                "https://space.bilibili.com/",
                "https://passport.bilibili.com/"
            ]));
        }
        return [url];
    }

    function hasLoginCookie(platform: string, cookies: Awaited<ReturnType<import("playwright").BrowserContext["cookies"]>>): boolean {
        const names = new Set(cookies.map(cookie => cookie.name));
        if (platform === "Bilibili") {
            return names.has("SESSDATA") || names.has("DedeUserID");
        }
        return names.has("_uuid");
    }

    function settleCloser(closer: () => void | Promise<void>): void {
        Promise.resolve()
            .then(closer)
            .catch(error => logger.warn({error}, "关闭登录会话失败"));
    }

    function waitForPort(
        name: string,
        port: number,
        process: ChildProcessWithoutNullStreams,
        host = "127.0.0.1",
        timeoutMs = START_SERVICE_WAITING_TIME
    ): Promise<void> {
        const startedAt = Date.now();
        return new Promise((resolve, reject) => {
            let settled = false;
            let retryTimer: NodeJS.Timeout | undefined;

            const cleanup = () => {
                settled = true;
                if (retryTimer) {
                    clearTimeout(retryTimer);
                    retryTimer = undefined;
                }
            };

            const fail = (message: string) => {
                cleanup();
                reject(new Error(message));
            };

            const tryConnect = () => {
                if (settled) {
                    return;
                }
                if (!isProcessRunning(process)) {
                    fail(`${name}启动失败，进程已退出，端口：${host}:${port}`);
                    return;
                }

                const socket = net.createConnection({host, port});
                let socketSettled = false;

                const retry = () => {
                    if (socketSettled || settled) {
                        return;
                    }
                    socketSettled = true;
                    socket.destroy();
                    if (Date.now() - startedAt >= timeoutMs) {
                        fail(`${name}启动超时，端口不可连接：${host}:${port}`);
                        return;
                    }
                    retryTimer = setTimeout(tryConnect, 100);
                };

                socket.setTimeout(1000);
                socket.once("connect", () => {
                    if (socketSettled || settled) {
                        return;
                    }
                    socketSettled = true;
                    socket.destroy();
                    cleanup();
                    resolve();
                });
                socket.once("error", retry);
                socket.once("timeout", retry);
            };

            tryConnect();
        });
    }

    async function cleanupDeadSession(session: string): Promise<boolean> {
        const current = loginSessions.get(session);
        if (!current) {
            return false;
        }
        if (isLoginSessionAlive(current)) {
            return true;
        }
        logger.warn({session}, "登录会话子进程已失效，准备重新创建");
        await close(session);
        return false;
    }

    export async function login(body: LoginRequest): Promise<LoginResponse>{
        const session = getSession(body.clientID, body.platform);
        if(await cleanupDeadSession(session))
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
                if (!isLoginSessionAlive(login)) {
                    settleCloser(() => close(session));
                    throw new Error("登录屏幕服务已经关闭，请重新发起登录");
                }
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
            "-forever",
            "-shared",
            "-nopw"
        ]);
        pipeProcessOutput(`vnc:${display}:${port}`, vnc);
        vnc.on("error", (err) => {
            logger.error({error: err}, "VNC 遇到错误");
        });
        vnc.on("exit", (code, signal) => {
            logger.warn({display, port, code, signal}, "VNC 已关闭");
        });
        await waitForPort("VNC", port, vnc);
        logger.info({display, port}, "VNC 已启动");
        return {
            vnc,
            port
        };
    }

    async function startWebsockify(closer: () => void,vnc: vnc): Promise<web>{
        const port = await validPort();
        const web = spawn("websockify",[
            "--web", "/usr/share/novnc",
            "--idle-timeout", `${LOGIN_IDLE_SECONDS}`,
            `${port}`, `localhost:${vnc.port}`
        ]);
        pipeProcessOutput(`websockify:${port}`, web);
        web.on("exit",(code,signal) => {
            logger.warn({port, vncPort: vnc.port, code, signal}, "Websockify 已关闭");
            settleCloser(closer);
        });
        web.on("error", (err) => {
            logger.error({error: err}, "Websockify 遇到错误");
            settleCloser(closer);
        });
        try {
            await waitForPort("Websockify", port, web);
        } catch (error) {
            stopProcess(web, "SIGKILL");
            throw error;
        }
        logger.info({port, vncPort: vnc.port}, "Websockify 已启动");
        return {
            websockify: web,
            port
        };
    }

    async function close(session: string): Promise<void>{
        if(!loginSessions.has(session))
            return;
        const {browser: browser,Xvfb: xvfb,Vnc: vnc,Websockify: web} = loginSessions.get(session);
        loginSessions.delete(session);
        await browser.close().catch(error => logger.warn({error, session}, "关闭登录浏览器失败"));
        stopProcess(xvfb.xvfb, "SIGQUIT");
        stopProcess(vnc.vnc, "SIGQUIT");
        stopProcess(web.websockify, "SIGQUIT");
    }

    export async function closeAll(): Promise<void>{
        await Promise.all(Array.from(loginSessions.keys()).map(close));
    }

    export interface CollectRequest extends RequestBody {
        token: string;
    }

    export interface CollectResponse extends BackBody {
        context: WorkContext
    }

    export async function collectContext(token: string,session: string): Promise<CollectResponse>{
        if(loginSessions.has(session)){
            logger.info({session}, "收集登录信息");
            const login: LoginSession = loginSessions.get(session);
            if(login.token == token) {
                const platform = platformFromSession(session);
                for (const context of login.browser.contexts()) {
                    const cookies = Array.from(await context.cookies(loginCookieUrls(platform, login.url)));
                    if(hasLoginCookie(platform, cookies)) {
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
                    logger.warn({session, platform, cookieNames: cookies.map(cookie => cookie.name)}, "当前浏览器上下文没有登录凭据");
                }
                logger.error({session}, "收集登录信息失败");
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

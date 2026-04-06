import {fastify} from "../PluginAPI";
import type {BackBody} from "../server";

const CASE_TIMEOUT_MS = 60000;
const TOTAL_TIMEOUT_MS = 600000;

process.env.BROWSER_SKIP_LISTEN = "1";

function withTimeout<T>(promise: Promise<T>, label: string, timeoutMs: number): Promise<T> {
    return new Promise<T>((resolve, reject) => {
        const timer = setTimeout(() => {
            reject(new Error(`${label} 超时(${timeoutMs}ms)`));
        }, timeoutMs);
        promise
            .then((value) => {
                clearTimeout(timer);
                resolve(value);
            })
            .catch((error) => {
                clearTimeout(timer);
                reject(error);
            });
    });
}

require("../server");

type Check = {
    name: string;
    passed: boolean;
    detail?: string;
};

function parseJson(payload: string): unknown {
    try {
        return JSON.parse(payload);
    } catch {
        return null;
    }
}

function isObject(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null;
}

function pushResult(results: Check[], name: string, passed: boolean, detail?: string): void {
    results.push({name, passed, detail});
    const prefix = passed ? "PASS" : "FAIL";
    const log = `[${prefix}] 测试名：${name}${detail ? ` -> ${detail}` : ""}`;
    passed ? console.log(log) : console.warn(log);
}

async function runCase(results: Check[], name: string, action: () => Promise<void>): Promise<void> {
    try {
        await withTimeout(action(), name, CASE_TIMEOUT_MS);
        pushResult(results, name, true);
    } catch (error) {
        const detail = error instanceof Error ? error.message : String(error);
        pushResult(results, name, false, detail);
    }
}

async function run(): Promise<number> {
    const results: Check[] = [];

    await withTimeout(Promise.resolve(fastify.ready()), "fastify.ready", CASE_TIMEOUT_MS);

    await runCase(results, "GET /test 正常返回", async () => {
        const res = await fastify.inject({method: "GET", url: "/test"});
        const body = parseJson(res.payload) as BackBody;
        if (!(res.statusCode === 200 && body.ok === true)) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "POST / 正常返回", async () => {
        const res = await fastify.inject({
            method: "POST",
            url: "/",
            payload: {
                clientID: "client-a",
                platform: "BiliBili",
                context: {cookie: {value: "", domain: "", path: "/"}},
                workers: [
                    {
                        type: "UrlAction",
                        info: {
                            url: "https://www.bilibili.com/"
                        }
                    }
                ]
            }
        });
        if (res.statusCode !== 200) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "POST / 异常返回(缺少workers)", async () => {
        const res = await fastify.inject({
            method: "POST",
            url: "/",
            payload: {
                clientID: "client-a",
                platform: "BiliBili",
                context: {cookie: {value: "", domain: "", path: "/"}}
            }
        });
        const body = parseJson(res.payload) as BackBody;
        const ok =
            res.statusCode === 200 &&
            isObject(body) &&
            body.ok === false &&
            Array.isArray(body.back);
        if (!ok) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "POST /other/closeWorker 正常返回", async () => {
        const res = await fastify.inject({
            method: "POST",
            url: "/other/closeWorker",
            payload: {
                clientID: "client-a",
                platform: "BiliBili",
                context: {cookie: {value: "", domain: "", path: "/"}},
                mode: "closeWorker"
            }
        });
        const body = parseJson(res.payload);
        if (!(res.statusCode === 200 && isObject(body) && typeof body.ok === "boolean")) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "POST /other/closeWorker 异常返回(错误mode)", async () => {
        const res = await fastify.inject({
            method: "POST",
            url: "/other/closeWorker",
            payload: {
                clientID: "client-a",
                platform: "BiliBili",
                context: {cookie: {value: "", domain: "", path: "/"}},
                mode: "bad-mode"
            }
        });
        const body = parseJson(res.payload);
        const ok =
            res.statusCode === 200 &&
            isObject(body) &&
            body.ok === false &&
            isObject(body.error) &&
            body.error.name === "Wrong request mode";
        if (!ok) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "POST /other/testContext 返回检查", async () => {
        const res = await fastify.inject({
            method: "POST",
            url: "/other/testContext",
            payload: {}
        });
        if (res.statusCode !== 302) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "GET /screen 异常返回(非法token/session)", async () => {
        const res = await fastify.inject({
            method: "GET",
            url: "/screen?token=invalid-token&session=invalid-session"
        });
        if (res.statusCode !== 500) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    await runCase(results, "POST /other/login/backend 异常返回(非法token)", async () => {
        const res = await fastify.inject({
            method: "POST",
            url: "/other/login/backend",
            payload: {
                clientID: "client-a",
                platform: "BiliBili",
                token: "invalid-token",
                context: {cookie: {value: "", domain: "", path: "/"}}
            }
        });
        const body = parseJson(res.payload);
        const ok =
            (res.statusCode === 200 && isObject(body) && body.ok === false) ||
            res.statusCode >= 500;
        if (!ok) {
            throw new Error(`status=${res.statusCode}, payload=${res.payload}`);
        }
    });

    const failed = results.filter((item) => !item.passed);
    const passedCount = results.length - failed.length;

    console.log("\n==== 测试汇总 ====");
    console.log(`总数: ${results.length}`);
    console.log(`通过: ${passedCount}`);
    console.log(`失败: ${failed.length}`);

    if (failed.length > 0) {
        console.log("失败列表:");
        for (const item of failed) {
            console.log(`- ${item.name}${item.detail ? ` (${item.detail})` : ""}`);
        }
    }

    await fastify.close();
    return failed.length === 0 ? 0 : 1;
}

withTimeout(run(), "全部测试", TOTAL_TIMEOUT_MS)
    .then((code) => {
        process.exitCode = code;
    })
    .catch((error) => {
        console.error(error);
        process.exitCode = 1;
    });

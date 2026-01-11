import {ChildProcessWithoutNullStreams, spawn} from "node:child_process";
import fs from "node:fs";

const XVFB_WIDTH = 1280;
const XVFB_HEIGHT = 720;
const XVFB_DEPTH = 24;
const XVFB_START_DISPLAY = 99;
const XVFB_MAX_DISPLAY = 110;
const XVFB_READY_DELAY_MS = 300;

export interface DisplaySession {
    env?: NodeJS.ProcessEnv;
    close(): Promise<void>;
}

type LaunchResult = {
    display: number;
    process: ChildProcessWithoutNullStreams;
};

export async function ensureDisplayForHeadedLaunch(headless: boolean): Promise<DisplaySession> {
    if (headless || process.env.DISPLAY) {
        return {
            env: process.env,
            async close(): Promise<void> {}
        };
    }

    const failures: string[] = [];
    for (let display = XVFB_START_DISPLAY; display <= XVFB_MAX_DISPLAY; display++) {
        if (displayExists(display)) {
            failures.push(`:${display} -> DISPLAY 已存在`);
            continue;
        }
        try {
            const launched = await launchXvfb(display);
            return {
                env: {
                    ...process.env,
                    DISPLAY: `:${launched.display}`
                },
                async close(): Promise<void> {
                    await stopProcess(launched.process);
                }
            };
        } catch (error) {
            const detail = error instanceof Error ? error.message : String(error);
            failures.push(`:${display} -> ${detail}`);
        }
    }

    throw new Error(`启动 Xvfb 失败，没有可用 DISPLAY。\n${failures.join("\n")}`);
}

function displayExists(display: number): boolean {
    return fs.existsSync(`/tmp/.X11-unix/X${display}`) || fs.existsSync(`/tmp/.X${display}-lock`);
}

async function launchXvfb(display: number): Promise<LaunchResult> {
    return await new Promise<LaunchResult>((resolve, reject) => {
        let settled = false;
        const xvfb = spawn(
            "Xvfb",
            [
                `:${display}`,
                "-screen",
                "0",
                `${XVFB_WIDTH}x${XVFB_HEIGHT}x${XVFB_DEPTH}`,
                "-nolisten",
                "tcp"
            ],
            {
                stdio: ["ignore", "pipe", "pipe"]
            }
        );

        const cleanup = (): void => {
            xvfb.off("error", onError);
            xvfb.off("exit", onExit);
        };
        const finishReject = (error: Error): void => {
            if (settled) {
                return;
            }
            settled = true;
            clearTimeout(timer);
            cleanup();
            reject(error);
        };
        const finishResolve = (): void => {
            if (settled) {
                return;
            }
            settled = true;
            clearTimeout(timer);
            cleanup();
            resolve({display, process: xvfb});
        };
        const onError = (error: Error): void => {
            finishReject(new Error(`Xvfb 进程启动失败: ${error.message}`));
        };
        const onExit = (code: number | null, signal: NodeJS.Signals | null): void => {
            finishReject(new Error(`Xvfb 提前退出(code=${code}, signal=${signal})`));
        };

        xvfb.stdout.on("data", (chunk) => {
            process.stdout.write(`[xvfb:${display}] ${chunk}`);
        });
        xvfb.stderr.on("data", (chunk) => {
            process.stderr.write(`[xvfb:${display}] ${chunk}`);
        });
        xvfb.on("error", onError);
        xvfb.on("exit", onExit);

        const timer = setTimeout(() => {
            finishResolve();
        }, XVFB_READY_DELAY_MS);
    });
}

async function stopProcess(child: ChildProcessWithoutNullStreams): Promise<void> {
    if (child.killed) {
        return;
    }

    await new Promise<void>((resolve) => {
        const timer = setTimeout(() => {
            if (!child.killed) {
                child.kill("SIGKILL");
            }
        }, 1000);

        child.once("exit", () => {
            clearTimeout(timer);
            resolve();
        });

        child.kill("SIGTERM");
    });
}

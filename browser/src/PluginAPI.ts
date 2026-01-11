import Fastify from 'fastify';
import fs from "fs";
import path from "path";
import {pluginPath} from "./env";
import {Handler, WorkerDescription, WorkResult} from "./server";
import {ChineseLogger, logger} from "./logger";

export const fastify = Fastify({
    logger: false,
    disableRequestLogging: true
});

export interface WorkerFactory{
    getWorker: (info: unknown) => Worker;
}

const registeredWorker : Record<string,WorkerFactory> = {}

export function registerWorker(name: string, workerFactory:WorkerFactory): boolean{
    if(registeredWorker[name])
        return false;
    registeredWorker[name] = workerFactory;
    return true;
}

export interface ServerInitContext {
    registerWorker: typeof registerWorker;
    fastify: typeof fastify;
    logger: ChineseLogger;
}

export interface ServerCloseContext {
    fastify: typeof fastify;
    logger: ChineseLogger;
}

export interface ServerPlugin {
    name?: string;
    onServerInit?: (context: ServerInitContext) => void | Promise<void>;
    onServerClose?: (context: ServerCloseContext) => void | Promise<void>;
}

const loadedPlugins: ServerPlugin[] = [];

export abstract class Worker {
    protected constructor(info: unknown) {}

    public abstract work(handler: Handler): Promise<WorkResult | WorkResult[]>;

    public static fromDescription(description: WorkerDescription): Worker {
        if (!registeredWorker[description.type])
            throw new Error(`${description.type} is not registered`);
        return registeredWorker[description.type].getWorker(description.info);
    }
}

type PluginConfig = {
    plugins?: string[];
};

function readPluginConfig(filePath: string): string[] {
    if (!fs.existsSync(filePath)) return [];
    const raw = fs.readFileSync(filePath, "utf8");
    const parsed = JSON.parse(raw) as PluginConfig;
    if (!Array.isArray(parsed.plugins)) return [];
    return parsed.plugins.filter((name): name is string => typeof name === "string" && Boolean(name.trim()));
}

function listThirdPartyPluginNames(): string[] {
    let byConfig: string[] = [];
    try {
        byConfig = readPluginConfig(pluginPath);
    } catch (error) {
        logger.warn({error, pluginPath}, "读取插件配置失败");
    }
    return byConfig
}

function isLocalPluginPath(name: string): boolean {
    return name.startsWith("./") || name.startsWith("../") || path.isAbsolute(name);
}

function resolvePluginModule(name: string): string {
    if (!isLocalPluginPath(name)) return name;
    const configDir = path.dirname(path.resolve(pluginPath));
    const resolvedPath = path.resolve(configDir, name);
    if (!fs.existsSync(resolvedPath)) {
        throw new Error(`plugin path '${name}' not found, resolved to '${resolvedPath}'`);
    }
    return resolvedPath;
}

function toServerPlugin(moduleValue: unknown, sourceName: string): ServerPlugin {
    const candidate = (moduleValue as { default?: unknown })?.default ?? moduleValue;
    if (!candidate || typeof candidate !== "object") {
        throw new Error(`plugin '${sourceName}' does not export an object`);
    }
    return candidate as ServerPlugin;
}

async function loadThirdPartyPlugin(name: string): Promise<ServerPlugin> {
    const moduleName = resolvePluginModule(name);
    const moduleValue = require(moduleName) as unknown;
    const plugin = toServerPlugin(moduleValue, name);
    if (typeof plugin.onServerInit !== "function") {
        throw new Error(`plugin '${name}' is missing onServerInit()`);
    }
    if (!plugin.name) plugin.name = name;
    return plugin;
}

export async function initServerPlugins(): Promise<void> {
    const initContext: ServerInitContext = {
        registerWorker,
        fastify,
        logger
    };

    const pluginNames = listThirdPartyPluginNames();
    for (const pluginName of pluginNames) {
        try {
            const plugin = await loadThirdPartyPlugin(pluginName);
            await plugin.onServerInit?.(initContext);
            loadedPlugins.push(plugin);
            logger.info({plugin: plugin.name}, "插件初始化完成");
        } catch (error) {
            logger.error({error, pluginName}, "插件初始化失败");
        }
    }
}

export async function closeServerPlugins(): Promise<void> {
    const closeContext: ServerCloseContext = {
        fastify,
        logger
    };
    for (const plugin of loadedPlugins) {
        try {
            await plugin.onServerClose?.(closeContext);
        } catch (error) {
            logger.error({error, plugin: plugin.name}, "第三方插件关闭失败");
        }
    }
}

import path from "path";

export const API_PORT = Number(process.env.PORT || 3000);

const DEFAULT_PLUGIN_CONFIG_PATH = path.resolve(process.cwd(), "config/plugins.json");
export const pluginPath = process.env.PLUGIN_PATH || DEFAULT_PLUGIN_CONFIG_PATH;

export const MAX_LOGIN_PORT = Number(process.env.MAX_LOGIN_PORT) || 10;
export const LOGIN_IDLE_SECONDS = Number(process.env.LOGIN_SECONDS) || 3000;

export const BROWSER_DEBUG_ENABLED = process.env.BROWSER_DEBUG === "1";
export const BROWSER_DEBUG_ARTIFACTS_DIR = path.resolve(
    process.env.BROWSER_DEBUG_ARTIFACTS_DIR || path.join(process.cwd(), "debug-artifacts")
);
export const BROWSER_DEBUG_HTML_LIMIT = Number(process.env.BROWSER_DEBUG_HTML_LIMIT) || 20000;
export const BROWSER_DEBUG_HEADLESS = process.env.BROWSER_DEBUG_HEADLESS !== "0";
export const BROWSER_DEBUG_KEEP_OPEN = process.env.BROWSER_DEBUG_KEEP_OPEN === "1";
import path from "path";

export const API_PORT = Number(process.env.PORT || 3000);

const DEFAULT_PLUGIN_CONFIG_PATH = path.resolve(process.cwd(), "config/plugins.json");
export const pluginPath = process.env.PLUGIN_PATH || DEFAULT_PLUGIN_CONFIG_PATH;

export const MAX_LOGIN_PORT = Number(process.env.MAX_LOGIN_PORT) || 10;
export const LOGIN_IDLE_SECONDS = Number(process.env.LOGIN_SECONDS) || 3000;
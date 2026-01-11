import {AsyncLocalStorage} from "node:async_hooks";
import {
    BROWSER_REQUEST_LOG_SUMMARY_ENABLED,
    BROWSER_REQUEST_LOG_SUMMARY_LIMIT,
    BROWSER_VERBOSE_LOG_ENABLED
} from "./env";

type LogLevel = "trace" | "debug" | "info" | "warn" | "error" | "fatal" | "silent";
type LogValue = string | number | boolean | null | undefined | Error | object;
type LogFields = Record<string, LogValue>;

const levelWeights: Record<LogLevel, number> = {
    trace: 10,
    debug: 20,
    info: 30,
    warn: 40,
    error: 50,
    fatal: 60,
    silent: 70
};

const levelLabels: Record<Exclude<LogLevel, "silent">, string> = {
    trace: "跟踪",
    debug: "调试",
    info: "信息",
    warn: "警告",
    error: "错误",
    fatal: "致命"
};

const requestLogSummaries = new Map<string, string[]>();

function normalizeLevel(value: string | undefined): LogLevel {
    if (!value) return "info";
    const level = value.toLowerCase() as LogLevel;
    return level in levelWeights ? level : "info";
}

function flattenWhitespace(value: string): string {
    const flattened = value.replace(/\s+/g, " ").trim();
    return flattened.length > 1000 ? `${flattened.slice(0, 1000)}...` : flattened;
}

function formatValue(value: LogValue): string | undefined {
    if (value === undefined) return undefined;
    if (value === null) return "空";
    if (value instanceof Error) return flattenWhitespace(`${value.name}: ${value.message}`);
    if (typeof value === "object") {
        try {
            return flattenWhitespace(JSON.stringify(value));
        } catch {
            return "无法序列化";
        }
    }
    return flattenWhitespace(String(value));
}

function formatFields(fields: LogFields): string {
    return Object.entries(fields)
        .map(([key, value]) => {
            const formatted = formatValue(value);
            return formatted === undefined ? undefined : `${key}=${formatted}`;
        })
        .filter((item): item is string => Boolean(item))
        .join(" ");
}

function appendRequestSummaryLine(requestId: string, line: string): void {
    const current = requestLogSummaries.get(requestId) || [];
    current.push(line);
    if (current.length > BROWSER_REQUEST_LOG_SUMMARY_LIMIT) {
        current.splice(0, current.length - BROWSER_REQUEST_LOG_SUMMARY_LIMIT);
    }
    requestLogSummaries.set(requestId, current);
}

export function takeRequestLogSummary(requestId: string): string[] {
    const lines = requestLogSummaries.get(requestId) || [];
    requestLogSummaries.delete(requestId);
    return lines;
}

export function clearRequestLogSummary(requestId: string): void {
    requestLogSummaries.delete(requestId);
}

export class ChineseLogger {
    private readonly bindings: LogFields;
    private readonly minLevel: LogLevel;

    public constructor(bindings: LogFields = {}, minLevel = normalizeLevel(process.env.BROWSER_LOG_LEVEL)) {
        this.bindings = bindings;
        this.minLevel = minLevel;
    }

    public child(bindings: LogFields): ChineseLogger {
        return new ChineseLogger({...this.bindings, ...bindings}, this.minLevel);
    }

    public getBinding(key: string): LogValue {
        return this.bindings[key];
    }

    public trace(fieldsOrMessage: LogFields | string, message?: string): void {
        this.write("trace", fieldsOrMessage, message);
    }

    public debug(fieldsOrMessage: LogFields | string, message?: string): void {
        this.write("debug", fieldsOrMessage, message);
    }

    public info(fieldsOrMessage: LogFields | string, message?: string): void {
        this.write("info", fieldsOrMessage, message);
    }

    public dev(fieldsOrMessage: LogFields | string, message?: string): void {
        if (!BROWSER_VERBOSE_LOG_ENABLED) return;
        this.write("info", fieldsOrMessage, message);
    }

    public warn(fieldsOrMessage: LogFields | string, message?: string): void {
        this.write("warn", fieldsOrMessage, message);
    }

    public error(fieldsOrMessage: LogFields | string, message?: string): void {
        this.write("error", fieldsOrMessage, message);
    }

    public fatal(fieldsOrMessage: LogFields | string, message?: string): void {
        this.write("fatal", fieldsOrMessage, message);
    }

    public silent(): void {}

    private shouldWrite(level: Exclude<LogLevel, "silent">): boolean {
        return levelWeights[level] >= levelWeights[this.minLevel];
    }

    private write(level: Exclude<LogLevel, "silent">, fieldsOrMessage: LogFields | string, message?: string): void {
        if (!this.shouldWrite(level)) return;
        const fields = typeof fieldsOrMessage === "string" ? this.bindings : {...this.bindings, ...fieldsOrMessage};
        const text = typeof fieldsOrMessage === "string" ? fieldsOrMessage : message || "";
        const fieldText = formatFields(fields);
        const line = `${new Date().toISOString()} [${levelLabels[level]}] ${text}${fieldText ? ` ${fieldText}` : ""}`;
        const requestId = fields.requestId;

        if (BROWSER_REQUEST_LOG_SUMMARY_ENABLED && typeof requestId === "string" && requestId.trim()) {
            appendRequestSummaryLine(requestId, line);
        }

        if (level === "error" || level === "fatal") {
            console.error(line);
        } else if (level === "warn") {
            console.warn(line);
        } else {
            console.log(line);
        }
    }
}

export const logger = new ChineseLogger();

const loggerStorage = new AsyncLocalStorage<ChineseLogger>();

export function getCurrentLogger(): ChineseLogger {
    return loggerStorage.getStore() || logger;
}

export function runWithLogger<T>(requestLogger: ChineseLogger, callback: () => Promise<T>): Promise<T> {
    return loggerStorage.run(requestLogger, callback);
}

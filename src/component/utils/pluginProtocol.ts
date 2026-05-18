import {fetchBackend} from "@/pages/settings/backendSetup.ts";

export const FRONTEND_PLUGIN_SETTINGS_PROTOCOL = "frontend-plugin-settings";
export const FRONTEND_PLUGIN_SETTINGS_VERSION = 1;

export type PluginScalarValue = string | number | boolean | null;
export type PluginFieldType = "string" | "number" | "boolean" | "select";
export type PluginFieldInput = "text" | "password" | "url";
export type PluginStatusTone = "ready" | "info" | "warning" | "error";

export interface PluginFieldOption {
  label: string;
  value: string;
}

export interface PluginFieldDefinition {
  key: string;
  label: string;
  type: PluginFieldType;
  description?: string;
  placeholder?: string;
  input?: PluginFieldInput;
  options?: PluginFieldOption[];
  defaultValue?: PluginScalarValue;
}

export interface PluginStatusDescriptor {
  type: PluginStatusTone;
  text: string;
}

export interface PluginSettingsDescriptor {
  plugin: string;
  name: string;
  description: string;
  status: PluginStatusDescriptor;
  fields: PluginFieldDefinition[];
  values: Record<string, PluginScalarValue>;
  submitLabel: string;
}

export interface PluginSettingsRequestData {
  protocol: typeof FRONTEND_PLUGIN_SETTINGS_PROTOCOL;
  version: typeof FRONTEND_PLUGIN_SETTINGS_VERSION;
  action: "describe" | "save";
  values?: Record<string, PluginScalarValue>;
}

export type PluginDescribeParseResult =
  | { kind: "empty" }
  | { kind: "standard"; descriptor: PluginSettingsDescriptor }
  | { kind: "unsupported"; rawText: string };

type UnknownRecord = Record<string, unknown>;

function isRecord(value: unknown): value is UnknownRecord {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isPluginScalarValue(value: unknown): value is PluginScalarValue {
  return (
    value === null
    || typeof value === "string"
    || typeof value === "number"
    || typeof value === "boolean"
  );
}

function normalizePluginScalarValue(value: unknown): PluginScalarValue | undefined {
  if (isPluginScalarValue(value)) {
    return value;
  }
  return undefined;
}

function normalizeFieldOption(value: unknown): PluginFieldOption | null {
  if (!isRecord(value)) {
    return null;
  }
  if (typeof value.label !== "string" || typeof value.value !== "string") {
    return null;
  }
  return {
    label: value.label,
    value: value.value,
  };
}

function normalizeFieldDefinition(value: unknown): PluginFieldDefinition | null {
  if (!isRecord(value)) {
    return null;
  }
  if (typeof value.key !== "string" || typeof value.label !== "string") {
    return null;
  }
  if (
    value.type !== "string"
    && value.type !== "number"
    && value.type !== "boolean"
    && value.type !== "select"
  ) {
    return null;
  }

  const field: PluginFieldDefinition = {
    key: value.key,
    label: value.label,
    type: value.type,
  };

  if (typeof value.description === "string") {
    field.description = value.description;
  }
  if (typeof value.placeholder === "string") {
    field.placeholder = value.placeholder;
  }
  if (value.input === "text" || value.input === "password" || value.input === "url") {
    field.input = value.input;
  }
  const defaultValue = normalizePluginScalarValue(value.default);
  if (defaultValue !== undefined) {
    field.defaultValue = defaultValue;
  }
  if (field.type === "select") {
    if (!Array.isArray(value.options)) {
      return null;
    }
    const options = value.options
      .map((option) => normalizeFieldOption(option))
      .filter((option): option is PluginFieldOption => option !== null);
    if (!options.length || options.length !== value.options.length) {
      return null;
    }
    field.options = options;
  }

  return field;
}

function normalizeStatus(value: unknown, fallbackText: string): PluginStatusDescriptor {
  if (typeof value === "string") {
    if (value === "ready" || value === "info" || value === "warning" || value === "error") {
      return {
        type: value,
        text: fallbackText,
      };
    }
    return {
      type: "info",
      text: value,
    };
  }

  if (isRecord(value)) {
    const type = value.type;
    const text = value.text;
    if (
      (type === "ready" || type === "info" || type === "warning" || type === "error")
      && typeof text === "string"
    ) {
      return {
        type,
        text,
      };
    }
  }

  return {
    type: "info",
    text: fallbackText,
  };
}

function normalizeFieldValue(field: PluginFieldDefinition, value: unknown): PluginScalarValue {
  switch (field.type) {
    case "boolean":
      if (typeof value === "boolean") {
        return value;
      }
      if (typeof value === "string") {
        if (value === "true") {
          return true;
        }
        if (value === "false") {
          return false;
        }
      }
      if (typeof value === "number") {
        return value !== 0;
      }
      if (typeof field.defaultValue === "boolean") {
        return field.defaultValue;
      }
      return false;
    case "number":
      if (typeof value === "number" && Number.isFinite(value)) {
        return value;
      }
      if (typeof value === "string" && value.trim() !== "") {
        const parsed = Number(value);
        if (Number.isFinite(parsed)) {
          return parsed;
        }
      }
      if (typeof field.defaultValue === "number") {
        return field.defaultValue;
      }
      return null;
    case "select":
      if (typeof value === "string" && field.options?.some((option) => option.value === value)) {
        return value;
      }
      if (typeof field.defaultValue === "string" && field.options?.some((option) => option.value === field.defaultValue)) {
        return field.defaultValue;
      }
      return field.options?.[0]?.value ?? "";
    case "string":
    default:
      if (typeof value === "string") {
        return value;
      }
      if (typeof field.defaultValue === "string") {
        return field.defaultValue;
      }
      if (value === null) {
        return "";
      }
      if (typeof value === "number" || typeof value === "boolean") {
        return String(value);
      }
      return "";
  }
}

function normalizeValues(
  fields: PluginFieldDefinition[],
  values: unknown,
): Record<string, PluginScalarValue> {
  const rawValues = isRecord(values) ? values : {};
  const normalized: Record<string, PluginScalarValue> = {};

  fields.forEach((field) => {
    normalized[field.key] = normalizeFieldValue(field, rawValues[field.key]);
  });

  return normalized;
}

export function createPluginDescribePayload(plugin: string): {
  plugin: string;
  data: PluginSettingsRequestData;
} {
  return {
    plugin,
    data: {
      protocol: FRONTEND_PLUGIN_SETTINGS_PROTOCOL,
      version: FRONTEND_PLUGIN_SETTINGS_VERSION,
      action: "describe",
    },
  };
}

export function createPluginSavePayload(
  plugin: string,
  values: Record<string, PluginScalarValue>,
): {
  plugin: string;
  data: PluginSettingsRequestData;
} {
  return {
    plugin,
    data: {
      protocol: FRONTEND_PLUGIN_SETTINGS_PROTOCOL,
      version: FRONTEND_PLUGIN_SETTINGS_VERSION,
      action: "save",
      values,
    },
  };
}

export function parsePluginListResponse(value: unknown): string[] | null {
  if (!isRecord(value) || !Array.isArray(value.plugin)) {
    return null;
  }

  const plugins = value.plugin.filter((plugin): plugin is string => typeof plugin === "string");
  if (plugins.length !== value.plugin.length) {
    return null;
  }

  return plugins;
}

export function parsePluginSettingsDescriptor(plugin: string, value: unknown): PluginSettingsDescriptor | null {
  if (!isRecord(value)) {
    return null;
  }
  if (
    value.protocol !== FRONTEND_PLUGIN_SETTINGS_PROTOCOL
    || value.version !== FRONTEND_PLUGIN_SETTINGS_VERSION
  ) {
    return null;
  }

  const fieldsSource = value.fields;
  if (fieldsSource !== undefined && !Array.isArray(fieldsSource)) {
    return null;
  }
  const fields = (fieldsSource ?? [])
    .map((field) => normalizeFieldDefinition(field))
    .filter((field): field is PluginFieldDefinition => field !== null);
  if (fields.length !== (fieldsSource ?? []).length) {
    return null;
  }

  const statusText = typeof value.statusText === "string"
    ? value.statusText
    : "插件已接入前端通用设置协议";
  const status = normalizeStatus(value.status, statusText);

  return {
    plugin,
    name: typeof value.name === "string" ? value.name : plugin,
    description: typeof value.description === "string" ? value.description : "",
    status,
    fields,
    values: normalizeValues(fields, value.values),
    submitLabel: typeof value.submitLabel === "string" ? value.submitLabel : "保存插件配置",
  };
}

export function inspectPluginDescribeResponse(
  plugin: string,
  rawText: string,
): PluginDescribeParseResult {
  if (rawText.trim() === "") {
    return { kind: "empty" };
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(rawText);
  } catch {
    return {
      kind: "unsupported",
      rawText,
    };
  }

  if (parsePluginListResponse(parsed)) {
    return {
      kind: "unsupported",
      rawText,
    };
  }

  const descriptor = parsePluginSettingsDescriptor(plugin, parsed);
  if (!descriptor) {
    return {
      kind: "unsupported",
      rawText,
    };
  }

  return {
    kind: "standard",
    descriptor,
  };
}

export async function fetchPluginNames(): Promise<string[]> {
  const response = await fetchBackend("/plugins", {
    loading: {
      title: "正在读取插件列表",
      detail: "后端正在收集可用插件，请稍等。",
    },
  });
  if (!response.ok) {
    throw new Error(`插件列表请求失败：${response.status}`);
  }
  const text = await response.text();
  const parsed = text.trim() === "" ? null : JSON.parse(text);
  const plugins = parsePluginListResponse(parsed);
  if (!plugins) {
    throw new Error("插件列表返回格式不正确");
  }
  return plugins;
}

export async function requestPluginDescribe(plugin: string): Promise<Response> {
  return fetchBackend("/plugins", {
    method: "POST",
    body: createPluginDescribePayload(plugin) as unknown as BodyInit,
  });
}

export async function requestPluginSave(
  plugin: string,
  values: Record<string, PluginScalarValue>,
): Promise<Response> {
  return fetchBackend("/plugins", {
    method: "POST",
    body: createPluginSavePayload(plugin, values) as unknown as BodyInit,
    loading: {
      title: `正在保存 ${plugin} 配置`,
      detail: "后端正在将设置提交给插件，请稍等。",
    },
  });
}

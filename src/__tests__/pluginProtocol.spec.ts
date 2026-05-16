import {describe, expect, it} from "vitest";
import {
  FRONTEND_PLUGIN_SETTINGS_PROTOCOL,
  FRONTEND_PLUGIN_SETTINGS_VERSION,
  inspectPluginDescribeResponse,
  parsePluginListResponse
} from "@/pages/settings/pluginProtocol.ts";

describe("pluginProtocol", () => {
  it("parses backend plugin list envelope", () => {
    expect(parsePluginListResponse({
      plugin: ["alpha", "beta"],
    })).toEqual(["alpha", "beta"]);
  });

  it("recognizes empty plugin replies as no configurable items", () => {
    expect(inspectPluginDescribeResponse("alpha", "")).toEqual({
      kind: "empty",
    });
  });

  it("recognizes standard frontend plugin settings descriptors", () => {
    const result = inspectPluginDescribeResponse("alpha", JSON.stringify({
      protocol: FRONTEND_PLUGIN_SETTINGS_PROTOCOL,
      version: FRONTEND_PLUGIN_SETTINGS_VERSION,
      name: "Alpha Plugin",
      description: "Plugin description",
      status: {
        type: "ready",
        text: "All good",
      },
      fields: [
        {
          key: "enabled",
          label: "Enabled",
          type: "boolean",
          default: true,
        },
        {
          key: "mode",
          label: "Mode",
          type: "select",
          options: [
            { label: "Safe", value: "safe" },
            { label: "Fast", value: "fast" },
          ],
          default: "safe",
        },
      ],
      values: {
        enabled: false,
        mode: "fast",
      },
    }));

    expect(result.kind).toBe("standard");
    if (result.kind !== "standard") {
      return;
    }
    expect(result.descriptor.name).toBe("Alpha Plugin");
    expect(result.descriptor.status.text).toBe("All good");
    expect(result.descriptor.values).toEqual({
      enabled: false,
      mode: "fast",
    });
  });

  it("marks non-standard plugin replies as unsupported", () => {
    const result = inspectPluginDescribeResponse("alpha", JSON.stringify({
      plugin: "alpha",
      custom: true,
    }));

    expect(result.kind).toBe("unsupported");
  });
});

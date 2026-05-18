import {describe, expect, it, vi} from "vitest";
import {mount} from "@vue/test-utils";

vi.mock("@/pages/settings/backendSetup.ts", () => ({
  getValid: () => false,
  initBackend: vi.fn(),
  setup: vi.fn().mockResolvedValue({}),
}));

Object.defineProperty(window, "matchMedia", {
  writable: true,
  value: vi.fn().mockImplementation(() => ({
    matches: false,
    media: "",
    onchange: null,
    addListener: vi.fn(),
    removeListener: vi.fn(),
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    dispatchEvent: vi.fn(),
  })),
});

describe("App", () => {
  it("renders primary navigation", async () => {
    const {default: App} = await import("../pages/App.vue");
    const wrapper = mount(App, {
      global: {
        stubs: {
          "router-link": {
            template: "<a><slot /></a>",
          },
          "router-view": {
            template: "<div />",
          },
        },
      },
    });

    expect(wrapper.text()).toContain("首页");
    expect(wrapper.text()).toContain("登录");
    expect(wrapper.text()).toContain("设置");
  });
});

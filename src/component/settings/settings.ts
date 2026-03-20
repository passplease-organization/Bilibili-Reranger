import type setCategory from "@/component/utils/settingsInterface.ts";
import {computed, nextTick, ref, type Ref} from "vue";
import { setPlatform} from "@/pages/settings/backendSetup.ts";

const BACKEND: string = "backend-url";
const BACKEND_DEFAULT_URL = "/backend";
const backendURL: Ref<string> = ref<string>("");
const nowBackend: Ref<string> = ref<string>(localStorage.getItem(BACKEND) || BACKEND_DEFAULT_URL);

const BROWSER: string = "browser-url"
const BROWSER_DEFAULT_URL = "/browser";
const browserURL: Ref<string> = ref<string>("");
const nowBrowser: Ref<string> = ref<string>(localStorage.getItem(BROWSER) || BROWSER_DEFAULT_URL);

function setBackend(url: string | null) {
    if(url?.endsWith('/'))
        url = url?.slice(0,url?.length - 1);
    nowBackend.value = url || BACKEND_DEFAULT_URL;
    localStorage.setItem(BACKEND,nowBackend.value);
    backendURL.value = "";
}
export function getBackendUrl(): string {
    const url = nowBackend.value;
    if (!url) return "";
    if (url.startsWith("/")) {
        return new URL(url, window.location.origin).toString();
    }
    if (url.startsWith("//")) {
        return `${window.location.protocol}${url}`;
    }
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return url;
    }
    return `https://${url}`;
}
function setBrowser(url: string | null) {
    if(url?.endsWith('/'))
        url = url?.slice(0,url?.length - 1);
    nowBrowser.value = url || BROWSER_DEFAULT_URL;
    localStorage.setItem(BROWSER,nowBrowser.value);
    browserURL.value = "";
}
export function getBrowserUrl(): string{
    const url = nowBrowser.value;
    if (!url) return "";
    if (url.startsWith("/")) {
        return new URL(url, window.location.origin).toString();
    }
    if (url.startsWith("//")) {
        return `${window.location.protocol}${url}`;
    }
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return url;
    }
    return `https://${url}`;
}

export const supportPlatform: Ref<string[]> = ref([]);
export const targetPlatform: string = "chosePlatform"
export const nowPlatform: Ref<string> = ref<string>(localStorage.getItem(targetPlatform) || "");
export const inputPlatform: Ref<string> = ref("");

const settings: setCategory[] = [
    {
        name: "播放设置",
        settings: [
            {
                title: "播放平台设置",
                description: "设置您想看的平台，推荐的视频将全是这个平台的视频",
                select: {
                    description: "选择您的平台",
                    v_model: nowPlatform,
                    values: computed(() => {
                        return supportPlatform.value.map(platform => ({
                            label: platform,
                            value: platform
                        }));
                    })
                },
                button: {
                    save: {
                        click: setPlatform,
                        enter: true
                    },
                    reset: {
                        click: () => {inputPlatform.value = ""}
                    }
                }
            }
        ]
    },
    {
        name: `服务器配置`,
        settings: [
            {
                title: `后端服务配置`,
                description: `配置推荐服务的后端，必须配置！！！若不设置，无法获取推荐的视频！！！\n\n/开头表示相对路径，写全https表示绝对路径，保存自动刷新页面`,
                input: {
                    type: `url`,
                    placeholder: nowBackend,
                    v_model: backendURL,
                    focusin: function (event: Event) {
                        backendURL.value = backendURL.value ? backendURL.value : nowBackend.value;
                        nextTick(() => (event.currentTarget as HTMLInputElement | null)?.select());
                    },
                    focusout: function () {
                        backendURL.value = backendURL.value === nowBackend.value ? `` : backendURL.value;
                    }
                },
                button: {
                    save: {
                        click: function () {
                            setBackend(backendURL.value);
                            document.location.reload();
                        },
                        enter: true
                    },
                    reset: {
                        click: function () {
                            setBackend(null);
                        },
                    }
                }
            },
            {
                title: `爬取浏览器配置`,
                description: `配置您与后端配合的爬取浏览器，请确保此服务器是安全的（您的登录数据将存储在那里）！！！\n\n/开头表示相对路径，写全https表示绝对路径，保存自动刷新页面`,
                input: {
                    type: `url`,
                    placeholder: nowBrowser,
                    v_model: browserURL,
                    focusin: function (event: Event) {
                        browserURL.value = browserURL.value ? browserURL.value : nowBrowser.value;
                        nextTick(() => (event.currentTarget as HTMLInputElement | null)?.select());
                    },
                    focusout: function () {
                        browserURL.value = browserURL.value === nowBrowser.value ? `` : browserURL.value;
                    }
                },
                button: {
                    save: {
                        click: function () {
                            setBrowser(browserURL.value);
                        },
                    },
                    reset: {
                        click: function () {
                            setBrowser(null);
                        },
                    }
                }
            },
        ],
    }
];

export default settings;
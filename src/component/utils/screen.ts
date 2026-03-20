import {ref} from "vue";

export type ToastType = "info" | "success" | "error";

export const toastMessage = ref("");
export const toastVisible = ref(false);
export const toastType = ref<ToastType>("info");

let hideTimer: number | null = null;

export function showPopup(
    message: string,
    options: { type?: ToastType; durationMs?: number } = {},
): void {
    toastMessage.value = message;
    toastType.value = options.type ?? "info";
    toastVisible.value = true;

    if (hideTimer !== null) {
        window.clearTimeout(hideTimer);
        hideTimer = null;
    }

    const duration = options.durationMs ?? 2400;
    hideTimer = window.setTimeout(() => {
        toastVisible.value = false;
        hideTimer = null;
    }, duration);
}

function eventHandler(event: Event) :void{}
async function asyncEventHandler(event: Event) :Promise<void>{}
export type handler = typeof eventHandler | typeof asyncEventHandler;
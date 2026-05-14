import {ref} from "vue";

export type ToastType = "info" | "success" | "error";
export interface ToastLink {
    href: string;
    label: string;
}
export interface LoadingStateInput {
    title: string;
    detail?: string;
    cancelLabel?: string;
    onCancel?: (() => void) | null;
}

export const toastMessage = ref("");
export const toastVisible = ref(false);
export const toastType = ref<ToastType>("info");
export const toastLink = ref<ToastLink | null>(null);
export const loadingVisible = ref(false);
export const loadingTitle = ref("");
export const loadingDetail = ref("");
export const loadingCancelLabel = ref("");
export const loadingCancelable = ref(false);

let hideTimer: number | null = null;
let loadingTokenId = 0;
const activeLoadings = new Map<number, LoadingStateInput>();

export function showPopup(
    message: string,
    options: { type?: ToastType; durationMs?: number; link?: ToastLink | null } = {},
): void {
    toastMessage.value = message;
    toastType.value = options.type ?? "info";
    toastLink.value = options.link ?? null;
    toastVisible.value = true;

    if (hideTimer !== null) {
        window.clearTimeout(hideTimer);
        hideTimer = null;
    }

    const duration = options.durationMs ?? 2400;
    hideTimer = window.setTimeout(() => {
        toastVisible.value = false;
        toastLink.value = null;
        hideTimer = null;
    }, duration);
}

function eventHandler(event: Event) :void{}
async function asyncEventHandler(event: Event) :Promise<void>{}
export type handler = typeof eventHandler | typeof asyncEventHandler;

function syncLoadingState(): void {
    const entries = Array.from(activeLoadings.entries());
    const latest = entries.length > 0 ? entries[entries.length - 1] : null;
    if (!latest) {
        loadingVisible.value = false;
        loadingTitle.value = "";
        loadingDetail.value = "";
        loadingCancelLabel.value = "";
        loadingCancelable.value = false;
        return;
    }
    loadingVisible.value = true;
    loadingTitle.value = latest[1].title;
    loadingDetail.value = latest[1].detail ?? "";
    loadingCancelLabel.value = latest[1].cancelLabel ?? "放弃等待";
    loadingCancelable.value = typeof latest[1].onCancel === "function";
}

export function showLoading(state: LoadingStateInput): number {
    loadingTokenId += 1;
    activeLoadings.set(loadingTokenId, state);
    syncLoadingState();
    return loadingTokenId;
}

export function updateLoading(token: number, state: LoadingStateInput): void {
    if (!activeLoadings.has(token)) {
        return;
    }
    activeLoadings.set(token, state);
    syncLoadingState();
}

export function hideLoading(token: number): void {
    activeLoadings.delete(token);
    syncLoadingState();
}

export function cancelLatestLoading(): void {
    const entries = Array.from(activeLoadings.entries());
    const latest = entries.length > 0 ? entries[entries.length - 1] : null;
    if (!latest) {
        return;
    }
    latest[1].onCancel?.();
}

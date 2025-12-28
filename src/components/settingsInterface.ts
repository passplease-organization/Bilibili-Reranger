import { WebSocket } from "vite";
import Event = WebSocket.Event;
import type { Ref } from "vue";

type handler = (event?: Event) => void;

export default interface setCategory {
  name: string;
  settings: settingsProps[];
}
export interface button {
  click: handler;
  enter?: boolean;
}

export interface settingsProps {
  title: string;
  description: string;
  input?: {
    type?: string;
    placeholder?: string | Ref<string>;
    v_model?: string | Ref<string>;
    click?: handler;
    focusin?: handler;
    focusout?: handler;
    save: button;
    reset: button;
  };
}

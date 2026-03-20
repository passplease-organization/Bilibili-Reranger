import {type handler} from '@/component/utils/screen.ts'
import type {InputSetting} from "@/component/utils/BaseInput.vue";
import type {SelectSetting} from "@/component/utils/BaseSelector.vue";

export default interface setCategory {
  name: string;
  settings: settingsProps[];
}
export interface button {
  click: handler;
  enter?: boolean;
}

interface BaseButton {
  save?: button;
  reset?: button;
}
export interface settingsProps {
  title: string;
  description: string;
  input?: InputSetting;
  select?: SelectSetting;
  button?: BaseButton;
}

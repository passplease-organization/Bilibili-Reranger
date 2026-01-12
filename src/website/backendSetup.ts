import { SimpleESA } from "@/components/SimpleESA.ts";
import { getBackendUrl, initBackendUrl } from "@/website/index.vue";

let esa: SimpleESA = {}

export function esa(): SimpleESA{
  return esa;
}

export async function setup(): Promise<boolean> {
  initBackendUrl();
  const backend: string = getBackendUrl()
  let clientID: string|null = localStorage.key('clientID')
  let esaKey: string|null = localStorage.getItem('esaKey');
  if(esaKey && clientID){
    if((await fetch(`${backend}/test?id=${clientID}`)).ok){

    }
  }
}

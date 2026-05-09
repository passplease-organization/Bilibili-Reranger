import { SimpleESA } from "@/components/SimpleESA.ts";

export const Docker: boolean = import.meta.env.VITE_DOCKER == "true";

export const backendURL: string = "backend_url";

export interface client{
  esa: SimpleESA,
  id: string;
  valid: boolean;
}

export function setupValid(): boolean{ return valid; }

export function getESA(): SimpleESA{
  return esa;
}

export function clientID(): string{return id}

export function decrypt(body: string){
  return JSON.parse(esa.decrypt(body));
}

export function encrypt(crypted: unknown): string{
  return esa.encrypt(JSON.stringify(crypted));
}

export function getBackendUrl(): string {
  // Not end url with /
  let url = localStorage.getItem(backendURL);
  if (url) return url;
  else {
    initBackendUrl();
    url = localStorage.getItem(backendURL);
    return url ? url : "";
  }
}

export function setBackendUrl(url: string | null): void {
  if (url) localStorage.setItem(backendURL, url);
  else localStorage.removeItem(backendURL);
}

export function initBackendUrl() {
  if (Docker && !localStorage.getItem(backendURL)) setBackendUrl("/backend");
}

export async function setup(): Promise<client> {
  initBackendUrl();
  const backend: string = getBackendUrl()
  const clientID: string|null = localStorage.key('clientID')
  if (localStorage.getItem("esaKey") && clientID) {
    const esa: SimpleESA = SimpleESA.fromKey(localStorage.getItem("esaKey"));
    if (
      (await fetch(`${backend}/test?id=${clientID}`)).ok &&
      (
        await fetch(`${backend}/test?id=${clientID}`, {
          body: esa.encrypt(),
        })
      ).ok
    ){
      return {
        esa: esa,
        id: clientID,
        valid: true
      };
    }
  }
  let response : Response = await fetch(`${backend}/key`)
  if(response && response.ok){
    const { key: rsaKey } = await response.json();
    const esa = new SimpleESA();
    response = await fetch(`${backend}/key`, {
      body: {
        key: await esa.rsaEncrypt(rsaKey)
      }
    })
    if(response && response.ok){
      return {
        esa: esa,
        id: JSON.parse(esa.decrypt(await response.text())).id,
        valid: true
      };
    }
  }
  return {
    esa: null,
    id: null,
    valid: false
  };
}

const { esa: esa, id: id, valid: valid } = setup();

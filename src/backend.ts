import type { Video } from '@/components/VideoCard.vue'
import { getBackendUrl } from "@/website/backendSetup.ts";

interface CategoryNames {
  category : string[]
}

const backend = getBackendUrl();
export async function requestCategories(): Promise<string[]> {
  const response : Response = await fetch(backend + '/all_category')
  if(response.ok){
    const c : CategoryNames = JSON.parse(await response.text())
    return c.category
  }else return []
}

interface CategoryData{
  [category : string] : Video[]
}

export async function refreshCategories(name : string): Promise<Video[]>{
  const response : Response = await fetch(backend + '/?category=' + name)
  if(response.ok){
    const v : CategoryData = JSON.parse(await response.text());
    return v[name] || [];
  }else return []
}

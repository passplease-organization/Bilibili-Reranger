export interface Video {
  author: string;
  description: string;
  popups: number;
  publishTime: string;
  title: string;
  url: string;
  videoTime: string;
  videoURL: string;
  views: number;
}

export interface VideoJson {
  [category: string]: Video[]
}

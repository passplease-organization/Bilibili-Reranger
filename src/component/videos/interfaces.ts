export interface RawVideoPayload {
  [key: string]: unknown;
}

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
  raw: RawVideoPayload;
}

export interface VideoJson {
  [category: string]: RawVideoPayload[]
}

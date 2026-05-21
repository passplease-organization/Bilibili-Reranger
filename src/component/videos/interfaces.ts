export interface RawVideoPayload {
  [key: string]: unknown;
}

export interface Video {
  author: string;
  authorId: string;
  category: string;
  description: string;
  keywords: string[];
  popups: number;
  publishTime: string;
  tags: string[];
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

export interface FeedbackRequest {
  platform: string;
  video: RawVideoPayload;
  score: number;
  overall?: {
    value: number;
    score: number;
    once: boolean;
  };
  author?: {
    value: string;
    score: number;
  };
  tags?: string[];
}

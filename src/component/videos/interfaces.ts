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

export interface FeedbackWeightedValue {
  value: string;
  weight: number;
  label?: string;
}

export interface FeedbackRequest {
  platform: string;
  video: RawVideoPayload;
  score?: number;
  overall?: {
    more?: number;
    less?: number;
    favorite?: boolean;
    dislike?: boolean;
    hideOnce?: boolean;
  };
  author?: {
    value: string;
    label?: string;
    more?: number;
    less?: number;
  };
  tags?: {
    more?: FeedbackWeightedValue[];
    less?: FeedbackWeightedValue[];
  };
  category?: {
    more?: FeedbackWeightedValue[];
    less?: FeedbackWeightedValue[];
  };
  keywords?: {
    more?: FeedbackWeightedValue[];
    less?: FeedbackWeightedValue[];
  };
}

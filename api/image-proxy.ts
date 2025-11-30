import {VercelRequest,VercelResponse} from "@vercel/node";

const IMAGE_BROWSER_CACHE_SECONDS = parseInt(process.env.VITE_IMAGE_CACHE_BROWSER_SECONDS || '3600', 10);
const IMAGE_CDN_CACHE_SECONDS = parseInt(process.env.VITE_IMAGE_CACHE_CDN_SECONDS || '0', 10);

export default async function handler(request: VercelRequest, response: VercelResponse){
  try {
    // 从请求的查询参数中获取原始图片URL
    const url = request.query.url;
    const imageUrl: string = Array.isArray(url) ? url[0] : url;
    console.log(imageUrl)

    // 安全性校验：确保URL参数存在
    if (!imageUrl) {
      return response.status(400).send('Image URL is required');
    }

    // 安全性校验：确保请求的域名是我们允许的B站图片域名
    const allowedDomains = ['i0.hdslb.com', 'i1.hdslb.com', 'i2.hdslb.com'];
    const urlObject = new URL(imageUrl);
    if (!allowedDomains.includes(urlObject.hostname)) {
      return response.status(400).send('Invalid image domain');
    }

    // 使用 fetch 向B站服务器请求图片，并伪装 Referer 来绕过防盗链
    const imageResponse = await fetch(imageUrl, {
      headers: {
        'Referer': 'https://www.bilibili.com/'
      }
    });

    // 如果B站服务器返回非200状态码，则透传错误
    if (!imageResponse.ok) {
      return response.status(imageResponse.status).send(imageResponse.statusText);
    }

    // 获取图片的二进制数据
    const imageBuffer = await imageResponse.arrayBuffer();

    // 【性能核心】设置缓存头，指示浏览器和Vercel CDN缓存此响应
    // max-age -> 用户浏览器缓存时间
    // s-maxage -> 云端缓存时间
    response.setHeader('Cache-Control', `public, s-maxage=${IMAGE_CDN_CACHE_SECONDS}, max-age=${IMAGE_BROWSER_CACHE_SECONDS}, stale-while-revalidate=30`);

    // 将B站返回的 Content-Type 头设置到我们自己的响应中
    response.setHeader('Content-Type', imageResponse.headers.get('content-type') || 'image/jpeg');

    // 将图片数据作为响应体发送回前端
    return response.send(Buffer.from(imageBuffer));

  } catch (error) {
    console.error('Image proxy error:', error);
    return response.status(500).send('Internal Server Error');
  }
}

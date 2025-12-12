const IMAGE_BROWSER_CACHE_SECONDS = parseInt(
  process.env.VITE_IMAGE_CACHE_BROWSER_SECONDS || '3600',
  10
)
const IMAGE_CDN_CACHE_SECONDS = parseInt(
  process.env.VITE_IMAGE_CACHE_CDN_SECONDS || '0',
  10
)

export default async function onRequest(context){
  const url = new URL(context.url);
  // 提取 URL 各个组成部分
  const {
    href, // 完整 URL
    protocol, // 协议（如 http:）
    hostname, // 主机名（如 example.com）
    port, // 端口（如果有指定）
    pathname, // 路径（如 /path）
    search, // 查询字符串（如 ?query=123）
    hash, // 井号后面的片段（如 #section）
  } = url;
  try {
    // 从请求的查询参数中获取原始图片URL
    const imageUrl = search.startsWith('?url=') ? search.replace('?url=','') : '';
    console.log(imageUrl)
    // 安全性校验：确保URL参数存在
    if (!imageUrl) {
      return new Response('Image URL is required',{status: 400});
    }

    // 安全性校验：确保请求的域名是我们允许的B站图片域名
    const allowedDomains = ['i0.hdslb.com', 'i1.hdslb.com', 'i2.hdslb.com'];
    const urlObject = new URL(imageUrl);
    if (!allowedDomains.includes(urlObject.hostname)) {
      return new Response('Invalid image domain',{status: 400});
    }

    // 使用 fetch 向B站服务器请求图片，并伪装 Referer 来绕过防盗链
    const imageResponse = await fetch(imageUrl, {
      headers: {
        'Referer': 'https://www.bilibili.com/'
      }
    });

    // 如果B站服务器返回非200状态码，则透传错误
    if (!imageResponse.ok) {
      return new Response(imageResponse.statusText,{status: imageResponse.status});
    }

    // 获取图片的二进制数据
    const imageBuffer = await imageResponse.arrayBuffer();
    // 将图片数据作为响应体发送回前端
    return new Response(Buffer.from(imageBuffer), {
      headers: {
        'Cache-Control': `public, s-maxage=${IMAGE_CDN_CACHE_SECONDS}, max-age=${IMAGE_BROWSER_CACHE_SECONDS}, stale-while-revalidate=30`,
        'Content-Type': imageResponse.headers.get('content-type') || 'image/jpeg',
      },
      status: 200
    })

  } catch (error) {
    console.error('Image proxy error:', error);
    return new Response('Internal Server Error',{status: 500})
  }
}

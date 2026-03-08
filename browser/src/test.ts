import {fastify} from "./PluginAPI";
import {API_PORT, COOKIE, UA} from "./env";
import {login} from "./login";
import LoginRequest = login.LoginRequest;
import LoginResponse = login.LoginResponse;

require('./server')

fastify.get('/dev_test/login',async (request, reply)=>{
    console.log('测试请求捕获')
    const body : LoginRequest = {
        clientID: 'a',
        platform: 'BiliBili',
        context: {
            cookie: COOKIE,
            user_agent: UA,
        },
        platform_url: "https://www.bilibili.com/",
        screen: {
            width: 1800,
            height: 720,
            depth: 24
        }
    } as LoginRequest;
    const back: Response | void = await fetch(`http://localhost:${API_PORT}/other/login`,{
        method: 'POST',
        headers:{
            "content-type": "application/json"
        },
        body: JSON.stringify(body)
    }).catch(err=>{console.log(err)});
    const data: string = back ? `http://${request.host}${((await back.json()) as LoginResponse).login}` : "{}";
    console.log(`登录测试得到${data}`)
    reply.send(data)
})

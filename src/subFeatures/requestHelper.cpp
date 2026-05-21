#include "requestHelper.h"
#include <cpr/api.h>
#include "utils/config.h"
#include "webAPIs/frontend/PlatformFormater.h"

#ifdef TEST
    #include "utils/BilibiliInterface.h"
    #include "webAPIs/platforms.h"
#endif

#include "PortListener.h"
#include "exit.h"
#include "interface.h"
#include "../Crawler.h"
#include "../PluginHandler.h"
#include "webAPIs/browse.h"
#include "webAPIs/socialAPI.h"
#include "utils/BilibiliInterface.h"

#define LOG(URL,NAME) \
    cppUtil::say("Accept URL: " URL);\
    cppUtil::say(NAME " working for that ...");
#define RELEASE_LOG(message) \
    if (config<bool>(DETAILS)) \
        cppUtil::say(message)
#define SESSION_LOG(session) \
    RELEASE_LOG("获取到session为：" + (session));

namespace webAPI{
    socialAPI* const& getHandler(Client const* client){
        return client -> _handler;
    }
}

std::map<string,Json> allReturns{};
std::mutex allReturnsMutex{};

int get(boost::asio::ip::tcp::socket& socket) {
    LOG(GET,GET_NO_SLASH);
    REQUIRE_CLIENT(socket);
    string session;
    for (const auto& param: crawlInfo -> params) {
        if (param.key == URL_PARAMS_SESSION) {
            session = param.value;
            break;
        }
    }
    allReturnsMutex.lock();
    if (!allReturns.contains(session)) {
        cppUtil::warn("此次请求" URL_PARAMS_SESSION "不存在，传递" URL_PARAMS_SESSION ": ",session);
        allReturnsMutex.unlock();
        Json backJson{};
        backJson[SESSION_FINISHED] = true;
        backJson[SESSION_OK] = false;
        backJson[SESSION_DATA] = {};
        return back(sendMessage(socket,backJson.dump(),true));
    }
    const auto data = allReturns[session];
    if (data[SESSION_FINISHED].get<bool>()) {
        cppUtil::say("成功获取Session数据，即将删除Session");
        allReturns.erase(session);
    }
    allReturnsMutex.unlock();
    return back(sendMessage(socket,data.dump(),!data[SESSION_OK].get<bool>()));
}

inline string getSession() {
    Json data{};
    data[SESSION_FINISHED] = false;
    data[SESSION_OK] = true;
    data[SESSION_DATA] = "";
    string back;
    do {
        allReturnsMutex.unlock();
        back = randomString(32);
        allReturnsMutex.lock();
    }while (allReturns.contains(back));
    allReturns.insert({back,data});
    allReturnsMutex.unlock();
    return back;
}

#ifdef TEST
template<class Data>
bool writeSession(const string& session,const Data& data,const bool& failed,const bool& finished)
#else
template<class Data>
inline bool writeSession(const string& session,const Data& data,const bool& failed,const bool& finished)
#endif
{
    static_assert(validData<Data>,"Invalid back data type !");

    Json back{};
    back[SESSION_DATA] = data;
    back[SESSION_OK] = !failed;
    back[SESSION_FINISHED] = finished;
    lock_guard<mutex> lock(allReturnsMutex);
    allReturns[session] = back;
    return allReturns.contains(session);
}

#ifdef TEST
template bool writeSession<Json>(const string& session,const Json& data,const bool& failed,const bool& finished);
#endif

inline void sendSession(boost::asio::ip::tcp::socket& socket,const string& session) {
    Json json;
    json[URL_PARAMS_SESSION] = session;
    sendMessage(socket,json.dump());
    SESSION_LOG(session)
}

int getAllCategories(boost::asio::ip::tcp::socket& socket) {
    LOG(GET_ALL_CATEGORIES,"getAllCategories");
    if (stop -> load())
        return failed();
    auto& groups = crawlTask::getAllGroups();
    dataStore::Data data{};
    for (const auto group : groups)
        data[group -> platform].push_back(group -> name);
    const Json json = data;
    const string&& payload = json.dump();
    return back(sendMessage(socket,payload,payload.empty()));
}

int login(boost::asio::ip::tcp::socket& socket) {
    LOG(LOGIN, LOGIN_NO_SLASH);
    REQUIRE_CLIENT(socket);
    string test;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == URL_PARAMS_TEST) {
            test = param.value;
            break;
        }
    }
    if (test == "true") {
        RELEASE_LOG("测试后台登录状态");
        const auto& session = getSession();
        sendSession(socket,session);
        if (crawlInfo -> client -> handler() != nullptr) {
            if (crawlInfo -> client -> handler() -> validBrowse())
                return back(writeSession(session,"有效COOKIE"));
            return back(writeSession(session,"无效COOKIE",true));
        }
        return back(writeSession(session,"未设置平台！",true));
    }
    if (!BODY_CONTAIN(BODY_PARAMS_PLATFORM)) {
        RELEASE_LOG("获取全部平台");
        return back(sendMessage(socket,webAPI::socialAPI::allPlatform(),false));
    }
    RELEASE_LOG("登录操作");
    if (!BODY_CONTAIN(LOGIN_SCREEN_SIZE) || !INFO_BODY(LOGIN_SCREEN_SIZE).is_object())
        return back(sendMessage(socket,"不正确的Screen参数",true));
    crawlInfo -> client -> getHandler(INFO_BODY(BODY_PARAMS_PLATFORM),stop);
    if (!crawlInfo -> client -> check())
        return failed("Client login failed !");
    const auto& session = getSession();
    sendSession(socket,session);
    auto const& handler = getHandler(crawlInfo -> client);
#if !ALL_CONTAINER_ONLINE
    return back(writeSession(session,"测试成功",handler == nullptr));
#else
    bool failed = false;
    Json response;
    response["url"] = handler -> login(crawlInfo -> clientId,failed).str();
    response["success"] = !failed;
    return back(writeSession(session,response,failed));
#endif
}

int key(boost::asio::ip::tcp::socket& socket){
    LOG(KEY,KEY_NO_SLASH);
    if (BODY_CONTAIN(BODY_PARAMS_ADMIN)) {
        RELEASE_LOG("管理员登录");
        REQUIRE_CLIENT(socket);
        const string& admin = INFO_BODY(BODY_PARAMS_ADMIN);
        const auto& c = webAPI::adminLogin(crawlInfo -> client -> getID(),admin);
        if (c == nullptr) {
            sendMessage(socket,"No this admin !",true);
            return failed();
        }
        const auto& esaKey = webAPI::client(c -> getID()) -> ESAKey(admin);
        Json json;
        json[URL_PARAMS_CLIENT_ID] = c -> getID();
        json[BODY_PARAMS_ENCRYPT_KEY] = esaKey;
        #if TEST
            cppUtil::say({false, nullptr}, "ESA key: ");
            cppUtil::say(c -> ESAKey("test"));
        #endif
        return back(sendMessage(socket,json.dump()));
    }
    string key;
    if (BODY_CONTAIN(BODY_PARAMS_ENCRYPT_KEY)) {
        key = INFO_BODY(BODY_PARAMS_ENCRYPT_KEY);
    }
    if (key.empty()){
        RELEASE_LOG("获取公钥");
        Json json;
        json[BODY_PARAMS_ENCRYPT_KEY] = webAPI::getRSA().publicKey();
        return back(sendMessage(socket, to_string(json),false));
    }
    RELEASE_LOG("注册新客户端");
    const auto& id = webAPI::createAndStoreClient(key);
    if (id.empty())
        return failed("Failed on setting client ID !");
    Json json;
    json[URL_PARAMS_CLIENT_ID] = id;
    return back(sendMessage(socket, webAPI::client(id) -> encrypt(json.dump()),false));
}

int testID(boost::asio::ip::tcp::socket& socket) {
    LOG(TEST_ID,"testID");
    bool valid = false;
    if (crawlInfo -> checkClient()) {
        valid = true;
    }
    string body = valid ? "ID still valid !" : "ID not valid !";
    if (valid && BODY_CONTAIN(BODY_PARAMS_ENCRYPT_KEY)) {
        RELEASE_LOG("检查对称加密秘钥");
        valid = crawlInfo -> client -> isKey(INFO_BODY(BODY_PARAMS_ENCRYPT_KEY));
        body = valid ? "correct key" : "wrong key";
    }
    return back(sendMessage(socket, body, !valid));
}

#define GET_LOGIN_DATA_URL "/other/login/backend"
int init(boost::asio::ip::tcp::socket& socket){
    LOG(INIT, INIT_NO_SLASH);
    REQUIRE_CLIENT(socket);
    const auto& session = getSession();
    sendSession(socket,session);
    bool gather = false;
    Json json;
    if (BODY_CONTAIN("token")) {
        json["token"] = INFO_BODY("token");
        gather = true;
    }
    bool ok = true;
    if (gather) {
        cppUtil::say("收集登录信息");
        json[CLIENT_ID] = crawlInfo -> clientId;
        json[PLATFORM] = crawlInfo -> client -> handler() -> support();
        const auto&& response = cpr::Post(
            cpr::Url{browseManagerUrl,GET_LOGIN_DATA_URL},
            cpr::Header{POST_JSON_HEADER},
            cpr::Body{json.dump()}
        );
        auto&& newContext = Json::parse(response.text);
        if (SUCCESS_BROWSE_REQUEST(newContext)) {
            const auto& cookie = newContext[CONTEXT]["cookie"];
            ok = crawlInfo -> client -> setHandlerContext({
                cookie["value"].get<string>(),
                cookie["domain"].get<string>(),
                cookie.value("path", "/"),
            }) &&
                dataBase.upsertHandler(crawlInfo -> client -> handler(),crawlInfo -> clientId);
            if (ok) {
                cppUtil::say("收集登录信息成功");
                goto Prepare;
            }
        }
        cppUtil::warn("收集登录信息失败");
        ok = false;
        goto Back;
    }
    Prepare:
        ok &= crawlInfo -> client -> prepare();
    Back:
    return back(writeSession(session,ok ? "准备过程完成" : "准备过程失败", !ok));
}

int set(boost::asio::ip::tcp::socket& socket){
    LOG(SET, SET_NO_SLASH);
    REQUIRE_CLIENT(socket);
    string platform;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == BODY_PARAMS_PLATFORM){
            platform = param.value;
            break;
        }
    }
    if (!platform.empty())
        crawlInfo -> client -> getHandler(platform,stop);
    return back(sendMessage(socket,"设置成功"));
}

int plugin(boost::asio::ip::tcp::socket& socket) {
    LOG(PLUGIN,PLUGIN_NO_SLASH);
    Json data;
    {
        string name;
        bool handle = true;
        if (BODY_CONTAIN("plugin"))
            name = INFO_BODY("plugin").get<string>();
        else handle = false;
        if (handle && BODY_CONTAIN("data"))
            data = INFO_BODY("data");
        else handle = false;
        if (handle)
            return PluginHandler::handleRequest(socket,name,data);
    }
    data[PLUGIN_NO_SLASH] = *PluginHandler::pluginNames;
    return back(sendMessage(socket,data.dump()));
}

int initMyAccount(boost::asio::ip::tcp::socket& socket) {
    LOG(INIT_ACCOUNT, INIT_ACCOUNT_NO_SLASH);
    REQUIRE_CLIENT(socket);
    string platform;
    if (BODY_CONTAIN(BODY_PARAMS_PLATFORM))
        platform = INFO_BODY(BODY_PARAMS_PLATFORM).get<string>();
    auto&& formater = PluginHandler::getFormater(platform);
    if (!BODY_CONTAIN(BODY_PARAMS_NAME) || platform.empty())
        return back(sendMessage(socket,Json(formater).dump()));
    sendMessage(socket,"开始工作");
    const auto& name = INFO_BODY(BODY_PARAMS_NAME).get<string>();
    cppUtil::say("检测到工作参数，正式开始格式化当前客户端",platform,"平台推送视频，使用模板：",name);
    for (const auto& f : formater) {
        if (f == name) {
            cppUtil::say("开始格式化");
            auto worker = f.starter();
            Json json = webAPI::getController().perform(worker);
            int count = 0;
            do {
                cppUtil::say("已工作",count + 1,"次");
                if (worker = f.judger(json);worker == webAPI::nullWorker())
                    return success();
                json = webAPI::getController().perform(worker);
            }while (count++ <= config<int>(MAX_CRAWL_COUNT));
            cppUtil::warn("工作超时！最大次数",config<int>(MAX_CRAWL_COUNT),"当前: ",count);
            return failed();
        }
    }
    cppUtil::warn("未找到匹配初始化项：",name);
    return failed();
}

int feedback(boost::asio::ip::tcp::socket& socket) {
    LOG(FEEDBACK,FEEDBACK_NO_SLASH)
    REQUIRE_CLIENT(socket);
    sendMessage(socket,"开始工作");
    if (!BODY_CONTAIN(BODY_PARAMS_PLATFORM) || !BODY_CONTAIN(BODY_PARAMS_VIDEO) || !BODY_CONTAIN(BODY_PARAMS_SCORE)) {
        #ifdef DEVELOP
            cppUtil::warn("当前请求内容为",crawlInfo -> body,"格式不准确！");
        #else
            cppUtil::warn("请求格式不准确，需要包含" BODY_PARAMS_PLATFORM "、" BODY_PARAMS_VIDEO "和" BODY_PARAMS_SCORE "字段");
        #endif
        return failed();
    }
    if (const auto& platform = INFO_BODY(BODY_PARAMS_PLATFORM).get<string>();crawlInfo -> client -> handler() == nullptr || platform != crawlInfo -> client -> handler() -> support()) {
        cppUtil::warn("平台不正确！传入平台：",platform);
        return failed();
    }
    PluginHandler::allFeedBack(webAPI::formater::FeedBack{crawlInfo -> body});
    return success();
}

handler checkURL(const std::string& url) {
    if (url.starts_with(GET))
        return get;
    else if (url.starts_with(GET_ALL_CATEGORIES))
        return getAllCategories;
    else if (url.starts_with(LOGIN))
        return login;
    else if (url.starts_with(KEY))
        return key;
    else if (url.starts_with(TEST_ID))
        return testID;
    else if (url.starts_with(INIT_ACCOUNT))
        return initMyAccount;
    else if (url.starts_with(INIT))
        return init;
    else if (url.starts_with(SET))
        return ::set;
    else if (url.starts_with(PLUGIN))
        return plugin;
    else if (url.starts_with(FEEDBACK))
        return feedback;
    return nullptr;
}

handler requireClient(){
    if (crawlInfo -> checkClient())
        return nullptr;
    return [](boost::asio::ip::tcp::socket& socket) -> int{
        RELEASE_LOG("无客户端ID请求！");
        sendMessage(socket,"Need Client Id !",true);
        return failed("Illegal Client");
    };
}

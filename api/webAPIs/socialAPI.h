#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <curl/curl.h>
#include <sodium.h>

#include "../utils/config.h"
#include "../interface.h"

namespace cpr {
    class Url;
}

namespace webAPI{
    struct HandlerRow;
    struct ClientRow;
}

class CrawlerHelper;

#define POST_JSON_HEADER std::pair{"Accept","application/json"},std::pair{"Content-Type", "application/json"}

namespace webAPI {

    /*
     * Done by Gemini's help
     */
    class SimpleRSA {
    private:
        unsigned char publickey[crypto_box_PUBLICKEYBYTES]{}; // 公钥
        unsigned char secretkey[crypto_box_SECRETKEYBYTES]{}; // 私钥

    public:
        API SimpleRSA();

        API ~SimpleRSA() = default;

        API SimpleRSA& operator= (SimpleRSA& other) = delete;
        API SimpleRSA& operator= (SimpleRSA* other) = delete;

        API SimpleRSA& operator= (SimpleRSA&& other) = default;

        API [[nodiscard]] std::string decrypt(const std::string& content) const;

        API [[nodiscard]] std::string publicKey() const;

        API [[nodiscard]] bool check() const;

        API std::string static encrypt(const std::string& RSAKey,const std::string& content);

        API std::string static encryptSodium(const std::string& SodiumKey,const std::string& content);
    };

    const SimpleRSA& getRSA();

    /*
     * Done by Gemini's and ChatG help
     */
    class SimpleESA {
    private:
        unsigned char key[crypto_secretbox_KEYBYTES]{};

    public:
        API SimpleESA(const string& key, bool rawKey = false);// false means decrypted automatically
        API static std::string randomKey();

        API SimpleESA(SimpleESA&& other) noexcept;

        API ~SimpleESA() = default;

        API SimpleESA& operator= (SimpleESA& other) = delete;

        API SimpleESA& operator= (SimpleESA* other) = delete;

        API [[nodiscard]] std::string encrypt(const std::string& content) const;

        API [[nodiscard]] std::string decrypt(const std::string& content) const;

        API [[nodiscard]] bool check() const;

        API [[nodiscard]] bool is(const std::string& key) const;

        API [[nodiscard]] std::string getKey(const std::string& adminKey) const;
    };

    class socialAPI;
    typedef socialAPI* (FUNCTION_CALLER *creator)(std::shared_ptr<const std::atomic<bool>>&);

    struct BrowseWorkingContext;
    class BrowseWorker;
    class Video;
    class socialAPI {
    protected:
        std::shared_ptr<const std::atomic<bool>> stop;

        dataStore::Data _subscribers;

        BrowseWorkingContext* context;

        bool prepared = false;

        map<string,Json> storedData;
    public:
        /**
         * Should call ensureContext() after called
         * @param stop the stop sign distributed to each thread
         */
        explicit socialAPI(std::shared_ptr<const std::atomic<bool>>& stop);

        /**
         * Init handler using data
         * @param data May some rows are empty, these should be handled
         * @return succeed or not
         */
        API [[nodiscard]] virtual bool fromData(const ::webAPI::HandlerRow& data) noexcept;

        API virtual ~socialAPI() = 0;

        API [[deprecated]] virtual string login(const std::string &name, const std::string &password,bool& failed) {
            failed = true;
            return "";
        }
        API [[nodiscard]] virtual cpr::Url login(const string &clientID, bool& failed) = 0;

        API virtual BrowseWorker getWorker(Nullable const crawlTask::Task *task) const = 0;

        API [[deprecated]] virtual bool dealJson(CrawlerHelper& helper, const Json& json, const crawlTask::Task* task) const {
            return false;
        }

        API virtual bool dealJson(const Json& json, const crawlTask::Task* task, Client* const& client) const = 0;

        API [[deprecated,nodiscard]] virtual bool refreshSubscribers(CrawlerHelper& helper, bool force) const {
            return false;
        }

        API [[nodiscard]] virtual bool validBrowse() const = 0;

        API virtual void writeToDataBase(::webAPI::HandlerRow& data) const;

        API virtual void ensureContext();

        /**
         * You should set context -> domain/path in this method
         * And at called time, the context will allways be valid
         */
        API virtual void setContextDomain() = 0;

        /**
         * Get if support to log in this specific platform, and the return value will never be changed in this thread
         * @param platform Checked Supported Platform
         * @param handler *handler must be nullptr ! Or else just return *handler
         * @return Success or not
         */
        API static bool instance(socialAPI** handler,const std::string& platform,std::shared_ptr<const std::atomic<bool>>& stop);

        API static std::string allPlatform();

        API virtual void init();

        API virtual const bool &prepare() = 0;

        /**
         * Register your platform login handler
         * @param platform The supported platform
         * @param function Creator of your object, please new an object, it will be tied to this thread
         * @return succeed or not
         */
        API static bool supportPlatform(const std::string& platform,creator function);

        API [[nodiscard]] virtual std::string support() const = 0;

        API [[nodiscard]] const dataStore::Data& subscribers() const{
            return _subscribers;
        }

        API void constexpr resetTimer(const std::shared_ptr<const std::atomic<bool>>& timer) noexcept {
            stop = timer;
        }

        API [[nodiscard]] bool checkVideo(const Video& video,unsigned short& score) const;

        API [[nodiscard]] BrowseWorkingContext* const& getContext() const noexcept {
            return context;
        }

        API [[nodiscard]] bool storeData(const string& category,const Json& videoData);

        API [[nodiscard]] bool getData(const string& category,Json& data);
    };

    class Client {
    friend socialAPI* const& getHandler(Client const* client);
    friend bool storeClient(NotNull Client* client);
    private:
        std::string ID;

        SimpleESA esa;

        unordered_map<crawlTask::Task,vector<Video>> preCrawlVideos = {};

    protected:
        socialAPI* _handler;
        vector<socialAPI*> handlers;

    public:
        API static void init();

        API static void initAndDataBase();

        API static void syncWithDatabase();

        static vector<Client*> allClients();

        API explicit Client(const std::string& key);

        API Client(Client&& other) noexcept;

        API explicit Client(const ::webAPI::ClientRow& data,const bool& rawKey = false);

        API Client& operator= (Client& other) = delete;

        API Client& operator= (Client* other) = delete;

        API [[nodiscard]] bool isKey(const string& tryKey) const {
            return esa.is(tryKey);
        }

        API [[nodiscard]] std::string encrypt(const std::string& content) const {
            return esa.encrypt(content);
        }

        API [[nodiscard]] std::string decrypt(const std::string& content) const {
            return esa.decrypt(content);
        }

        API [[nodiscard]] bool checkESA(const std::string& checkedKey) const;

        API [[nodiscard]] const string& getID() const;

        API void getHandler(const std::string& platform,std::shared_ptr<const std::atomic<bool>>& stop);

        API [[nodiscard]] Nullable const socialAPI* handler() const{
            return _handler;
        }

        API bool handlerFromData(const vector<::webAPI::HandlerRow>& data) noexcept;

        API [[nodiscard]] bool setHandlerContext(const BrowseWorkingContext& context) const;

        API [[nodiscard]] bool prepare() const {
        #if ALL_CONTAINER_ONLINE
            return _handler && _handler -> prepare();
        #else
            return true;
        #endif
        }

        API [[nodiscard]] bool check() const noexcept;

        API [[nodiscard]] std::string ESAKey(const std::string& adminKey) const;

        API void constexpr resetTimer(const std::shared_ptr<const std::atomic<bool>>& timer) const noexcept {
            _handler -> resetTimer(timer);
        }

        API [[nodiscard]] bool storeData(const string& category,const Json& videoData) const {
            return _handler != nullptr && _handler -> storeData(category,videoData);
        }

        API [[nodiscard]] bool getData(const string& category,Json& data) const {
            return _handler != nullptr && _handler -> getData(category,data);
        }

        /**
        * Clear data, prepare for next pre-crawl
        */
        API void nextPreCrawl();

        API void syncPreCrawlDataBase();

        API [[nodiscard]] bool crawlEnough(Nullable crawlTask::Task const* const& task) const;

        API bool storeVideo(const Video& video,const crawlTask::Task& task,const unsigned short& score);

        API [[nodiscard]] vector<Video> getVideos(crawlTask::Task const* const& task,unsigned int offset = 0) const;
    };

    API Nullable Client* client(const std::string& ID);

    API bool storeClient(NotNull Client* client);

    API std::string createAndStoreClient(const std::string& key);

    API const Client* adminLogin(const string& id,const std::string& adminKey);

    class CurlHelper {
    private:
        static size_t saveData(char *data, size_t size, size_t member, void *userdata);
    public:
        CurlHelper();

        virtual ~CurlHelper();
    protected:
        CURL *curl;

        Json json = Json();

        string tempData;

        string url;

        void clear(){
            json.clear();
            tempData.clear();
        }

        virtual bool dealJson(){return false;}

        virtual bool dealJson(const Json& _json) {
            clear();
            this -> json = _json;
            auto&& back = dealJson();
            clear();
            return back;
        }

        void clearURL() {
            url = "";
        }
    public:
        [[nodiscard]] CURL* getCurl() const{
            return curl;
        }

        [[nodiscard]] const string& nextURL() const {
            return url;
        }

        virtual void setURL(const string& url) {
            this -> url = url;
        }

        virtual bool connect(bool deal);
        virtual bool connect(){
            return connect(false);
        }

        virtual const Json& getJson() noexcept {
            if (!tempData.empty()) {
                try {
                    json = Json::parse(tempData);
                }catch(...) {
                    if (config<bool>(DETAILS)) {
                        cppUtil::say("Json格式错误！当前：");
                        cppUtil::say(tempData);
                    }
                    json = Json();
                }
            }
            return json;
        }

        virtual void curlSetup();
    };

    class AutoCurlHelper : public CurlHelper {
        typedef bool (FUNCTION_CALLER* dealer)(Json& json);
    public:
        const dealer deal;

        AutoCurlHelper(const dealer& _deal) : deal(_deal) {}

        ~AutoCurlHelper() override = default;

    protected:
        bool dealJson() override;
    };
}

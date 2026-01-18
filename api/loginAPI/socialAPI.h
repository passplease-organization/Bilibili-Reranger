#pragma once

#include <string>
#include <cpr/response.h>
#include <sodium.h>

#include "../config.h"
#include "../interface.h"

class CrawlerHelper;

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

        API std::string static encrypt(const std::string& key,const std::string& content);
    };

    const SimpleRSA& getRSA();

    /*
     * Done by Gemini's help
     */
    class SimpleESA {
    private:
        unsigned char key[crypto_secretbox_KEYBYTES]{};

    public:
        API SimpleESA(const string& key);

        API SimpleESA(SimpleESA&& other) noexcept;

        API ~SimpleESA() = default;

        API SimpleESA& operator= (SimpleESA& other) = delete;

        API SimpleESA& operator= (SimpleESA* other) = delete;

        API [[nodiscard]] std::string encrypt(const std::string& content) const;

        API [[nodiscard]] std::string decrypt(const std::string& content) const;

        API [[nodiscard]] bool check() const;

        API [[nodiscard]] std::string getKey(const std::string& adminKey) const;
    };

    class socialAPI;
    typedef socialAPI* (FUNCTION_CALLER *creator)(std::shared_ptr<const std::atomic<bool>>&);

    class socialAPI {
    protected:
        std::shared_ptr<const std::atomic<bool>> stop;

        dataStore::Data _subscribers;

    public:
        socialAPI(std::shared_ptr<const std::atomic<bool>>& stop);

        API virtual ~socialAPI() = 0;

        API virtual string login(const std::string &name, const std::string &password,bool& failed) = 0;

        API virtual string getURL(const crawlTask::Task* task) const = 0;

        API virtual bool dealJson(CrawlerHelper& helper, const Json& json, const crawlTask::Task* task) const = 0;

        API virtual bool refreshSubscribers(CrawlerHelper& helper, bool force) const = 0;

        API [[nodiscard]] virtual bool validCOOKIE() const = 0;

        /**
         * Get if support to log in this specific platform, and the return value will never be changed in this thread
         * @param platform Checked Supported Platform
         * @param handler *handler must be nullptr ! Or else just return *handler
         * @return Success or not
         */
        API static bool instance(socialAPI** handler,const std::string& platform,std::shared_ptr<const std::atomic<bool>>& stop);

        API static std::string allPlatform();

        API void init();

        API bool prepare();

        /**
         * Register your platform login handler
         * @param platform The supported platform
         * @param function Creator of your object, please new an object, it will be tied to this thread
         * @return succeed or not
         */
        API static bool supportPlatform(const std::string& platform,creator function);

        /**
         * Called when main function needs
         */
        API [[nodiscard]] const string virtual &getCOOKIE() const = 0;

        API [[nodiscard]] virtual std::string support() const = 0;

        API [[nodiscard]] const dataStore::Data& subscribers() const{
            return _subscribers;
        }

        API void constexpr resetTimer(const std::shared_ptr<const std::atomic<bool>>& timer) noexcept {
            stop = timer;
        }
    };

    class Client {
    friend socialAPI* getHandler(Client const* client);
    friend bool storeClient(NotNull Client* client);
    private:
        std::string ID;

        SimpleESA esa;

    protected:
        socialAPI* _handler;
        vector<socialAPI*> handlers;

    public:
        API static void init();

        API Client(const std::string& key);

        API Client(Client&& other) noexcept;

        API Client& operator= (Client& other) = delete;

        API Client& operator= (Client* other) = delete;

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

        API [[nodiscard]] bool prepare() const {
            return _handler -> prepare();
        }

        API [[nodiscard]] bool check() const noexcept;

        API [[nodiscard]] std::string ESAKey(const std::string& adminKey) const;

        API void constexpr resetTimer(const std::shared_ptr<const std::atomic<bool>>& timer) noexcept {
            _handler -> resetTimer(timer);
        }
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
                        say("Json格式错误！当前：");
                        say(tempData.c_str());
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

        bool dealJson() override;
    };
}

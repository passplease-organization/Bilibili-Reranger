#pragma once

#include <string>
#include <cpr/response.h>
#include <sodium.h>
#include "../interface.h"

namespace webAPI {

    /*
     * Done by Gemini's help
     */
    class SimpleRSA {
    private:
        unsigned char publickey[crypto_box_PUBLICKEYBYTES]{}; // 公钥
        unsigned char secretkey[crypto_box_SECRETKEYBYTES]{}; // 私钥

    public:
        SimpleRSA();

        ~SimpleRSA() = default;

        SimpleRSA& operator= (SimpleRSA& other) = delete;
        SimpleRSA& operator= (SimpleRSA* other) = delete;

        SimpleRSA& operator= (SimpleRSA&& other) = default;

        [[nodiscard]] std::string decrypt(const std::string& content) const;

        [[nodiscard]] std::string publicKey() const;
    };

    const SimpleRSA& getRSA();

    /*
     * Done by Gemini's help
     */
    class SimpleESA {
    private:
        unsigned char key[crypto_secretbox_KEYBYTES]{};
    public:
        SimpleESA(const string& key);

        ~SimpleESA() = default;

        SimpleESA& operator= (SimpleESA& other) = delete;

        SimpleESA& operator= (SimpleESA* other) = delete;

        SimpleESA& operator= (SimpleESA&& other) = default;

        [[nodiscard]] std::string encrypt(const std::string& content) const;

        [[nodiscard]] std::string decrypt(const std::string& content) const;
    };

    SimpleESA& getESA();

    class socialAPI;
    typedef socialAPI* (FUNCTION_CALLER *creator)(std::shared_ptr<const std::atomic<bool>>&);

    class socialAPI {
    protected:
        std::shared_ptr<const std::atomic<bool>>& stop;

    public:
        socialAPI(std::shared_ptr<const std::atomic<bool>>& stop);

        virtual ~socialAPI() = 0;

        virtual bool login(const std::string& name,const std::string& password) = 0;

        /**
         * Get if support to log in this specific platform, and the return value will never be changed in this thread
         * @param platform Checked Supported Platform
         * @param handler *handler must be nullptr ! Or else just return *handler
         * @return Instance which support, or else, null
         */
        static socialAPI* instance(socialAPI** handler,const std::string& platform,std::shared_ptr<const std::atomic<bool>>& stop);

        static std::string allPlatform();

        void init();

        /**
         * Register your platform login handler
         * @param platform The supported platform
         * @param function Creator of your object, please new an object, it will be tied to this thread
         * @return succeed or not
         */
        static bool supportPlatform(const std::string& platform,creator function);

        virtual cpr::Response requestVerificationCode() = 0;

        /**
         * Called when main function needs
         */
        [[nodiscard]] std::string virtual getCOOKIE() const = 0;
    };

    class Client {
    private:
        string ID;

        const SimpleESA esa;
    protected:
        socialAPI* _handler;
    public:
        Client(const string& key);

        Client& operator= (Client& other) = delete;

        Client& operator= (Client* other) = delete;

        Client& operator= (Client&& other) = default;

        [[nodiscard]] std::string encrypt(const std::string& content) const {
            return esa.encrypt(content);
        };

        [[nodiscard]] std::string decrypt(const std::string& content) const {
            return esa.decrypt(content);
        };

        [[nodiscard]] const string& getID() const;

        [[nodiscard]] const socialAPI* handler() const{
            return _handler;
        }
    };

    Client* client();

    void storeClient(Client* client);
}

#pragma once

#include <string>
#include <cpr/response.h>
#include "../interface.h"
#include <openssl/rsa.h>
#include <openssl/evp.h>

namespace webAPI {

    class SimpleRSA {
    private:
        std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pKey;

    public:
        SimpleRSA();

        ~SimpleRSA() = default;

        void operator= (const SimpleRSA& other);

        string decrypt(const string& content);

        string publicKey();
    };

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
         * @return Instance which support, or else, null
         */
        static socialAPI* instance(const std::string& platform,std::shared_ptr<const std::atomic<bool>>& stop);

        /**
         * Get existed handler
         * @return existed handler
         */
        static socialAPI* instance();

        static string allPlatform();

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
        std::string virtual getCOOKIE() = 0;

    protected:
        /**
         * Decrypt messages from frontend
         * @return decrypted message
         */
        string decrypt(const string& content);

    private:
        /**
         * Encrypt messages transported between frontend and backend
         * @return public key
         */
        const string& encrypt(const string& content);

        SimpleRSA rsa;
    };
}

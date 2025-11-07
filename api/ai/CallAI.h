#include "../APIStatus.h"
#include "string"
#include "../BilibiliInterface.h"

extern "C"{
    /**
     * Only support OpenAI format, for gemini, you can use <a href="https://github.com/PublicAffairs/openai-gemini">this repository</a> to proxy that
     * */
    class AI{
    private:
        std::string url;
        std::string key;
    public:
        std::string model;
        float temperature;
        int maxTokens;

        API AI(const char* url,const char* key);

        [[nodiscard]] API const std::string& getURLS() const;

        [[nodiscard]] API const char* getURL() const;

        [[nodiscard]] API const std::string& getKeyS() const;

        [[nodiscard]] API const char* getKey() const;

        API void setURLS(const std::string& url);

        API void setURL(const char* url);

        API void setKeyS(const std::string& key);

        API void setKey(const char* key);

        API void setModel(const char* model);

        [[nodiscard]] API const char* getModel() const;

        API const char* toString(const char* msg, const char* message_system);

        API const char* toString(bilibili::Video& video);
    };
/**
 * @return empty means fails
 * */
    API const char* request(AI& ai, const char* message, const char* system = nullptr);

    API const char* introduceVideo(AI& ai, bilibili::Video& video);
}
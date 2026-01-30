#pragma once

#include <string>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

namespace webAPI {
    enum struct BrowseDataMode {
        DOM,
        HTTP_REQUEST
    };

    struct BrowseWorkingContext {
        std::string url;
        std::string cookie;
        std::string ua;
    };

    class BrowseWorker {
    public:
        BrowseDataMode mode;
        const BrowseWorkingContext* &context;
    };

    class BrowseController {
    private:
        std::string browseIP;
    public:
        explicit BrowseController(std::string ip);

        Json preform(const BrowseWorker& worker);
    };

    BrowseController& getController();
}

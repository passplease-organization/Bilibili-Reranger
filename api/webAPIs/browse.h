#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "../APIStatus.h"

namespace cpr {
    class Url;
}

bool checkEnv();

extern "C"{
    void throwError(const char*);
    void warn(const char*,bool);
}

using Json = nlohmann::json;

namespace webAPI {
    class ElementSelector final {
    public:
        enum struct SelectMode {
            ID,
            CLASS,
            TAG,
            TAGNAME
        };
    private:
        SelectMode mode;
        std::string param;
        unsigned int index;
    public:
        ElementSelector(const SelectMode mode,std::string param,const unsigned int index = 0):
        mode(mode),param(std::move(param)),index(index){}

        inline static std::string modeToString(const SelectMode& mode) {
            switch (mode) {
                case SelectMode::ID: return "ID";
                case SelectMode::CLASS: return "CLASS";
                case SelectMode::TAG: return "TAG";
                case SelectMode::TAGNAME: return "TAGNAME";
                default: return "";
            }
        }

        [[nodiscard]] Json toJson() const {
            Json json;
            json["mode"] = modeToString(mode);
            json["param"] = param;
            json["index"] = index;
            return json;
        }

        bool operator==(const ElementSelector &other) const = default;
    };

    struct BrowseWorkingContext {
        std::string cookie;
        std::string ua;

        static BrowseWorkingContext EMPTY;

        bool operator==(const BrowseWorkingContext &other) const {
            return other.cookie == cookie && other.ua == ua;
        }

        Json toJson() {
            Json json;
            json["cookie"] = cookie;
            json["user_agent"] = ua;
            return json;
        }
    };

    #define validWorkerData(data) data.is_array()
    #define getWorkerData(data,index) data[index]

    class BrowseAction {
    protected:
        [[nodiscard]] virtual Json _toJson() const = 0;
    public:
        enum struct BrowseDataMode {
            DOM,
            HTTP_REQUEST,
            OTHER,
            NODATA
        };

        BrowseAction() = default;
        virtual ~BrowseAction() = 0;

        bool virtual operator==(const BrowseAction &) const = default;

        [[nodiscard]] Json toJson() const;
        #define ACTION_FLAG "type"
        #define ACTION_DATA "info"

        inline static std::string modeToString(const BrowseDataMode& mode) {
            switch (mode) {
                case BrowseDataMode::DOM: return "DOM";
                case BrowseDataMode::HTTP_REQUEST: return "HTTP_REQUEST";
                case BrowseDataMode::OTHER: return "JS";
                default: return "";
            }
        }

        [[nodiscard]] virtual const std::string& name() const = 0;
    };

    #define ActionsFlag "workers"
    class BrowseWorker {
    public:
        BrowseWorkingContext* const& context;
        std::vector<std::shared_ptr<BrowseAction>> actions;

        template<class... Action>
        BrowseWorker(BrowseWorkingContext* const& context,Action&&... actions):
        context(context) {
            static_assert(((std::is_base_of_v<BrowseAction,std::decay_t<Action>> || std::convertible_to<Action,std::shared_ptr<BrowseAction>>) && ...),"Action must inherit BrowseAction");
            this -> actions.reserve(sizeof...(Action));
            auto add = [this]<typename T0>(T0&& a) {
                using U = std::decay_t<T0>;
                if constexpr (std::convertible_to<U,std::shared_ptr<BrowseAction>>) {
                    this->actions.emplace_back(std::forward<T0>(a));
                } else {
                    this->actions.emplace_back(std::make_shared<U>(std::forward<T0>(a)));
                }
            };
            (add(std::forward<Action>(actions)), ...);
        }

        void* operator new(std::size_t size) = delete;
        void* operator new[](std::size_t size) = delete;
        void operator delete(void* ptr) = delete;
        void operator delete[](void* ptr) = delete;

        bool operator ==(const BrowseWorker& worker) const {
            return (context == worker.context || *context == *worker.context) && actions == worker.actions;
        }

        BrowseWorker& operator =(const BrowseWorker& other) {
            *context = *other.context;
            actions = other.actions;
            return *this;
        }

        [[nodiscard]] bool valid() const;
    };

    const BrowseWorker& nullWorker();

    #define CLIENT_ID "clientID"
    #define PLATFORM "platform"
    #define CONTEXT "context"
    #define OTHER_MODE "mode"

    #define ANSWER_SATUS "ok"
    #define SUCCESS_BROWSE_REQUEST(back) (back.contains(ANSWER_SATUS) && (back)[ANSWER_SATUS].get<bool>())
    #define ANSWER_ERROR "error"
    #define ANSWER_DATA "back"
    class BrowseController {
    friend const BrowseController& getController();
    friend bool ::checkEnv();
    private:
        const cpr::Url* browseIP;
        static BrowseController controller;
    public:
        explicit BrowseController(const std::string& ip);

        ~BrowseController();

        [[nodiscard]] bool testConnection() const;

        [[nodiscard]] Json perform(const BrowseWorker& worker) const;

        [[nodiscard]] bool closeWorker(Nullable BrowseWorkingContext* const& context) const;
        #define CLOSE_WORKER "closeWorker"

        [[nodiscard]] bool testContext(Nullable BrowseWorkingContext* const& context) const;
        #define TEST_CONTEXT "testContext"

        [[nodiscard]] cpr::Url openBridge(const std::string& clientID,cpr::Url url) const;
        #define OPEN_BRIDGE "login"
    };

    inline const BrowseController& getController() {
        return BrowseController::controller;
    }

    class UrlAction : public BrowseAction {
    private:
        static std::string _name;
        std::string url;
    protected:
        [[nodiscard]] Json _toJson() const override;
    public:
        explicit UrlAction(std::string url) : BrowseAction(), url(std::move(url)) {}

        ~UrlAction() override = default;

        bool operator==(const UrlAction &other) const = default;

        [[nodiscard]] const std::string &name() const override {
            return _name;
        }
    };

    class ClickAction : public BrowseAction {
    private:
        static std::string _name;
    protected:
        ElementSelector selector;

        [[nodiscard]] Json _toJson() const override;
    public:
        explicit ClickAction(ElementSelector selector) : BrowseAction(), selector(std::move(selector)) {}

        template<class... Args>
            requires std::constructible_from<ElementSelector,Args ...>
        explicit ClickAction(Args&& ... args) : BrowseAction(), selector(std::forward<Args>(args)...) {}

        ~ClickAction() override = default;

        bool operator==(const ClickAction &other) const = default;

        [[nodiscard]] const std::string& name() const override{
            return _name;
        }
    };

    class CrawlAction : public BrowseAction {
    private:
        static std::string _name;
    protected:
        BrowseDataMode mode;
        Json description;

        [[nodiscard]] Json _toJson() const override {
            return description;
        }
    public:
        inline CrawlAction(const BrowseDataMode mode,const std::string& description) :
            CrawlAction(mode,easyDescribe(mode,description)){}
        template<class WorkingData>
            requires std::convertible_to<WorkingData,Json>
        CrawlAction(const BrowseDataMode mode,WorkingData&& description):
            BrowseAction(),mode(mode),description(Json(std::forward<WorkingData>(description))) {
            if (validDescription(mode,this -> description)) {
                warn("Description: ",false);
                warn(this -> description.dump().c_str(),true);
                throwError("Invalid description of information gathering instuction !");
            }
        }
        ~CrawlAction() override = default;

        #define CRAWL_DATAMODE "data_mode"
        #define CRAWL_TARGET "target"
        static bool validDescription(const BrowseDataMode& mode,const Json& json);
        static Json easyDescribe(const BrowseDataMode& mode,const std::string& description);

        [[nodiscard]] const std::string& name() const override{
            return _name;
        }
    };

    #define validWhileData(data) validWorkerData(data)
    #define forEachWhileDataHelper(data,name,...) for(const auto& name : data)
    #define forEachWhileData(...) forEachWhileDataHelper(__VA_ARGS__,_crawlData)
    #define DoWhileMode "failOrSucceeded"
    #define DoWhileMAXWorkCount "maxCount"
    #define DoWhileAllActions ActionsFlag
    class DoWhileAction : public BrowseAction {
    private:
        static std::string _name;
    protected:
        bool failOrSucceeded;
        int maxCount;
        std::vector<std::shared_ptr<BrowseAction>> actions;

        [[nodiscard]] Json _toJson() const override;
    public:
        template<class... Action>
        explicit DoWhileAction(const bool& failOrSucceeded,const int& maxCount = INT32_MAX,Action&& ... actions) :
            BrowseAction(),failOrSucceeded(failOrSucceeded),maxCount(maxCount) {
            static_assert(((std::is_base_of_v<BrowseAction,std::decay_t<Action>> || std::convertible_to<std::shared_ptr<BrowseAction>,std::decay_t<Action>>) && ...),"Not right class type, which should be base of BrowseAction");
            this -> actions.reserve(sizeof...(Action));
            auto add = [this]<typename A0>(A0 a){
                using A = std::decay_t<A0>;
                if constexpr (std::is_base_of_v<BrowseAction,std::decay_t<A>>) {
                    this -> actions.push_back(std::make_shared<A>(std::forward<A0>(a)));
                }else {
                    this -> actions.push_back(std::make_shared<BrowseAction>(std::forward<A0>(a)));
                }
            };
            (add(actions), ...);
        }

        template<class... Action>
        inline explicit DoWhileAction(const bool& failOrSucceeded,Action&& ... actions): DoWhileAction(failOrSucceeded,INT32_MAX,actions...){}

        ~DoWhileAction() override = default;

        [[nodiscard]] const std::string &name() const override {
            return _name;
        }
    };
}

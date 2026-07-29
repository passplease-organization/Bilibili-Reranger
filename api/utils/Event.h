#pragma once
#include "../interface.h"
#include "../APIStatus.h"

namespace Event {
    class Event {
    public:
        virtual ~Event() = 0;

        virtual std::string description() const noexcept = 0;

        virtual bool cancellable() const noexcept = 0;

        virtual void cancel() = 0;
    };
    inline Event::~Event() = default;

    class NextPreCrawlGroupEvent : public Event {
    public:
        ~NextPreCrawlGroupEvent() override = default;

        std::string description() const noexcept override {
            return "下一组预爬取事件，已完成一组任务爬取，即将进入下一组";
        }

        constexpr bool cancellable() const noexcept override {
            return false;
        }

        void cancel() override {
            cppUtil::throwError("NextPreCrawlGroupEvent事件不得取消！");
        }
    };

    class NextPreCrawlClientEvent : public Event {
    public:

        ~NextPreCrawlClientEvent() override = default;

        std::string description() const noexcept override {
            return "下一个客户端预爬取事件，已完成一个客户端任务爬取，即将进入下一个客户端";
        }

        constexpr bool cancellable() const noexcept override {
            return false;
        }

        void cancel() override {
            cppUtil::throwError("NextPreCrawlClientEvent事件不得取消！");
        }
    };

    class PrePareEvent : public Event {
    public:
        const dataStore::Data& subscribers;

        explicit PrePareEvent(const dataStore::Data& _subscribers): subscribers(_subscribers) {}

        std::string description() const noexcept override {
            return "handler初始化事件";
        }

        bool cancellable() const noexcept override {
            return false;
        }

        void cancel() override {
            cppUtil::throwError("PrePareEvent事件不得取消！");
        }
    };

    API void exportEvent(Event* const& event);
    template <class E>
    requires (!std::is_pointer_v<E>)
    inline API void exportEvent(E& event) {
        static_assert(std::is_base_of_v<Event, E>, "E must be derived from Event");
        exportEvent(&event);
    }
    template <class E>
    requires (!std::is_pointer_v<E>)
    inline API void exportEvent(E event) {
        static_assert(std::is_base_of_v<Event, E>, "E must be derived from Event");
        exportEvent(&event);
    }

    /**
     * Due to .so and handler is in main program, this function is just a bridge
     * @param exporter Only for main program to set exporter
     */
    void setExporter(void(FUNCTION_CALLER* exporter)(Event* const&));
}
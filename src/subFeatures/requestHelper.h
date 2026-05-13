#pragma once

#include "develop/flags.h"

#include "interface.h"
#include <boost/asio/ip/tcp.hpp>

typedef int (FUNCTION_CALLER *handler)(boost::asio::ip::tcp::socket& socket);
#define GET_ALL_CATEGORIES_NO_SLASH "all_category"
#define GET_ALL_CATEGORIES "/" GET_ALL_CATEGORIES_NO_SLASH
#define LOGIN_NO_SLASH "login"
#define LOGIN "/" LOGIN_NO_SLASH
#define KEY_NO_SLASH "key"
#define KEY "/" KEY_NO_SLASH
#define TEST_ID_NO_SLASH "test"
#define TEST_ID "/" TEST_ID_NO_SLASH
#define INIT_NO_SLASH "init"
#define INIT "/" INIT_NO_SLASH
#define SET_NO_SLASH "set"
#define SET "/" SET_NO_SLASH
#define GET_NO_SLASH "get"
#define GET "/" GET_NO_SLASH
#define PLUGIN_NO_SLASH "plugin"
#define PLUGIN "/" PLUGIN_NO_SLASH

#define NEED_NORMAL_HANDLE -1
handler checkURL(const std::string& url);

Nullable handler requireClient();

#define REQUIRE_CLIENT(socket) \
    if(const auto& handler = requireClient(); handler != nullptr) \
        return handler(socket);

#define SESSION_DATA "data"
#define SESSION_FINISHED "finished"
#define SESSION_OK "ok"

template<class Data>
concept validData = requires(Json& back,Data&& data){
    {back = data} -> std::same_as<Json&>;
};

API string getSession();

template<class Data>
API bool writeSession(const string& session,const Data& data,const bool& failed = false,const bool& finished = true);

API void sendSession(boost::asio::ip::tcp::socket& socket,const string& session);
#pragma once

#include "develop/flags.h"

#if NEED_PORT
#include "interface.h"
#include "../Crawler.h"
#include <boost/asio/ip/tcp.hpp>

void dealParams(CrawlerHelper& helper);

typedef int (FUNCTION_CALLER *handler)(boost::asio::ip::tcp::socket& socket);
#define GET_ALL_CATEGORIES_NO_SLASH "all_category"
#define GET_ALL_CATEGORIES "/" GET_ALL_CATEGORIES_NO_SLASH
#define SET_COOKIE_NO_SLASH "set_cookie"
#define SET_COOKIE "/" SET_COOKIE_NO_SLASH
#define LOGIN_NO_SLASH "login"
#define LOGIN "/" LOGIN_NO_SLASH
#define KEY_NO_SLASH "key"
#define KEY "/" KEY_NO_SLASH
#define TEST_ID_NO_SLASH "test"
#define TEST_ID "/" TEST_ID_NO_SLASH

#define NEED_NORMAL_HANDLE -1
handler checkURL(const std::string& url);
#endif
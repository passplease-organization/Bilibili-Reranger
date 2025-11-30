#pragma once

#include "../develop/flags.h"

#if NEED_PORT
#include "interface.h"
#include "../Crawler.h"
#include <boost/asio/ip/tcp.hpp>

void dealParams(CurlHelper& helper);

typedef int (FUNCTION_CALLER *handler)(boost::asio::ip::tcp::socket& socket);
#define GET_ALL_CATEGORIES_NO_SLASH "all_category"
#define GET_ALL_CATEGORIES "/" GET_ALL_CATEGORIES_NO_SLASH
#define SET_COOKIE_NO_SLASH "set_cookie"
#define SET_COOKIE "/" SET_COOKIE_NO_SLASH

#define NEED_NORMAL_HANDLE -1
handler checkURL(const std::string& url);
#endif
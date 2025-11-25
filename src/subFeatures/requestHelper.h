#pragma once

#include "../develop/flags.h"

#if NEED_PORT
#include "interface.h"
#include <boost/asio/ip/tcp.hpp>

typedef int (FUNCTION_CALLER *handler)(boost::asio::ip::tcp::socket& socket);
#define GET_ALL_CATEGORIES "/all_category"

handler checkURL(const std::string& url);
#endif
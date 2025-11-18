#pragma once

#include <boost/asio.hpp>

void startWork();

void sendMessage(boost::asio::ip::tcp::socket& socket);
#pragma once

#include "../develop/flags.h"

#if NEED_PORT
#include <atomic>

extern std::atomic<bool> testFinished;

void startTestThread();

void test();
#endif
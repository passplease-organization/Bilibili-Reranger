#pragma once

#ifdef TEST
#include "develop/flags.h"
#include <atomic>

extern std::atomic<bool> testFinished;

void startTestThread();

void test();
#endif
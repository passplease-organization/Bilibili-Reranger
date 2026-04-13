#pragma once

#include "develop/flags.h"
#include <atomic>

extern std::atomic<bool> testFinished;

void startTestThread();

void test();

#pragma once
#include <atomic>

extern std::atomic<bool> testFinished;

void startTestThread();

void test();
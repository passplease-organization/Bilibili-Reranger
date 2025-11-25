#pragma once

#include "develop/flags.h"
#include "Util.h"
#if NEED_PORT
    #include "PortListener.h"
#endif

void clean();

inline int success() {
#if NEED_PORT
    say("Thread: ",false);
    say(std::to_string(crawlInfo -> id).c_str());
    say("运行成功，本线程即将结束");
#else
    say("运行成功，现在将退出程序！");
#endif
    clean();
    return 0;
}

inline int failed() {
#if NEED_PORT
    if (stop -> load())
        warn("运行超时，本线程自动退出");
#endif
    warn("运行失败，请检查具体原因！");
    clean();
    return 1;
}

inline int back(const bool& back) {
    return back ? success() : failed();
}
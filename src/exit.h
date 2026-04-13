#pragma once

#include "develop/flags.h"
#include "Util.h"
#include "PortListener.h"

void clean();

inline int success() {
    say("Thread: ",false);
    say(std::to_string(crawlInfo -> id).c_str());
    say("运行成功，本线程即将结束");
    clean();
    return 0;
}

inline int failed(const std::string& msg = "") {
    if (!msg.empty())
        warn(msg.c_str());
    if (stop -> load())
        warn("运行超时，本线程自动退出");
    warn("运行失败，请检查具体原因！");
    clean();
    return 1;
}

inline int back(const bool& back) {
    if (!back && config<bool>(DETAILS))
        say("操作失败！");
    return back ? success() : failed();
}

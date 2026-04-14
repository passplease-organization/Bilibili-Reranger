#pragma once

#include "develop/flags.h"
#include "Util.h"
#include "PortListener.h"

void clean();

inline int success() {
    cppUtil::say({false, nullptr}, "Thread: ");
    cppUtil::say(crawlInfo -> id);
    cppUtil::say("运行成功，本线程即将结束");
    clean();
    return 0;
}

inline int failed(const std::string& msg = "") {
    if (!msg.empty())
        cppUtil::warn(msg);
    if (stop -> load())
        cppUtil::warn("运行超时，本线程自动退出");
    cppUtil::warn("运行失败，请检查具体原因！");
    clean();
    return 1;
}

inline int back(const bool& back) {
    if (!back && config<bool>(DETAILS))
        cppUtil::say("操作失败！");
    return back ? success() : failed();
}

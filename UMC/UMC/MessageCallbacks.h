#pragma once
#include <string>
#include <functional>

using fnMessageCallBack = std::function<bool(std::string_view)>;

struct MessageCallBacks
{
    fnMessageCallBack Error;
    fnMessageCallBack Warning;
    fnMessageCallBack Info;
};

#pragma once
#include <string>
#include <functional>

using fnMessageCallBack = std::function<void(std::string_view)>;

struct MessageCallBacks
{
    fnMessageCallBack Error;
    fnMessageCallBack Warning;
    fnMessageCallBack Message;
    fnMessageCallBack Info;
    fnMessageCallBack Debug;
};

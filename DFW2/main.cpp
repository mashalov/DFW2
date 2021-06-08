﻿
#include <cstdio>
#include <iostream>

typedef long int __int64;

#include "Header.h"
#include "DynaModel.h"


int main()
{
    DFW2::CDynaModel Network;
    
    try
    {
        Network.DeSerialize("/home/eugene/projects/DFW2/DFW2/lf.json");
        Network.RunLoadFlow();
    }
    catch (dfw2error& err)
    {
        Network.Log(DFW2::CDFW2Messages::DFW2LOG_FATAL, fmt::format("Ошибка : {}", err.what()));
        Network.Serialize("/home/eugene/projects/DFW2/DFW2/lf.json");
    }
 
    return 0;
}
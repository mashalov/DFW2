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
        Network.CheckFolderStructure();
        Network.Automatic().CompileModels();
        Network.AutomaticDevice.ConnectDLL(Network.Automatic().GetModulePath());
	    Network.AutomaticDevice.CreateDevices(1);
	    Network.AutomaticDevice.BuildStructure();

        Network.RunLoadFlow();
    }
    catch (dfw2error& err)
    {
        Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_FATAL, fmt::format("Ошибка : {}", err.what()));
        Network.Serialize("/home/eugene/projects/DFW2/DFW2/lf.json");
    }
 
    return 0;
}
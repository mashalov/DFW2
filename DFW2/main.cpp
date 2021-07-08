
#include <cstdio>
#include <iostream>

typedef long int __int64;

#include "Header.h"
#include "DynaModel.h"

using namespace DFW2;

int main()
{
    CDynaModel::DynaModelParameters parameters;
    parameters.m_bDisableResultsWriter = true;

    DFW2::CDynaModel Network(parameters);
    
    try
    {
        Network.DeSerialize("/home/eugene/Русский тест/Raiden/lf_1500.json");
		Network.RunTransient();
    }
    catch (dfw2error& err)
    {
        Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_FATAL, fmt::format("Ошибка : {}", err.what()));
        Network.Serialize("/home/eugene/projects/DFW2/DFW2/lf.json");
    }
 
    return 0;
}
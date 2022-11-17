// BatchTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Test.h"

int main()
{
    CBatchTest test;
    test.AddCase(L"E:\\Temp\\sztest\\РМ_mdp_debug_1_111");
    test.AddCase(L"E:\\Temp\\sztest\\РМ_mdp_debug_1_111_027440");
    test.AddCase(L"E:\\Temp\\sztest\\РМ_mdp_debug_1_111_m004560");
    test.AddCase(L"E:\\Temp\\sztest\\РМ_mdp_debug_1_111_m005560");
    test.AddContingency(L"E:\\Temp\\sztest\\102_1ф.КЗ с УРОВ на КАЭС (откл. КАЭС - Княжегубская №1).dfw");
    test.Run();
}

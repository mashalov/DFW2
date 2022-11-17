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
    test.AddContingency(L"E:\\Temp\\sztest\\102_1ф.КЗ с УРОВ на КАЭС (откл. КАЭС - Княжегубская №1).scn");
    test.AddContingency(L"E:\\Temp\\sztest\\103_1ф.КЗ с УРОВ на ПС 330 кВ Княжегубская (с откл. Княжегубская-Лоухи №2 и АТ-1).scn");
    test.AddContingency(L"E:\\Temp\\sztest\\105_1ф.КЗ с УРОВ ВЛ-392-2 на ПС 330кВ Кондопога (откл.ВЛ 330кВ Каменный Бор – Кондопога и ВЛ 330кВ Кондопога – Петрозаводск).scn");
    test.AddContingency(L"E:\\Temp\\sztest\\106_БЕЗ ШУНТА ПС 330 кВ Петрозаводск (с откл.Кондопога-Петрозаводск).scn");
    test.AddContingency(L"E:\\Temp\\sztest\\109_ 1ф. КЗ с УРОВ на КАЭС с отключением ВЛ 330 кВ Кольская АЭС - Мончегорск №2.scn");
    test.Run();
}

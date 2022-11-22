// BatchTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Test.h"

// разная оценка неусточивости
//test.AddCase(L"E:\\Temp\\sztest\\РМ_mdp_debug_1_111_m004560");
//test.AddContingency(L"E:\\Temp\\sztest\\109_ 1ф. КЗ с УРОВ на КАЭС с отключением ВЛ 330 кВ Кольская АЭС - Мончегорск №2.scn");
int main(int argc, char* argv[])
{
    SetConsoleOutputCP(65001);
    try
    {
        std::filesystem::path parametersPath("config.json");
        if (argc > 1)
            parametersPath = argv[1];
        CBatchTest test(parametersPath);
        test.Run();
    }
    catch (const std::runtime_error& ex)
    {
        std::cout << "Ошибка: " << ex.what();
    }
}

// CompareResults.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Result.h"
int main()
{
    try
    {
        CoInitialize(NULL);
        CLog log;
        CResult result1(log), result2(log);
        result1.Load("c:\\users\\masha\\documents\\Русский тест\\Raiden\\Results\\binresultcom.rst");
        //result1.Load("c:\\tmp\\000001.sna");
        result2.Load("c:\\tmp\\000002.sna");
        result1.Compare(result2, { 4.8, 5.0 });

        auto plot{ result1.GetPlot(16, 10, "P") };
        auto denseplot{ plot.DenseOutput(0.01) };
        denseplot.WriteCSV("c:\\tmp\\testcsv.csv");
        plot.WriteCSV("c:\\tmp\\testcsv2.csv");

    }
    catch (dfw2error& err)
    {
        std::cout << err.what();
    }
    
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

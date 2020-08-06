#pragma once
#include"..\DFW2\ICustomDevice.h"
namespace DFW2
{
    class CCustomDevice : public ICustomDevice
    {
    protected:
        VariableIndex A3, A4, A5, A6, A7, A8, L1, L2, L3, L4, LT1, LT2, LT3, LT4;
        VariableIndexRefVec m_StateVariables = { A3, A4, A5, A6, A7, A8, L1, L2, L3, L4,
                                                 LT1, LT2, LT3, LT4 };
        std::vector<std::reference_wrapper<double>> m_ConstantVariables = {  };
        VariableIndexExternalRefVec m_ExternalVariables = {  };
        PRIMITIVEVECTOR m_Primitives = { { PBT_RELAYDELAYLOGIC, _T(""),  { { 0, LT2 } },  { { 0, L2 } } },
                                         { PBT_RELAYDELAYLOGIC, _T(""),  { { 0, LT1 } },  { { 0, L1 } } },
                                         { PBT_RELAYDELAYLOGIC, _T(""),  { { 0, LT3 } },  { { 0, L3 } } },
                                         { PBT_RELAYDELAYLOGIC, _T(""),  { { 0, LT4 } },  { { 0, L4 } } } };
        DOUBLEVECTOR m_BlockParameters;
    public:
        const VariableIndexRefVec& GetVariables() override { return m_StateVariables; }
        const VariableIndexExternalRefVec& GetExternalVariables() override { return m_ExternalVariables; }
        const PRIMITIVEVECTOR& GetPrimitives() override { return m_Primitives; }
        void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) override
        {
            DeviceProps.SetType(DEVTYPE_AUTOMATIC);
            DeviceProps.SetClassName(_T("Automatic & scenario"), _T("Automatic"));
            DeviceProps.AddLinkFrom(DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER);
            DeviceProps.m_VarMap = { { _T("A3"), { 0, VARUNIT_NOTSET } }, { _T("A4"), { 1, VARUNIT_NOTSET } },
                                          { _T("A5"), { 2, VARUNIT_NOTSET } }, { _T("A6"), { 3, VARUNIT_NOTSET } },
                                          { _T("A7"), { 4, VARUNIT_NOTSET } }, { _T("A8"), { 5, VARUNIT_NOTSET } },
                                          { _T("L1"), { 6, VARUNIT_NOTSET } }, { _T("L2"), { 7, VARUNIT_NOTSET } },
                                          { _T("L3"), { 8, VARUNIT_NOTSET } }, { _T("L4"), { 9, VARUNIT_NOTSET } },
                                          { _T("LT1"), { 10, VARUNIT_NOTSET } }, { _T("LT2"), { 11, VARUNIT_NOTSET } },
                                          { _T("LT3"), { 12, VARUNIT_NOTSET } }, { _T("LT4"), { 13, VARUNIT_NOTSET } } };
            DeviceProps.m_ExtVarMap = {  };
            DeviceProps.m_ConstVarMap = {  };
            DeviceProps.nEquationsCount = DeviceProps.m_VarMap.size();
        }
        bool SetSourceConstant(size_t Index, double Value) override
        {
            if (Index < m_ConstantVariables.size())
            {
                m_ConstantVariables[Index] = Value;
                return true;
            }
            else
                return false;
        }
        void SetConstsDefaultValues() override
        {
        }
        const DOUBLEVECTOR& GetBlockParameters(ptrdiff_t nBlockIndex) override
        {
            m_BlockParameters.clear();
            switch (nBlockIndex)
            {
            case 1:
                m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.15 };
                break;
            case 0:
                m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.55 };
                break;
            case 2:
                m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.5 };
                break;
            case 3:
                m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.1 };
                break;
            }
            return m_BlockParameters;
        }
        void BuildRightHand(CCustomDeviceData& CustomDeviceData) override
        {
            CustomDeviceData.SetFunction(A8, A8);
            CustomDeviceData.SetFunction(A7, A7);
            CustomDeviceData.SetFunction(A6, A6 - 1e-07);
            CustomDeviceData.SetFunction(A5, A5);
            CustomDeviceData.SetFunction(A4, A4 - 1);
            CustomDeviceData.SetFunction(A3, A3);
            CustomDeviceData.SetFunction(L3, L3 - 1);
            // LT3-relay(L3) is for host block 
            CustomDeviceData.SetFunction(L4, L4 - LT3);
            // LT4-relay(L4) is for host block 
            CustomDeviceData.SetFunction(L1, L1 - 1);
            // LT1-relay(L1) is for host block 
            CustomDeviceData.SetFunction(L2, L2 - LT1);
            // LT2-relay(L2) is for host block 
        }
        void BuildDerivatives(CCustomDeviceData& CustomDeviceData) override
        {
        }
        void BuildEquations(CCustomDeviceData& CustomDeviceData) override
        {
            CustomDeviceData.SetElement(A8, A8, 1);   // A8 = 0 by A8
            CustomDeviceData.SetElement(A7, A7, 1);   // A7 = 0 by A7
            CustomDeviceData.SetElement(A6, A6, 1);   // A6-1e-07 = 0 by A6
            CustomDeviceData.SetElement(A5, A5, 1);   // A5 = 0 by A5
            CustomDeviceData.SetElement(A4, A4, 1);   // A4-1 = 0 by A4
            CustomDeviceData.SetElement(A3, A3, 1);   // A3 = 0 by A3
            CustomDeviceData.SetElement(L3, L3, 1);   // L3-1 = 0 by L3
            CustomDeviceData.SetElement(L4, L4, 1);   // L4-LT3 = 0 by L4
            CustomDeviceData.SetElement(L4, LT3, -1);   // L4-LT3 = 0 by LT3
            CustomDeviceData.SetElement(L1, L1, 1);   // L1-1 = 0 by L1
            CustomDeviceData.SetElement(L2, L2, 1);   // L2-LT1 = 0 by L2
            CustomDeviceData.SetElement(L2, LT1, -1);   // L2-LT1 = 0 by LT1
        }
        eDEVICEFUNCTIONSTATUS Init(CCustomDeviceData& CustomDeviceData) override
        {
            A8 = 0;
            A7 = 0;
            A6 = 1e-07;
            A5 = 0;
            A4 = 1;
            A3 = 0;
            L3 = 1;
            CustomDeviceData.InitPrimitive(2); // LT3 = relay(L3)
            L4 = LT3;
            CustomDeviceData.InitPrimitive(3); // LT4 = relay(L4)
            L1 = 1;
            CustomDeviceData.InitPrimitive(0); // LT1 = relay(L1)
            L2 = LT1;
            CustomDeviceData.InitPrimitive(1); // LT2 = relay(L2)
            return eDEVICEFUNCTIONSTATUS::DFS_OK;
        }
        eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData) override
        {
            A8 = 0;
            A7 = 0;
            A6 = 1e-07;
            A5 = 0;
            A4 = 1;
            A3 = 0;
            L3 = 1;
            CustomDeviceData.ProcessPrimitiveDisco(2); // LT3 = relay(L3)
            L4 = LT3;
            CustomDeviceData.ProcessPrimitiveDisco(3); // LT4 = relay(L4)
            L1 = 1;
            CustomDeviceData.ProcessPrimitiveDisco(0); // LT1 = relay(L1)
            L2 = LT1;
            CustomDeviceData.ProcessPrimitiveDisco(1); // LT2 = relay(L2)
            return eDEVICEFUNCTIONSTATUS::DFS_OK;
        }
        void Destroy() override { delete this; }
        static inline constexpr const char* zipSource = "H4sIAAAAAAAAAMtNzMzjqubyMVSwVTDk8gkB0UWpOYmVGj6GOgoGegYgwtRUk8vHCCgDlAeqMUKoMYKr"
            "MQSpMYaaYoxQYYwwBajABGyIMVCJCUKJCcIQTS5HkF4DLkcTsFGOphCeGYjnqmvO5WgOEbAAU7VcAKEl"
            "cLjAAAAA";
    };
}

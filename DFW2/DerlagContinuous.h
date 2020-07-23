#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	// �������-���������������� �����
	class CDerlagContinuous : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
		VariableIndex& m_Y2;
	public:
		// ���� �������� ������ ��������� � ����� ��������. ������ ����� ������������ ��������������� ������� ��������������� ���������
		CDerlagContinuous(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitive(Device, OutputVariable, Input, ExtraOutputVariables), m_Y2(ExtraOutputVariables.begin()->get()) {}
		virtual ~CDerlagContinuous() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		void SetTK(double T, double K)  { m_K = K;  m_T = T; }
		const _TCHAR* GetVerbalName() override { return _T("��� �� ������������"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagContinuous); }
		static long EquationsCount()  { return 2; }
	};

	/*
	class CDerlagNordsieck : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
	public:
		VariableIndex 
		CDerlagNordsieck(CDevice *pDevice, VariableIndex& OutputVariable, std::initializer_list<PrimitiveVariableBase*> Input) :
			CDynaPrimitive(pDevice, OutputVariable, Input), m_Dummy1(pOutput + 1), m_Dummy2(pOutput + 2) {}
		virtual ~CDerlagNordsieck() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		//virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		void SetTK(double T, double K) { m_K = K;  m_T = T; }
		const _TCHAR* GetVerbalName() override { return _T("�� Nordsieck"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagNordsieck); }
		static long EquationsCount() { return 3; }
	};
	*/


#define CDerlag CDerlagContinuous
//#define CDerlag CDerlagNordsieck

}


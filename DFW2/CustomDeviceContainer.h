#pragma once
#include "DeviceContainer.h"
#include "CustomDeviceDLL.h"

#include "LimitedLag.h"
#include "LimiterConst.h"
#include "Relay.h"
#include "RSTrigger.h"
#include "DerlagContinuous.h"
#include "Comparator.h"
#include "AndOrNot.h"
#include "Abs.h"
#include "ZCDetector.h"
#include "DeadBand.h"
#include "Expand.h"


namespace DFW2
{

	// ��� ������ ��� ����-������
	struct PrimitivePoolElement
	{
		unsigned char *m_pPrimitive;			// ������ ����
		unsigned char *m_pHead;					// ������� ������� � ����
		size_t nCount;							// ���������� ��������� � ����
		PrimitivePoolElement() : m_pPrimitive(nullptr), 
							     nCount(0) {}
	};

	// �������� ����-�����
	struct PrimitiveInfo
	{
		size_t nSize;							// ������ � ������
		long nEquationsCount;					// ���������� ���������
		PrimitiveInfo(size_t Size, long EquationsCount) : nSize(Size),
														  nEquationsCount(EquationsCount) {}
	};

	class CCustomDeviceContainer : public CDeviceContainer
	{
	protected:
		CCustomDeviceDLL m_DLL;
		// ���� ��� ���� ��������� ����������
		PrimitivePoolElement m_PrimitivePool[PrimitiveBlockType::PBT_LAST];			// ������� ����� ��� ������� ���� ����-������
		std::vector<PrimitiveVariable> m_PrimitiveVarsPool;							// ��� ����������
		std::vector<PrimitiveVariableExternal> m_PrimitiveExtVarsPool;				// ��� ������� ���������� (�� �������� ������ ������)
		std::vector<ExternalVariable> m_ExternalVarsPool;							// ��� ������� ���������� ����������
		std::vector<double> m_DoubleVarsPool;										// ��� double - ���������� 
		ExternalVariable *m_pExternalVarsHead;
		double *m_pDoubleVarsHead;

		size_t m_nBlockEquationsCount;												// ���������� ���������� ��������� ����-������
		size_t m_nPrimitiveVarsCount;												// ���������� ���������� ���������� ����������
		size_t m_nDoubleVarsCount;													// ���������� ����������� ������ ���������� double-����������
		size_t m_nExternalVarsCount;												// ���������� ������� ����������

		void CleanUp();
		std::vector<double> m_ParameterBuffer;
	public:
		CCustomDeviceContainer(CDynaModel *pDynaModel);
		bool ConnectDLL(const _TCHAR *cszDLLFilePath);
		virtual ~CCustomDeviceContainer();
		bool BuildStructure();
		bool InitDLLEquations(BuildEquationsArgs *pArgs);
		bool BuildDLLEquations(BuildEquationsArgs *pArgs);
		bool BuildDLLRightHand(BuildEquationsArgs *pArgs);
		bool BuildDLLDerivatives(BuildEquationsArgs *pArgs);
		bool ProcessDLLDiscontinuity(BuildEquationsArgs *pArgs);

		PrimitiveInfo GetPrimitiveInfo(PrimitiveBlockType eType);
		size_t PrimitiveSize(PrimitiveBlockType eType);
		long PrimitiveEquationsCount(PrimitiveBlockType eType);
		PrimitiveVariable* NewPrimitiveVariable(ptrdiff_t nIndex, double& Value);
		PrimitiveVariableExternal* NewPrimitiveExtVariables();
		double* NewDoubleVariables();
		ExternalVariable* NewExternalVariables();
		void* NewPrimitive(PrimitiveBlockType eType);
		const CCustomDeviceDLL& DLL() { return m_DLL; }
		long GetParametersValues(ptrdiff_t nId, BuildEquationsArgs* pArgs, long nBlockIndex, double **ppParameters);
		size_t GetInputsCount()				{  return m_DLL.GetInputsInfo().size();     }
		size_t GetConstsCount()				{  return m_DLL.GetConstsInfo().size();     }
		size_t GetSetPointsCount()			{  return m_DLL.GetSetPointsInfo().size();  }
	};
}


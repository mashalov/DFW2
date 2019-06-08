#pragma once
#include "DynaNode.h"
#include "klu.h"
#include "klu_version.h"
#include "cs.h"

namespace DFW2
{
	class CDynaModel;
	class CLoadFlow
	{
	public:

		// "�����������" ����� ��� ����. �������� ����� ��������� ���������� ����� ��� ���������
		// ���������� ������������ ������ ������������� Y. ��� ��� ��������� - ��� ��� �����
		// ����� ���������� �������� ���� � ����������� ��� ������� �����
		struct _VirtualBranch
		{
			cplx Y;
			CDynaNodeBase *pNode;
		};

		// ������� ���� � ������ �������
		struct _MatrixInfo
		{
			size_t nRowCount;														// ���������� ��������� � ������ �������
			size_t nBranchCount;													// ���������� ����������� ������ �� ���� (������� ��)
			CDynaNodeBase *pNode;													// ����, � �������� ��������� ������ Info
			_VirtualBranch *pBranches;												// ������ ����������� ������ ����
			ptrdiff_t m_nPVSwitchCount;												// ������� ������������ PV-PQ
			double m_dImbP, m_dImbQ;												// ��������� �� P � Q
			_MatrixInfo::_MatrixInfo() : nRowCount(0), 
										 nBranchCount(0), 
										 m_nPVSwitchCount(0)
										 {}
		};

		struct _MaxNodeDiff
		{
			_MatrixInfo *m_pMatrixInfo;
			double m_dDiff;
			_MaxNodeDiff() : m_pMatrixInfo(nullptr),
							 m_dDiff(0.0)
							{}

			ptrdiff_t GetId()
			{
				if (m_pMatrixInfo && m_pMatrixInfo->pNode)
					return m_pMatrixInfo->pNode->GetId();
				return -1;
			}

			double GetDiff()
			{
				if (GetId() >= 0)
					return m_dDiff;
				return -1.0;
			}
		};

		struct _IterationControl
		{
			_MaxNodeDiff m_MaxImbP;
			_MaxNodeDiff m_MaxImbQ;
			_MaxNodeDiff m_MaxV;
			_MaxNodeDiff m_MinV;

			_IterationControl()
							{}
		};

		struct Parameters
		{
			double m_Imb;							// ���������� �������� ��������
			bool m_bFlat;							// ������� �����
			double m_dSeidellStep;					// ��� ��������� ������ �������	
			ptrdiff_t m_nEnableSwitchIteration;		// ����� ��������, � ������� ����������� ������������ PV-PQ
			Parameters() : m_Imb(1E-4),
						   m_dSeidellStep(1.0),
						   m_nEnableSwitchIteration(2)
						   {}
		};


		CLoadFlow(CDynaModel *pDynaModel);
		~CLoadFlow();
		bool Run();
	protected:
		void CleanUp();
		bool Estimate();
		bool Seidell();
		bool BuildMatrix();
		bool Start();

		static bool NodeInMatrix(CDynaNodeBase *pNode);
				
		CDynaModel *m_pDynaModel;
		CDynaNodeContainer *pNodes;
		size_t m_nMatrixSize;
		size_t m_nNonZeroCount;
		size_t m_nBranchesCount;
		

		double *Ax;				// ������ ������� �����
		double *b;				// ������ ������ �����
		ptrdiff_t *Ai;			// ������ �����
		ptrdiff_t *Ap;			// ������ ��������

		_MatrixInfo *m_pMatrixInfo;
		_MatrixInfo *m_pMatrixInfoEnd;
		_MatrixInfo *m_pMatrixInfoSlackEnd;
		_VirtualBranch *m_pVirtualBranches;

		KLU_symbolic *Symbolic;
		KLU_common Common;

		Parameters m_Parameters;
		_IterationControl m_IterationControl;

		void ResetIterationControl();
		void UpdateIterationControl(_MatrixInfo *pMatrixInfo);
		void DumpIterationControl();

	};
}


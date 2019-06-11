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

		struct Parameters
		{
			double m_Imb;							// ���������� �������� ��������
			bool m_bFlat;							// ������� �����
			bool m_bStartup;						// ��������� ����� �������
			double m_dSeidellStep;					// ��� ��������� ������ �������	
			ptrdiff_t m_nSeidellIterations;			// ���������� �������� ��������
			ptrdiff_t m_nEnableSwitchIteration;		// ����� ��������, � ������� ����������� ������������ PV-PQ
			ptrdiff_t m_nMaxIterations;				// ������������ ���������� �������� ��������
			Parameters() : m_Imb(1E-4),
						   m_dSeidellStep(1.05),
						   m_bStartup(true),
						   m_nEnableSwitchIteration(2),
						   m_nSeidellIterations(7), 
						   m_nMaxIterations(100)
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
		bool CheckLF();

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
		static bool SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs);
		void AddToQueue(_MatrixInfo *pMatrixInfo, QUEUE& queue);
		void GetNodeImb(_MatrixInfo *pMatrixInfo);
	};
}


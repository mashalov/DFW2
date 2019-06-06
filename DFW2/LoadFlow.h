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

		// ������� ���� � ������ �������
		struct _MatrixInfo
		{
			size_t nRowCount;														// ���������� ��������� � ������ �������
			_MatrixInfo::_MatrixInfo() : nRowCount(0) {}
		};


		CLoadFlow(CDynaModel *pDynaModel);
		~CLoadFlow();
		bool Run();
	protected:
		void CleanUp();
		bool Estimate();
				
		CDynaModel *m_pDynaModel;
		CDynaNodeContainer *pNodes;
		bool SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double& dValue, bool bAdd = false);
		size_t m_nMatrixSize;
		size_t m_nNonZeroCount;
		

		double *Ax;				// ������ ������� �����
		double *b;				// ������ ������ �����
		ptrdiff_t *Ai;			// ������ �����
		ptrdiff_t *Ap;			// ������ ��������

		_MatrixInfo *m_pMatrixInfo;

		KLU_symbolic *Symbolic;
		KLU_common Common;
	};
}


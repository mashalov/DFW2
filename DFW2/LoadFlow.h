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
			_MatrixInfo::_MatrixInfo() : nRowCount(0), nBranchCount(0) {}
		};


		CLoadFlow(CDynaModel *pDynaModel);
		~CLoadFlow();
		bool Run();
	protected:
		void CleanUp();
		bool Estimate();
		bool BuildMatrix();
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
	};
}


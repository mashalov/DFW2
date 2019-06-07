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

		// "виртуальная" ветвь для узла. Заменяет собой настоящую включенную ветвь или несколько
		// включенных параллельных ветвей эквивалентным Y. Эти два параметра - все что нужно
		// чтобы рассчитать небаланс узла и производные для смежных узлов
		struct _VirtualBranch
		{
			cplx Y;
			CDynaNodeBase *pNode;
		};

		// маппинг узла в строки матрица
		struct _MatrixInfo
		{
			size_t nRowCount;														// количество элементов в строке матрицы
			size_t nBranchCount;													// количество виртуальных ветвей от узла (включая БУ)
			CDynaNodeBase *pNode;													// узел, к которому относится данное Info
			_VirtualBranch *pBranches;												// список виртуальных ветвей узла
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
		

		double *Ax;				// данные матрицы якоби
		double *b;				// вектор правой части
		ptrdiff_t *Ai;			// номера строк
		ptrdiff_t *Ap;			// номера столбцов

		_MatrixInfo *m_pMatrixInfo;
		_MatrixInfo *m_pMatrixInfoEnd;
		_MatrixInfo *m_pMatrixInfoSlackEnd;
		_VirtualBranch *m_pVirtualBranches;

		KLU_symbolic *Symbolic;
		KLU_common Common;
	};
}


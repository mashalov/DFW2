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
			ptrdiff_t m_nPVSwitchCount;												// счетчик переключений PV-PQ
			double m_dImbP, m_dImbQ;												// небалансы по P и Q
			bool bVisited;															// признак просмотра для графовых алгоритмов
			_MatrixInfo::_MatrixInfo() : nRowCount(0),
				nBranchCount(0),
				m_nPVSwitchCount(0),
				bVisited(false)
			{}
		};

		typedef vector<_MatrixInfo*> MATRIXINFO;
		typedef MATRIXINFO::iterator MATRIXINFOITR;
		typedef list<_MatrixInfo*> QUEUE;

		class _MaxNodeDiff
		{
		protected:
			_MatrixInfo *m_pMatrixInfo;
			double m_dDiff;
			typedef bool (OperatorFunc)(double lhs, double rhs);

			void UpdateOp(_MatrixInfo* pMatrixInfo, double dValue, OperatorFunc OpFunc)
			{
				if (pMatrixInfo && pMatrixInfo->pNode)
				{
					if (m_pMatrixInfo)
					{
						_ASSERTE(m_pMatrixInfo->pNode);
						if (OpFunc(dValue, m_dDiff))
						{
							m_pMatrixInfo = pMatrixInfo;
							m_dDiff = dValue;
						}
					}
					else
					{
						m_pMatrixInfo = pMatrixInfo;
						m_dDiff = dValue;
					}
				}
				else
					_ASSERTE(pMatrixInfo && pMatrixInfo->pNode);
			}

		public:
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

			void UpdateMin(_MatrixInfo *pMatrixInfo, double Value)
			{
				UpdateOp(pMatrixInfo, Value, [](double lhs, double rhs) -> bool { return lhs < rhs; });
			}

			void UpdateMax(_MatrixInfo *pMatrixInfo, double Value)
			{
				UpdateOp(pMatrixInfo, Value, [](double lhs, double rhs) -> bool { return lhs > rhs; });
			}

			void UpdateMaxAbs(_MatrixInfo *pMatrixInfo, double Value)
			{
				UpdateOp(pMatrixInfo, Value, [](double lhs, double rhs) -> bool { return fabs(lhs) > fabs(rhs) ; });
			}
		};

		struct _IterationControl
		{
			_MaxNodeDiff m_MaxImbP;
			_MaxNodeDiff m_MaxImbQ;
			_MaxNodeDiff m_MaxV;
			_MaxNodeDiff m_MinV;
			ptrdiff_t m_nQviolated;
			_IterationControl() :m_nQviolated(0)
							{}

			bool Converged(double m_dToleratedImb)
			{
				return fabs(m_MaxImbP.GetDiff()) < m_dToleratedImb && fabs(m_MaxImbQ.GetDiff()) < m_dToleratedImb && m_nQviolated == 0;
			}
		};

		struct Parameters
		{
			double m_Imb;							// допустимый небаланс мощности
			bool m_bFlat;							// плоский старт
			bool m_bStartup;						// стартовый метод Зейделя
			double m_dSeidellStep;					// шаг ускорения метода Зейделя	
			ptrdiff_t m_nSeidellIterations;			// количество итераций Зейделем
			ptrdiff_t m_nEnableSwitchIteration;		// номер итерации, с которой разрешается переключение PV-PQ
			ptrdiff_t m_nMaxIterations;				// максимальное количество итераций Ньютоном
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

		Parameters m_Parameters;
		_IterationControl m_IterationControl;

		void ResetIterationControl();
		void UpdateIterationControl(_MatrixInfo *pMatrixInfo);
		void DumpIterationControl();
		static bool SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs);
		void AddToQueue(_MatrixInfo *pMatrixInfo, QUEUE& queue);
		void GetNodeImb(_MatrixInfo *pMatrixInfo);
	};
}


#pragma once
#include "DynaNode.h"
#include "klu.h"
#include "klu_version.h"
#include "cs.h"
#include "dfw2exception.h"

class KLUWrapperData
{
protected:
	unique_ptr<double[]>	pAx,			// данные матрицы якоби
							pb;				// вектор правой части
	unique_ptr<ptrdiff_t[]> pAi,			// номера строк
							pAp;			// номера столбцов
	ptrdiff_t m_nMatrixSize;
	ptrdiff_t m_nNonZeroCount;

	template <typename T>
	class KLUCommonDeleter
	{
		T *m_p;
		KLU_common& Common;
		void CleanUp(KLU_numeric*)
		{
			KLU_free_numeric(&m_p, &Common);
		}
		void CleanUp(KLU_symbolic*)
		{
			KLU_free_symbolic(&m_p, &Common);
		}

	public:
		KLUCommonDeleter(T* p, KLU_common& common) : m_p(p), Common(common) {}
		virtual ~KLUCommonDeleter()
		{
			if (m_p)
				CleanUp(m_p);
		}
		T* GetKLUObject() { return m_p; }

	};

	using KLUSymbolic = KLUCommonDeleter<KLU_symbolic>;
	using KLUNumeric = KLUCommonDeleter<KLU_numeric>;
	unique_ptr<KLUSymbolic> pSymbolic;
	unique_ptr<KLUNumeric>  pNumeric;
	KLU_common pCommon;
public:
	KLUWrapperData()
	{
		KLU_defaults(&pCommon);
	}
	void SetSize(ptrdiff_t nMatrixSize, ptrdiff_t nNonZeroCount)
	{
		m_nMatrixSize = nMatrixSize;
		m_nNonZeroCount = nNonZeroCount;
		pAx = make_unique<double[]>(m_nNonZeroCount);				// числа матрицы
		pb	= make_unique<double[]>(m_nMatrixSize);					// вектор правой части
		pAi = make_unique<ptrdiff_t[]>(m_nMatrixSize + 1);			// строки матрицы
		pAp = make_unique<ptrdiff_t[]>(m_nNonZeroCount);			// столбцы матрицы
	}
	ptrdiff_t MatrixSize() { return m_nMatrixSize; }
	ptrdiff_t NonZeroCount() { return m_nNonZeroCount; }
	double* Ax()		{ return pAx.get(); }
	double* B()			{ return pb.get(); }
	ptrdiff_t* Ai()		{ return pAi.get(); }
	ptrdiff_t* Ap()		{ return pAp.get(); }
	KLU_symbolic* Symbolic() { return pSymbolic->GetKLUObject(); }
	KLU_common* Common() { return &pCommon; }
	void Analyze()
	{
		pSymbolic = make_unique<KLUSymbolic>(KLU_analyze(m_nMatrixSize, pAi.get(), pAp.get(), &pCommon), pCommon);
	}
	void Solve()
	{
		if (!pSymbolic)
			Analyze();
		pNumeric = make_unique<KLUNumeric>(KLU_factor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), &pCommon), pCommon);
		if (pNumeric->GetKLUObject())
		{
			ptrdiff_t solveRes = KLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon);
		}
	}
};


namespace DFW2
{
	class CDynaModel;
	class CLoadFlow
	{
	public:
		struct StoreParameters
		{
			CDynaNodeBase *m_pNode;
			double LFQmin;
			double LFQmax;
			CDynaNodeBase::eLFNodeType LFNodeType;
			StoreParameters(CDynaNodeBase *pNode);
			void Restore();
		};

		typedef list<StoreParameters> SUPERNODEPARAMETERSLIST;

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

		SUPERNODEPARAMETERSLIST m_SuperNodeParameters;

		void GetPnrQnr(CDynaNodeBase *pNode);
		void GetPnrQnrSuper(CDynaNodeBase *pNode);
		void CleanUp();
		bool Estimate();
		bool Seidell();
		bool BuildMatrix();
		bool Start();
		bool CheckLF();
		void UpdateQToGenerators();					// обновление данных генераторов по результату расчета PV-узлов
		void UpdatePQFromGenerators();				// обновление данных PV-узлов по исходным данным генераторов
		void DumpNodes();

		// возвращает true если узел учитывается в матрице якоби
		static bool NodeInMatrix(CDynaNodeBase *pNode);
				
		CDynaModel *m_pDynaModel = nullptr;
		CDynaNodeContainer *pNodes = nullptr;

		KLUWrapperData	klu;
		unique_ptr<_MatrixInfo[]> m_pMatrixInfo;				// вектор узлов отнесенных к строкам матрицы якоби
		_MatrixInfo *m_pMatrixInfoEnd;			// конец вектора узлов PV-PQ в якоби
		_MatrixInfo *m_pMatrixInfoSlackEnd;		// конец вектора узлов с учетом базисных

		Parameters m_Parameters;
		// определение порядка PV узлов для Зейделя
		static bool SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs);
		void AddToQueue(_MatrixInfo *pMatrixInfo, QUEUE& queue);
		void GetNodeImb(_MatrixInfo *pMatrixInfo);
	};
}


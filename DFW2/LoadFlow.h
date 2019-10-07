#pragma once
#include "DynaNode.h"
#include "KLUWrapper.h"

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


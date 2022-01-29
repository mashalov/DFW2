#pragma once
#include "DynaNode.h"
#include "KLUWrapper.h"

namespace DFW2
{
	class CDynaModel;
	class CLoadFlow
	{
	public:

		enum class eLoadFlowFormulation
		{
			Power,
			Current,
			Tanh
		};

		struct LoadFlowParameters
		{
			double m_Imb = 1E-4;						// допустимый небаланс мощности
			bool m_bFlat = false;						// плоский старт
			bool m_bStartup = true;						// стартовый метод Зейделя
			double m_dSeidellStep = 1.0;				// шаг ускорения метода Зейделя	
			ptrdiff_t m_nSeidellIterations = 17;		// количество итераций Зейделем
			ptrdiff_t m_nEnableSwitchIteration = 2;		// номер итерации, с которой разрешается переключение PV-PQ
			ptrdiff_t m_nMaxIterations = 100;			// максимальное количество итераций Ньютоном
			ptrdiff_t m_nMaxPVPQSwitches = 5;			// максимальное количество переключений PV-PQ
			ptrdiff_t m_nPVPQSwitchPerIt = 10;			// количество переключений типов узлов на одной итерации
			double m_dVoltageNewtonStep = 0.3;			// максимальное относительное приращение шага Ньютона по напряжению
			double m_dNodeAngleNewtonStep = 1.5;		// максимальное приращение шага Ньютона по углу узла
			double m_dBranchAngleNewtonStep = 0.5;		// максимальное приращение шага Ньютона по углу связи
			double ForceSwitchLambda = 1e-2;			// шаг Ньютона, меньше которого бэктрэк выключается и выполняется переключение типов узлов
			eLoadFlowFormulation m_LFFormulation = eLoadFlowFormulation::Current;	// способ представления модели УР
			bool m_bAllowNegativeLRC = true;			// разрешить учет СХН для узлов с отрицательной нагрузкой
			double m_dVdifference = 1E-4;				// порог сравнения модуля напряжения
			double m_dLRCMinSlope = 0.0;				// минимальная крутизна СХН
			double m_dLRCMaxSlope = 5.0;				// максимальная крутизна СХН
		};

		CLoadFlow(CDynaModel *pDynaModel);
		bool Run();
	protected:

		void GetPnrQnr(CDynaNodeBase *pNode);
		void GetPnrQnrSuper(CDynaNodeBase *pNode);
		void AllocateSupernodes();
		void Estimate();
		void Seidell();
		void SeidellTanh();
		void Newton();
		void NewtonTanh();
		double GetNewtonRatio();
		void UpdateVDelta(double dStep = 1.0);
		void BuildMatrixPower();
		void BuildMatrixCurrent();
		void BuildMatrixTanh();
		void Start();
		bool CheckLF();
		void SolveLinearSystem();
		void UpdateQToGenerators();					// обновление данных генераторов по результату расчета PV-узлов
		void UpdatePQFromGenerators();				// обновление данных PV-узлов по исходным данным генераторов
		void UpdateSupernodesPQ();					// обновление генерации в суперузлах
		void DumpNodes();
		void CompareWithRastr();
		double Qgtanh(CDynaNodeBase* pNode);
		void CalculateBranchFlows();
		bool CheckNodeBalances();

		// возвращает true если узел учитывается в матрице якоби
		static bool NodeInMatrix(const CDynaNodeBase* pNode);
				
		CDynaModel *m_pDynaModel = nullptr;
		CDynaNodeContainer *pNodes = nullptr;

		KLUWrapper<double>	klu;
		std::unique_ptr<_MatrixInfo[]> m_pMatrixInfo;				// вектор узлов отнесенных к строкам матрицы якоби
		_MatrixInfo *m_pMatrixInfoEnd = nullptr;					// конец вектора узлов PV-PQ в якоби
		_MatrixInfo *m_pMatrixInfoSlackEnd = nullptr;				// конец вектора узлов с учетом базисных
		
		double m_dTanhBeta = 500.0;
		ptrdiff_t m_nNodeTypeSwitchesDone = 0;

		const LoadFlowParameters& m_Parameters;
		// определение порядка PV узлов для Зейделя
		static bool SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs);
		void AddToQueue(_MatrixInfo *pMatrixInfo, QUEUE& queue);
		void GetNodeImb(_MatrixInfo *pMatrixInfo);
		static double ImbNorm(double x, double y);

		void StoreVDelta();
		void RestoreVDelta();
		void UpdateSlackBusesImbalance();
		double GetSquaredImb();
		void CheckFeasible();
		void DumpNewtonIterationControl() const;
		void LogNodeSwitch(_MatrixInfo* pNode, std::string_view Title);
		LFNewtonStepRatio m_NewtonStepRatio;
		std::unique_ptr<double[]> m_Vbackup;
		std::unique_ptr<double[]> m_Dbackup;
		std::unique_ptr<double[]> m_Rh;		// невязки до итерации
		std::vector<CDynaBranch*> m_BranchAngleCheck;
		std::set<CDynaNodeBase*> m_FailedPVPQNodes;
	};
}


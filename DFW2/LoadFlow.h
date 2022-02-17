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
			Current,
			Power,
			Tanh
		};

		enum class eLoadFlowStartupMethod
		{
			None,
			Seidell,
			Tanh
		};

		struct LoadFlowParameters
		{
			double Imb = 1E-4;							// допустимый небаланс мощности
			bool Flat = false;							// плоский старт
			eLoadFlowStartupMethod Startup;				// стартовый метод 
			double SeidellStep = 1.0;					// шаг ускорения метода Зейделя	
			ptrdiff_t SeidellIterations = 17;			// количество итераций Зейделем
			ptrdiff_t EnableSwitchIteration = 2;		// номер итерации, с которой разрешается переключение PV-PQ
			ptrdiff_t MaxIterations = 100;				// максимальное количество итераций Ньютоном
			ptrdiff_t MaxPVPQSwitches = 5;				// максимальное количество переключений PV-PQ
			ptrdiff_t PVPQSwitchPerIt = 10;				// количество переключений типов узлов на одной итерации
			double VoltageNewtonStep = 0.3;				// максимальное относительное приращение шага Ньютона по напряжению
			double NodeAngleNewtonStep = 1.5;			// максимальное приращение шага Ньютона по углу узла
			double BranchAngleNewtonStep = 0.5;			// максимальное приращение шага Ньютона по углу связи
			double ForceSwitchLambda = 1e-2;			// шаг Ньютона, меньше которого бэктрэк выключается и выполняется переключение типов узлов
			eLoadFlowFormulation LFFormulation	= eLoadFlowFormulation::Current;	// способ представления модели УР
			bool AllowNegativeLRC = true;				// разрешить учет СХН для узлов с отрицательной нагрузкой
			double Vdifference = 1E-4;					// порог сравнения модуля напряжения
			double LRCMinSlope = 0.0;					// минимальная крутизна СХН
			double LRCMaxSlope = 5.0;					// максимальная крутизна СХН
		};

		CLoadFlow(CDynaModel *pDynaModel);
		bool Run();
	protected:

		void GetPnrQnr(CDynaNodeBase *pNode);
		void GetPnrQnrSuper(CDynaNodeBase *pNode);
		void AllocateSupernodes();
		void Estimate();
		void Seidell();
		void BuildSeidellOrder(MATRIXINFO& SeidellOrder);
		void BuildSeidellOrder2(MATRIXINFO& SeidellOrder);
		void Gauss();
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
		void RestoreSuperNodes();

		// возвращает true если узел учитывается в матрице якоби
		static bool NodeInMatrix(const CDynaNodeBase* pNode);
				
		CDynaModel *pDynaModel = nullptr;
		CDynaNodeContainer *pNodes = nullptr;

		KLUWrapper<double>	klu;
		std::unique_ptr<_MatrixInfo[]> pMatrixInfo_;			// вектор узлов отнесенных к строкам матрицы якоби
		_MatrixInfo *pMatrixInfoEnd = nullptr;					// конец вектора узлов PV-PQ в якоби
		_MatrixInfo *pMatrixInfoSlackEnd = nullptr;				// конец вектора узлов с учетом базисных
		
		double TanhBeta = 500.0;
		ptrdiff_t NodeTypeSwitchesDone = 0;

		const LoadFlowParameters& Parameters;
		// определение порядка PV узлов для Зейделя
		static bool SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs);
		void GetNodeImb(_MatrixInfo *pMatrixInfo);
		void GetNodeImbSSE(_MatrixInfo* pMatrixInfo);
		static double ImbNorm(double x, double y);

		void StoreVDelta();
		void RestoreVDelta();
		void UpdateSlackBusesImbalance();
		double GetSquaredImb();
		void CheckFeasible();
		void DumpNewtonIterationControl() const;
		void LogNodeSwitch(_MatrixInfo* pNode, std::string_view Title);
		LFNewtonStepRatio NewtonStepRatio;
		std::unique_ptr<double[]> pVbackup;
		std::unique_ptr<double[]> pDbackup;
		std::unique_ptr<double[]> pRh;		// невязки до итерации
		std::vector<CDynaBranch*> BranchAngleCheck;
		std::set<CDynaNodeBase*> FailedPVPQNodes;
	};
}


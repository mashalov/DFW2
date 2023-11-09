#pragma once
#include <filesystem>
#include "DynaNode.h"
#include "SVC.h"
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
			Tanh,
			RKF,
		};

		struct LoadFlowParameters
		{
			double Imb = 1E-4;							// допустимый небаланс мощности
			bool Flat = true;							// плоский старт
			eLoadFlowStartupMethod Startup = eLoadFlowStartupMethod::Seidell;	// стартовый метод 
			double SeidellStep = 1.0;					// шаг ускорения метода Зейделя	
			size_t SeidellIterations = 5;				// количество итераций Зейделем
			size_t EnableSwitchIteration = 3;			// номер итерации, с которой разрешается переключение PV-PQ
			size_t MaxIterations = 100;					// максимальное количество итераций Ньютоном
			size_t MaxPVPQSwitches = 5;					// максимальное количество переключений PV-PQ
			size_t PVPQSwitchPerIt = 0;					// количество переключений типов узлов на одной итерации
			double VoltageNewtonStep = 0.3;				// максимальное относительное приращение шага Ньютона по напряжению
			double NodeAngleNewtonStep = 1.5;			// максимальное приращение шага Ньютона по углу узла
			double BranchAngleNewtonStep = 0.5;			// максимальное приращение шага Ньютона по углу связи
			double ForceSwitchLambda = 1e-2;			// шаг Ньютона, меньше которого бэктрэк выключается и выполняется переключение типов узлов
			eLoadFlowFormulation LFFormulation	= eLoadFlowFormulation::Current;	// способ представления модели УР
			bool AllowNegativeLRC = true;				// разрешить учет СХН для узлов с отрицательной нагрузкой в УР
			double Vdifference = 1E-6;					// порог сравнения модуля напряжения
			double LRCMinSlope = 0.0;					// минимальная крутизна СХН
			double LRCMaxSlope = 5.0;					// максимальная крутизна СХН
			unsigned long ThreadId_ = 0;
			std::filesystem::path ModuleFilePath_;
			virtual ~LoadFlowParameters() = default;
			SerializerPtr GetSerializer();
			SerializerValidatorRulesPtr GetValidator() const;

			static constexpr const char* m_cszLFImbalance = "LFImbalance";
			static constexpr const char* m_cszLFFlat = "LFFlat";
			static constexpr const char* m_cszLFStartup = "LFStartup";
			static constexpr const char* m_cszLFSeidellStep = "SeidellStep";
			static constexpr const char* m_cszLFSeidellIterations = "SeidellIterations";
			static constexpr const char* m_cszLFEnableSwitchIteration = "EnableSwitchIteration";
			static constexpr const char* m_cszLFMaxIterations = "MaxIterations";
			static constexpr const char* m_cszLFNewtonMaxVoltageStep = "VoltageNewtonStep";
			static constexpr const char* m_cszLFNewtonMaxNodeAngleStep = "NodeAngleNewtonStep";
			static constexpr const char* m_cszLFNewtonMaxBranchAngleStep = "BranchAngleNewtonStep";
			static constexpr const char* m_cszLFForceSwitchLambda = "ForceSwitchLambda";
			static constexpr const char* m_cszLFFormulation = "LFFormulation";
			static constexpr const char* m_cszLFAllowNegativeLRC = "AllowNegativeLRC";
			static constexpr const char* m_cszLFLRCMinSlope = "LRCMinSlope";
			static constexpr const char* m_cszLFLRCMaxSlope = "LRCMaxSlope";
			static constexpr const char* m_cszLRCSmoothingRange = "LRCSmoothingRange";
			static constexpr const char* m_cszMaxPVPQSwitches = "MaxPVPQSwitches";
			static constexpr const char* m_cszPVPQSwitchPerIt = "PVPQSwitchesPerIteration";
			static constexpr const char* m_cszLFStartupNames[4] = { "None","Seidell", "Tanh", "RKF" };
			static constexpr const char* m_cszLFFormulationTypeNames[3] = { "Current", "Power", "Tanh" };
			static inline CValidationRuleFromSerializerLess MinLRCSLopeValidator = { LoadFlowParameters::m_cszLFLRCMaxSlope };
		};

		class Limits
		{
			MATRIXINFO PV_PQmax, PV_PQmin, PQmax_PV, PQmin_PV;
			std::set<CDynaNodeBase*> FailedPVPQNodes;
			CLoadFlow& LoadFlow_;
		public:
			Limits(CLoadFlow& LoadFlow);
			void Clear();
			void Select(_MatrixInfo* pMatrixInfo);
			size_t ViolatedCount() const;
			void Apply();
			void CheckFeasible();
		};

		CLoadFlow(CDynaModel *pDynaModel);
		bool Run();
	protected:

		// список включенных компенсаторов

		struct SVCExtra
		{
			CDynaSVCBase* SVC = nullptr;
			CDynaNodeBase* OriginalNode = nullptr;
			CDynaNodeBase* ExtraNode = nullptr;
		};

		using SVCCONTAINER = std::list<SVCExtra>;
		SVCCONTAINER  SVCs_;

		ptrdiff_t MatrixSize_ = 0;
		ptrdiff_t NonZeroCount_ =0;

		class CExtraNodes
		{
		protected:
			std::map<CDynaNodeBase*, std::list<SVCExtra*>> OriginalNodeSet_;
			CLoadFlow& LoadFlow_;
			CDynaNodeContainer *OriginalNodes_;
			CDynaNodeContainer Nodes_;
			std::unique_ptr<VirtualBranch[]> Branches_;
		public:
			CExtraNodes(CLoadFlow& LoadFlow) :
				LoadFlow_(LoadFlow),
				OriginalNodes_(LoadFlow.pNodes),
				Nodes_(LoadFlow.pDynaModel)
				{  }
			void CreateSVCNodes();
			void UpdateVirtualBranches();
			ptrdiff_t Count() const { return Nodes_.Count(); }
			_MatrixInfo* UpdateDimensions(_MatrixInfo* pMatrixInfo);
			void Restore();
		};

		std::unique_ptr<CExtraNodes>  ExtraNodes_;

		void GetPnrQnr(CDynaNodeBase *pNode);
		void GetPnrQnrSuper(CDynaNodeBase *pNode);
		void AllocateSupernodes();
		void Estimate();
		void Seidell();
		void BuildSeidellOrder(MATRIXINFO& SeidellOrder);
		void BuildSeidellOrder2(MATRIXINFO& SeidellOrder);
		void Gauss();
		void Z0();
		void SeidellTanh();
		void Newton();
		void NewtonTanh();
		void ContinuousNewton();
		double GetNewtonRatio(double const* b);
		void UpdateVDelta(double const* b, double Mult = 1.0);
		void UpdateVDelta(double dStep = 1.0);
		void BuildMatrixPower();
		void BuildMatrixCurrent();
		void BuildMatrixTanh();
		void Start();
		bool CheckLF();
		void SolveLinearSystem();
		void UpdateQToGenerators();					// обновление данных генераторов по результату расчета PV-узлов
		void UpdatePQFromGenerators();				// обновление данных PV-узлов по исходным данным генераторов
		void CollectSVCData();						// выделение данных по компенсаторам с допонительными узлами
		void UpdateSupernodesPQ();					// обновление генерации в суперузлах
		void DumpNodes();
		void CompareWithRastr();
		double Qgtanh(CDynaNodeBase* pNode);
		void CalculateBranchFlows();
		bool CheckNodeBalances();
		void RestoreSuperNodes();

		// расчет функциональных ограничений
		const ControlledLimit& NodeLFQmin(CDynaNodeBase* pNode);
		const ControlledLimit& NodeLFQmax(CDynaNodeBase* pNode);

		// возвращает true если узел учитывается в матрице якоби
		static bool NodeInMatrix(const CDynaNodeBase* pNode);
				
		CDynaModel *pDynaModel = nullptr;
		CDynaNodeContainer *pNodes = nullptr;

		KLUWrapper<double>	klu;
		std::unique_ptr<_MatrixInfo[]> pMatrixInfo_;			// вектор узлов отнесенных к строкам матрицы якоби
		_MatrixInfo *pMatrixInfoEnd = nullptr;					// конец вектора узлов PV-PQ в якоби
		_MatrixInfo *pMatrixInfoSlackEnd = nullptr;				// конец вектора узлов с учетом базисных
		
		double TanhBeta = 500.0;
		double lambda_ = 1.0;
		ptrdiff_t NodeTypeSwitchesDone = 0;

		const LoadFlowParameters& Parameters;
		// определение порядка PV узлов для Зейделя
		static bool SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs);
		void GetNodeImb(_MatrixInfo *pMatrixInfo);
		void GetNodeImbSSE(_MatrixInfo* pMatrixInfo);
		static double ImbNorm(double x, double y);

		void StoreVDelta();
		void RestoreVDelta();
		void RestoreUpdateVDelta(double const* b, double Mult = 1.0);
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
	};
}


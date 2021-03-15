#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CLimitedLag : public CDynaPrimitiveLimited
	{
	protected:
		double m_T;
		double m_K;
		double OnStateMax(CDynaModel *pDynaModel) override;
		double OnStateMin(CDynaModel *pDynaModel) override;
		double OnStateMid(CDynaModel *pDynaModel) override;
	public:

		CLimitedLag(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitiveLimited(Device, Output, Input) {}
		CLimitedLag(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CLimitedLag(Device, ORange(Output), IRange(Input)) { }

		virtual ~CLimitedLag() {}
		void SetMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K);
		void ChangeMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K);
		void ChangeTimeConstant(double TexcNew);
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const char* GetVerbalName() override { return "Апериодическое звено с ограничениями"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;

		static size_t PrimitiveSize() { return sizeof(CLimitedLag); }
		static long EquationsCount()  { return 1; }
	};
}

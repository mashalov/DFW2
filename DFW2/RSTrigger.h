﻿#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRSTrigger : public CDynaPrimitiveBinary
	{
	protected:
		InputVariable m_Input1;
		bool m_bResetPriority;
	public:


		CRSTrigger(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinary(Device, Output, Input), m_Input1(Input[1]) {}
		CRSTrigger(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CRSTrigger(Device, ORange(Output), IRange(Input)) { }

		virtual ~CRSTrigger() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const char* GetVerbalName() noexcept  override { return "RS-Триггер"; }
		static size_t PrimitiveSize() noexcept { return sizeof(CRSTrigger); }
		static long EquationsCount()  noexcept { return 1; }
	};
}


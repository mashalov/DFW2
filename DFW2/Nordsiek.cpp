#include "stdafx.h"
#include "DynaModel.h"
#include <immintrin.h>

//#define _AVX2

using namespace DFW2;

void CDynaModel::InitDevicesNordsiek()
{
	ChangeOrder(1);
	for (auto&& it : DeviceContainers_)
		it->InitNordsieck(this);
}



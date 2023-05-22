#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

IntegratorBase::vecType::iterator IntegratorBase::VerifyVector(IntegratorBase::vecType& vec)
{
	if (vec.size() != DynaModel_.MatrixSize())
		vec.resize(DynaModel_.MatrixSize());
	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorBase::FromModel(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.RightVectorRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::FromModel vector size mismatch");

	for (auto vit{ vec.begin() }; vit!= vec.end(); vit++, range.begin++)
		*vit = *range.begin->pValue;
	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorBase::ToModel(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.RightVectorRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::ToModel vector size mismatch");

	for (auto vit{ vec.begin() }; vit != vec.end(); vit++, range.begin++)
		*range.begin->pValue  = *vit;

	return vec.begin();
}


IntegratorBase::vecType::iterator IntegratorBase::FromB(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.BRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::FromB vector size mismatch");

	for (auto vit{ vec.begin() }; vit != vec.end(); vit++, range.begin++)
		*vit = *range.begin;

	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorBase::ToB(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.BRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::ToB vector size mismatch");

	for (auto vit{ vec.begin() }; vit != vec.end(); vit++, range.begin++)
		*range.begin = *vit;

	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorMultiStageBase::f(IntegratorBase::vecType& vec)
{
	for (auto&& it : DynaModel_.DeviceContainersPredict())
		it->Predict();
	DynaModel_.NewtonUpdateDevices();
	DynaModel_.BuildRightHand();
	return FromB(vec);
}
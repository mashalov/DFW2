#include "stdafx.h"
#include "CustomDeviceContainer.h"
#include "CustomDevice.h"

using namespace DFW2;

void CCustomDeviceCPPContainer::ConnectDLL(std::filesystem::path DLLFilePath)
{
	m_pDLL = std::make_shared<CCustomDeviceCPPDLL>(DLLFilePath, "CustomDeviceFactory");
	CCustomDeviceDLLWrapper pDevice(m_pDLL);
	pDevice->GetDeviceProperties(ContainerProps_);
	for (const auto& prim : pDevice->GetPrimitives())
	{
		m_PrimitivePools.CountPrimitive(prim.eBlockType);
		// имена притивов размещаем в списке
		// чтобы ссылаться на них из всех
		// экземпляров примитивов
		PrimitiveNames_.emplace_back(prim.Name);
	}
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszUserModelModuleLoaded, 
		stringutils::utf8_encode(DLLFilePath.c_str())));
}



void CCustomDeviceCPPContainer::BuildStructure()
{
	m_PrimitivePools.Allocate(DevVec.size());
	for (auto& dit : DevVec)
		static_cast<CCustomDeviceCPP*>(dit)->CreateDLLDeviceInstance(*this);
}

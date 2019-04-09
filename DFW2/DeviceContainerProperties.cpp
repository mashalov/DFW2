#include "stdafx.h"
#include "DeviceContainerProperties.h"
using namespace DFW2;

CDeviceContainerProperties::CDeviceContainerProperties()
{
	bNewtonUpdate = false;
	bCheckZeroCrossing = false;
	bPredict = false;
	nPossibleLinksCount = nEquationsCount = 0;
	eDeviceType = DEVTYPE_UNKNOWN;
}


CDeviceContainerProperties::~CDeviceContainerProperties()
{
}


void CDeviceContainerProperties::SetType(eDFW2DEVICETYPE eDevType)
{
	m_TypeInfoSet.insert(eDeviceType = eDevType);
}


void CDeviceContainerProperties::AddLinkTo(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency, const _TCHAR* cszstrIdField)
{
	if (m_LinksTo.find(eDevType) == m_LinksTo.end())
	{
		m_LinksTo.insert(std::make_pair(eDevType, LinkDirectionTo(eLinkMode, Dependency, m_LinksTo.size() + m_LinksFrom.size(), cszstrIdField)));
		if (eLinkMode == DLM_SINGLE)
			nPossibleLinksCount++;
	}
}

void CDeviceContainerProperties::AddLinkFrom(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency)
{
	if (m_LinksFrom.find(eDeviceType) == m_LinksFrom.end())
	{
		m_LinksFrom.insert(std::make_pair(eDevType, LinkDirectionFrom(eLinkMode, Dependency, m_LinksTo.size() + m_LinksFrom.size())));
		if(eLinkMode == DLM_SINGLE)
			nPossibleLinksCount++;
	}
}


const _TCHAR *CDeviceContainerProperties::m_cszNameGenerator1C = _T("��������� 1�");
const _TCHAR *CDeviceContainerProperties::m_cszNameGenerator3C = _T("��������� 3�");
const _TCHAR *CDeviceContainerProperties::m_cszNameGeneratorMustang = _T("��������� Mustang");
const _TCHAR *CDeviceContainerProperties::m_cszNameGeneratorInfPower = _T("���");
const _TCHAR *CDeviceContainerProperties::m_cszNameGeneratorMotion = _T("��������� ��");
const _TCHAR *CDeviceContainerProperties::m_cszNameExciterMustang = _T("����������� Mustang");
const _TCHAR *CDeviceContainerProperties::m_cszNameExcConMustang = _T("��� �������");
const _TCHAR *CDeviceContainerProperties::m_cszNameDECMustang = _T("���������� �������");
const _TCHAR *CDeviceContainerProperties::m_cszNameNode = _T("����");
const _TCHAR *CDeviceContainerProperties::m_cszNameBranch = _T("�����");
const _TCHAR *CDeviceContainerProperties::m_cszNameBranchMeasure = _T("��������� �����");
const _TCHAR *CDeviceContainerProperties::m_cszAliasNode = _T("node");
const _TCHAR *CDeviceContainerProperties::m_cszAliasBranch = _T("vetv");
const _TCHAR *CDeviceContainerProperties::m_cszAliasGenerator = _T("Generator");
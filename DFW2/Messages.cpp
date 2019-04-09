#include "stdafx.h"
#include "Messages.h"


using namespace DFW2;

CDFW2Messages::CDFW2Messages()
{
	m_VarNameMap[VARUNIT_KVOLTS]	= _T("��");
	m_VarNameMap[VARUNIT_VOLTS]		= _T("�");
	m_VarNameMap[VARUNIT_KAMPERES]	= _T("��");
	m_VarNameMap[VARUNIT_AMPERES]	= _T("�");
	m_VarNameMap[VARUNIT_DEGREES]	= _T("�");
	m_VarNameMap[VARUNIT_RADIANS]	= _T("���");
	m_VarNameMap[VARUNIT_PU]		= _T("�.�.");
	m_VarNameMap[VARUNIT_MW]		= _T("���");
	m_VarNameMap[VARUNIT_MVAR]		= _T("����");
	m_VarNameMap[VARUNIT_MVA]		= _T("���");
}

const _TCHAR* CDFW2Messages::m_cszBranchNodeNotFound = _T("���� %d �� ������ ��� �����");
const _TCHAR* CDFW2Messages::m_cszDuplicateDevice    = _T("������ %s ����� ������������ �������������");
const _TCHAR* CDFW2Messages::m_cszBranchLooped = _T("����� ���������� � ������������� � ���� %d");
const _TCHAR* CDFW2Messages::m_cszKLUOk = _T("KLU - OK");
const _TCHAR* CDFW2Messages::m_cszKLUSingular = _T("KLU - ����������� �������");
const _TCHAR* CDFW2Messages::m_cszKLUOutOfMemory = _T("KLU - ������������ ������");
const _TCHAR* CDFW2Messages::m_cszKLUInvalidInput = _T("KLU - ������������ ������");
const _TCHAR* CDFW2Messages::m_cszKLUIntOverflow = _T("KLU - ������������ ������ �����");
const _TCHAR* CDFW2Messages::m_cszInitLoopedInfinitely = _T("��� ������������� ��������� ��������� ����������� ����");
const _TCHAR* CDFW2Messages::m_cszDeviceContainerFailedToInit = _T("����� ������������� ��� ��������� ���� %d ��� %d");
const _TCHAR* CDFW2Messages::m_cszStepAndOrderChanged = _T("t=%g ������� � ��� ������ ������� : ������� %d ��� %g �");
const _TCHAR* CDFW2Messages::m_cszStepChanged = _T("t=%g ��� ������ ������� : ��� %g � (%g), ������� %d");
const _TCHAR* CDFW2Messages::m_cszStepAndOrderChangedOnNewton = _T("t=%g ������� � ��� ������ ������� �� �������: ������� %d ��� %g �");
const _TCHAR* CDFW2Messages::m_cszZeroCrossingStep = _T("t=%g ��� ������ ������� ��� ������ �����������: ��� %g �");
const _TCHAR* CDFW2Messages::m_cszStepChangedOnError = _T("t=%g ��� ������ ������� �� ����������: ��� %g �, ������ %g � %s �� %g \"%s\" Nordsiek[%g;%g]");
const _TCHAR* CDFW2Messages::m_cszStepAdjustedToDiscontinuity = _T("t=%g ��� ������ ������� ��� ��������� �������: ��� %g");
const _TCHAR* CDFW2Messages::m_cszSynchroZoneCountChanged = _T("��������� ���������� ���������� ��� : %d");
const _TCHAR* CDFW2Messages::m_cszAllNodesOff = _T("��� ���� ���������");
const _TCHAR* CDFW2Messages::m_cszNodeTripDueToZone = _T("���� %s ��������, ��� ��� ��������� � ���� ��� ���������� ����������");
const _TCHAR* CDFW2Messages::m_cszNodeRiseDueToZone = _T("���� %s ������� ��� ����������, ��� ��� ��������� � ���� � ��� ����������");
const _TCHAR* CDFW2Messages::m_cszUnknown = _T("����������");
const _TCHAR* CDFW2Messages::m_cszLRCDiscontinuityAt = _T("��������� ������ ��� %s ��� ���������� %g [�.�]. ��������  %g � %g");
const _TCHAR* CDFW2Messages::m_cszAmbigousLRCSegment = _T("� ��� %d ���������� ����� ������ �������� ��� ���������� %g. ����� �������������� ������ ������� %g+%gV+%gVV");
const _TCHAR* CDFW2Messages::m_cszLRCStartsNotFrom0 = _T("��� %d ���������� �� � �������� ���������� %g. ��������� ������� ����� \"���������\" �� ����");
const _TCHAR* CDFW2Messages::m_cszLRC1And2Reserved = _T("��� %d, �������� ������������� ������������. ������ ��� 1 � 2 ��������������� ��� ����������� ��������������");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveLimits = _T("������������ ����������� � ����� %s � ���������� %s : %g > %g");
const _TCHAR* CDFW2Messages::m_cszWrongDeadBandParameter = _T("������������ �������� ��������� � ����� %s � ���������� %s : %g");
const _TCHAR* CDFW2Messages::m_cszTightPrimitiveLimits = _T("����������� ����� %s � ���������� %s ������ ����� �������� �� ����� %g : %g � %g");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveTimeConstant = _T("������������ �������� ���������� ������� ����� %s � ���������� %s : %g �");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveInitialConditions = _T("��������� ������� ����� %s � ���������� %s �� ������������� ������������: %g �� ��������� � ��������� [%g;%g]");
const _TCHAR* CDFW2Messages::m_cszProcessDiscontinuityLoopedInfinitely = _T("��� ��������� ������� ��������� ��������� ����������� ����");
const _TCHAR* CDFW2Messages::m_cszDLLLoadFailed = _T("������ �������� DLL ����������������� ���������� %s");
const _TCHAR* CDFW2Messages::m_cszDLLFuncMissing = _T("� DLL ����������������� ���������� %s ���������� ���������� ��� ������ �������");
const _TCHAR* CDFW2Messages::m_cszDLLBadBlocks = _T("� DLL ����������������� ���������� %s ���������� ������������ �������� ������");
const _TCHAR* CDFW2Messages::m_cszPrimitiveExternalOutput = _T("� DLL ����������������� ���������� %s � ����� %s ������� ����� �����");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveinDLL = _T("� DLL ����������������� ���������� %s ��������� ����������� ��� ����� � ������� %d");
const _TCHAR* CDFW2Messages::m_cszTableNotFoundForCustomDevice = _T("��� �������� ������ ����������������� ���������� \"%s\" ���������� ������ \"%s\"");
const _TCHAR* CDFW2Messages::m_cszExtVarNotFoundInDevice = _T("��������� ��� ���������� \"%s\" �������� ���������� \"%s\" �� ������� � ���������� \"%s\"");
const _TCHAR* CDFW2Messages::m_cszExtVarNoDeviceFor = _T("��� ���������� � �������� ���������� \"%s\" ��� ���������� \"%s\"");
const _TCHAR* CDFW2Messages::m_cszConstVarNotFoundInDevice = _T("��������� ��� ���������� \"%s\" ��������� \"%s\" �� ������� � ���������� \"%s\"");
const _TCHAR* CDFW2Messages::m_cszConstVarNoDeviceFor = _T("��� ���������� � ���������� \"%s\" ��� ���������� \"%s\"");
const _TCHAR* CDFW2Messages::m_cszVarSearchStackDepthNotEnough = _T("������������ ������� ����� ��� ������ ���������� ��������� (%d)");
const _TCHAR* CDFW2Messages::m_cszWrongSingleLinkIndex = _T("������� ������������ ����� �=%d ��� ���������� %s. �������� %d ������");
const _TCHAR* CDFW2Messages::m_cszDeviceAlreadyLinked = _T("���������� %s �� ����� ���� ������� � ����������� %s, ��� ��� ��� ������� � ����������� %s");
const _TCHAR* CDFW2Messages::m_cszDeviceForDeviceNotFound = _T("���������� Id=%d �� ������� ��� ���������� %s");
const _TCHAR* CDFW2Messages::m_cszIncompatibleLinkModes = _T("������������� ������ ����� ��� ���������");
const _TCHAR* CDFW2Messages::m_cszFilePostion = _T("������� ����� %ld");
const _TCHAR* CDFW2Messages::m_cszFileReadError = _T("������ ������ �����.");
const _TCHAR* CDFW2Messages::m_cszFileWriteError = _T("������ ������ �����.");
const _TCHAR* CDFW2Messages::m_cszResultFileHasNewerVersion = _T("���� ����������� ����� ����� ����� ������ (%d) �� ��������� � ������� ���������� ����������� (%d). ");
const _TCHAR* CDFW2Messages::m_cszNoMemory = _T("������������ ������. ");
const _TCHAR* CDFW2Messages::m_cszWrongResultFile = _T("�������� ������ ����� �����������. ");
const _TCHAR* CDFW2Messages::m_cszResultFileNotLoadedProperly = _T("���� ����������� �� ��������. ");
const _TCHAR* CDFW2Messages::m_cszResultRoot = _T("������");
const _TCHAR* CDFW2Messages::m_cszWrongSymbolicLink = _T("�������� ������ ������������� ������: %s");
const _TCHAR* CDFW2Messages::m_cszObjectNotFoundByAlias = _T("�� ������ ������ ���� %s �� ������������� ������ %s");
const _TCHAR* CDFW2Messages::m_cszWrongKeyForSymbolicLink = _T("�������� ������ ����� %s � ������������� ������ %s");
const _TCHAR* CDFW2Messages::m_cszObjectNotFoundBySymbolicLink = _T("������ �� ������ �� ������������� ������ %s");
const _TCHAR* CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink = _T("������ �� ����� �������� %s, ���������� � ������������� ������ %s");
const _TCHAR* CDFW2Messages::m_cszNoCompilerDLL = _T("���������� ���������������� ������� ����������");
const _TCHAR* CDFW2Messages::m_cszActionNotFoundInDLL = _T("�������� %s �� ������� � ���������������� dll");
const _TCHAR* CDFW2Messages::m_cszLogicNotFoundInDLL = _T("������ %s �� ������� � ���������������� dll");
const _TCHAR* CDFW2Messages::m_cszWrongActionInLogicList = _T("�������� ������ ������ �� �������� \"%s\" � ������ �������� \"%s\" �������� ������ L%d");
const _TCHAR* CDFW2Messages::m_cszDuplicateActionGroupInLogic = _T("������ �������� A%d ������ � ������ �������� \"%s\" �������� ������ L%d ����� ������ ����");
const _TCHAR* CDFW2Messages::m_cszNoActionGroupFoundInLogic = _T("������ �������� A%d, ��������� � ������ \"%s\" �������� ������ L%d �� ���������� � ���������");
const _TCHAR* CDFW2Messages::m_cszDuplicatedVariableUnit = _T("������� ��������� � ����� %d ��� ����������");
const _TCHAR* CDFW2Messages::m_cszDuplicatedDeviceType = _T("��� ���������� %d ��� ���������");
const _TCHAR* CDFW2Messages::m_cszTooMuchDevicesInResult = _T("������� �������� %d ��������� �� %d ���������");
const _TCHAR* CDFW2Messages::m_cszWrongParameter = _T("�������� ��������");
const _TCHAR* CDFW2Messages::m_cszDuplicatedVariableName = _T("������������ ��� ���������� %s ��� ���������� ���� %s");
const _TCHAR* CDFW2Messages::m_cszUnknownError = _T("����������� ������");
const _TCHAR* CDFW2Messages::m_cszMemoryAllocError = _T("������ ������������� ������ %s");
const _TCHAR* CDFW2Messages::m_cszLULFConverged = _T("�������� ����� ������� � ������������ %g �� %d ��������");





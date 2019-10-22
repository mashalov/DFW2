#pragma once
#include "map"
#include "DeviceTypes.h"

namespace DFW2
{
	// карта типов единиц измерения к названиям
	// названия нужны только для взаимодействия с пользователем,
	// поэтому названия единиц измерения вводим в описании классов сообщений
	typedef std::map<ptrdiff_t, std::wstring> VARNAMEMAP;
	typedef VARNAMEMAP::const_iterator VARNAMEITRCONST;

	// класс сообщений и логирования
	class CDFW2Messages
	{
	protected:
		// карта типов единиц измерения по сути глобальная
		VARNAMEMAP m_VarNameMap;
	public:
		enum DFW2MessageStatus
		{
			DFW2LOG_FATAL,
			DFW2LOG_ERROR,
			DFW2LOG_WARNING,
			DFW2LOG_MESSAGE,
			DFW2LOG_INFO,
			DFW2LOG_DEBUG
		};

		CDFW2Messages();

		const VARNAMEMAP& VarNameMap() const { return m_VarNameMap; }

		static const _TCHAR* m_cszBranchNodeNotFound;
		static const _TCHAR* m_cszDuplicateDevice;
		static const _TCHAR* m_cszBranchLooped; 
		static const _TCHAR* m_cszKLUOk;
		static const _TCHAR* m_cszKLUSingular;
		static const _TCHAR* m_cszKLUOutOfMemory;
		static const _TCHAR* m_cszKLUInvalidInput;
		static const _TCHAR* m_cszKLUIntOverflow;
		static const _TCHAR* m_cszKLUUnknownError;
		static const _TCHAR* m_cszInitLoopedInfinitely;
		static const _TCHAR* m_cszDeviceContainerFailedToInit;
		static const _TCHAR* m_cszStepAndOrderChanged;
		static const _TCHAR* m_cszStepChanged;
		static const _TCHAR* m_cszStepAndOrderChangedOnNewton;
		static const _TCHAR* m_cszZeroCrossingStep;
		static const _TCHAR* m_cszStepChangedOnError;
		static const _TCHAR* m_cszStepAdjustedToDiscontinuity;
		static const _TCHAR* m_cszSynchroZoneCountChanged;
		static const _TCHAR* m_cszAllNodesOff;
		static const _TCHAR* m_cszNodeTripDueToZone;
		static const _TCHAR* m_cszNodeRiseDueToZone;
		static const _TCHAR* m_cszUnknown;
		static const _TCHAR* m_cszLRCDiscontinuityAt;
		static const _TCHAR* m_cszAmbigousLRCSegment;
		static const _TCHAR* m_cszLRCStartsNotFrom0;
		static const _TCHAR* m_cszLRC1And2Reserved;
		static const _TCHAR* m_cszWrongPrimitiveLimits;
		static const _TCHAR* m_cszWrongDeadBandParameter;
		static const _TCHAR* m_cszTightPrimitiveLimits;
		static const _TCHAR* m_cszWrongPrimitiveTimeConstant;
		static const _TCHAR* m_cszWrongPrimitiveInitialConditions;
		static const _TCHAR* m_cszProcessDiscontinuityLoopedInfinitely;
		static const _TCHAR* m_cszDLLLoadFailed;
		static const _TCHAR* m_cszDLLFuncMissing;
		static const _TCHAR* m_cszDLLBadBlocks;
		static const _TCHAR* m_cszPrimitiveExternalOutput;
		static const _TCHAR* m_cszWrongPrimitiveinDLL;
		static const _TCHAR* m_cszTableNotFoundForCustomDevice;
		static const _TCHAR* m_cszExtVarNotFoundInDevice;
		static const _TCHAR* m_cszExtVarFromOffDevice;
		static const _TCHAR* m_cszExtVarNoDeviceFor;
		static const _TCHAR* m_cszConstVarNotFoundInDevice;
		static const _TCHAR* m_cszConstVarNoDeviceFor;
		static const _TCHAR* m_cszVarSearchStackDepthNotEnough;
		static const _TCHAR* m_cszWrongSingleLinkIndex;
		static const _TCHAR* m_cszDeviceAlreadyLinked;
		static const _TCHAR* m_cszDeviceForDeviceNotFound;
		static const _TCHAR* m_cszIncompatibleLinkModes;
		static const _TCHAR* m_cszFilePostion;
		static const _TCHAR* m_cszFileReadError;
		static const _TCHAR* m_cszFileWriteError;
		static const _TCHAR* m_cszResultFileHasNewerVersion;
		static const _TCHAR* m_cszNoMemory;
		static const _TCHAR* m_cszWrongResultFile;
		static const _TCHAR* m_cszResultFileNotLoadedProperly;
		static const _TCHAR* m_cszResultRoot;
		static const _TCHAR* m_cszWrongSymbolicLink;
		static const _TCHAR* m_cszObjectNotFoundByAlias;
		static const _TCHAR* m_cszWrongKeyForSymbolicLink;
		static const _TCHAR* m_cszObjectNotFoundBySymbolicLink;
		static const _TCHAR* m_cszObjectHasNoPropBySymbolicLink; 
		static const _TCHAR* m_cszNoCompilerDLL;
		static const _TCHAR* m_cszActionNotFoundInDLL;
		static const _TCHAR* m_cszLogicNotFoundInDLL;
		static const _TCHAR* m_cszWrongActionInLogicList;
		static const _TCHAR* m_cszDuplicateActionGroupInLogic;
		static const _TCHAR* m_cszNoActionGroupFoundInLogic;
		static const _TCHAR* m_cszDuplicatedVariableUnit;
		static const _TCHAR* m_cszDuplicatedDeviceType;
		static const _TCHAR* m_cszTooMuchDevicesInResult;
		static const _TCHAR* m_cszWrongParameter;
		static const _TCHAR* m_cszDuplicatedVariableName;
		static const _TCHAR* m_cszUnknownError;
		static const _TCHAR* m_cszMemoryAllocError;
		static const _TCHAR* m_cszLULFConverged;
		static const _TCHAR* m_cszLFRunningNewton;
		static const _TCHAR* m_cszLFRunningSeidell;
		static const _TCHAR* m_cszLFNoConvergence;
		static const _TCHAR* m_cszLFNodeVTooHigh;
		static const _TCHAR* m_cszLFNodeVTooLow;
		static const _TCHAR* m_cszLFBranchAngleExceeds90;
		static const _TCHAR* m_cszWrongGeneratorsNumberFixed;
		static const _TCHAR* m_cszFailureAtMinimalStep;
		static const _TCHAR* m_cszMustBeConstPowerLRC;
		static const _TCHAR* m_cszResultFileWrongCompressedBlockType;
		static const _TCHAR* m_cszResultFilePointsCountMismatch;
		static const _TCHAR* m_cszDeivceDoesNotHaveAccessToModel;
		static const _TCHAR* m_cszAdamsDamping;
		static const _TCHAR* m_cszOn;
		static const _TCHAR* m_cszOff;
		static const _TCHAR* m_cszIslandOfSuperNode;
		static const _TCHAR* m_cszSwitchedOffNode;
		static const _TCHAR* m_cszSwitchedOffBranch;
		static const _TCHAR* m_cszIslandSlackBusesCount;
		static const _TCHAR* m_cszIslandCount;
		static const _TCHAR* m_cszIslandNoSlackBusesShutDown;
		static const _TCHAR* m_cszNoNodesForLF;
		static const _TCHAR* m_cszUnacceptableLF;
	};
}


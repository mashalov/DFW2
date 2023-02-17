#pragma once
#include "map"
#include "string"
#include "DeviceTypes.h"

namespace DFW2
{
	// карта типов единиц измерения к названиям
	// названия нужны только для взаимодействия с пользователем,
	// поэтому названия единиц измерения вводим в описании классов сообщений
	class VARNAMEMAP : public std::map<ptrdiff_t, std::string>
	{
		static inline std::string empty = "???";
	public:
		const std::string& VerbalUnits(ptrdiff_t Unit) const
		{
			if (const auto it = find(Unit); it != end())
				return it->second;
			else
				return VARNAMEMAP::empty;
		}
	};

	enum class DFW2MessageStatus
	{
		DFW2LOG_NONE,
		DFW2LOG_FATAL,
		DFW2LOG_ERROR,
		DFW2LOG_WARNING,
		DFW2LOG_MESSAGE,
		DFW2LOG_INFO,
		DFW2LOG_DEBUG
	};


	// класс сообщений и логирования
	class CDFW2Messages
	{
	protected:
		// карта типов единиц измерения по сути глобальная
		VARNAMEMAP m_VarNameMap;
	public:
		CDFW2Messages();

		const VARNAMEMAP& VarNameMap() const noexcept { return m_VarNameMap; }

		static const char* m_cszProjectName;
		static const char* m_cszBranchNodeNotFound;
		static const char* m_cszDuplicateDevice;
		static const char* m_cszBranchLooped; 
		static const char* m_cszKLUOk;
		static const char* m_cszKLUSingular;
		static const char* m_cszKLUOutOfMemory;
		static const char* m_cszKLUInvalidInput;
		static const char* m_cszKLUIntOverflow;
		static const char* m_cszKLUUnknownError;
		static const char* m_cszInitLoopedInfinitely;
		static const char* m_cszDeviceContainerFailedToInit;
		static const char* m_cszStepAndOrderChanged;
		static const char* m_cszStepChanged;
		static const char* m_cszStepAndOrderChangedOnNewton;
		static const char* m_cszZeroCrossingStep;
		static const char* m_cszStepChangedOnError;
		static const char* m_cszStepAdjustedToDiscontinuity;
		static const char* m_cszSynchroZoneCountChanged;
		static const char* m_cszAllNodesOff;
		static const char* m_cszNodeTripDueToZone;
		static const char* m_cszNodeRiseDueToZone;
		static const char* m_cszUnknown;
		static const char* m_cszLRCDiscontinuityAt;
		static const char* m_cszLRCSlopeViolated;
		static const char* m_cszLRCNonUnity;
		static const char* m_cszAmbigousLRCSegment;
		static const char* m_cszLRCStartsNotFrom0;
		static const char* m_cszLRC1And2Reserved;
		static const char* m_cszWrongPrimitiveLimits;
		static const char* m_cszWrongDeadBandParameter;
		static const char* m_cszTightPrimitiveLimits;
		static const char* m_cszWrongPrimitiveTimeConstant;
		static const char* m_cszWrongPrimitiveInitialConditions;
		static const char* m_cszProcessDiscontinuityLoopedInfinitely;
		static const char* m_cszDLLLoadFailed;
		static const char* m_cszDLLFuncMissing;
		static const char* m_cszDLLBadBlocks;
		static const char* m_cszPrimitiveExternalOutput;
		static const char* m_cszWrongPrimitiveinDLL;
		static const char* m_cszTableNotFoundForCustomDevice;
		static const char* m_cszExtVarNotFoundInDevice;
		static const char* m_cszExtVarFromOffDevice;
		static const char* m_cszExtVarNoDeviceFor;
		static const char* m_cszConstVarNotFoundInDevice;
		static const char* m_cszConstVarNoDeviceFor;
		static const char* m_cszVarSearchStackDepthNotEnough;
		static const char* m_cszWrongSingleLinkIndex;
		static const char* m_cszDeviceAlreadyLinked;
		static const char* m_cszDeviceForDeviceNotFound;
		static const char* m_cszIncompatibleLinkModes;
		static const char* m_cszFilePostion;
		static const char* m_cszFileReadError;
		static const char* m_cszFileWriteError;
		static const char* m_cszResultFileHasNewerVersion;
		static const char* m_cszNoMemory;
		static const char* m_cszWrongResultFile;
		static const char* m_cszResultFileNotLoadedProperly;
		static const char* m_cszResultRoot;
		static const char* m_cszWrongSymbolicLink;
		static const char* m_cszObjectNotFoundByAlias;
		static const char* m_cszAmbigousObjectsFoundByAlias;
		static const char* m_cszWrongKeyForSymbolicLink;
		static const char* m_cszObjectNotFoundBySymbolicLink;
		static const char* m_cszObjectHasNoPropBySymbolicLink; 
		static const char* m_cszNoCompilerDLL;
		static const char* m_cszActionNotFoundInDLL;
		static const char* m_cszLogicNotFoundInDLL;
		static const char* m_cszWrongActionInLogicList;
		static const char* m_cszDuplicateActionGroupInLogic;
		static const char* m_cszNoActionGroupFoundInLogic;
		static const char* m_cszActionTypeNotFound;
		static const char* m_cszDuplicatedVariableUnit;
		static const char* m_cszDuplicatedDeviceType;
		static const char* m_cszTooMuchDevicesInResult;
		static const char* m_cszWrongParameter;
		static const char* m_cszDuplicatedVariableName;
		static const char* m_cszUnknownError;
		static const char* m_cszMemoryAllocError;
		static const char* m_cszLULFConverged;
		static const char* m_cszLFRunningNewton;
		static const char* m_cszLFRunningContinuousNewton;
		static const char* m_cszLFRunningSeidell;
		static const char* m_cszLFNoConvergence;
		static const char* m_cszLFNodeVTooHigh;
		static const char* m_cszLFNodeVTooLow;
		static const char* m_cszLFBranchAngleExceeds90;
		static const char* m_cszWrongGeneratorsNumberFixed;
		static const char* m_cszWrongGeneratorQlimitsFixed;
		static const char* m_cszFailureAtMinimalStep;
		static const char* m_cszMustBeEmbeddedLRC;
		static const char* m_cszMustBeDefaultDynamicLRC;
		static const char* m_cszResultFileWrongCompressedBlockType;
		static const char* m_cszResultFilePointsCountMismatch;
		static const char* m_cszDeivceDoesNotHaveAccessToModel;
		static const char* m_cszAdamsDamping;
		static const char* m_cszOn;
		static const char* m_cszOff;
		static const char* m_cszIslandOfSuperNode;
		static const char* m_cszSwitchedOffNode;
		static const char* m_cszSwitchedOffBranch;
		static const char* m_cszSwitchedOffBranchHead;
		static const char* m_cszSwitchedOffBranchTail;
		static const char* m_cszSwitchedOffBranchComplete;
		static const char* m_cszIslandSlackBusesCount;
		static const char* m_cszIslandCount;
		static const char* m_cszIslandNoSlackBusesShutDown;
		static const char* m_cszNodeShutDownAsNotLinkedToSlack;
		static const char* m_cszNoNodesForLF;
		static const char* m_cszUnacceptableLF;
		static const char* m_cszTurningOffDeviceByMasterDevice;
		static const char* m_cszTurningOffDeviceDueToNoMasterDevice;
		static const char* m_cszMatrixSize;
		static const char* m_cszTurnOnDeviceImpossibleDueToMaster;
		static const char* m_cszAutomaticOrScenarioFailedToInitialize;
		static const char* m_cszLFWrongQrangeForNode;
		static const char* m_cszLFWrongQrangeForSuperNode;
		static const char* m_cszLFError;
		static const char* m_cszCannotChangePermanentDeviceState;
		static const char* m_cszWrongUnom;
		static const char* m_cszWrongSourceData;
		static const char* m_cszUserLRCChangedToStandard;
		static const char* m_cszLRCSmoothingTooBigFor;
		static const char* m_cszUserOverrideOfStandardLRC;
		static const char* m_cszLRCVminChanged;
		static const char* m_cszLRCIdNotFound;
		static const char* m_cszNodeNotFoundForReactor;
		static const char* m_cszBranchNotFoundForReactor;
		static const char* m_cszLoadingModelFormat;
		static const char* m_cszLoadingParameters;
		static const char* m_cszStdFileStreamError;
		static const char* m_cszJsonParserError;
		static const char* m_cszFoundContainerData;
		static const char* m_cszNoNodesOrBranchesForLF;
		static const char* m_cszWrongLimits;
		static const char* m_cszEmptyLimits;
		static const char* m_cszBranchAngleExceedsPI;
		static const char* m_cszGeneratorAngleExceedsPI;
		static const char* m_cszGeneratorPowerExceedsRated;
		static const char* m_cszUnomMismatch;
		static const char* m_cszValidationSuspiciousLow;
		static const char* m_cszNoRastrWin3FoundInRegistry;
		static const char* m_cszDecayDetected;
		static const char* m_cszCannotUseRastrWin3;
		static const char* m_cszStopCommandReceived;
		static const char* m_cszUserModelModuleLoaded;
		static const char* m_cszFailedToCreateFolder;
		static const char* m_cszFailedToCreateCOMResultsWriter;
		static const char* m_cszModuleLoadError;
		static const char* m_cszDLLFunctionNotFound;
		static const char* m_cszDLLIsNotReadyForCreateCall;
		static const char* m_cszPathShouldBeFolder;
		static const char* m_cszNoUserModelInFolder;
		static const char* m_cszNoUserModelReferenceFolder;
		static const char* m_cszCouldNotCopyUserModelReference;
		static const char* m_cszUserModelCompiled;
		static const char* m_cszUserModelFailedToCompile;
		static const char* m_cszFileCopyError;
		static const char* m_cszRemovingExcessCachedModule;
		static const char* m_cszTooManyCachedModules;
		static const char* m_cszNextCacheClieanIn;
		static const char* m_cszUserModelFailedToOpenSource;
		static const char* m_cszUserModelAlreadyCompiled;
		static const char* m_cszCachedUserModelFound;
		static const char* m_cszCustomModelModule;
		static const char* m_cszCustomModelHasNoMetadata;
		static const char* m_cszUserModelShouldBeCompiled;
		static const char* m_cszCompilerVersionMismatch;
		static const char* m_cszCustomModelChanged;
		static const char* m_cszUserModelCannotSaveFile;
		static const char* m_cszDebug;
		static const char* m_cszInfo;
		static const char* m_cszMessage;
		static const char* m_cszWarning;
		static const char* m_cszError;
		static const char* m_cszEvent;
		static const char* m_cszLogStarted;
		static const char* m_cszOS;
		static const char* m_cszLFNodeImbalance;
		static const char* m_cszMaxBranchAngle;
		static const char* m_cszMaxGeneratorAngle;
		static const char* m_cszValidationBiggerThanZero;
		static const char* m_cszValidationBiggerThanUnity;
		static const char* m_cszValidationNegative;
		static const char* m_cszValidationNonNegative;
		static const char* m_cszValidationChangedTo;
		static const char* m_cszValidationRange;
		static const char* m_cszValidationTfOfMustangExcCon;
		static const char* m_cszValidationBiggerThanNamed;
		static const char* m_cszValidationBiggerOrEqualThanNamed;
		static const char* m_cszValidationLessOrEqualThanNamed;
		static const char* m_cszValidationLessThanNamed;
		static const char* m_cszDiscontinuityProcessing;
		static const char* m_cszAutomaticName;
		static const char* m_cszScenarioName;
		static const char* m_cszCannotConvertShortCircuitConstants;
		static const char* m_cszParameterIsOutOfRange;
		static const char* m_cszWrongTimeConstants;
		static const char* m_cszCannotGetParkParameters;
		static const char* m_cszParkParametersNiiptMethodFailed;
		static const char* m_cszParkParametersNiiptPlusMethodFailed;
		static const char* m_cszParkParametersCanayMethodFailed;
		static const char* m_cszActionNotInitialized;
		static const char* m_cszProgressCaption;
		static const char* m_cszResultWriterDisabled;
		static const char* m_cszResultFileCreated;
		static const char* m_cszTopologyNodesCreated;
		static const char* m_cszLFOverswitchedNode;
		static const char* m_cszCompilerAndRaidenVersionMismatch;
		static const char* m_cszLFZ0isForFlatOnly;
		static const char* m_cszLFRunningZ0;
		static const char* m_cszNoSSE2Support;
		static const char* m_cszWrongTaggedPath;
		static const char* m_cszPrimitiveChangesState;
		static const char* m_cszRunningAction;
		static const char* m_cszGeneratorOverspeed;
		static const char* m_cszExternalParameterAccepted;
	};
}


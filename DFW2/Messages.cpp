﻿#include "stdafx.h"
#include "Messages.h"


using namespace DFW2;

CDFW2Messages::CDFW2Messages()
{
	m_VarNameMap[VARUNIT_KVOLTS]	= "кВ";
	m_VarNameMap[VARUNIT_VOLTS]		= "В";
	m_VarNameMap[VARUNIT_KAMPERES]	= "кА";
	m_VarNameMap[VARUNIT_AMPERES]	= "А";
	m_VarNameMap[VARUNIT_DEGREES]	= "°";
	m_VarNameMap[VARUNIT_RADIANS]	= "рад";
	m_VarNameMap[VARUNIT_PU]		= "о.е.";
	m_VarNameMap[VARUNIT_MW]		= "МВт";
	m_VarNameMap[VARUNIT_MVAR]		= "МВар";
	m_VarNameMap[VARUNIT_MVA]		= "МВА";
	m_VarNameMap[VARUNIT_SIEMENS]	= "См";
	m_VarNameMap[VARUNIT_PIECES]	= "Шт";
	m_VarNameMap[VARUNIT_SECONDS]   = "с";
	m_VarNameMap[VARUNIT_OHM]		= "Ом";
}

#ifdef _MSC_VER
#ifdef _WIN64
const char* CDFW2Messages::m_cszOS = "Windows x64";
#else
const char* CDFW2Messages::m_cszOS = "Windows x86";
#endif
#else
#ifdef __x86_64__
const char* CDFW2Messages::m_cszOS = "Linux x64";
#else
const char* CDFW2Messages::m_cszOS = "Linux x86";
#endif
#endif

const char* CDFW2Messages::m_cszProjectName = "Raiden";
const char* CDFW2Messages::m_cszBranchNodeNotFound = "Узел {} не найден для ветви {}-{} ({})";
const char* CDFW2Messages::m_cszDuplicateDevice    = "Объект \"{}\" имеет неуникальный идентификатор";
const char* CDFW2Messages::m_cszBranchLooped = "Ветвь начинается и заканчивается в узле {}";
const char* CDFW2Messages::m_cszKLUOk = "KLU - OK";
const char* CDFW2Messages::m_cszKLUSingular = "KLU - Сингулярная матрица";
const char* CDFW2Messages::m_cszKLUOutOfMemory = "KLU - Недостаточно памяти";
const char* CDFW2Messages::m_cszKLUInvalidInput = "KLU - Неправильные данные";
const char* CDFW2Messages::m_cszKLUIntOverflow = "KLU - Переполнение целого числа";
const char* CDFW2Messages::m_cszKLUUnknownError = "KLU - Неизвестная ошибка с кодом {}";
const char* CDFW2Messages::m_cszInitLoopedInfinitely = "При инициализации устройств обнаружен бесконечный цикл";
const char* CDFW2Messages::m_cszDeviceContainerFailedToInit = "Отказ инициализации для устройств типа {} код {}";
const char* CDFW2Messages::m_cszStepAndOrderChanged = "t={:15.012f} {:>3} Порядок и шаг метода изменен : порядок {} шаг {} с";
const char* CDFW2Messages::m_cszStepChanged = "t={:15.012f} {:>3} Шаг метода изменен : шаг {} с ({}), порядок {}";
const char* CDFW2Messages::m_cszStepAndOrderChangedOnNewton = "t={:15.012f} {:>3} Порядок и шаг метода изменен по Ньютону: порядок {} шаг {} с";
const char* CDFW2Messages::m_cszZeroCrossingStep = "t={:15.012f} {:>3} Шаг метода изменен для поиска ограничения: шаг {} с в устройстве {}";
const char* CDFW2Messages::m_cszStepChangedOnError = "t={:15.012f} {:>3} Шаг метода изменен по корректору: шаг {} с, ошибка {} в {} от {} \"{}\" Nordsiek[{};{}]";
const char* CDFW2Messages::m_cszStepAdjustedToDiscontinuity = "t={:15.012f} {:>3} Шаг метода изменен для обработки события: шаг {}";
const char* CDFW2Messages::m_cszSynchroZoneCountChanged = "Обновлено количество синхронных зон : {}";
const char* CDFW2Messages::m_cszAllNodesOff = "Все узлы отключены";
const char* CDFW2Messages::m_cszNodeTripDueToZone = "Узел \"{}\" отключен, так как находится в зоне без источников напряжения";
const char* CDFW2Messages::m_cszNodeRiseDueToZone = "Узел \"{}\" включен под напряжение, так как находится в зоне с его источником";
const char* CDFW2Messages::m_cszUnknown = "Неизвестно";
const char* CDFW2Messages::m_cszLRCDiscontinuityAt = "Обнаружен разрыв СХН {} при напряжении {} [о.е]. Значения  {} и {}";
const char* CDFW2Messages::m_cszAmbigousLRCSegment = "В СХН {} обнаружено более одного сегмента для напряжения {}. Будет использоваться первый сегмент {}+{}V+{}VV";
const char* CDFW2Messages::m_cszLRCStartsNotFrom0 = "СХН {} начинается не с нулевого напряжения {}. Начальный сегмент будет \"продолжен\" до нуля";
const char* CDFW2Messages::m_cszLRC1And2Reserved = "СХН {}, заданная пользователем игнорирована. Номера СХН 1 и 2 зарезервированы под стандартные характеристики";
const char* CDFW2Messages::m_cszWrongPrimitiveLimits = "Недопустимые ограничения в блоке {} в устройстве \"{}\" : {} > {}";
const char* CDFW2Messages::m_cszWrongDeadBandParameter = "Недопустимое значение параметра в блоке {} в устройстве \"{}\" : {}";
const char* CDFW2Messages::m_cszTightPrimitiveLimits = "Ограничения блока {} в устройстве \"{}\" должны иметь разность не менее {} : {} и {}";
const char* CDFW2Messages::m_cszWrongPrimitiveTimeConstant = "Недопустимое значение постоянной времени блока {} в устройстве \"{}\" : {} с";
const char* CDFW2Messages::m_cszWrongPrimitiveInitialConditions = "Начальные условия блока {} в устройстве \"{}\" не соответствуют ограничениям: {} не находится в интервале [{};{}]";
const char* CDFW2Messages::m_cszProcessDiscontinuityLoopedInfinitely = "При обработке разрыва устройств обнаружен бесконечный цикл";
const char* CDFW2Messages::m_cszDLLLoadFailed = "Ошибка загрузки DLL пользовательского устройства \"{}\"";
const char* CDFW2Messages::m_cszDLLFuncMissing = "В DLL пользовательского устройства \"{}\" недоступны необходмые для работы функции";
const char* CDFW2Messages::m_cszDLLBadBlocks = "В DLL пользовательского устройства \"{}\" обнаружено неправильное описание блоков";
const char* CDFW2Messages::m_cszPrimitiveExternalOutput = "В DLL пользовательского устройства {} в блоке {} неверно задан выход";
const char* CDFW2Messages::m_cszWrongPrimitiveinDLL = "В DLL пользовательского устройства \"{}\" обнаружен неизвестный тип блока с номером {}";
const char* CDFW2Messages::m_cszTableNotFoundForCustomDevice = "При загрузке данных пользовательского устройства \"{}\" обнаружена ошибка \"{}\"";
const char* CDFW2Messages::m_cszExtVarNotFoundInDevice = "Требуемая для устройства \"{}\" выходная переменная \"{}\" не найдена в устройстве \"{}\"";
const char* CDFW2Messages::m_cszExtVarNoDeviceFor = "Нет устройства с выходной переменной \"{}\" для устройства \"{}\"";
const char* CDFW2Messages::m_cszConstVarNotFoundInDevice = "Требуемая для устройства \"{}\" константа \"{}\" не найдена в устройстве \"{}\"";
const char* CDFW2Messages::m_cszConstVarNoDeviceFor = "Нет устройства с константой \"{}\" для устройства \"{}\"";
const char* CDFW2Messages::m_cszVarSearchStackDepthNotEnough = "Недостаточна глубина стека для поиска переменных устройств ({})";
const char* CDFW2Messages::m_cszWrongSingleLinkIndex = "Попытка использовать связь №={} для устройства {}. Доступно {} связей";
const char* CDFW2Messages::m_cszDeviceAlreadyLinked = "Устройство {} не может быть связано с устройством {}, так как уже связано с устройством {}";
const char* CDFW2Messages::m_cszDeviceForDeviceNotFound = "Устройство Id={} не найдено для устройства {}";
const char* CDFW2Messages::m_cszIncompatibleLinkModes = "Несовместимые режимы связи для устройств";
const char* CDFW2Messages::m_cszFilePostion = " Позиция файла: {}. ";
const char* CDFW2Messages::m_cszFileReadError = "Ошибка чтения файла.";
const char* CDFW2Messages::m_cszFileWriteError = "Ошибка записи файла.";
const char* CDFW2Messages::m_cszResultFileHasNewerVersion = "Файл результатов имеет более новую версию ({}) по сравнению с версией загрузчика результатов ({}). ";
const char* CDFW2Messages::m_cszNoMemory = "Недостаточно памяти. ";
const char* CDFW2Messages::m_cszWrongResultFile = "Неверный формат файла результатов. ";
const char* CDFW2Messages::m_cszResultFileNotLoadedProperly = "Файл результатов не загружен. ";
const char* CDFW2Messages::m_cszResultRoot = "Модель";
const char* CDFW2Messages::m_cszWrongSymbolicLink = "Неверный формат символической ссылки: \"{}\"";
const char* CDFW2Messages::m_cszObjectNotFoundByAlias = "Не найден объект типа \"{}\" по символической ссылке \"{}\"";
const char* CDFW2Messages::m_cszWrongKeyForSymbolicLink = "Неверный формат ключа \"{}\" в символической ссылке \"{}\"";
const char* CDFW2Messages::m_cszObjectNotFoundBySymbolicLink = "Объект не найден по символической ссылке \"{}\"";
const char* CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink = "Объект не имеет свойства \"{}\", указанного в символической ссылке \"{}\"";
const char* CDFW2Messages::m_cszNoCompilerDLL = "Компилятор пользовательских моделей недоступен";
const char* CDFW2Messages::m_cszActionNotFoundInDLL = "Действие {} не найдено в скомпилированной dll";
const char* CDFW2Messages::m_cszLogicNotFoundInDLL = "Логика {} не найдена в скомпилированной dll";
const char* CDFW2Messages::m_cszActionTypeNotFound = "Тип действия автоматики {} с номером \"{}\" не поддерживается";
const char* CDFW2Messages::m_cszWrongActionInLogicList = "Неверный формат ссылки на действие \"{}\" в списке действий \"{}\" элемента логики L{}";
const char* CDFW2Messages::m_cszDuplicateActionGroupInLogic = "Группа действий A{} задана в списке действий \"{}\" элемента логики L{} более одного раза";
const char* CDFW2Messages::m_cszNoActionGroupFoundInLogic = "Группа действий A{}, указанная в списке \"{}\" элемента логики L{} не определена в действиях";
const char* CDFW2Messages::m_cszDuplicatedVariableUnit = "Единицы измерения с типом \"{}\" уже определены";
const char* CDFW2Messages::m_cszDuplicatedDeviceType = "Тип устройства \"{}\" уже определен";
const char* CDFW2Messages::m_cszTooMuchDevicesInResult = "Попытка добавить {} устройств из {} возможных";
const char* CDFW2Messages::m_cszWrongParameter = "Неверный параметр";
const char* CDFW2Messages::m_cszDuplicatedVariableName = "Неуникальное имя переменной \"{}\" для устройства типа \"{}\"";
const char* CDFW2Messages::m_cszUnknownError = "Неизвестная ошибка";
const char* CDFW2Messages::m_cszMemoryAllocError = "Ошибка распределения памяти {}";
const char* CDFW2Messages::m_cszLULFConverged = "Линейный метод сошелся с погрешностью {} за {} итераций";
const char* CDFW2Messages::m_cszLFRunningNewton = "Расчет УР методом Ньютона";
const char* CDFW2Messages::m_cszLFRunningSeidell = "Расчет УР методом Зейделя";
const char* CDFW2Messages::m_cszLFNoConvergence = "Не удалось сбалансировать установившийся режим";
const char* CDFW2Messages::m_cszLFNodeVTooHigh = "Недопустимое напряжение в узле \"{}\" - {} номинального";
const char* CDFW2Messages::m_cszLFNodeVTooLow = CDFW2Messages::m_cszLFNodeVTooHigh;
const char* CDFW2Messages::m_cszLFBranchAngleExceeds90 = "Угол по связи {} - {} превысил 90 град - {}";
const char* CDFW2Messages::m_cszWrongGeneratorsNumberFixed = "Количество генераторов для \"{}\" задано неверно - {:.0f}, установлено значение 1";
const char* CDFW2Messages::m_cszWrongGeneratirQlimitsFixed = "Ограничения реактивной мощности для генератора \"{}\" задано неверно {} > {}. Установлено Qmin=Qmax={}";
const char* CDFW2Messages::m_cszFailureAtMinimalStep = "Необходимая точность решения не может быть достигнута на минимальном шаге t={} {:>3} порядок {} шаг={}";
const char* CDFW2Messages::m_cszMustBeConstPowerLRC = "Не найдена типовая СХН на постоянную мощность с номером -1";
const char* CDFW2Messages::m_cszMustBeDefaultDynamicLRC = "Не найдена типовая СХН нагрузки в динамике с номером 0";
const char* CDFW2Messages::m_cszResultFileWrongCompressedBlockType = "Неверный тип сжатого блока данных в файле результатов";
const char* CDFW2Messages::m_cszResultFilePointsCountMismatch = "Размерность канала {} {} не совпадает с количеством точек в файле {}";
const char* CDFW2Messages::m_cszDeivceDoesNotHaveAccessToModel = "Устройство не имеет доступа к модели";
const char* CDFW2Messages::m_cszAdamsDamping = "Демпфирование Адамса-2 {}";
const char* CDFW2Messages::m_cszOn = "вкл";
const char* CDFW2Messages::m_cszOff = "выкл";
const char* CDFW2Messages::m_cszIslandOfSuperNode = "Суперузел-представитель острова {}";
const char* CDFW2Messages::m_cszSwitchedOffNode = "Отключен \"{}\" так как все ветви отключены";
const char* CDFW2Messages::m_cszSwitchedOffBranch = "{} {} отключена {} так как узел \"{}\" отключен";
const char* CDFW2Messages::m_cszSwitchedOffBranchHead		= "в начале";
const char* CDFW2Messages::m_cszSwitchedOffBranchTail		= "в конце";
const char* CDFW2Messages::m_cszSwitchedOffBranchComplete = "полностью";
const char* CDFW2Messages::m_cszIslandCount = "Найдено синхронных зон {}";
const char* CDFW2Messages::m_cszIslandSlackBusesCount = "Синхронная зона содержит {} включенных узлов, из них базисных узлов {}";
const char* CDFW2Messages::m_cszIslandNoSlackBusesShutDown = "В синхронной зоне нет базисных узлов. Синхронная зона будет полностью отключена";
const char* CDFW2Messages::m_cszNodeShutDownAsNotLinkedToSlack = "Узел \"{}\" отключен так как находится в зоне, не содержащей базисного узла";
const char* CDFW2Messages::m_cszNoNodesForLF = "Для расчета УР нет включенных узлов";
const char* CDFW2Messages::m_cszNoNodesOrBranchesForLF = "Количество узлов или ветвей равно нулю";
const char* CDFW2Messages::m_cszUnacceptableLF = "Получен недопустимый режим";
const char* CDFW2Messages::m_cszExtVarFromOffDevice = "Требуемая для устройства \"{}\" выходная переменная \"{}\" найдена в отключенном устройстве \"{}\"";
const char* CDFW2Messages::m_cszTurningOffDeviceByMasterDevice = "Устройство \"{}\" отключено, так как отключено связанное ведущее устройство \"{}\"";
const char* CDFW2Messages::m_cszTurningOffDeviceDueToNoMasterDevice = "Устройство \"{}\" отключено, так как не найдено связанное ведущее устройство";
const char* CDFW2Messages::m_cszMatrixSize = "Размерность матрицы {}, количество ненулевых элементов {}";
const char* CDFW2Messages::m_cszTurnOnDeviceImpossibleDueToMaster = "Невозможно включить устройство \"{}\", так как отключено по крайней мере одно ведущее устройство \"{}\"";
const char* CDFW2Messages::m_cszAutomaticOrScenarioFailedToInitialize = "При инициализации автоматики или сценария обнаружены ошибки";
const char* CDFW2Messages::m_cszLFWrongQrangeForNode = "Для узла \"{}\" невозможно распределение реактивной мощности {} по генераторам c суммарным диапазоном [{};{}]";
const char* CDFW2Messages::m_cszLFWrongQrangeForSuperNode = "Для суперузла \"{}\" невозможно распределение реактивной мощности {} по узлам c суммарным диапазоном [{};{}]";
const char* CDFW2Messages::m_cszLFError = "Ошибка при расчете УР";
const char* CDFW2Messages::m_cszCannotChangePermanentDeviceState = "Невозможно изменить состояние устройства \"{}\"";
const char* CDFW2Messages::m_cszWrongUnom = "Недопустимое значение номинального напряжения \"{}\" Uном = {} кВ";
const char* CDFW2Messages::m_cszWrongSourceData = "Обнаружена неустранимая ошибка в исходных данных";
const char* CDFW2Messages::m_cszUserLRCChangedToStandard = "СХН № {}, заданная пользователем, заменена на стандартную в соответствии с заданным параметром расчета";
const char* CDFW2Messages::m_cszUserOverrideOfStandardLRC = "СХН с номером {} является стандартной, но задана пользователем";
const char* CDFW2Messages::m_cszLRCVminChanged = "Значение Vmin={} недопустимо для обработки СХН. Значение изменено на {}";
const char* CDFW2Messages::m_cszLRCIdNotFound = "СХН {} не найдена для узла \"{}\"}";
const char* CDFW2Messages::m_cszLoadingModelFormat = "Загрузка модели в формате {} \"{}\"";
const char* CDFW2Messages::m_cszStdFileStreamError = "Ошибка работы с файлом - \"{}\"";
const char* CDFW2Messages::m_cszJsonParserError = "Ошибка парсера json \"{}\"";
const char* CDFW2Messages::m_cszFoundContainerData = "Найдены данные для ввода \"{}\" (\"{}\"). Количество объектов {}";
const char* CDFW2Messages::m_cszWrongLimits = "Неправильно заданы ограничения [{};{}] = [{};{}] в устройстве \"{}\"";
const char* CDFW2Messages::m_cszEmptyLimits = "Ограничения [{};{}] = [{};{}] в устройстве \"{}\". Ограничения не будут учитываться";
const char* CDFW2Messages::m_cszBranchAngleExceedsPI = "Асинхронный режим по связи \"{}\". Угол {} при t={}";
const char* CDFW2Messages::m_cszGeneratorAngleExceedsPI = "Асинхронный режим по генератору \"{}\". Угол {} при t={}";
const char* CDFW2Messages::m_cszGeneratorPowerExceedsRated = "Мощность генератора \"{}\" |S={:.5g}|={:.5g} МВА превышает 1.05*Sном={:.5g} МВА";
const char* CDFW2Messages::m_cszUnomMismatch = "Номинальное напряжение \"{}\" Uном={:.5g} кВ отличается от номинального напряжения узла \"{}\" Unom={} кВ более чем на 15%";
const char* CDFW2Messages::m_cszWrongPnom = "Неверно задана номинальная мощность для \"{}\" Pnom={} МВт";
const char* CDFW2Messages::m_cszGeneratorSuspiciousMj = "Значение Tj={} c для генератора \"{}\" подозрительно мало";
const char* CDFW2Messages::m_cszNoRastrWin3FoundInRegistry = "Ошибка доступа к ключу/значению в реестре для поиска шаблонов RastrWin3";
const char* CDFW2Messages::m_cszDecayDetected = "Зафиксирован критерий затухания переходного процесса при t={} с";
const char* CDFW2Messages::m_cszCannotUseRastrWin3 = "Невозможно использование компонентов RastrWin3";
const char* CDFW2Messages::m_cszStopCommandReceived = "Получена команда останова при t={} с";
const char* CDFW2Messages::m_cszUserModelModuleLoaded = "Загружен модуль пользовательского устройства \"{}\"";
const char* CDFW2Messages::m_cszFailedToCreateFolder = "Невозможно создать каталог \"{}\"";
const char* CDFW2Messages::m_cszFailedToCreateCOMResultsWriter = "Невозможно создание COM-модуля записи результатов";
const char* CDFW2Messages::m_cszModuleLoadError = "Ошибка загрузки динамической библиотеки \"{}\"";
const char* CDFW2Messages::m_cszPathShouldBeFolder = "Путь \"{}\" должен оканчиваться символом каталога";
const char* CDFW2Messages::m_cszNoUserModelInFolder = "В каталоге \"{}\" не найден файл скомпилированного пользовательского устройства \"{}\"";
const char* CDFW2Messages::m_cszNoUserModelReferenceFolder = "Не найден каталог исходных файлов для сборки пользовательской модели \"{}\"";
const char* CDFW2Messages::m_cszCouldNotCopyUserModelReference = "Невозможно копирование файлов исходных текстов пользовательской модели из \"{}\" в \"{}\"";
const char* CDFW2Messages::m_cszUserModelCompiled = "Выполнена компиляция модуля пользовательской модели \"{}\"";
const char* CDFW2Messages::m_cszUserModelFailedToCompile = "Ошибка компиляции модуля пользовательской модели \"{}\"";
const char* CDFW2Messages::m_cszUserModelFailedToOpenSource = "Ошибка доступа к файлу пользовательской модели \"{}\"";
const char* CDFW2Messages::m_cszUserModelAlreadyCompiled = "Модуль пользовательской модели \"{}\" не нуждается в повторной компиляции";
const char* CDFW2Messages::m_cszUserModelShouldBeCompiled = "Необходима компиляция модуля пользовательской модели \"{}\"";
const char* CDFW2Messages::m_cszUserModelCannotSaveFile = "Невозможно сохранить промежуточный файл пользовательской модели \"{}\"";
const char* CDFW2Messages::m_cszDebug = "Отладка: ";
const char* CDFW2Messages::m_cszInfo  = "Информация: ";
const char* CDFW2Messages::m_cszMessage = "Сообщение: ";
const char* CDFW2Messages::m_cszWarning = "Предупреждение: ";
const char* CDFW2Messages::m_cszError = "Ошибка: ";
const char* CDFW2Messages::m_cszEvent = "Событие: ";
const char* CDFW2Messages::m_cszLogStarted = "{} {} от {} на {}. Протокол запущен в режиме \"{}\"";
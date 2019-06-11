﻿#include "stdafx.h"
#include "Messages.h"


using namespace DFW2;

CDFW2Messages::CDFW2Messages()
{
	m_VarNameMap[VARUNIT_KVOLTS]	= _T("кВ");
	m_VarNameMap[VARUNIT_VOLTS]		= _T("В");
	m_VarNameMap[VARUNIT_KAMPERES]	= _T("кА");
	m_VarNameMap[VARUNIT_AMPERES]	= _T("А");
	m_VarNameMap[VARUNIT_DEGREES]	= _T("°");
	m_VarNameMap[VARUNIT_RADIANS]	= _T("рад");
	m_VarNameMap[VARUNIT_PU]		= _T("о.е.");
	m_VarNameMap[VARUNIT_MW]		= _T("МВт");
	m_VarNameMap[VARUNIT_MVAR]		= _T("МВар");
	m_VarNameMap[VARUNIT_MVA]		= _T("МВА");
}

const _TCHAR* CDFW2Messages::m_cszBranchNodeNotFound = _T("Узел %d не найден для ветви");
const _TCHAR* CDFW2Messages::m_cszDuplicateDevice    = _T("Объект %s имеет неуникальный идентификатор");
const _TCHAR* CDFW2Messages::m_cszBranchLooped = _T("Ветвь начинается и заканчивается в узле %d");
const _TCHAR* CDFW2Messages::m_cszKLUOk = _T("KLU - OK");
const _TCHAR* CDFW2Messages::m_cszKLUSingular = _T("KLU - Сингулярная матрица");
const _TCHAR* CDFW2Messages::m_cszKLUOutOfMemory = _T("KLU - Недостаточно памяти");
const _TCHAR* CDFW2Messages::m_cszKLUInvalidInput = _T("KLU - Неправильные данные");
const _TCHAR* CDFW2Messages::m_cszKLUIntOverflow = _T("KLU - Переполнение целого числа");
const _TCHAR* CDFW2Messages::m_cszInitLoopedInfinitely = _T("При инициализации устройств обнаружен бесконечный цикл");
const _TCHAR* CDFW2Messages::m_cszDeviceContainerFailedToInit = _T("Отказ инициализации для устройств типа %d код %d");
const _TCHAR* CDFW2Messages::m_cszStepAndOrderChanged = _T("t=%g Порядок и шаг метода изменен : порядок %d шаг %g с");
const _TCHAR* CDFW2Messages::m_cszStepChanged = _T("t=%g Шаг метода изменен : шаг %g с (%g), порядок %d");
const _TCHAR* CDFW2Messages::m_cszStepAndOrderChangedOnNewton = _T("t=%g Порядок и шаг метода изменен по Ньютону: порядок %d шаг %g с");
const _TCHAR* CDFW2Messages::m_cszZeroCrossingStep = _T("t=%g Шаг метода изменен для поиска ограничения: шаг %g с");
const _TCHAR* CDFW2Messages::m_cszStepChangedOnError = _T("t=%g Шаг метода изменен по корректору: шаг %g с, ошибка %g в %s от %g \"%s\" Nordsiek[%g;%g]");
const _TCHAR* CDFW2Messages::m_cszStepAdjustedToDiscontinuity = _T("t=%g Шаг метода изменен для обработки события: шаг %g");
const _TCHAR* CDFW2Messages::m_cszSynchroZoneCountChanged = _T("Обновлено количество синхронных зон : %d");
const _TCHAR* CDFW2Messages::m_cszAllNodesOff = _T("Все узлы отключены");
const _TCHAR* CDFW2Messages::m_cszNodeTripDueToZone = _T("Узел %s отключен, так как находится в зоне без источников напряжения");
const _TCHAR* CDFW2Messages::m_cszNodeRiseDueToZone = _T("Узел %s включен под напряжение, так как находится в зоне с его источником");
const _TCHAR* CDFW2Messages::m_cszUnknown = _T("Неизвестно");
const _TCHAR* CDFW2Messages::m_cszLRCDiscontinuityAt = _T("Обнаружен разрыв СХН %s при напряжении %g [о.е]. Значения  %g и %g");
const _TCHAR* CDFW2Messages::m_cszAmbigousLRCSegment = _T("В СХН %d обнаружено более одного сегмента для напряжения %g. Будет использоваться первый сегмент %g+%gV+%gVV");
const _TCHAR* CDFW2Messages::m_cszLRCStartsNotFrom0 = _T("СХН %d начинается не с нулевого напряжения %g. Начальный сегмент будет \"продолжен\" до нуля");
const _TCHAR* CDFW2Messages::m_cszLRC1And2Reserved = _T("СХН %d, заданная пользователем игнорирована. Номера СХН 1 и 2 зарезервированы под стандартные характеристики");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveLimits = _T("Недопустимые ограничения в блоке %s в устройстве %s : %g > %g");
const _TCHAR* CDFW2Messages::m_cszWrongDeadBandParameter = _T("Недопустимое значение параметра в блоке %s в устройстве %s : %g");
const _TCHAR* CDFW2Messages::m_cszTightPrimitiveLimits = _T("Ограничения блока %s в устройстве %s должны иметь разность не менее %g : %g и %g");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveTimeConstant = _T("Недопустимое значение постоянной времени блока %s в устройстве %s : %g с");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveInitialConditions = _T("Начальные условия блока %s в устройстве %s не соответствуют ограничениям: %g не находится в интервале [%g;%g]");
const _TCHAR* CDFW2Messages::m_cszProcessDiscontinuityLoopedInfinitely = _T("При обработке разрыва устройств обнаружен бесконечный цикл");
const _TCHAR* CDFW2Messages::m_cszDLLLoadFailed = _T("Ошибка загрузки DLL пользовательского устройства %s");
const _TCHAR* CDFW2Messages::m_cszDLLFuncMissing = _T("В DLL пользовательского устройства %s недоступны необходмые для работы функции");
const _TCHAR* CDFW2Messages::m_cszDLLBadBlocks = _T("В DLL пользовательского устройства %s обнаружено неправильное описание блоков");
const _TCHAR* CDFW2Messages::m_cszPrimitiveExternalOutput = _T("В DLL пользовательского устройства %s в блоке %s неверно задан выход");
const _TCHAR* CDFW2Messages::m_cszWrongPrimitiveinDLL = _T("В DLL пользовательского устройства %s обнаружен неизвестный тип блока с номером %d");
const _TCHAR* CDFW2Messages::m_cszTableNotFoundForCustomDevice = _T("При загрузке данных пользовательского устройства \"%s\" обнаружена ошибка \"%s\"");
const _TCHAR* CDFW2Messages::m_cszExtVarNotFoundInDevice = _T("Требуемая для устройства \"%s\" выходная переменная \"%s\" не найдена в устройстве \"%s\"");
const _TCHAR* CDFW2Messages::m_cszExtVarNoDeviceFor = _T("Нет устройства с выходной переменной \"%s\" для устройства \"%s\"");
const _TCHAR* CDFW2Messages::m_cszConstVarNotFoundInDevice = _T("Требуемая для устройства \"%s\" константа \"%s\" не найдена в устройстве \"%s\"");
const _TCHAR* CDFW2Messages::m_cszConstVarNoDeviceFor = _T("Нет устройства с константой \"%s\" для устройства \"%s\"");
const _TCHAR* CDFW2Messages::m_cszVarSearchStackDepthNotEnough = _T("Недостаточна глубина стека для поиска переменных устройств (%d)");
const _TCHAR* CDFW2Messages::m_cszWrongSingleLinkIndex = _T("Попытка использовать связь №=%d для устройства %s. Доступно %d связей");
const _TCHAR* CDFW2Messages::m_cszDeviceAlreadyLinked = _T("Устройство %s не может быть связано с устройством %s, так как уже связано с устройством %s");
const _TCHAR* CDFW2Messages::m_cszDeviceForDeviceNotFound = _T("Устройство Id=%d не найдено для устройства %s");
const _TCHAR* CDFW2Messages::m_cszIncompatibleLinkModes = _T("Несовместимые режимы связи для устройств");
const _TCHAR* CDFW2Messages::m_cszFilePostion = _T("Позиция файла %ld");
const _TCHAR* CDFW2Messages::m_cszFileReadError = _T("Ошибка чтения файла.");
const _TCHAR* CDFW2Messages::m_cszFileWriteError = _T("Ошибка записи файла.");
const _TCHAR* CDFW2Messages::m_cszResultFileHasNewerVersion = _T("Файл результатов имеет более новую версию (%d) по сравнению с версией загрузчика результатов (%d). ");
const _TCHAR* CDFW2Messages::m_cszNoMemory = _T("Недостаточно памяти. ");
const _TCHAR* CDFW2Messages::m_cszWrongResultFile = _T("Неверный формат файла результатов. ");
const _TCHAR* CDFW2Messages::m_cszResultFileNotLoadedProperly = _T("Файл результатов не загружен. ");
const _TCHAR* CDFW2Messages::m_cszResultRoot = _T("Модель");
const _TCHAR* CDFW2Messages::m_cszWrongSymbolicLink = _T("Неверный формат символической ссылки: %s");
const _TCHAR* CDFW2Messages::m_cszObjectNotFoundByAlias = _T("Не найден объект типа %s по символической ссылке %s");
const _TCHAR* CDFW2Messages::m_cszWrongKeyForSymbolicLink = _T("Неверный формат ключа %s в символической ссылке %s");
const _TCHAR* CDFW2Messages::m_cszObjectNotFoundBySymbolicLink = _T("Объект не найден по символической ссылке %s");
const _TCHAR* CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink = _T("Объект не имеет свойства %s, указанного в символической ссылке %s");
const _TCHAR* CDFW2Messages::m_cszNoCompilerDLL = _T("Компилятор пользовательских моделей недоступен");
const _TCHAR* CDFW2Messages::m_cszActionNotFoundInDLL = _T("Действие %s не найдено в скомпилированной dll");
const _TCHAR* CDFW2Messages::m_cszLogicNotFoundInDLL = _T("Логика %s не найдена в скомпилированной dll");
const _TCHAR* CDFW2Messages::m_cszWrongActionInLogicList = _T("Неверный формат ссылки на действие \"%s\" в списке действий \"%s\" элемента логики L%d");
const _TCHAR* CDFW2Messages::m_cszDuplicateActionGroupInLogic = _T("Группа действий A%d задана в списке действий \"%s\" элемента логики L%d более одного раза");
const _TCHAR* CDFW2Messages::m_cszNoActionGroupFoundInLogic = _T("Группа действий A%d, указанная в списке \"%s\" элемента логики L%d не определена в действиях");
const _TCHAR* CDFW2Messages::m_cszDuplicatedVariableUnit = _T("Единицы измерения с типом %d уже определены");
const _TCHAR* CDFW2Messages::m_cszDuplicatedDeviceType = _T("Тип устройства %d уже определен");
const _TCHAR* CDFW2Messages::m_cszTooMuchDevicesInResult = _T("Попытка добавить %d устройств из %d возможных");
const _TCHAR* CDFW2Messages::m_cszWrongParameter = _T("Неверный параметр");
const _TCHAR* CDFW2Messages::m_cszDuplicatedVariableName = _T("Неуникальное имя переменной %s для устройства типа %s");
const _TCHAR* CDFW2Messages::m_cszUnknownError = _T("Неизвестная ошибка");
const _TCHAR* CDFW2Messages::m_cszMemoryAllocError = _T("Ошибка распределения памяти %s");
const _TCHAR* CDFW2Messages::m_cszLULFConverged = _T("Линейный метод сошелся с погрешностью %g за %d итераций");
const _TCHAR* CDFW2Messages::m_cszLFRunningNewton = _T("Расчет УР методом Ньютона");
const _TCHAR* CDFW2Messages::m_cszLFRunningSeidell = _T("Расчет УР методом Зейделя");
const _TCHAR* CDFW2Messages::m_cszLFNoConvergence = _T("Ну удалось сбалансировать установившийся режим");
const _TCHAR* CDFW2Messages::m_cszLFNodeVTooHigh = _T("Недопустимое напряжение в узле %s - %g номинального");
const _TCHAR* CDFW2Messages::m_cszLFNodeVTooLow = CDFW2Messages::m_cszLFNodeVTooHigh;
const _TCHAR* CDFW2Messages::m_cszLFBranchAngleExceeds90 = _T("Угол по связи %s - %s превысил 90 град - %g");




#include "stdafx.h"
#include "RastrImport.h"
using namespace DFW2;
#import "C:\Program Files (x86)\RastrWin3\astra.dll" no_namespace, named_guids, no_dual_interfaces, no_implementation 


CRastrImport::CRastrImport()
{
}


CRastrImport::~CRastrImport()
{

}

bool GetConstFromField(const ConstVarsInfo* pVarsInfo)
{
	bool bRes = false;
	if (pVarsInfo && !(pVarsInfo->VarFlags & (CVF_INTERNALCONST)) && pVarsInfo->eDeviceType == DEVTYPE_UNKNOWN)
		bRes = true;
	return bRes;
}

bool CRastrImport::GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceContainer& CustomDeviceContainer)
{
	bool bRes = false;
	if (spRastr)
	{
		try
		{
			ITablePtr spSourceTable = spRastr->Tables->Item(ConnectInfo.m_TableName.c_str());
			IColsPtr spSourceCols = spSourceTable->Cols;
			const DFW2::CCustomDeviceDLL& DLL = CustomDeviceContainer.DLL();

			// get and check constants fields from storage
			size_t nConstsCount = DLL.GetConstsCount();
			typedef vector<pair<IColPtr,ptrdiff_t> > COLVECTOR;
			typedef COLVECTOR::iterator COLITR;
			COLVECTOR Cols;
			Cols.reserve(nConstsCount);

			IColPtr spColId		= spSourceCols->Item(_T("Id"));
			IColPtr spColName	= spSourceCols->Item(_T("Name"));

			for (size_t ConstIndex = 0; ConstIndex < nConstsCount; ConstIndex++)
			{
				const ConstVarsInfo *pVarInfo = DLL.GetConstInfo(ConstIndex);
				if (pVarInfo)
				{
					if (GetConstFromField(pVarInfo))
						Cols.push_back(make_pair(spSourceCols->Item(pVarInfo->VarInfo.Name),ConstIndex));
				}
				else
				{
					Network.Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLBadBlocks,CustomDeviceContainer.DLL().GetModuleFilePath()));
					break;
				}
			}

			// count model types in storage
			IColPtr spModelType = spSourceCols->Item(ConnectInfo.m_ModelTypeField.c_str());

			long nTableIndex = 0;
			long nTableSize = spSourceTable->GetSize();
			long nModelsCount = 0;

			for (; nTableIndex < nTableSize; nTableIndex++)
			{
				if (spModelType->GetZ(nTableIndex).lVal == ConnectInfo.m_nModelType)
					nModelsCount++;
			}

			if (nModelsCount)
			{
				// create models for count given
				CCustomDevice *pCustomDevices = new CCustomDevice[nModelsCount];
				Network.CustomDevice.AddDevices(pCustomDevices, nModelsCount);
				Network.CustomDevice.BuildStructure();

				// put constants to each model
				long nModelIndex = 0;
				for (nTableIndex = 0 ; nTableIndex < nTableSize; nTableIndex++)
				{
					if (spModelType->GetZ(nTableIndex).lVal == ConnectInfo.m_nModelType)
					{
						if (nModelIndex >= nModelsCount)
						{
							Network.Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLBadBlocks, CustomDeviceContainer.DLL().GetModuleFilePath()));
							break;
						}

						CCustomDevice *pDevice = pCustomDevices + nModelIndex;
						if (pDevice->SetConstDefaultValues())
						{
							pDevice->SetId(spColId->GetZ(nTableIndex).lVal);
							pDevice->SetName(spColId->GetZS(nTableIndex));
							for (COLITR cit = Cols.begin(); cit != Cols.end(); cit++)
							{
								if (!pDevice->SetConstValue(cit->second, cit->first->GetZ(nTableIndex).dblVal))
								{
									Network.Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLBadBlocks, CustomDeviceContainer.DLL().GetModuleFilePath()));
									break;
								}
							}
						}
						else
						{
							Network.Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLBadBlocks, CustomDeviceContainer.DLL().GetModuleFilePath()));
							break;
						}
						nModelIndex++;
					}
				}
			}
		}
		catch (_com_error &err)
		{
			Network.Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszTableNotFoundForCustomDevice,
																CustomDeviceContainer.DLL().GetModuleFilePath(), 
																static_cast<const _TCHAR*>(err.Description())));
		}
	}
	return bRes;
}

void CRastrImport::GetData(CDynaModel& Network)
{
	IRastrPtr spRastr;
	HRESULT hr = spRastr.CreateInstance(CLSID_Rastr);


	//spRastr->Load(RG_REPL, L"..\\tests\\test92.rst", "");
	//spRastr->Load(RG_REPL, L"C:\\Users\\Bug\\Documents\\Visual Studio 2013\\Projects\\DFW2\\tests\\test92.rst", "");
	//spRastr->NewFile(L"C:\\Users\\Bug\\Documents\\RastrWin3\\SHABLON\\автоматика.dfw");
	//spRastr->Load(RG_REPL, L"..\\tests\\test93.rst", "");
	spRastr->Load(RG_REPL, L"..\\tests\\mdp_debug_1", ""); 
	//spRastr->Load(RG_REPL, L"..\\tests\\oos", "");
	//spRastr->Load(RG_REPL, L"..\\tests\\gen_lf_no_calc", "");
	//spRastr->Load(RG_REPL, L"..\\tests\\mdp_debug_unstable", "");
	//spRastr->Load(RG_REPL, L"..\\tests\\mdp_debug_5", "");
	//spRastr->Load(RG_REPL, L"..\\tests\\test9_sc", ""); 
	//spRastr->Load(RG_REPL, L"C:\\Users\\Bug\\Documents\\RastrWin3\\test-rastr\\test9_qmin.rst", "");
	//spRastr->Load(RG_REPL, L"C:\\Users\\Bug\\Documents\\RastrWin3\\test-rastr\\cx195.rg2",L"C:\\Users\\Bug\\Documents\\RastrWin3\\SHABLON\\динамика.rst");
	//spRastr->NewFile(L"C:\\Users\\Bug\\Documents\\RastrWin3\\SHABLON\\автоматика.dfw");
	//spRastr->NewFile(L"C:\\Users\\Bug\\Documents\\RastrWin3\\SHABLON\\сценарий.scn");

	
	//spRastr->Load(RG_REPL, L"..\\tests\\mdp_debug_5", "");

	if (!Network.Automatic().PrepareCompiler())
		return;

	ITablesPtr spTables = spRastr->Tables;

	ITablePtr spAutoStarters = spTables->Item("DFWAutoStarter");
	IColsPtr spASCols = spAutoStarters->Cols;
	IColPtr spASId			= spASCols->Item(_T("Id"));
	IColPtr spASName		= spASCols->Item(_T("Name"));
	IColPtr spASType		= spASCols->Item(_T("Type"));
	IColPtr spASExpr		= spASCols->Item(_T("Formula"));
	IColPtr spASObjClass	= spASCols->Item(_T("ObjectClass"));
	IColPtr spASObjKey		= spASCols->Item(_T("ObjectKey"));
	IColPtr spASObjProp		= spASCols->Item(_T("ObjectProp"));


	for (int i = 0; i < spAutoStarters->GetSize(); i++)
	{
		Network.Automatic().AddStarter(spASType->GetZ(i),
			spASId->GetZ(i).lVal,
			spASName->GetZ(i).bstrVal,
			spASExpr->GetZ(i).bstrVal,
			0,
			spASObjClass->GetZ(i).bstrVal,
			spASObjKey->GetZ(i).bstrVal,
			spASObjProp->GetZ(i).bstrVal);
	}

	ITablePtr spAutoLogic = spTables->Item("DFWAutoLogic");
	IColsPtr spALCols = spAutoLogic->Cols;
	IColPtr spALId			= spALCols->Item(_T("Id"));
	IColPtr spALName		= spALCols->Item(_T("Name"));
	IColPtr spALType		= spALCols->Item(_T("Type"));
	IColPtr spALExpr		= spALCols->Item(_T("Formula"));
	IColPtr spALActions		= spALCols->Item(_T("Actions"));
	IColPtr spALDelay		= spALCols->Item(_T("Delay"));
	IColPtr spALOutputMode	= spALCols->Item(_T("OutputMode"));

	for (int i = 0; i < spAutoLogic->GetSize(); i++)
	{
		Network.Automatic().AddLogic(spALType->GetZ(i),
			spALId->GetZ(i).lVal,
			spALName->GetZ(i).bstrVal,
			spALExpr->GetZ(i).bstrVal,
			spALActions->GetZ(i).bstrVal,
			spALDelay->GetZ(i).bstrVal,
			spALOutputMode->GetZ(i).lVal);
	}


	ITablePtr spAutoActions = spTables->Item("DFWAutoAction");
	IColsPtr spAACols = spAutoActions->Cols;
	IColPtr spAAId = spAACols->Item(_T("Id"));
	IColPtr spAAName = spAACols->Item(_T("Name"));
	IColPtr spAAType = spAACols->Item(_T("Type"));
	IColPtr spAAExpr = spAACols->Item(_T("Formula"));
	IColPtr spAAObjClass = spAACols->Item(_T("ObjectClass"));
	IColPtr spAAObjKey = spAACols->Item(_T("ObjectKey"));
	IColPtr spAAObjProp = spAACols->Item(_T("ObjectProp"));
	IColPtr spAAOutputMode = spAACols->Item(_T("OutputMode"));
	IColPtr spAAActionGroup = spAACols->Item(_T("ParentId"));
	IColPtr spAAORunsCount = spAACols->Item(_T("RunsCount"));
	
	for (int i = 0; i < spAutoActions->GetSize(); i++)
	{
		Network.Automatic().AddAction(spAAType->GetZ(i),
			spAAId->GetZ(i).lVal,
			spAAName->GetZ(i).bstrVal,
			spAAExpr->GetZ(i).bstrVal,
			0,
			spAAObjClass->GetZ(i).bstrVal,
			spAAObjKey->GetZ(i).bstrVal,
			spAAObjProp->GetZ(i).bstrVal,
			spAAActionGroup->GetZ(i).lVal,
			spAAOutputMode->GetZ(i).lVal,
			spAAORunsCount->GetZ(i).lVal);
	}

	if (!Network.Automatic().CompileModels())
		return;

#ifdef _WIN64
	if (!Network.AutomaticDevice.ConnectDLL(_T("C:\\tmp\\CustomModels\\dllx64\\autodll.dll")))
		return;
#else
	if (!Network.AutomaticDevice.ConnectDLL(_T("C:\\tmp\\CustomModels\\dll\\autodll.dll")))
			return;
#endif
	
	CCustomDevice *pCustomDevices = new CCustomDevice[1];
	Network.AutomaticDevice.AddDevices(pCustomDevices, 1);
	Network.AutomaticDevice.BuildStructure();
	
	

	
	if (!Network.CustomDevice.ConnectDLL(_T("DeviceDLL.dll")))
		return;
	CustomDeviceConnectInfo ci(_T("ExcControl"),2);
	ITablePtr spExAddXcomp = spRastr->Tables->Item("ExcControl");
	spExAddXcomp->Cols->Add("Xcomp", PR_REAL);
	GetCustomDeviceData(Network, spRastr, ci, Network.CustomDevice);
	
	ITablePtr spLRC = spTables->Item("polin");
	IColsPtr spLRCCols = spLRC->Cols;

	IColPtr spLCLCId = spLRCCols->Item("nsx");
	IColPtr spP0 = spLRCCols->Item("p0");
	IColPtr spP1 = spLRCCols->Item("p1");
	IColPtr spP2 = spLRCCols->Item("p2");
	IColPtr spP3 = spLRCCols->Item("p3");
	IColPtr spP4 = spLRCCols->Item("p4");

	IColPtr spQ0 = spLRCCols->Item("q0");
	IColPtr spQ1 = spLRCCols->Item("q1");
	IColPtr spQ2 = spLRCCols->Item("q2");
	IColPtr spQ3 = spLRCCols->Item("q3");
	IColPtr spQ4 = spLRCCols->Item("q4");

	IColPtr spLCUmin = spLRCCols->Item("umin");
	IColPtr spLCFreq = spLRCCols->Item("frec");

	DBSLC *pLRCBuffer = new DBSLC[spLRC->GetSize()];
	for (int i = 0; i < spLRC->GetSize(); i++)
	{
		DBSLC *pSLC = pLRCBuffer + i;
		pSLC->m_Id = spLCLCId->GetZ(i).lVal;
		pSLC->Vmin = spLCUmin->GetZ(i).dblVal;
		pSLC->P0 = spP0->GetZ(i).dblVal;
		pSLC->P1 = spP1->GetZ(i).dblVal;
		pSLC->P2 = spP2->GetZ(i).dblVal;
		pSLC->Q0 = spQ0->GetZ(i).dblVal;
		pSLC->Q1 = spQ1->GetZ(i).dblVal;
		pSLC->Q2 = spQ2->GetZ(i).dblVal;
	}

	CreateLRCFromDBSLCS(Network, pLRCBuffer, spLRC->GetSize());
	delete [] pLRCBuffer;
		
	ITablePtr spNode = spTables->Item("node");
	IColsPtr spNodeCols = spNode->Cols;
	CDynaNode *pNodes = new CDynaNode[spNode->Size];

	IColPtr spNy = spNodeCols->Item("ny");
	IColPtr spNtype = spNodeCols->Item("tip");
	IColPtr spName = spNodeCols->Item("name");
	IColPtr spUnom = spNodeCols->Item("uhom");
	IColPtr spV = spNodeCols->Item("vras");
	IColPtr spDelta= spNodeCols->Item("delta");
	IColPtr spPn = spNodeCols->Item("pn");
	IColPtr spQn = spNodeCols->Item("qn");
	IColPtr spPnr = spNodeCols->Item("pnr");
	IColPtr spQnr = spNodeCols->Item("qnr");
	IColPtr spPg = spNodeCols->Item("pg");
	IColPtr spQg = spNodeCols->Item("qg");
	IColPtr spG = spNodeCols->Item("gsh");
	IColPtr spB = spNodeCols->Item("bsh");
	IColPtr spLCIdLF = spNodeCols->Item("nsx");
	IColPtr spLCId = spNodeCols->Item("dnsx");
	IColPtr spSta = spNodeCols->Item("sta");
	IColPtr spG0 = spNodeCols->Item("grk");
	IColPtr spB0 = spNodeCols->Item("brk");
	IColPtr spNr = spNodeCols->Item("nrk");
	IColPtr spVref = spNodeCols->Item("vzd");
	IColPtr spQmin = spNodeCols->Item("qmin");
	IColPtr spQmax = spNodeCols->Item("qmax");


	Network.Nodes.AddDevices(pNodes, spNode->Size);

	for (int i = 0; i < spNode->Size; i++)
	{
		pNodes->SetId(spNy->GetZ(i));
		pNodes->SetName(spName->GetZ(i).bstrVal);
		pNodes->SetState(spSta->GetZ(i).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
		pNodes->Unom = spUnom->GetZ(i);
		pNodes->SetDBIndex(i);
		pNodes->V = spV->GetZ(i);
		pNodes->m_eLFNodeType = NodeTypeFromRastr(spNtype->GetZ(i).lVal);
		pNodes->Delta = spDelta->GetZ(i);
		pNodes->Pn = spPnr->GetZ(i);
		pNodes->Qn = spQnr->GetZ(i);
		pNodes->Pnr = spPn->GetZ(i);
		pNodes->Qnr = spQn->GetZ(i);
		pNodes->Pg = spPg->GetZ(i);
		pNodes->Qg = spQg->GetZ(i);
		pNodes->G = spG->GetZ(i);
		pNodes->B = -spB->GetZ(i).dblVal;
		pNodes->Br0 = -spB0->GetZ(i).dblVal;
		pNodes->Gr0 = spG0->GetZ(i);
		pNodes->Nr = spNr->GetZ(i);
		pNodes->LFVref = spVref->GetZ(i);
		pNodes->LFQmin = spQmin->GetZ(i);
		pNodes->LFQmax = spQmax->GetZ(i);
		CDynaLRC *pDynLRC;
		if (Network.LRCs.GetDevice(spLCId->GetZ(i), pDynLRC))
		{
			pNodes->m_pLRC = pDynLRC;
			//if (!LcId && pNodes->V > 0.0)
			//	pNodes->m_dLRCKdef = (pNodes->Unom * pNodes->Unom / pNodes->V / pNodes->V);
		}

		ptrdiff_t nLRCLF = spLCIdLF->GetZ(i);
		if (nLRCLF > 0)
		{
			if (Network.LRCs.GetDevice(nLRCLF, pDynLRC))
				pNodes->m_pLRCLF = pDynLRC;
		}
		pNodes++;
	}

	

	ITablePtr spBranch = spTables->Item("vetv");
	CDynaBranch *pBranches = new CDynaBranch[spBranch->Size];
	IColsPtr spBranchCols = spBranch->Cols;
	IColPtr spIp = spBranchCols->Item("ip");
	IColPtr spIq = spBranchCols->Item("iq");
	IColPtr spNp = spBranchCols->Item("np");
	IColPtr spBSta = spBranchCols->Item("sta");
	IColPtr spR = spBranchCols->Item("r");
	IColPtr spX = spBranchCols->Item("x");
	IColPtr spKtr = spBranchCols->Item("ktr");
	IColPtr spKti = spBranchCols->Item("kti");
	IColPtr spGip = spBranchCols->Item("g");
	IColPtr spBip = spBranchCols->Item("b");
	IColPtr spGrip0 = spBranchCols->Item("gr_ip");
	IColPtr spBrip0 = spBranchCols->Item("br_ip");
	IColPtr spGriq0 = spBranchCols->Item("gr_iq");
	IColPtr spBriq0 = spBranchCols->Item("br_iq");
	IColPtr spNrIp = spBranchCols->Item("nr_ip");
	IColPtr spNrIq = spBranchCols->Item("nr_iq");
	IColPtr spPlIp = spBranchCols->Item("pl_ip");
	IColPtr spQlIp = spBranchCols->Item("ql_ip");
	IColPtr spPlIq = spBranchCols->Item("pl_iq");
	IColPtr spQlIq = spBranchCols->Item("ql_iq");
	IColPtr spIdop = spBranchCols->Item("i_dop");
	IColPtr spI = spBranchCols->Item("ie");

	CDynaBranch::BranchState StateDecode[4] = { CDynaBranch::BRANCH_ON, CDynaBranch::BRANCH_OFF, CDynaBranch::BRANCH_TRIPIP, CDynaBranch::BRANCH_TRIPIQ };

	for (int i = 0; i < spBranch->Size; i++)
	{
		(pBranches + i)->SetId(i);
		int State = spBSta->GetZ(i).lVal;
		(pBranches + i)->m_BranchState = StateDecode[0];
		if (State > 0 && State < 4)
			(pBranches + i)->m_BranchState = StateDecode[State];
		(pBranches + i)->Ip = spIp->GetZ(i);
		(pBranches + i)->Iq = spIq->GetZ(i);
		(pBranches + i)->Np = spNp->GetZ(i);
		(pBranches + i)->R  = spR->GetZ(i);
		(pBranches + i)->X  = spX->GetZ(i);
		(pBranches + i)->Ktr = spKtr->GetZ(i);
		(pBranches + i)->Kti = spKti->GetZ(i);
		(pBranches + i)->G = spGip->GetZ(i);
		(pBranches + i)->B = -spBip->GetZ(i).dblVal;
		(pBranches + i)->GrIp = spGrip0->GetZ(i).dblVal;
		(pBranches + i)->BrIp = -spBrip0->GetZ(i).dblVal;
		(pBranches + i)->GrIq = spGriq0->GetZ(i).dblVal;
		(pBranches + i)->BrIq = -spBriq0->GetZ(i).dblVal;
		(pBranches + i)->NrIp = spNrIp->GetZ(i).lVal;
		(pBranches + i)->NrIq = spNrIq->GetZ(i).lVal;
	}
	Network.Branches.AddDevices(pBranches, spBranch->Size);
	


	ITablePtr spGen = spTables->Item("Generator");
	IColsPtr spGenCols = spGen->Cols;

	IColPtr spGenId = spGenCols->Item("Id");
	IColPtr spGenName = spGenCols->Item("Name");
	IColPtr spGenSta = spGenCols->Item("sta");

	IColPtr spGenUnom = spGenCols->Item("Ugnom");
	IColPtr spGenPnom = spGenCols->Item("Pnom");
	IColPtr spGenCos  = spGenCols->Item("cosFi");
	IColPtr spGenNode = spGenCols->Item("Node");
	IColPtr spGenKgen = spGenCols->Item("NumBrand");
	IColPtr spGenModelType = spGenCols->Item("ModelType");

	IColPtr spGenDemp = spGenCols->Item("Demp");
	IColPtr spGenMj   = spGenCols->Item("Mj");
	IColPtr spGenXd = spGenCols->Item("xd");
	IColPtr spGenXd1  = spGenCols->Item("xd1");
	IColPtr spGenXq   = spGenCols->Item("xq");
	IColPtr spGenTd01 = spGenCols->Item("td01");
	IColPtr spExciterId = spGenCols->Item("ExciterId");

	IColPtr spGenP = spGenCols->Item("P");
	IColPtr spGenQ = spGenCols->Item("Q");

	IColPtr spGenQmin = spGenCols->Item("Qmin");
	IColPtr spGenQmax = spGenCols->Item("Qmax");

	IColPtr spGenXd2 = spGenCols->Item("xd2");
	IColPtr spGenXq1 = spGenCols->Item("xq1");
	IColPtr spGenXq2 = spGenCols->Item("xq2");
	IColPtr spGenTd0ss = spGenCols->Item("td02");
	IColPtr spGenTq0ss = spGenCols->Item("tq02");


	size_t nGen1CCount = 0;
	size_t nGen3CCount = 0;
	size_t nGenMustangCount = 0;
	size_t nGenMotCount = 0;
	size_t nGenInfBusCount = 0;

	for (int i = 0; i < spGen->Size; i++)
	{
		switch (spGenModelType->GetZ(i).lVal)
		{
		case 6:
			nGenMustangCount++;
			break;
		case 5:
			nGen3CCount++;
			break;
		case 4:
			nGen1CCount++;
			break;
		case 3:
			nGenMotCount++;
			break;
		case 2:
			nGenInfBusCount++;
			break;
		}
	}

	CDynaGeneratorMustang	*pGensMu  = new CDynaGeneratorMustang[nGenMustangCount];
	CDynaGenerator3C		*pGens3C  = new CDynaGenerator3C[nGen3CCount];
	CDynaGenerator1C		*pGens1C  = new CDynaGenerator1C[nGen1CCount];
	CDynaGeneratorInfBus	*pGensInf = new CDynaGeneratorInfBus[nGenInfBusCount];
	CDynaGeneratorMotion	*pGensMot = new CDynaGeneratorMotion[nGenMotCount];

	Network.Generators3C.AddDevices(pGens3C, nGen3CCount);
	Network.GeneratorsMustang.AddDevices(pGensMu, nGenMustangCount);
	Network.Generators1C.AddDevices(pGens1C, nGen1CCount);
	Network.GeneratorsMotion.AddDevices(pGensMot, nGenMotCount);
	Network.GeneratorsInfBus.AddDevices(pGensInf, nGenInfBusCount);


	for (int i = 0; i < spGen->Size; i++)
	{
		switch (spGenModelType->GetZ(i).lVal)
		{
		case 6:
			pGensMu->SetId(spGenId->GetZ(i));
			pGensMu->SetName(spGenName->GetZ(i).bstrVal);
			pGensMu->SetState(spGenSta->GetZ(i).boolVal ? DS_OFF : DS_ON, DSC_EXTERNAL);
			pGensMu->NodeId = spGenNode->GetZ(i);
			pGensMu->Kdemp = spGenDemp->GetZ(i);
			pGensMu->Kgen = spGenKgen->GetZ(i);
			pGensMu->P = spGenP->GetZ(i);
			pGensMu->Q = spGenQ->GetZ(i);
			pGensMu->cosPhinom = spGenCos->GetZ(i);
			pGensMu->Unom = spGenUnom->GetZ(i);
			pGensMu->Mj = spGenMj->GetZ(i);
			pGensMu->xd1 = spGenXd1->GetZ(i);
			pGensMu->xq = spGenXq->GetZ(i);
			pGensMu->xd = spGenXd->GetZ(i);
			pGensMu->Td01 = spGenTd01->GetZ(i);
			pGensMu->Pnom = spGenPnom->GetZ(i);
			pGensMu->m_ExciterId = spExciterId->GetZ(i);
			pGensMu->xd2 = spGenXd2->GetZ(i);
			pGensMu->xq1 = spGenXq1->GetZ(i);
			pGensMu->xq2 = spGenXq2->GetZ(i);
			pGensMu->Td0ss = spGenTd0ss->GetZ(i);
			pGensMu->Tq0ss = spGenTq0ss->GetZ(i);
			pGensMu->LFQmax = spGenQmax->GetZ(i);
			pGensMu->LFQmin = spGenQmin->GetZ(i);
			pGensMu++;
			break;
		case 5:
			pGens3C->SetId(spGenId->GetZ(i));
			pGens3C->SetName(spGenName->GetZ(i).bstrVal);
			pGens3C->SetState(spGenSta->GetZ(i).boolVal ? DS_OFF : DS_ON, DSC_EXTERNAL);
			pGens3C->NodeId = spGenNode->GetZ(i);
			pGens3C->Kdemp = spGenDemp->GetZ(i);
			pGens3C->Kgen  = spGenKgen->GetZ(i);
			pGens3C->P = spGenP->GetZ(i);
			pGens3C->Q = spGenQ->GetZ(i);
			pGens3C->cosPhinom = spGenCos->GetZ(i);
			pGens3C->Unom = spGenUnom->GetZ(i);
			pGens3C->Mj = spGenMj->GetZ(i);
			pGens3C->xd1 = spGenXd1->GetZ(i);
			pGens3C->xq = spGenXq->GetZ(i);
			pGens3C->xd = spGenXd->GetZ(i);
			pGens3C->Td01 = spGenTd01->GetZ(i);
			pGens3C->Pnom = spGenPnom->GetZ(i);
			pGens3C->m_ExciterId = spExciterId->GetZ(i);
			pGens3C->xd2 = spGenXd2->GetZ(i);
			pGens3C->xq1 = spGenXq1->GetZ(i);
			pGens3C->xq2 = spGenXq2->GetZ(i);
			pGens3C->Td0ss = spGenTd0ss->GetZ(i);
			pGens3C->Tq0ss = spGenTq0ss->GetZ(i);
			pGens3C->LFQmax = spGenQmax->GetZ(i);
			pGens3C->LFQmin = spGenQmin->GetZ(i);
			pGens3C++;
			break;
		case 4:
			pGens1C->SetId(spGenId->GetZ(i));
			pGens1C->SetName(spGenName->GetZ(i).bstrVal);
			pGens1C->SetState(spGenSta->GetZ(i).boolVal ? DS_OFF : DS_ON, DSC_EXTERNAL);
			pGens1C->NodeId = spGenNode->GetZ(i);
			pGens1C->Kgen = spGenKgen->GetZ(i);
			pGens1C->Kdemp = spGenDemp->GetZ(i);
			pGens1C->P = spGenP->GetZ(i);
			pGens1C->Q = spGenQ->GetZ(i);
			pGens1C->cosPhinom = spGenCos->GetZ(i);
			pGens1C->Unom = spGenUnom->GetZ(i);
			pGens1C->Mj = spGenMj->GetZ(i);
			pGens1C->xd1 = spGenXd1->GetZ(i);
			pGens1C->xq = spGenXq->GetZ(i);
			pGens1C->xd = spGenXd->GetZ(i);
			pGens1C->Td01 = spGenTd01->GetZ(i);
			pGens1C->Pnom = spGenPnom->GetZ(i);
			pGens1C->m_ExciterId = spExciterId->GetZ(i);
			pGens1C->LFQmax = spGenQmax->GetZ(i);
			pGens1C->LFQmin = spGenQmin->GetZ(i);
			pGens1C++;
			break;
		case 2:
			pGensInf->SetId(spGenId->GetZ(i));
			pGensInf->SetName(spGenName->GetZ(i).bstrVal);
			pGensInf->SetState(spGenSta->GetZ(i).boolVal ? DS_OFF : DS_ON, DSC_EXTERNAL);
			pGensInf->NodeId = spGenNode->GetZ(i);
			pGensInf->Kgen = spGenKgen->GetZ(i);
			pGensInf->P = spGenP->GetZ(i);
			pGensInf->Q = spGenQ->GetZ(i);
			pGensInf->xd1 = spGenXd1->GetZ(i);
			pGensInf->LFQmax = spGenQmax->GetZ(i);
			pGensInf->LFQmin = spGenQmin->GetZ(i);
			pGensInf++;
			break;
		case 3:
			pGensMot->SetId(spGenId->GetZ(i));
			pGensMot->SetName(spGenName->GetZ(i).bstrVal);
			pGensMot->SetState(spGenSta->GetZ(i).boolVal ? DS_OFF : DS_ON, DSC_EXTERNAL);
			pGensMot->NodeId = spGenNode->GetZ(i);
			pGensMot->Kgen = spGenKgen->GetZ(i);
			pGensMot->Kdemp = spGenDemp->GetZ(i);
			pGensMot->P = spGenP->GetZ(i);
			pGensMot->Q = spGenQ->GetZ(i);
			pGensMot->cosPhinom = spGenCos->GetZ(i);
			pGensMot->Unom = spGenUnom->GetZ(i);
			pGensMot->Mj = spGenMj->GetZ(i);
			pGensMot->xd1 = spGenXd1->GetZ(i);
			pGensMot->Pnom = spGenPnom->GetZ(i);
			pGensMot->LFQmax = spGenQmax->GetZ(i);
			pGensMot->LFQmin = spGenQmin->GetZ(i);
			pGensMot++;
		}
	}

	ITablePtr spExciter = spTables->Item("Exciter");
	IColsPtr  spExcCols = spExciter->Cols;
	IColPtr spExcId		= spExcCols->Item("Id");
	IColPtr spExcName	= spExcCols->Item("Name");
	IColPtr spExcTexc	= spExcCols->Item("Texc");
	IColPtr spExcUmin   = spExcCols->Item("Uf_min");
	IColPtr spExcUmax   = spExcCols->Item("Uf_max");
	IColPtr spExcImin	= spExcCols->Item("If_min");
	IColPtr spExcImax	= spExcCols->Item("If_max");
	IColPtr spExcKif	= spExcCols->Item("Kif");
	IColPtr spExcKig	= spExcCols->Item("Kig");
	IColPtr spExcVD		= spExcCols->Item("Type_rg");
	IColPtr spExcDECId = spExcCols->Item("ForcerId");
	IColPtr spExcRegId = spExcCols->Item("ExcControlId");

	size_t nMustangExcitersCount = spExciter->Size;

	CDynaExciterMustang *pExcitersMustang = new CDynaExciterMustang[nMustangExcitersCount];
	Network.ExcitersMustang.AddDevices(pExcitersMustang, nMustangExcitersCount);

	for (int i = 0; i < spExciter->Size; i++)
	{
		pExcitersMustang->SetId(spExcId->GetZ(i));
		pExcitersMustang->SetName(spExcName->GetZ(i).bstrVal);
		pExcitersMustang->Texc = spExcTexc->GetZ(i);
		pExcitersMustang->Umin = spExcUmin->GetZ(i);
		pExcitersMustang->Umax = spExcUmax->GetZ(i);
		pExcitersMustang->Imin = spExcImin->GetZ(i);
		pExcitersMustang->Imax = spExcImax->GetZ(i);
		pExcitersMustang->Kig  = spExcKig->GetZ(i);
		pExcitersMustang->Kif  = spExcKif->GetZ(i);
		pExcitersMustang->DECId = spExcDECId->GetZ(i);
		pExcitersMustang->RegId = spExcRegId->GetZ(i);
		pExcitersMustang->bVoltageDependent = (spExcVD->GetZ(i).lVal == 1) ? true : false ;
		pExcitersMustang++;
	}


	ITablePtr spForcer = spTables->Item("Forcer");
	IColsPtr  spForcerCols = spForcer->Cols;

	IColPtr spForcerId	 = spForcerCols->Item("Id");
	IColPtr spForcerName = spForcerCols->Item("Name");
	IColPtr spForcerVEnfOn = spForcerCols->Item("Ubf");
	IColPtr spForcerVEnfOff = spForcerCols->Item("Uef");
	IColPtr spForcerVDefOn = spForcerCols->Item("Ubrf");
	IColPtr spForcerVDefOff = spForcerCols->Item("Uerf");
	IColPtr spForcerEnfRatio = spForcerCols->Item("Rf");
	IColPtr spForcerDefRatio = spForcerCols->Item("Rrf");
	IColPtr spForcerEnfTexc = spForcerCols->Item("Texc_f");
	IColPtr spForcerDefTexc = spForcerCols->Item("Texc_rf");
	IColPtr spForcerTdelOn = spForcerCols->Item("Tz_in");
	IColPtr spForcerTdelOff = spForcerCols->Item("Tz_out");

	size_t nMustangDECsCount = spForcer->Size;

	CDynaDECMustang *pDECsMustang = new CDynaDECMustang[nMustangDECsCount];
	Network.DECsMustang.AddDevices(pDECsMustang, nMustangDECsCount);

	for (int i = 0; i < static_cast<ptrdiff_t>(nMustangDECsCount); i++)
	{
		pDECsMustang->SetId(spForcerId->GetZ(i));
		pDECsMustang->SetName(spForcerName->GetZ(i).bstrVal);
		pDECsMustang->VEnfOn = spForcerVEnfOn->GetZ(i);
		pDECsMustang->VEnfOff = spForcerVEnfOff->GetZ(i);
		pDECsMustang->VDefOn = spForcerVDefOn->GetZ(i);
		pDECsMustang->VDefOff = spForcerVDefOff->GetZ(i);
		pDECsMustang->EnfRatio = spForcerEnfRatio->GetZ(i);
		pDECsMustang->DefRatio = spForcerDefRatio->GetZ(i);
		pDECsMustang->EnfTexc = spForcerEnfTexc->GetZ(i);
		pDECsMustang->DefTexc = spForcerDefTexc->GetZ(i);
		pDECsMustang->TdelOn = spForcerTdelOn->GetZ(i);
		pDECsMustang->TdelOff = spForcerTdelOff->GetZ(i);
		pDECsMustang++;
	}

	ITablePtr spExcConMustang = spTables->Item("ExcControl");
	IColsPtr spEСMCols = spExcConMustang->Cols;

	IColPtr spECMId = spEСMCols->Item("Id");
	IColPtr spECMName = spEСMCols->Item("Name");
	IColPtr spECMTr = spEСMCols->Item("Trv");
	IColPtr spECMK0u = spEСMCols->Item("Ku");
	IColPtr spECMK1u = spEСMCols->Item("Ku1");
	IColPtr spECMK1if = spEСMCols->Item("Kif1");
	IColPtr spECMK0f = spEСMCols->Item("Kf");
	IColPtr spECMK1f = spEСMCols->Item("Kf1");
	IColPtr spECMTf = spEСMCols->Item("Tf");
	IColPtr spECMUmin = spEСMCols->Item("Urv_min");
	IColPtr spECMUmax = spEСMCols->Item("Urv_max");
	IColPtr spECMA = spEСMCols->Item("Alpha");

	size_t nMustangExcConsCount = spExcConMustang->Size;

	CDynaExcConMustang *pExcConsMustang = new CDynaExcConMustang[nMustangExcConsCount];
	Network.ExcConMustang.AddDevices(pExcConsMustang, nMustangExcConsCount);

	for (int i = 0; i < spExcConMustang->Size; i++)
	{
		pExcConsMustang->SetId(spECMId->GetZ(i));
		pExcConsMustang->SetName(spECMName->GetZ(i).bstrVal);
		pExcConsMustang->Alpha = spECMA->GetZ(i);
		pExcConsMustang->Umin = spECMUmin->GetZ(i);
		pExcConsMustang->Umax = spECMUmax->GetZ(i);
		pExcConsMustang->Tf   = spECMTf->GetZ(i);
		pExcConsMustang->K1f  = spECMK1f->GetZ(i);
		pExcConsMustang->K0f = spECMK0f->GetZ(i);
		pExcConsMustang->K1if = spECMK1if->GetZ(i);
		pExcConsMustang->K1u  = spECMK1u->GetZ(i);
		pExcConsMustang->K0u  = spECMK0u->GetZ(i);
		pExcConsMustang->Tr   = spECMTr->GetZ(i);
		pExcConsMustang++;
	}
	
}

bool CRastrImport::CreateLRCFromDBSLCS(CDynaModel& Network, DBSLC *pLRCBuffer, ptrdiff_t nLRCCount)
{
	bool bRes = true;
	CSLCLoader slcloader;
	SLCSITERATOR slit;

	double Vmin = Network.GetLRCToShuntVmin();

	for (ptrdiff_t i = 0; i < nLRCCount; i++)
	{
		DBSLC *pSLC = pLRCBuffer + i;
		if (pSLC->m_Id == 1 || pSLC->m_Id == 2)
		{
			Network.Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszLRC1And2Reserved,pSLC->m_Id));
			continue;
		}

		slit = slcloader.find(pSLC->m_Id);
		if (slit == slcloader.end())
			slit = slcloader.insert(make_pair(pSLC->m_Id, new CStorageSLC())).first;

		CSLCPolynom P(pSLC->Vmin, pSLC->P0, pSLC->P1, pSLC->P2);
		CSLCPolynom Q(pSLC->Vmin, pSLC->Q0, pSLC->Q1, pSLC->Q2);

		if (!P.IsEmpty())
			slit->second->P.push_back(P);
		if (!Q.IsEmpty())
			slit->second->Q.push_back(Q);
	}

	for (slit = slcloader.begin(); slit != slcloader.end();)
	{
		if (!slit->second->P.size() && !slit->second->Q.size())
		{
			delete slit->second;
			slcloader.erase(slit++);
		}
		else
			slit++;
	}

	SLCPOLYITR  itpoly;

	for (slit = slcloader.begin(); slit != slcloader.end(); slit++)
	{
		for (int pq = 0; pq < 2; pq++)
		{
			SLCPOLY& poly = pq ? slit->second->Q : slit->second->P;

			poly.sort();

			if (poly.size())
			{
				double Vbeg = poly.front().m_kV;
				if (Vbeg > 0.0)
				{
					Network.Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszLRCStartsNotFrom0, slit->first, Vbeg));
					poly.front().m_kV = 0.0;
				}
			}

			itpoly = poly.begin();
			bool bAmbiguityWarned = false;
			while (itpoly != poly.end())
			{
				SLCPOLYITR itpolyNext = itpoly;
				itpolyNext++;
				if (itpolyNext != poly.end())
				{
					if (Equal(itpoly->m_kV, itpolyNext->m_kV))
					{
						if (!bAmbiguityWarned)
						{
							if (!itpoly->CompareWith(*itpolyNext))
							{
								Network.Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszAmbigousLRCSegment, slit->first, 
																													    itpoly->m_kV, 
																														itpoly->m_k0,
																														itpoly->m_k1, 
																														itpoly->m_k2));
								bAmbiguityWarned = true;
							}
						}
						poly.erase(itpolyNext);
					}
					itpoly++;
				}
				else
					break;
			}
		}
	}

	// типовые СХН Rastr 1 и 2
	if (slcloader.find(1) == slcloader.end())
	{
		slit = slcloader.insert(make_pair(1, new CStorageSLC())).first;
		slit->second->P.push_back(CSLCPolynom(0.0, 0.83, -0.3, 0.47));
		slit->second->Q.push_back(CSLCPolynom(0.0, 0.721, 0.15971, 0.0));
		slit->second->Q.push_back(CSLCPolynom(0.815, 3.7, -7.0, 4.3));
		slit->second->Q.push_back(CSLCPolynom(1.2, 1.492, 0.0, 0.0));
	}
	if (slcloader.find(2) == slcloader.end())
	{
		slit = slcloader.insert(make_pair(2, new CStorageSLC())).first;
		slit->second->P.push_back(CSLCPolynom(0.0, 0.83, -0.3, 0.47));
		slit->second->Q.push_back(CSLCPolynom(0.0, 0.657, 0.159135, 0.0));
		slit->second->Q.push_back(CSLCPolynom(0.815, 4.9, -10.1, 6.2));
		slit->second->Q.push_back(CSLCPolynom(1.2, 1.708, 0.0, 0.0));
	}

	// СХН шунт с Id=0
	slit = slcloader.insert(make_pair(0, new CStorageSLC())).first;
	slit->second->P.push_back(CSLCPolynom(0.0, 0.0, 0.0, 1.0));
	slit->second->Q.push_back(CSLCPolynom(0.0, 0.0, 0.0, 1.0));

	// СХН с постоянной мощностью с Id=-1
	slit = slcloader.insert(make_pair(-1, new CStorageSLC())).first;
	slit->second->P.push_back(CSLCPolynom(0.0, 1.0, 0.0, 0.0));
	slit->second->Q.push_back(CSLCPolynom(0.0, 1.0, 0.0, 0.0));

	// insert shunt LRC
	for (slit = slcloader.begin(); slit != slcloader.end(); slit++)
	{
		bRes = bRes && slit->second->P.InsertLRCToShuntVmin(Vmin);
		bRes = bRes && slit->second->Q.InsertLRCToShuntVmin(Vmin);
	}
	
	if (bRes)
	{
		// переписываем СХН из загрузчика в котейнер СХН

		CDynaLRC *pLRCs = new CDynaLRC[slcloader.size()];
		CDynaLRC *pLRC = pLRCs;

		for (slit = slcloader.begin(); slit != slcloader.end(); slit++, pLRC++)
		{
			pLRC->SetId(slit->first);
			pLRC->SetNpcs(slit->second->P.size(), slit->second->Q.size());

			CLRCData *pData = pLRC->P;

			for (itpoly = slit->second->P.begin(); itpoly != slit->second->P.end(); itpoly++)
			{
				pData->V	=	itpoly->m_kV;
				pData->a0   =	itpoly->m_k0;
				pData->a1	=	itpoly->m_k1;
				pData->a2	=	itpoly->m_k2;
				pData++;
			}

			pData = pLRC->Q;

			for (itpoly = slit->second->Q.begin(); itpoly != slit->second->Q.end(); itpoly++)
			{
				pData->V	=	itpoly->m_kV;
				pData->a0	=	itpoly->m_k0;
				pData->a1	=	itpoly->m_k1;
				pData->a2	=	itpoly->m_k2;
				pData++;
			}
		}
		Network.LRCs.AddDevices(pLRCs, slcloader.size());
	}
	return bRes;
}

// вставляет в СХН сегмент шунта от нуля до Vmin
bool SLCPOLY::InsertLRCToShuntVmin(double Vmin)
{
	bool bRes = true;
	if (Vmin > 0.0)
	{
		// сортируем сегменты по напряжению
		sort();
		double LrcV = 0.0;
		bool bInsert = false;

		// обходим сегменты справа, для первого найденного с напряжением
		// меньше чем Vmin считаем мощность по этом сегменту от Vmin
		// и меняем ему напряжение на Vmin
		for (SLCPOLYRITR itrpoly = rbegin(); itrpoly != rend(); itrpoly++)
		{
			if (itrpoly->m_kV < Vmin)
			{
				LrcV = itrpoly->m_k0 + Vmin * itrpoly->m_k1 + Vmin * Vmin * itrpoly->m_k2;
				itrpoly->m_kV = Vmin;
				bInsert = true;
				break;
			}
		}
		if (bInsert)
		{
			// если нашли сегмент для Vmin удаляем все сегменты
			// с напряжением меньше чем Vmin
			while (size())
			{
				SLCPOLYITR itpoly = begin();
				if (itpoly->m_kV < Vmin)
					erase(itpoly);
				else
					break;
			}
			// вставляем новый сегмент шунта от нуля
			insert(begin(),CSLCPolynom(0.0, 0.0, 0.0, LrcV / Vmin / Vmin));
		}

		// снова сортируем по напряжению
		sort();
		SLCPOLYITR itpoly = begin();

		// ищем последовательные сегменты с одинаковыми коэффициентами
		// и удаляем их как изыбыточные
		while(itpoly != end())
		{
			SLCPOLYITR itpolyNext = itpoly;
			itpolyNext++;
			if (itpolyNext != end())
			{
				if (itpoly->CompareWith(*itpolyNext))
					erase(itpolyNext);
				itpoly++;
			}
			else
				break;
		}
	}
	return bRes;
}

CDynaNodeBase::eLFNodeType CRastrImport::NodeTypeFromRastr(long RastrType)
{
	if (RastrType >= 0 && RastrType < _countof(RastrTypesMap))
		return RastrTypesMap[RastrType];

	_ASSERTE(RastrType >= 0 && RastrType < _countof(RastrTypesMap));
	return CDynaNodeBase::eLFNodeType::LFNT_PQ;
}

const CDynaNodeBase::eLFNodeType CRastrImport::RastrTypesMap[5] = {   CDynaNodeBase::eLFNodeType::LFNT_BASE, 
																	  CDynaNodeBase::eLFNodeType::LFNT_PQ, 
																	  CDynaNodeBase::eLFNodeType::LFNT_PV, 
																	  CDynaNodeBase::eLFNodeType::LFNT_PVQMAX, 
																	  CDynaNodeBase::eLFNodeType::LFNT_PVQMIN 
															      };

#include "stdafx.h"
#include "rg_ss.h"

// ��� sp  = TRUE //������ ��������. �������� PQ(+/-)->PV

int steady_state::con_q(ptrdiff_t ny,int sp)
{
	int imin=FALSE, imax=FALSE, iab=FALSE, iko=FALSE, ikf=FALSE;
	pnod1  pny;
		int    nRes = 0;
		int    nFactsIndx = CFACTSs::NO_FACTS;
		compl  sn;
		compl  rt2;
		compl  ret;
		compl  retf( 0., 0. );
		int    nKod = 0;
		double dBF = 0.;
bool  ab_ogr=false;		
	double dvr,dq;
	if(z_ogr) return(0);
	bool repeat=false;

	do 
	{
		// � ���������� ��������� ���� �������� ���� ���
		repeat=false;
		for (pny=nod1_s;pny<nod1_s+ny;pny++)
		{
			// ��������� ����������� ���� ?
			pny->nkg=ZERO;
			// ���� � �������������� ������������� ���������
			if (method_ogr==DOP_OGR && pny->block_ogr) continue;
			if(pny->tconnect==N_SLAVE) continue;
			// �������� ��� ����������� ���� �� ���� ����������� �������
			int tipp = pny->get_tip(fwc);
			// if(pny->ltip==ACTT||pny->ltip==CONTR) continue;
			{
				switch(tipp)
				{
				case GEN:
					//			if(sp) break;
					// ��� ������������� ���� ����� �������� ���������� ��������
					dq=-pny->r.imag();
					// ���� ��� ������ ������������ ����������� � ������ ����������� ���������
					if(dq< pny->qmin-consb.q)
					{
						//		if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"=QMIN :%d Q�=%.2f Qmin=%.2f\n",pny->npu,-pny->r.imag(),pny->qmin);
						//	if (pny->action==V_MAX) pny->anormal=V_MAX;

						// ���� �� ��������� � ������ ������ ������ 
						if (!sp)
						{
							// ��������� ���� �� qmin, ������� ���� ��� ���� ����������� qmin
							pny->nkg=Q_MIN;
							imin=TRUE;
						}
					}
					// ���� �������� ������ ������������� �����������
					else if(dq> pny->qmax+consb.q)
					{
						//		if(ogr_out) log_printf(LOG_INFO,pny->rindex,NODE_V,"=QMAX :%d Q�=%.2f Qmax=%.2f\n",pny->npu,-pny->r.imag(),pny->qmax);
						//	if (pny->action==V_MIN) pny->anormal=V_MIN;
						// ���� �� ��������� � ������ ������ ������ 
						if (!sp)
						{
							// ��������� ���� �� qmax, ������� ���� ��� ���� ����������� qmax
							pny->nkg=Q_MAX; 
							imax=TRUE;
						}
					}
					break;

				case UPR_TRANS:
					if ( sp ) break;
					{
						pregul_tr tr= pregul_tr (pny->pgen);
						if (tr->kt > tr->kt_max)
						{
							pny->nkg=KT_MAX;
							imax=TRUE;

						}

						if (tr->kt < tr->kt_min)
						{
							pny->nkg=KT_MIN;
							imin=TRUE;

						}
					}
					break;
				case GEN_MAX:

//					if(pny->v > pny->vzd)
					// ��� ���� �� ������� �����������
					// ��������� ���������� �� �������
					dvr = pny->get_difv();

					if (dvr >consb.v)
					{
						// ���� ���������� ������ �������
						//if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"=VMAX :%d V=%.2f > V��=%.2f \n",pny->npu,pny->v,pny->vzd);

						if(pny->sstip==ACTT )		//sstip ������� � ����������� ����� ������
						{
							if(pny->v <=pny->umax)
							{
								pny->vzd=pny->v;
								continue;
							}
							pny->vzd=pny->umax;
						}
					
						
						if(pny->action==Q_MAX && dvr > consb.vabn/*0.01*/)
						{
							// ���� ���� �� qmax � ���������� �� ������� ����� ��������
							// ������ ���� ������� ������������� (���� �� �� qmax � ���������� ���� ������� ����� ��� �� �������)
							pny->anormal=int( max(dvr*100,20));
							// ������ ���� �� Vmin � ������� ���� ��� ���� ���� � ����������� ���������� �� �������
							pny->nkg=V_MIN;
							iko=TRUE;
							//       if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"���������� :%d V=%.2f > V��=%.2f \n",pny->npu,pny->v,pny->vzd);
						}


						// ���� � ������ ������ ������
						if(sp)
						{
							// ���� ���������� �� ������� ������ 10%
							if(dvr> consb.dv2)		//consb.dv1 > consb.dv2
							{
								// ������ ���� �� Vmin 
								// � ������� ���� ��� ���� ab ���������� ����
								pny->nkg=V_MIN;
								iab=TRUE;
							}
							else if(dvr>consb.dv1)
							{
								// ���� ���������� �� ������� ������ 5%
								// ������ �� Vmin � ������� ���� ��� ���� �����-�� ����������
								pny->nkg=V_MIN;
								iko=TRUE;
							}
						}
						else
						{
							// ��� ������� ������� ������ ������ ������ ���� �� Vmin � ��������
							// ��� ���� ����������� qmin
							pny->nkg=V_MIN;
							imin=TRUE;
						}
					}
					break;
				case UPR_TRANS_MAX:
					if(pny->v > pny->vzd)
					{
						pny->nkg=KT_MAX_REST;
						imin=TRUE;

					}
					break;
				case GEN_MIN:
					dvr = pny->get_difv();
//					if(pny->v < pny->vzd)

					if (dvr <-consb.v)
					{	
						// ���� ���������� ���� �� �������� ������ �������

						//if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"=VMIN :%d V=%.2f  < V��=%.2f \n",pny->npu,pny->v,pny->vzd);
						if(pny->sstip==ACTT)		//sstip ������� � ����������� ����� ������
						{
							if(pny->v >=pny->umin)
							{
								pny->vzd=pny->v;
								continue;
							}
							pny->vzd=pny->umin;

						}
//						dvr=(pny->vzd - pny->v)/pny->vzd;

						if(pny->action==Q_MIN && dvr < -consb.vabn)
						{
							// ���� ���� �� �������� � ���������� ������ 1%
							int idv=int(abs(dvr)*100);
							// ������� ���������� � ������ ��� � ������� �������������� ����
							if (idv >20) idv=21;
							pny->anormal=idv;
							// ���� ��������� �� Vmax
							// � �������� ��� ���� ���������� � �������
							pny->nkg=V_MAX;
							iko=TRUE;
							//     if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"���������� :%d V=%.2f < V��=%.2f \n",pny->npu,pny->v,pny->vzd);
						}
						if(sp)
						{
							// ���� � ������ ������ ������ ���������� ���������� ������ 10%
							if(dvr< -consb.dv2)
							{
								// ��������� ���� �� Vmax
								// ������ ���� ab ���������� �����
								pny->nkg=V_MAX;
								iab=TRUE;
							}
							else if(dvr<-consb.dv1 )
							{
								// ���� ���������� ������ 5%
								// ��������� �� Vmax � ������ ���� ��� ���� ���� � �����������
								pny->nkg=V_MAX;
								iko=TRUE;
							}
						}
						else
						{
							//if (pny->action==Q_MIN) pny->anormal=Q_MIN;

							// ��� ������� ������� ������ ������ ������ ���� �� Vmax
							pny->nkg=V_MAX;
							imax=TRUE;
						}
					}
					break;
				case UPR_TRANS_MIN:
					if(pny->v < pny->vzd)
					{	
						pny->nkg=KT_MIN_REST;
						imax=TRUE;
					}
					break;
				default     :
					break;
				}
			}
		}
		/* window(1,cas->str_hlp-cas->str_hlp/5+1,80,cas->str_hlp-1);textattr(cas->atr6);clrscr();*/

		for( pny = nod1_s ; pny < nod1_s + ny ; pny++ )
		{
			nRes = m_FACTSs.HaveNodeFacts( pny - nod1_s, &nFactsIndx );
			if( ( nRes > 0 ) && ( nFactsIndx != CFACTSs::NO_FACTS ) )
			{
				double dB = 0.;
				dB = m_FACTSs.GetFACTS( nFactsIndx )->dBF;
				retf = m_FACTSs.Calc( sp, nFactsIndx, pny->v, pny->uhom, &sn, rt2, fwc, dff, pny->kfc, pny->pg_nom, &dBF, &nKod );
				dB -= m_FACTSs.GetFACTS( nFactsIndx )->dBF;
				if( abs( dB ) > 1 )
					ikf = 1;
				/*
				if( nKod != 0 )			
				ikf = 1;
				//pny->nobr->nhp->y = pny->y_ish + compl( 0, -dBF * 1E-6 );
				*/
			};
			ATLASSERT( nRes > 0 );
		};

		// ������ ���� �����-�� ��� �������� ��� ������������
		if(test_iter && method_ogr==DOP_OGR) 
		{

			bool all_anormal=true;
			bool have_anormal=false;
			int max_anormal=0;;

			for (pny=nod1_s;pny<nod1_s+ny;pny++)
			{
				// ���� � ���� ���� �����-�� �������� �� ��������� � ��� �� ������������, �� ���������� ����, ��� ��� ���� ������������
				if (pny->action && (pny->action!=pny->anormal) ) all_anormal=false;		
				// ���� ������������ ������������ ���������� �����
				if(abs(pny->anormal) > max_anormal) max_anormal=abs(pny->anormal);
			}
			//		if (all_anormal)
			if (max_anormal >21) max_anormal=21;		// ������������ ������������ ���������� ������-�� 21
			// ���������� ������������ ������������ ���������� (������ �� ���� ������)
			glob_anormal=max(max_anormal,glob_anormal);
			// ������������ ������������ ���������� �� 2/3
			 max_anormal=int (double(max_anormal)* 2/3);
			 // ���� ����� ����� ������������ ���������� ������ 2%, �� ���������� ���
			if (max_anormal	<=2) max_anormal=0;
			for(pny=nod1_s;pny<nod1_s+ny;pny++)
			{
				if(abs(pny->anormal) >max_anormal)
				{ 
					// ���� ���������� ���� ������ ������������
				have_anormal=true;	// ������ ���� ��� ���� �����������
				pny->block_ogr=true;	// ��������� ����������� ������ ����
				pny->count_bloc+=pny->anormal;	// ����������� ������� ���������� ���� �� ������������ ���������� ?
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"����������� :%d V=%.2f > V��=%.2f \n",pny->npu,pny->v,pny->vzd);
				}
			}
			// ���� ���� ������������ ����, ������� ���� �������������, ��� ����� � ���������������� ������������� ������� ��� ����������� ?
			if (have_anormal ) { iab=true; for (pny=nod1_s;pny<nod1_s+ny;pny++) if (!pny->block_ogr) pny->nkg=0;}
			// ���� ������������ ����� ���, �������� �������� ��������
			else test_iter=0;
		//	if (all_anormal)	iab=TRUE;
		}

		// ���������� ���� ������������ �����������
		 ab_ogr=false;		
		 // ���� ��� ����������� �� ��������, �� ���������, ������������, ���������� ���������� � �������, � ����� �� ��������� � ������ ����������
		if( !imax && !imin && !iab && !iko  && !ikf )    if ( !sp) 
		{
			for (pny=nod1_s;pny<nod1_s+ny;pny++) 
			{
				if (pny->block_ogr && abs(pny->count_bloc)<=glob_anormal) 
				{ 
					// ���� � ���� ������������� ����������� � ���������� ����������� ������ ����������� (�� ���� ������)
					// ��������� ����� �����������
				repeat=true;
				pny->block_ogr=false; // ������� ���������� ����������� ����
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"����a ���������� :%d Q=%.2f  Qmin=%.2f Qmax=%.2f \n",pny->npu,-pny->r.imag(),pny->qmin,pny->qmax);

				}
				// ���� ����� ���� � ������������� (� � ���� ������� ������ ������ ��� ���������� �������), ������� ���� ������������ �����������
				if (pny->block_ogr) ab_ogr=true;
			}

		}
	}while(repeat);

	if 	( ab_ogr		)
	{
		// ���� ���� ������������ �����������
int stag=_Module.Logger.OpenStage ("���������� ��������� ����������� � �����������");
		// ������� ������ ������������ ����� � ������������� �������������
			for (pny=nod1_s;pny<nod1_s+ny;pny++) if (pny->block_ogr)
			{
			log_printf(LOG_ERROR,pny->rindex,NODE_V,"���� :%d Q=%.2f  Qmin=%.2f Qmax=%.2f \n",pny->npu,-pny->r.imag(),pny->qmin,pny->qmax);
			}
			_Module.Logger.CloseStage (stag);
			// ������� � ���������� ������
		return (AB);
	}

	// ���� ��� ����������� �� ��������, �������, ������������, ���������� ���������� � ������� - �������, ���� ��� ������
	if( !imax && !imin && !iab && !iko  && !ikf )   return(ZERO);


	// ���� ���� - ������� ������������ ������
	if( ikf == 1 )
	{
		for( pny = nod1_s ; pny < nod1_s + ny ; pny++ )
		{
			nRes = m_FACTSs.HaveNodeFacts( pny - nod1_s, &nFactsIndx );
			if( ( nRes > 0 ) && ( nFactsIndx != CFACTSs::NO_FACTS ) )
			{
				retf = m_FACTSs.Calc( sp, nFactsIndx, pny->v, pny->uhom, &sn, rt2, fwc, dff, pny->kfc, pny->pg_nom, &dBF, &nKod );
				compl dy( 0., 0. );
				dy = pny->nobr->nhp->y;
				pny->nobr->nhp->y = pny->y_ish + compl( 0, dBF * 1E-6 );
				dy -= pny->nobr->nhp->y;
				compl dQ( 0., 0. );
				dQ = dy * pny->u* conj( pny->u );
				pny->r += dQ;
				max_p( pny );
			};
			ATLASSERT( nRes > 0 );
		};
	};

	// fputs(sp?"V-":"VQ-",stpro);
	//if(sp){if(iab) fputs("�⬥�� ���樨:\r\n",stpro);
	//		else fputs("����⠭������ V:\r\n",stpro);
	//		}	
	// else{ 
	//	 if(imax) fputs("Qmax,V:\r\n",stpro);
	//	 else fputs("Qmin,V:\r\n",stpro);
	// }

	int stag=-1;
	if(ogr_out   )
	{
		// ���� � �����-�� ���� ������ � ���� ������������ �� ���������� 5 � 10 %
		if (sp && (iab || iko) )
		{
			// ���� ���� ��������� �� ����� ��� ������ �������� 
			char *sss=(iab)?"������ �������� ��-�� ���������� �����������":"����� �����������  �� ��������";	
			stag=_Module.Logger.OpenStage (sss);
		}

		else if ( imax || imin)
		{
			// ���� ����������� �� ��� ��� ���� ����� ��� �� ��������
			char *sss=(imax)?"�������� ����������� -max":"�������� ����������� -min";	
			stag=_Module.Logger.OpenStage (sss);
		}
	}

	//	if ( imax || imin)

	// ���������� � ����� �������� � ��������� �������������
	for (pny=nod1_s;pny<nod1_s+ny;pny++) {pny->action=0;pny->anormal=0;}


	bool find=false;		// ���� ��������� ����������� Qmin/Qmax � Vmin/Vmax � �������� ������
	bool is_vmin_vmax = false;

	for (pny=nod1_s;pny<nod1_s+ny;pny++)
	{
		if(sp)
		{
			// � �����-�� ����-������ ��������� ������ ���� �������������� � PV
			switch(pny->nkg)
			{
			case V_MAX:
				// ���� �� Vmax, ���� ���������� ������������ ������ ����������� (4) - �������
				if (pny->countu_max > max_coun_u_ogr) break;
				// ������� ���������� ������������
				pny->countu_max++;
				dq=pny->qmin;
				// ������ ��� ���������
				pny->set_tip(fwc, GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMAX :%d V=%.2f V��=%.2f \n",pny->npu,pny->v,pny->vzd);
				//	       fprintf(stpro,"%d(V=%.1f;Vz=%.1f)\r\n",pny->npu,pny->v,pny->vzd);
				//	       if(wherex()>65) cputs("\n\r");
				// ��������������� ����������, ���������� ���� ������ 0
				restv(pny,dq,&dvr);
				pny->dv=0.;
				// �������� ��� ���� ������������ �� vmin/vmax
				is_vmin_vmax = true;;
				break;
			case V_MIN:
				// ��� ���� �� �������� ��� ����� ��� ��� ���� �� ���������
				if (pny->countu_min > max_coun_u_ogr) break;
				pny->countu_min++;
				dq = pny->qmax;
				pny->set_tip(fwc, GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMIN :%d V=%.2f V��=%.2f\n",pny->npu,pny->v,pny->vzd);
				//	       fprintf(stpro,"%d(V=%.1f;Vz=%.1f),\r\n",pny->npu,pny->v,pny->vzd);
				//	       if(wherex()>65) cputs("\n\r");
				restv(pny,dq,&dvr);
				pny->dv=0.;
				is_vmin_vmax = true;
				break;
			default   :

				break;
			}
		}
		else if(imax)
		{
			// ���� ���� ����������� �� Qmax
			switch(pny->nkg)
			{

			case Q_MAX:
				//	       dq=pny->qmax;
				pny->set_tip(fwc, GEN_MAX);

				pny->rt=compl(0.,0.);
				dq=get_q(pny);

				if(ogr_out) log_printf(LOG_INFO,pny->rindex,NODE_V,"QMAX :%d Q�=%.2f Qmax=%.2f\n",pny->npu,-pny->r.imag(),pny->qmax);
				//	       fprintf(stpro,"%d(Q�=%.1f;Qmax=%.1f),\r\n",pny->npu,-pny->r.im,pny->qmax);
				//	       if(wherex()>65) cputs("\n\r");
				restq(pny,dq);
				//				if (pny->action==Q_MAX) pny->anormal=Q_MAX;
				pny->action=Q_MAX;
				
				find=true;
				break;
			case V_MAX:
				// ���� �� ��������� ��������� � ������������
				if (pny->countu_max > max_coun_u_ogr ) break;
				pny->countu_max++;
				dq=pny->qmin;
				pny->set_tip(fwc,GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMAX :%d V=%.2f V��=%.2f \n",pny->npu,pny->v,pny->vzd);
				//	       fprintf(stpro,"%d(V=%.1f;Vz=%.1f),\r\n",pny->npu,pny->v,pny->vzd);
				//	       if(wherex()>65) cputs("\n\r");
				restv(pny,dq,&dvr);
				// �����-�� ������� �������� � ��������� ������������ ��������
				ineb(pny,dvr);
				max_p(pny);
				//	pny->action=V_MAX;
				find=true;
				break;
			case KT_MIN_REST:
				{
					pny->tip=UPR_TRANS; // ���������� �� � ������������� ������
					pregul_tr tr= pregul_tr (pny->pgen);
					if(ogr_out)log_printf(LOG_INFO,tr->rindex,VETV_V,"KTMIN_RS :%d-%d V=%.2f < V��=%.2f ��� Kt=Kt_min=%.2f \n",tr->pnb->npu,tr->pne->npu,pny->v,pny->vzd,tr->kt_min);
					double  dv=pny->vzd/pny->v;
					pny->v=pny->vzd;
					pny->u *= dv;
					ineb(pny,dv);
					max_p(pny);
				}
				break;
			case KT_MAX:
				{
					pny->tip=UPR_TRANS_MAX; // ������������� �� � ���������� ������
					pregul_tr tr= pregul_tr (pny->pgen);
					if(ogr_out) log_printf(LOG_INFO,tr->rindex,VETV_V,"KTMAX :%d-%d Kt=%.2f Kt_max=%.2f \n",tr->pnb->npu,tr->pne->npu,tr->kt,tr->kt_max);

					compl kt_old=compl(tr->kt,tr->kti),
						kt_new=compl(tr->kt_max,tr->kti);
					kor_el_maty (tr,kt_old,kt_new);
					ineb(pny,0.);
					max_p(pny);
				} 
				break;
			default   :
				pny->action=0;
				break;
			}
		}
		else if(imin)
		{
			// ���� ���� ����������� �� Qmin	
			switch(pny->nkg)
			{
			case Q_MIN:
				//	       dq=pny->qmin;
				pny->set_tip(fwc,GEN_MIN);
				pny->rt=compl(0.,0.);
				dq=get_q(pny);	

				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"QMIN :%d Q�=%.2f Qmin=%.2f\n",pny->npu,-pny->r.imag(),pny->qmin);
				//	       fprintf(stpro,"%d(Q�=%.1f;Qmin=%.1f),\r\n",pny->npu,-pny->r.im,pny->qmin);
				//	       if(wherex()>65) cputs("\n\r");
				restq(pny,dq);
				pny->action=Q_MIN;
				find=true;
			break;
			case V_MIN:
				if (pny->countu_min > max_coun_u_ogr ) break;
				pny->countu_min++;
				dq=pny->qmax;
				pny->set_tip(fwc,GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMIN :%d V=%.2f V��=%.2f \n",pny->npu,pny->v,pny->vzd);
				//	       fprintf(stpro,"%d(V=%.1f;Vz=%.1f),\r\n",pny->npu,pny->v,pny->vzd);
				//	       if(wherex()>65) cputs("\n\r");
				restv(pny,dq,&dvr);
				ineb(pny,dvr);
				max_p(pny);
				//			pny->action=V_MIN;
				find=true;
				break;
			case KT_MAX_REST:
				{
					pny->tip=UPR_TRANS; // ���������� �� � ������������� ������
					pregul_tr tr= pregul_tr (pny->pgen);
					if(ogr_out)log_printf(LOG_INFO,tr->rindex,VETV_V,"KTMAX_RS :%d-%d V=%.2f > V��=%.2f ��� Kt=Kt_max=%.2f \n",tr->pnb->npu,tr->pne->npu,pny->v,pny->vzd,tr->kt_max);
					double  dv=pny->vzd/pny->v;
					pny->v=pny->vzd;
					pny->u *= dv;
					ineb(pny,dv);
					max_p(pny);
				}
				break;
			case KT_MIN:
				{
					pny->tip=UPR_TRANS_MIN; // ������������� �� � ���������� ������
					pregul_tr tr= pregul_tr (pny->pgen);
					if(ogr_out)log_printf(LOG_INFO,tr->rindex,VETV_V,"KTMIN :%d-%d Kt=%.2f Kt_min=%.2f \n",tr->pnb->npu,tr->pne->npu,tr->kt,tr->kt_min);
					compl kt_old=compl(tr->kt,tr->kti),
						kt_new=compl(tr->kt_min,tr->kti);
					kor_el_maty (tr,kt_old,kt_new);
					ineb(pny,0.);
					max_p(pny);
				} 
				break;
			default   :
				break;
			}
		}
	}

	if(stag >=0) _Module.Logger.CloseStage (stag);
	// ���� ���� ����������� �� qmin/qmax �� �� �� �� ����� - ���������� ������� �����������
	if (imax && 	!find ) imax=0;
	if (imin && 	!find ) imin=0;

	// ���� ���� ����������� �� Qmax - ���������� ����������
	if(imax) return(test_iter=Q_MAX);
	else if(imin) return(test_iter=Q_MIN);
	// ���������� �������� �����
	test_iter=0;
	// ���� ���� ���������� ���������� ���������� V_MAX, � ���� > 5% - V_MIN. ������.
	if (iab && is_vmin_vmax) return(V_MAX);
	if (iko && is_vmin_vmax) return(V_MIN);



	return(0);
}

void steady_state::restq(pnod1 pny, double dq)
{
	pny->r+=compl(0.,dq);
	pny->s-=compl(0.,dq);
	pny->sg=compl(pny->sg.real(),dq);
	max_p(pny);
}

void steady_state::restv(pnod1 pny,double dq,double *dv)
{
	pny->s+=compl(0.,dq);
	pny->sg=compl(pny->sg.real(),0);
	*dv=pny->vzd/pny->v;
	pny->v=pny->vzd;
	// pny->u.re*=*dv;
	// pny->u.im*=*dv;
	pny->u *= *dv;

}

int steady_state::oneb(ptrdiff_t ny)
{
	pnod1 pn;
	double r,dq;
	for(pn=nod1_s;pn<nod1_s+ny;pn++)
	{
		if(pn->tip ==GEN )
		{
			dq=-pn->r.imag();
			r=0.;
			if(dq< pn->qmin) r=dq-pn->qmin;
			if(dq> pn->qmax) r=dq-pn->qmax;
			rval.sqrq+=r*r;
		}
	}
	return 0;
}




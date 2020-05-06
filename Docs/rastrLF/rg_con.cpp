#include "stdafx.h"
#include "rg_ss.h"

// при sp  = TRUE //расчет неудачен. вправить PQ(+/-)->PV

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
		// в нормальном состоянии цикл проходим один раз
		repeat=false;
		for (pny=nod1_s;pny<nod1_s+ny;pny++)
		{
			// отключаем ограничение узла ?
			pny->nkg=ZERO;
			// узел с блокированными ограничениями пропускам
			if (method_ogr==DOP_OGR && pny->block_ogr) continue;
			if(pny->tconnect==N_SLAVE) continue;
			// выбираем тип ограничения узла по типу ограничения острова
			int tipp = pny->get_tip(fwc);
			// if(pny->ltip==ACTT||pny->ltip==CONTR) continue;
			{
				switch(tipp)
				{
				case GEN:
					//			if(sp) break;
					// для генераторного узла берем инъекцию реактивной мощности
					dq=-pny->r.imag();
					// если она меньше минимального ограничения с учетом допустимого небаланса
					if(dq< pny->qmin-consb.q)
					{
						//		if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"=QMIN :%d Qг=%.2f Qmin=%.2f\n",pny->npu,-pny->r.imag(),pny->qmin);
						//	if (pny->action==V_MAX) pny->anormal=V_MAX;

						// если не находимся в некоем особом режиме 
						if (!sp)
						{
							// переводим узел на qmin, взводим флаг что есть ограничения qmin
							pny->nkg=Q_MIN;
							imin=TRUE;
						}
					}
					// если инъекция больше максимального ограничения
					else if(dq> pny->qmax+consb.q)
					{
						//		if(ogr_out) log_printf(LOG_INFO,pny->rindex,NODE_V,"=QMAX :%d Qг=%.2f Qmax=%.2f\n",pny->npu,-pny->r.imag(),pny->qmax);
						//	if (pny->action==V_MIN) pny->anormal=V_MIN;
						// если не находимся в некоем особом режиме 
						if (!sp)
						{
							// переводим узел на qmax, взводим флаг что есть ограничения qmax
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
					// для узла на верхнем ограничении
					// проверяем отклонение от уставки
					dvr = pny->get_difv();

					if (dvr >consb.v)
					{
						// если напряжение больше уставки
						//if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"=VMAX :%d V=%.2f > Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);

						if(pny->sstip==ACTT )		//sstip юзается в оптимизации вроде только
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
							// если узел на qmax и отклонение от уставки более процента
							// ставим узлу признак ненормального (типа он на qmax и напряжение выше уставки более чем на процент)
							pny->anormal=int( max(dvr*100,20));
							// ставим узел на Vmin и взводим флаг что есть узлы с отклонением напряжения от уставки
							pny->nkg=V_MIN;
							iko=TRUE;
							//       if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"Анормально :%d V=%.2f > Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
						}


						// если в некоем особом режиме
						if(sp)
						{
							// если отклонение от уставки больше 10%
							if(dvr> consb.dv2)		//consb.dv1 > consb.dv2
							{
								// ставим узел на Vmin 
								// и взводим флаг что есть ab нормальные узды
								pny->nkg=V_MIN;
								iab=TRUE;
							}
							else if(dvr>consb.dv1)
							{
								// если отклонение от уставки больше 5%
								// ставим на Vmin и взводим флаг что есть какое-то отклонение
								pny->nkg=V_MIN;
								iko=TRUE;
							}
						}
						else
						{
							// вне некоего особого режима просто ставим узел на Vmin и отмечаем
							// что есть ограничения qmin
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
						// если напряжение узла на минимуме меньше уставки

						//if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"=VMIN :%d V=%.2f  < Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
						if(pny->sstip==ACTT)		//sstip юзается в оптимизации вроде только
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
							// если узел на минимуме и отклонение больше 1%
							int idv=int(abs(dvr)*100);
							// считаем отклонение и ставим его в атрибут ненормальности узла
							if (idv >20) idv=21;
							pny->anormal=idv;
							// узел переводим на Vmax
							// и отмечаем что есть отклонение о уставки
							pny->nkg=V_MAX;
							iko=TRUE;
							//     if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"Анормально :%d V=%.2f < Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
						}
						if(sp)
						{
							// если в некоем особом режиме отклонение напряжения больше 10%
							if(dvr< -consb.dv2)
							{
								// переводим узел на Vmax
								// ставим флаг ab нормальных узлов
								pny->nkg=V_MAX;
								iab=TRUE;
							}
							else if(dvr<-consb.dv1 )
							{
								// если отклонение больше 5%
								// переводим на Vmax и ставим флаг что есть узел с отклонением
								pny->nkg=V_MAX;
								iko=TRUE;
							}
						}
						else
						{
							//if (pny->action==Q_MIN) pny->anormal=Q_MIN;

							// вне некоего особого режима просто ставим узел на Vmax
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

		// видимо есть какой-то тип итерации для тестирования
		if(test_iter && method_ogr==DOP_OGR) 
		{

			bool all_anormal=true;
			bool have_anormal=false;
			int max_anormal=0;;

			for (pny=nod1_s;pny<nod1_s+ny;pny++)
			{
				// если у узла есть какое-то действие на изменение и оно не абнормальное, то сбрасываем флаг, что все узлы абнормальные
				if (pny->action && (pny->action!=pny->anormal) ) all_anormal=false;		
				// ищем максимальное ненормальное отклонение узлов
				if(abs(pny->anormal) > max_anormal) max_anormal=abs(pny->anormal);
			}
			//		if (all_anormal)
			if (max_anormal >21) max_anormal=21;		// ограничиваем максимальное отклонение почему-то 21
			// определяем максимальное ненормальное отклонение (видимо за весь расчет)
			glob_anormal=max(max_anormal,glob_anormal);
			// масштабируем максимальное отклонение на 2/3
			 max_anormal=int (double(max_anormal)* 2/3);
			 // если после этого максимальное отклонение меньше 2%, то игнорируем его
			if (max_anormal	<=2) max_anormal=0;
			for(pny=nod1_s;pny<nod1_s+ny;pny++)
			{
				if(abs(pny->anormal) >max_anormal)
				{ 
					// если отклонение узла больше анормального
				have_anormal=true;	// ставим флаг что есть анормальные
				pny->block_ogr=true;	// блокируем ограничение такого узла
				pny->count_bloc+=pny->anormal;	// увеличиваем счетчик блокировок узла на ненормальное отклонение ?
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"Блокировано :%d V=%.2f > Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
				}
			}
			// если есть ненормальные узлы, взводим флаг абнормального, для узлов с неблокированными ограничениями убираем тип ограничения ?
			if (have_anormal ) { iab=true; for (pny=nod1_s;pny<nod1_s+ny;pny++) if (!pny->block_ogr) pny->nkg=0;}
			// если ненормальных узлов нет, отменяем тестовую итерацию
			else test_iter=0;
		//	if (all_anormal)	iab=TRUE;
		}

		// сбрасываем флаг ненормальных ограничений
		 ab_ogr=false;		
		 // если нет ограничений на минимуме, на максимуме, абнормальных, отклонений напряжения и фактсов, а также не находимся в некоем спецрежиме
		if( !imax && !imin && !iab && !iko  && !ikf )    if ( !sp) 
		{
			for (pny=nod1_s;pny<nod1_s+ny;pny++) 
			{
				if (pny->block_ogr && abs(pny->count_bloc)<=glob_anormal) 
				{ 
					// если у узла заблокированы ограничения и количество ограничений меньше глобального (за весь расчет)
					// повторяем выбор ограничений
				repeat=true;
				pny->block_ogr=false; // снимаем блокировку ограничений узла
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"Снятa блокировка :%d Q=%.2f  Qmin=%.2f Qmax=%.2f \n",pny->npu,-pny->r.imag(),pny->qmin,pny->qmax);

				}
				// если нашли узел с ограничениями (и у него счетчик видимо больше чем глобальный счетчик), взводим флаг ненормальных ограничений
				if (pny->block_ogr) ab_ogr=true;
			}

		}
	}while(repeat);

	if 	( ab_ogr		)
	{
		// если есть ненормальные ограничения
int stag=_Module.Logger.OpenStage ("Невозможно выдержать ограничения в генераторах");
		// выводим список генераторных узлов с ненормальными ограничениями
			for (pny=nod1_s;pny<nod1_s+ny;pny++) if (pny->block_ogr)
			{
			log_printf(LOG_ERROR,pny->rindex,NODE_V,"Узел :%d Q=%.2f  Qmin=%.2f Qmax=%.2f \n",pny->npu,-pny->r.imag(),pny->qmin,pny->qmax);
			}
			_Module.Logger.CloseStage (stag);
			// выходим с индикацией аварии
		return (AB);
	}

	// если нет ограничений на максимум, минимум, абнормальных, отклонений напряжения и фактсов - выходим, типа все хорошо
	if( !imax && !imin && !iab && !iko  && !ikf )   return(ZERO);


	// если есть - сначала обрабатываем фактсы
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
	//if(sp){if(iab) fputs("Ћв¬Ґ­  ЁвҐа жЁЁ:\r\n",stpro);
	//		else fputs("‚®ббв ­®ў«Ґ­л V:\r\n",stpro);
	//		}	
	// else{ 
	//	 if(imax) fputs("Qmax,V:\r\n",stpro);
	//	 else fputs("Qmin,V:\r\n",stpro);
	// }

	int stag=-1;
	if(ogr_out   )
	{
		// если в каком-то спец режиме и есть абнормальные по напряжению 5 и 10 %
		if (sp && (iab || iko) )
		{
			// если есть абнормалы то пишем про отмену итерации 
			char *sss=(iab)?"Отмена итерации из-за нарушенных ограничений":"Смена ограничений  на итерации";	
			stag=_Module.Logger.OpenStage (sss);
		}

		else if ( imax || imin)
		{
			// если ограничения на мин или макс пишем про их контроль
			char *sss=(imax)?"Контроль ограничений -max":"Контроль ограничений -min";	
			stag=_Module.Logger.OpenStage (sss);
		}
	}

	//	if ( imax || imin)

	// сбрасываем у узлов действия и атрибутиы анормальности
	for (pny=nod1_s;pny<nod1_s+ny;pny++) {pny->action=0;pny->anormal=0;}


	bool find=false;		// флаг изменения ограничений Qmin/Qmax и Vmin/Vmax в неособом режиме
	bool is_vmin_vmax = false;

	for (pny=nod1_s;pny<nod1_s+ny;pny++)
	{
		if(sp)
		{
			// в каком-то спец-режиме проверяем только узлы возвращающиеся в PV
			switch(pny->nkg)
			{
			case V_MAX:
				// узел на Vmax, если количество переключений больше допустимого (4) - выходим
				if (pny->countu_max > max_coun_u_ogr) break;
				// считаем количество переключений
				pny->countu_max++;
				dq=pny->qmin;
				// ставим тип генератор
				pny->set_tip(fwc, GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMAX :%d V=%.2f Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
				//	       fprintf(stpro,"%d(V=%.1f;Vz=%.1f)\r\n",pny->npu,pny->v,pny->vzd);
				//	       if(wherex()>65) cputs("\n\r");
				// восстанавливаем напряжение, отклонение узла ставим 0
				restv(pny,dq,&dvr);
				pny->dv=0.;
				// отмечаем что есть переключение на vmin/vmax
				is_vmin_vmax = true;;
				break;
			case V_MIN:
				// для узла на минимуме все также как для узла на максимуме
				if (pny->countu_min > max_coun_u_ogr) break;
				pny->countu_min++;
				dq = pny->qmax;
				pny->set_tip(fwc, GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMIN :%d V=%.2f Vзд=%.2f\n",pny->npu,pny->v,pny->vzd);
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
			// если есть ограничения по Qmax
			switch(pny->nkg)
			{

			case Q_MAX:
				//	       dq=pny->qmax;
				pny->set_tip(fwc, GEN_MAX);

				pny->rt=compl(0.,0.);
				dq=get_q(pny);

				if(ogr_out) log_printf(LOG_INFO,pny->rindex,NODE_V,"QMAX :%d Qг=%.2f Qmax=%.2f\n",pny->npu,-pny->r.imag(),pny->qmax);
				//	       fprintf(stpro,"%d(QЈ=%.1f;Qmax=%.1f),\r\n",pny->npu,-pny->r.im,pny->qmax);
				//	       if(wherex()>65) cputs("\n\r");
				restq(pny,dq);
				//				if (pny->action==Q_MAX) pny->anormal=Q_MAX;
				pny->action=Q_MAX;
				
				find=true;
				break;
			case V_MAX:
				// узел на максимуме переводим в генераторный
				if (pny->countu_max > max_coun_u_ogr ) break;
				pny->countu_max++;
				dq=pny->qmin;
				pny->set_tip(fwc,GEN);
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMAX :%d V=%.2f Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
				//	       fprintf(stpro,"%d(V=%.1f;Vz=%.1f),\r\n",pny->npu,pny->v,pny->vzd);
				//	       if(wherex()>65) cputs("\n\r");
				restv(pny,dq,&dvr);
				// зачем-то считаем небаланс и обновляем максимальный небаланс
				ineb(pny,dvr);
				max_p(pny);
				//	pny->action=V_MAX;
				find=true;
				break;
			case KT_MIN_REST:
				{
					pny->tip=UPR_TRANS; // освободить кт и зафиксировать модуль
					pregul_tr tr= pregul_tr (pny->pgen);
					if(ogr_out)log_printf(LOG_INFO,tr->rindex,VETV_V,"KTMIN_RS :%d-%d V=%.2f < Vзд=%.2f при Kt=Kt_min=%.2f \n",tr->pnb->npu,tr->pne->npu,pny->v,pny->vzd,tr->kt_min);
					double  dv=pny->vzd/pny->v;
					pny->v=pny->vzd;
					pny->u *= dv;
					ineb(pny,dv);
					max_p(pny);
				}
				break;
			case KT_MAX:
				{
					pny->tip=UPR_TRANS_MAX; // зафиксировать кт и освободить модулб
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
			// если есть ограничения по Qmin	
			switch(pny->nkg)
			{
			case Q_MIN:
				//	       dq=pny->qmin;
				pny->set_tip(fwc,GEN_MIN);
				pny->rt=compl(0.,0.);
				dq=get_q(pny);	

				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"QMIN :%d Qг=%.2f Qmin=%.2f\n",pny->npu,-pny->r.imag(),pny->qmin);
				//	       fprintf(stpro,"%d(QЈ=%.1f;Qmin=%.1f),\r\n",pny->npu,-pny->r.im,pny->qmin);
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
				if(ogr_out)log_printf(LOG_INFO,pny->rindex,NODE_V,"VMIN :%d V=%.2f Vзд=%.2f \n",pny->npu,pny->v,pny->vzd);
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
					pny->tip=UPR_TRANS; // освободить кт и зафиксировать модуль
					pregul_tr tr= pregul_tr (pny->pgen);
					if(ogr_out)log_printf(LOG_INFO,tr->rindex,VETV_V,"KTMAX_RS :%d-%d V=%.2f > Vзд=%.2f при Kt=Kt_max=%.2f \n",tr->pnb->npu,tr->pne->npu,pny->v,pny->vzd,tr->kt_max);
					double  dv=pny->vzd/pny->v;
					pny->v=pny->vzd;
					pny->u *= dv;
					ineb(pny,dv);
					max_p(pny);
				}
				break;
			case KT_MIN:
				{
					pny->tip=UPR_TRANS_MIN; // зафиксировать кт и освободить модуль
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
	// если были ограничения по qmin/qmax но мы их не нашли - сбрасываем счетчик ограничений
	if (imax && 	!find ) imax=0;
	if (imin && 	!find ) imin=0;

	// если были ограничения по Qmax - возвращаем индикаторы
	if(imax) return(test_iter=Q_MAX);
	else if(imin) return(test_iter=Q_MIN);
	// сбрасываем итерацию теста
	test_iter=0;
	// если были ограничния абнормалов возвращаем V_MAX, а если > 5% - V_MIN. Логика.
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




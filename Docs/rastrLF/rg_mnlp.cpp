
#include "stdafx.h"
#include "common.h"
#include "rg_ss.h"
#include "..\RES\resource.h"

#include "mtimer.h"

double steady_state::vetv_data::min_resistance = 0.01;


int steady_state::main_loop()
{
	int matr = 0;
	return
		rg_main_loop(ny_save, nby_save, &matr);

}

int steady_state::main_loop_with_utr(bool utr, bool restsart )
{
	int matr = 0;
	for (pnod1 pny = nod1_s; pny < nod1_s + ny_save; pny++) pny->r_saved = pny->r_fixed = 0.;

if (!utr)	return			rg_main_loop(ny_save, nby_save, &matr);

	// начальный расчет
	set_save_first(true);
//	calc_det(ny_niyp, nby_save);
	int kod = rg_main_loop(ny_save, nby_save, &matr, 1);
	
	if (restsart && kod != 0.)
	{
		set_zeidel(true);
		set_save_first(false);
		plos(ny_save);
		kod = rg_main_loop(ny_save, nby_save, &matr);
		set_zeidel(false);

	}


	if (!utr)	return kod;
	

//	calc_det(ny_niyp, nby_save);

	if (kod == 0) return kod;
	double step = 0.5;
	double length = step;
	bool repeat = true;
	bool finish = false;
	int successful_steps = 0, steps = 0;

	//  -- константы
	int  successful_steps_max = 3;
	int steps_max = 10;
	double eps = 10. ;
	bool test_finish = true,is_test_finish=false;
	do
	{

		if (kod != 0) restore_it0();
		double nb_max = 0.;
		for (pnod1 pny = nod1_s; pny < nod1_s + ny_save; pny++)
		{
			if (pny->tip == BAZA) continue;
			pny->r_fixed = (length - 1.)*pny->r_saved;
			compl p = pny->r_saved*step;
			double nb = (pny->tip != GEN) ? max(abs(real(p)), abs(imag(p))) : abs(real(p));
			nb_max = max(nb_max, nb);
		}
		if (!finish && (length == 1. || steps > steps_max || nb_max < eps)) finish = true;

		kod = rg_main_loop(ny_save, nby_save, &matr);

		steps++;
		//if (kod == 0) 
		//{
		//	calc_det(ny_niyp, nby_save);
		//	
		//	if (m_Det.get_val() < 0) kod = 1;
		//}

		if (kod == 0)
		{
//			calc_det(ny_niyp, nby_save);
			save_it(ny_save, 1.);
			// Проверяем сходимость
			if (finish)
			{
				//if (test_finish || is_test_finish)
				//{
				//	is_test_finish = true;
				//	test_finish = false;
				//	if (length == 1.) return 0;
				//	length += step;
				//	if (length > 1.)length = 1;
				//	continue;
				//}
				if (length == 1.) return 0;
				else return 1;

			}

			// делим вперед
			//if (successful_steps < successful_steps_max)
			//{
				step /= 2.;
				length += step;
			//}
			//else if (successful_steps == successful_steps_max)
			//{
			//	length += step;
			//}
			//else
			//{
			//	step *= 2.;
			//	length += step;
			//}
			if (length > 1.) length = 1;
			successful_steps++;

		}
		else
		{
			is_test_finish = true;

			if (!finish) step /= 2.;
			else 			is_test_finish = false;
			length -= step;
			if (length <= 0.)
			{
				restore_it0();
				for (pnod1 pny = nod1_s; pny < nod1_s + ny_save; pny++) pny->r_saved = pny->r_fixed = 0.;
				set_save_first(true);
				//	calc_det(ny_niyp, nby_save);
				int kod = rg_main_loop(ny_save, nby_save, &matr);
				return 1;
			}
			successful_steps = 0;
		}

	} while (repeat);
}



//#include "rastr.h"


int steady_state::rg_main_loop (ptrdiff_t ny, ptrdiff_t nby,int *matr0 , int save2)
{
	bool blOneCycle = false;
	if( one_cycle == 1 ) 
		blOneCycle = true;
	int test = 0;
	form_NodeGen_from_Generators();

	bool is_saved = false;
	//method=M_OLD; 
	//	method_ogr=DOP_OGR;
	//	method_ogr=OLD_OGR;
	int stag=-1;
	//	ogr_out=true; // временно для отладки

	// максимальное количество переключений типов генераторных узлов
	if (max_coun_u_ogr <=0)max_coun_u_ogr=4;
	if(!blok_out) stag=_Module.Logger.OpenStage ("Расчет установившегося режима");
	int kod=NORM,kod1,nb=0;;
	// какой-то счетчик анормальных отклонений напряжения
	glob_anormal=0;
	pnod1 pn;
	char st;
	double ro_min=1.e10;
	double rk_ab=1.;
	rval.iter=0;
	rval.is_max=rval.ivp=rval.ivm=rval.il1=rval.il2=nod1_s;
	if(ny <=0 ) return kod;
	// out_ms(2,A_IDS_RG_FZ);
	log_printf(LOG_INFO,-1,"",get_Rstring(A_IDS_RG_FZ));
	for(pn=nod1_s;pn<nod1_s+ny;pn++)
	{
		// обнуляем счетчики переключений типов узлов
		pn->countu_min=pn->countu_max=0;
		// обнуляем действия переключения узлов и степени ненормальных отклонений
		pn->action = 0; pn->anormal = 0;
		if( pn->tip==GEN)		 pn->sg=compl(pn->sg.real(),0.);
		if (pn->tip == BAZA &&fwc  && pn->pi->tip == islands::I_GEN) pn->sg = compl(pn->sg.real(), 0.);
		if (pn->tip == BAZA && fwc == 0) pn->sg = compl(0., 0.);
		if (pn->tip==UPR_TRANS) 
		{
			if (pn->vzd !=0.) pn->v=pn->vzd;
			else pn->tip=NAG;
		}
	}
	//	facts_set_ust();
	if (facts_set_ust()) nzei=1;
	calc_vpt(); 

	for (pn = nod1_s; pn < nod1_s + ny; pn++)
	{

		polin* psx = NULL;
		int ik = pn->nsx - 3;
		if (polin_s != NULL && ik >= 0 && ik<npol)  { psx = &polin_s[ik]; }
		double z = getzdiv(pn->v, pn->uhom);
		pn->current_polin = NULL;
		if (psx != NULL)
		{
			const polin *st = psx->getcurp(z);
			pn->current_polin = (polin *) malloc(sizeof(polin));
			memcpy(pn->current_polin, st, sizeof(polin));
			double df = 0;
			if (islands_s != NULL && pn->pi != NULL)
			{
				df = pn->pi->frec;
			}
			if (fwc == 0) df = 0;
			pn->current_polin->set_zt(z, df);
			pn->current_polin->count = 0;
		}
	}



	mtimer time_trian,time_all,time_formj,time_solv,time_neb,time_new;
	bool newton_step=false;
	bool use_zeidel=true;
	bool not_save2 = true;
	test_iter=0;
	// константы для метода выбора шага
	double neb_crit=0.99999;   // критическое уменьшение небалансов
	double decr_step=0.6; // уменьшение шага при отклонении от 1 Уменьшения небалансов
	double decr_abend=0.4; // уменьшение шага при фиксации аварийного завершения
	double min_step=0.00001;
	
	time_all.start();
	do
	{


//		char buf[20000];

		// по умолчанию итерацию не повторяем
		bool repeat_state=false;
		// шаг итерации по умолчанию 0
		if (rval.iter == ZERO)rval.step=1;
		rval.rk=0;
		double rk_dop=0;;
		do 
		{
			repeat_state=false;
			// определяем максимальное отклонение напряжения узла от номинального и максимальный угол по ветви
			max_vd(ny); // расчет максимального  отклонения напр. от ном. , макс угла, макс кт
			if (method==M_OLD)
			{
				kod=NORM;
				// не на первой итерации проверяем аварийное завершение по превышению угла и напряжения
				// и выбираем шаг чтобы не выйти за ограничения
				if(rval.iter != ZERO) kod=ab_kon(ny,rk_dop); //проверка аварийного завершения по отклонению рассчитанному max_VD 
				// если шаг не нулевой повторяем шаг
				if ( rk_dop !=0. ) // обработка шага при аварийном завершении 
				{			
					// что-то корректируем в матрице
					kor_maty(rk_dop);
					// считаем новые модули и углы
					new_v(ny,rk_dop);
					// повторяем контроль
					max_vd(ny);
				}
			}

			time_new.start();
			// считаем модули и углы после итерации
			new_u(ny);
			time_new.finish();
			rval.max_neb_q = rval.s_max = rval.sqrp = rval.sqrq = 0.;
			//			(nod1_s+1)->sn+=rval.iter%2?compl(2,2):-compl(2,2);
			if (fwc) freq_test_ogr(ny);
			time_neb.start(); 

			//sij_all_line(ny);
			//pneb2(ny);   // расчет небалансов


//			for (pnod1 pnk = nod1_s; pnk < nod1_s + ny; pnk++) if (pnk->npu == 909) 		log_printf(LOG_INFO, -1, "", "Текущее  V=%.2f  \n", pnk->v);

			pneb(ny);   // расчет небалансов

			// здесь сохраняются небалансы узлов по внешнему флагу. Наверное для утяжеления
			if (save2 == 1 && not_save2) {
				not_save2 = false; 	
				for (pnod1 pny = nod1_s; pny < nod1_s + ny; pny++) 
				{
					if (pny->tip != GEN)  pny->r_saved = pny->r; else pny->r_saved = real(pny->r);
			}
			}
			//if (test)
			//{
			//	FILE *str = fopen("c:\\tmp\\itp.csv", "a");
			//	fprintf(str, "----------------%d------------------------- \n", rval.iter);
			//	//fprintf(str, "-Нагрузка \n");
			//	//for (pnod1 pn = nod1_s; pn < nod1_s + ny; ++pn)
			//	//{
			//	//	fprintf(str, "%8d %8d %12.2f %12.2f  \n ", pn->npu,pn->tip, pn->sn.real(), pn->sn.imag());
			//	//}
			//	//fprintf(str, "-Генерация \n");
			//	//for (pnod1 pn = nod1_s; pn < nod1_s + ny; ++pn)
			//	//{
			//	//	fprintf(str, "%8d %8d %12.3f %12.3f  \n ", pn->npu,pn->tip, pn->sg.real(), pn->sg.imag());
			//	//}
			//	fprintf(str, "-Напряжения \n");
			//	for (pnod1 pn = nod1_s; pn < nod1_s + ny; ++pn)
			//	{
			//		fprintf(str, "%8d %12.2f %12.2f %12.2f %12.2f  \n ", pn->npu, pn->v, pn->a.delta, pn->r.real(), pn->r.imag());
			//	}
			//	//fprintf(str, "-Небалансы \n");
			//	//for (pnod1 pn = nod1_s; pn < nod1_s + ny; ++pn)
			//	//{
			//	//	fprintf(str, "%8d %12.3f %12.3f \n ", pn->npu, pn->r.real(), pn->r.imag());
			//	//}

			//	fclose(str);

			//}




			facts_slave_neb();
			// определяем узлы с максимальными небалансами
			// и заодно считаем квадраты небаналсов rval.sqrp / rval.sqrq
			for(pnod1 pny=nod1_s;pny<nod1_s+ny;pny++) max_p(pny);
			time_neb.finish();
			st=' ';
			// проверка длины шага по величине небалансов

			//			double const1=0.9;
			double r = sqrt(rval.sqrp + rval.sqrq);
				if (r == 0.) r = 1.;
			double rk= sqrt(rval.sqrp_old+rval.sqrq_old)/r;
			// рассчитываем шаг по небалансу
			rval.rk=rk;
			/*			double rk_inf=max(rval.s_max_old,rval.max_neb_q_old)/max(rval.s_max,rval.max_neb_q);
			double rk_p=sqrt(rval.sqrp_old)/sqrt(rval.sqrp),kr_q=sqrt(rval.sqrq_old)/sqrt(rval.sqrq);
			if (0)log_printf(LOG_INFO,-1,"","2_norma= %.2f / %.2f = %.2f |2_normaP= %.2f / %.2f = %.2f |2_normaQ= %.2f / %.2f = %.2f"  
			"inf_norma= %.2f / %.2f = %.2f |inf_normaP= %.2f / %.2f = %.2f |inf_normaQ= %.2f / %.2f = %.2f \n",
			sqrt(rval.sqrp_old+rval.sqrq_old),sqrt(rval.sqrp+rval.sqrq),rk,
			sqrt(rval.sqrp_old),sqrt(rval.sqrp),sqrt(rval.sqrp_old)/sqrt(rval.sqrp),
			sqrt(rval.sqrq_old),sqrt(rval.sqrq),sqrt(rval.sqrq_old)/sqrt(rval.sqrq),
			max(rval.s_max_old,rval.max_neb_q_old),max(rval.s_max,rval.max_neb_q),max(rval.s_max_old,rval.max_neb_q_old)/max(rval.s_max,rval.max_neb_q),
			rval.s_max_old,rval.s_max,rval.s_max_old/rval.s_max,
			rval.max_neb_q_old,rval.max_neb_q,rval.max_neb_q_old/rval.max_neb_q);
			*/
			if (method==M_STEP && newton_step)
			{
				// если небаланс меньше критического уменьшения (?) повторяем итерацию 
				// видимо имеется в виду что если он меньше 1 то надо пересчитать матрицу и новые напряжения
				// а если нет - то продолжить так
				if (rk <neb_crit) 
				{
					rval.step=decr_step*rk*rk_ab;
					kor_maty(-rk_ab+rval.step);
					new_v(ny,-rk_ab+rval.step);
					repeat_state=true;

				}
				else rval.step=rk_ab;
			}

			// если повтор итерации пока не нужен - проверяем ограничения по Q
			if( method==M_STEP && repeat_state==false &&  con_q(ny,TRUE) == V_MAX) 
			{ 
				// если есть ограничения по Vmax > 10% - отменяем итерацию полностью (вычитаем rval.step) пересчитываем матрицу и напряжения и повторяем итерацию
				kor_maty(-rval.step);
				new_v(ny,-rval.step);
				repeat_state=true;

			}

			newton_step=false;

		}
		while (repeat_state);
		kod1=Q_MAX;
		int kod_v = 0, kod_f = 0;;

		// контроль ограничений делаем после того, как небаланс по Q снизился ниже заданного

		if( rval.max_neb_q < consb.qk ) // контроль ограничений Q в нормальном состоянии
		{ 
			kod1 = con_q( ny, FALSE );
			if (fwc && kod1) { 
				kod_v=freq_test_v(ny);  
				kod_f = freq_test_ogr(ny, true);
			
			}
			//		use_zeidel=true;	
		}

		if( rval.s_max     > consb.p  ) kod1 = Q_MAX;
		if( rval.max_neb_q > consb.q  ) kod1 = Q_MAX;

		if(best==1)
		{
			double d;

			// видимо считается набаланс от ограничения
			oneb(ny);
			d=rval.sqrp+consb.kfd*rval.sqrq;
			if(d<ro_min) { //save_it(ny); 
				ro_min=d; st='*';nb=rval.iter;}
		}

		if (kod1==ZERO && calc_vpt()==false) kod1=VPT;
		if (kod1==ZERO && facts_test_nb()==false ) kod1=Q_MAX; 

		// запоминаем текущие небалансы для выбора шага
		rval.sqrp_old =rval.sqrp; 
		rval.sqrq_old=rval.sqrq;
		rval.max_neb_q_old=rval.max_neb_q;
		rval.s_max_old=rval.s_max;

		// выводим результат итерации
		out_tb(st);
		if (method==M_STEP && kod != NORM && rval.step > min_step) kod=NORM; 
		if (fwc && kod_v) kod = AB_CODE_FREC_Q;
		if (fwc && kod_f) kod = AB_CODE_FREC_P;
		//ustas if( (kod1==ZERO || kod !=NORM) && !jakk) break;
		//if( (kod1==ZERO || kod !=NORM) && (!jakk) && ( one_cycle != 1 )) break;
		//one_cycle = -1;// сбросить переключатель выполнения хотя бы одного цикла расчета режима
		if( (kod1==ZERO ||kod1==AB || kod !=NORM) && (!jakk) && ( blOneCycle == false )) break;
		if ((blOneCycle == true || jakk) && kod1 == ZERO) nzei = 1;
		blOneCycle = false;// сбросить переключатель выполнения хотя бы одного цикла расчета режима

		if(use_zeidel && !nzei)	
		{
//			consb.itz = 200;

//			zeid(ny,nby);
			zeid2(ny, nby);


			use_zeidel=false;
		}
		else
		{
			//	pri_pv(ny);

			time_formj.start ();
			formj(ny_niyp,nby);

			//form_vetv_funk();
			//formj2(ny_niyp, nby);
			 


			if (fwc) freq_form(ny);

			//SaveMatrixY("c:\\projects\\MatrixRG.txt", ny);// - nby);
			time_formj.finish();
			// масштабирование небалансов
			scale_nb(ny);
			if( method==M_POWELL)
			{
				double mu=calc_grad(ny-nby);
				dd_grad(ny); 
				new_v(ny,-mu);
				if (rval.iter >40) method=M_STEP;
				continue;
			}
			if(jakk) return print_jakobi (ny-nby,nn_nv);


	/*		if (test)
			{



				if (rval.iter == 28 )
				{
					 test_print_jakobi(ny - nby, nn_nv);

					FILE *str = fopen("c:\\tmp\\itp.csv", "a");
					fprintf(str, "----------------%d------------------------- \n", rval.iter);


					fprintf(str, "-Напряжения \n");
					for (pnod2 ptn = nod2_s; ptn < nod2_s + ny_niyp; ptn++)
					{
						pnod1 pny = ptn->niyp;
						double *ri = (double *)&(pny->r);
						int kk = int(ptn->ng);
						fprintf(str, "%8d %8d %12.2f %12.2f  \n ", pny->npu, kk, pny->v, pny->a.delta*DELR);
					}


					fprintf(str, "-Небалансы \n");
					for (pnod2 ptn = nod2_s; ptn < nod2_s + ny_niyp; ptn++)
					{
						pnod1 pny = ptn->niyp;
						double *ri = (double *)&(pny->r);
						int kk = int(ptn->ng);
						fprintf(str, "%8d %8d %12.2f %12.2f  \n ", pny->npu, kk, ri[0], ri[1]);
					}


					fclose(str);
				}
			}
*/

			time_trian.start();
			(this->*trian)(ny_niyp+nby,nby);
			time_trian.finish();
			*matr0=1;
			time_solv.start();
			solv(ny_niyp,nby);
			time_solv.finish();
	/*		if (test)
			{
				if (rval.iter == 28 )
				{
					FILE *str = fopen("c:\\tmp\\itp.csv", "a");
					fprintf(str, "----------------%d------------------------- \n", rval.iter);

					fprintf(str, "-Incremeny \n");
					for (pnod2 ptn = nod2_s; ptn < nod2_s + ny_niyp; ptn++)
					{
						pnod1 pny = ptn->niyp;
						double *ri = (double *)&(pny->r);
						int kk = int(ptn->ng);
						fprintf(str, "%8d %8d %12.2f %12.2f   \n ", pny->npu, kk, ri[0]*DELR, ri[1]*pny->v);
					}

					fclose(str);

					test_print_jakobi(ny - nby, nn_nv);

				}
			}

*/


			dd(ny);
			//	facts_an_dd();
			newton_step=true;
			//		if(spec&&rval.iter==0) /* specf(ny)*/;
			if(step_mtd > 0)	save_v(ny);
			bool full_test = false;
			if ( method==M_STEP) full_test = true;
			// проверяем длину шага
				kod=test_new_v(full_test, ny,rk_ab);
				// если шаг должен быть уменьшен - домножаем его на аварийный коэффициент
				if (rk_ab < 1. && full_test) rk_ab*=decr_abend; 
				rval.step = rk_ab;

			if (save_first && !is_saved) {
				save_it(ny, rk_ab); is_saved = true;
	//			for (pnod1 pnk = nod1_s; pnk < nod1_s + ny; pnk++) if (pnk->npu == 909) 		log_printf(LOG_INFO, -1, "", "Сохранено  V=%.2f  \n", pnk->v);

			}


			kor_maty(rk_ab);
			new_v(ny,rk_ab);

			// проверяем не вышли ли на критические уровни напряжения
			if( method ==M_OLD &&con_q(ny,TRUE) == V_MAX ) 
			{ 
				// если вышли - отменяем итерацию
				kor_maty(-rk_ab);
				new_v(ny,-rk_ab);
				newton_step=false;
			}
			else if (step_mtd > 0 && old_neb<=0.) {}
		}		
	}
	while (++rval.iter < consb.iter );
	if(rval.iter>=consb.iter)
	{
		kod = ITER;
	}
	if(kod != NORM) 
	{
		out_ms(2,kod);
		//	   log_printf(LOG_ERROR,-1,"",get_Rstring(kod));
	}

	new_u(ny);
	sij_all_line(ny);
	pneb(ny);   // расчет небалансов



	for(pn=nod1_s;pn<nod1_s+ny;pn++) 
	{
		//		pn->ext_gen=false;
		if (pn->tconnect== N_SLAVE ) continue;
		double dvr = pn->get_difv();
//		if (pn->countu_min > max_coun_u_ogr &&	pn->v > pn->vzd )
		if (pn->countu_min > max_coun_u_ogr &&	dvr > consb.v)
		{
			log_printf(LOG_WARNING,pn->rindex,NODE_V,"Игнорировано VMIN :%d V=%.2f Vзд=%.2f \n",pn->npu,pn->v,pn->vzd);
			kod = AB_CODE_IGNOGRV;
		}
//		if (pn->countu_max > max_coun_u_ogr 	&&	pn->v < pn->vzd) 
		if (pn->countu_max > max_coun_u_ogr 	&&	dvr < -consb.v)
		{
			log_printf(LOG_WARNING,pn->rindex,NODE_V,"Игнорировано VMAX :%d V=%.2f Vзд=%.2f \n",pn->npu,pn->v,pn->vzd);
			kod = AB_CODE_IGNOGRV;
		}

		if( pn->tip==GEN) pn->sg=compl(pn->sg.real(),-pn->r.imag());
		if( pn->tip==BAZA && fwc==0) pn->sg=compl(-pn->r.real(),-pn->r.imag());
		if (pn->tip == BAZA && fwc != 0) {
			if (pn->pi->tip==islands::I_GEN) pn->sg = compl(pn->sg.real(), -pn->r.imag());
		}
		if (pn->tconnect== N_SUPER && (pn->tip==GEN || pn->tip==GEN_MAX || pn->tip== GEN_MIN) )
		{

			double dqm=(pn->sg.imag()-pn->qmin)/(pn->qmax-pn->qmin);

			double vr=pn->vzd;

			for (pnod1 pnn=pn->next_in;  pnn!=NULL ; pnn=pnn->next_in)
			{
				if (pnn->tip== GEN || pnn->tip== GEN_MAX || pnn->tip== GEN_MIN) 
				{ 

					pnn->vzd=vr*abs(pnn->kt); 
					double qg=(pnn->qmax-pnn->qmin)*dqm+pnn->qmin;
					pnn->tip=pn->tip;
					pnn->sg=compl(pnn->sg.real(),qg);
				}
			}
		}
		if (pn->tconnect== N_SUPER && pn->tip==BAZA )
		{
			int count=0;
			compl sg(0.,0.);
				for (pnod1 pnn=pn->next_in;  pnn!=NULL ; pnn=pnn->next_in) 
				{ 
					if (pnn->tip == BAZA ) count++;
					else if (pnn->tip == NAG ) sg += pnn->sg;
					else if(pnn->tip== GEN || pnn->tip== GEN_MAX || pnn->tip== GEN_MIN) 
					{ 
						double dmax=0; 
						if (pnn->qmax <0 ) dmax=pnn->qmax; 
						if (pnn->qmin >0. ) dmax=pnn->qmin;
						pnn->sg= compl(pnn->sg.real(),dmax);
						sg += pnn->sg;
					}
				}
				pn->sg -=sg;
				pn->sg /= count;
				for (pnod1 pnn=pn->next_in;  pnn!=NULL ; pnn=pnn->next_in) if (pnn->tip== BAZA ) pnn->sg= pn->sg;
		}





	}


	for (pnod1 pny = nod1_s; pny<nod1_s + ny; pny++)
		if (pny->genas)
		{
			if (pny->tip == BAZA)			//для базы делим P узла пропорционально старым значениям
			{
				double psum = 0;
				int num_g = 0;
				for (pgenass pgg = pny->genas; pgg; pgg = pgg->nx) if (pgg->state == 0)
				{
					psum += pgg->p; num_g++;
				}
				if (psum != 0. && !isnan(psum))
				{
					for (pgenass pgg = pny->genas; pgg; pgg = pgg->nx) if (pgg->state == 0)
						pgg->p = pgg->p / psum * pny->sg.real();
				}
				else
				{
					for (pgenass pgg = pny->genas; pgg; pgg = pgg->nx)if (pgg->state == 0)
						pgg->p = pny->sg.real() / num_g;
				}


			}
			calc_Q2(gen_p, pny->sg.real(), pny->sg.imag(), pny);
		}



	for (pn = nod1_s; pn < nod1_s + ny; pn++)
	{
		if (pn->current_polin != NULL) free(pn->current_polin);
	}

	if (kod != 0 && save_first && is_saved) {
		restore_it(ny);
		new_u(ny);
		sij_all_line(ny);
//		for (pnod1 pnk = nod1_s; pnk < nod1_s + ny; pnk++) if (pnk->npu==909) 		log_printf(LOG_INFO, -1, "", "Восстановлено  V=%.2f  \n",  pnk->v);

	}




	facts_wrt_gen();
	time_all.finish();




	time_all.print (" Время расчета ");
	time_trian.print (" Время триангуляции ");
	time_neb.print (" Время расчета небалансов ");
	time_formj.print (" Время формирование матрицы ");
	time_solv.print (" Время решения LU ");
	time_new.print (" Время нов напр ");

	if(stag >=0) _Module.Logger.CloseStage (stag);
	return kod;
}

//void steady_state::SaveMatrixY(const char * FileName, int NodeCount)
//{
//	HANDLE f = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
//	char b[256];
//	DWORD d;
//	sprintf(b, "NodeCount = %d", NodeCount);//, BranchCount);
//	WriteFile(f, b, strlen(b), &d, 0);
//
//	for(pnod1 n = nod1_s; n < nod1_s + NodeCount; n++)
//	{
//		pmaty dg = 0;
//		for(pmaty l = n->nobr->nhp; l < n->nobr[1].nhp; l = l++)
//			if(l->nqy->niyp == n) dg = l;
//		sprintf(b, "\nNode %d(%f, %f):", n->npu, dg->y.real(), dg->y.imag());
//		WriteFile(f, b, strlen(b), &d, 0);
//
//		unsigned int Min, Pos = 0;
//		while(true)
//		{
//			pmaty t = 0;
//			Min = -1;
//			for(pmaty l = n->nobr->nhp; l < n->nobr[1].nhp; l = l++)
//				if(unsigned(l->nqy->niyp->npu) > Pos && unsigned(l->nqy->niyp->npu) < Min && l != dg) { Min = l->nqy->niyp->npu; t = l;}
//				if(!t) break;
//				sprintf(b, " %d(%f, %f)", t->nqy->niyp->npu, t->y.real(), t->y.imag());
//				WriteFile(f, b, strlen(b), &d, 0);
//				Pos = Min;
//		}
//	}
//
//	CloseHandle(f);
//	return;
//}

int steady_state::out_ms(int lev,int kod)
{
	switch (kod )
	{
	case V_P:
		{pnod1 pn=rval.ivp;
		log_printf(LOG_ERROR,pn->rindex,NODE_V,get_Rstring(kod),pn->npu);
		}
		break;
	case V_M:
		{pnod1 pn=rval.ivm;
		log_printf(LOG_ERROR,pn->rindex,NODE_V,get_Rstring(kod),pn->npu);
		}
		break;
	case UGOL:
		{pnod1 pn1=rval.il1,pn2=rval.il2;

		log_printf(LOG_ERROR,-1,"",get_Rstring(kod),pn1->npu,pn2->npu);
		}break;
	case   AB_CODE_FREC_Q:
	case   AB_CODE_FREC_P:
		log_printf(LOG_ERROR, -1, "", "Невозможно выдержать баланс мощности при расчете с учетом частоты");
		break;
	case ITER:
	default:
		log_printf(LOG_ERROR,-1,"",get_Rstring(kod));
		break;
	}

	// log_printf(LOG_INFO,-1,"",get_Rstring(kod));
	// fcprintf(lev,get_Rstring(kod));
	return 0;
}



int steady_state::out_tb(char st) // вывод стpоки на итеpации
{
	string s;
	s = build_str( get_Rstring(A_IDS_RG_FM),		
					   st,
					   rval.iter, 
					   rval.s_max,                                     // max neblanse P
					   rval.max_neb_q,                                 // max neblanse Q
					   rval.is_max->npu,                               // node max nebalance P
					   (rval.is_qmax != NULL) ? rval.is_qmax->npu : 0, // node max nebalance Q
					   rval.dvp,                                       // max otklon up U
					   rval.ivp->npu,                                  // node max otklon up U
					   rval.dvm,                                       // max otklon down U
					   rval.ivm->npu,                                  // node max otklon down U
					   rval.dd_max*57.296,                             // max ugol vetv
					   rval.il1->npu,                                  // ip  max ugol vetv
					   rval.il2->npu,                                  // iq  max ugol vetv
					   rval.rk,
					   rval.step
					  );
	if( print_mode != NONE_PRINT )	
	{
		log_printf( LOG_MESSAGE, -1, "", s.c_str() );
	};
	ATLTRACE( s.c_str() );
	return 0;
}

int steady_state::log_printf(LogErrorCodes kd,int index,char *table,const char *form, ...)
{
	if(blok_out) return 0;

	va_list argptr;
	char buf[400];
	CLog::LogEntry inf_log;
	inf_log.m_SrcTable		=table;
	inf_log.m_nSrcTableIndex=index;
	inf_log.m_Code			=kd;

	va_start(argptr, form);

	if(r!=NULL) r->vsqprintf(2,form, argptr);
	_vsnprintf(buf, sizeof(buf)/sizeof(char)-1, form, argptr);
	char    *rn, *r2=buf;
	while( (rn=strchr(r2,'\n')) !=NULL)
	{
		*rn=0;
		_Module.Logger.Log(inf_log,r2);
		r2=rn+1;
	}
	if(strlen(r2) ) _Module.Logger.Log(inf_log,r2); 


	va_end(argptr);
	return 0;
}

int steady_state::log_open (const char *form, ...)
{
	if(blok_out) return 0;
	char buff[400];
	va_list argptr;
	va_start(argptr, form);
	vsprintf(buff, form, argptr);

	if(r!=NULL) r->vsqprintf(2,form, argptr);
	int kd=_Module.Logger.OpenStage(buff);
	va_end(argptr);
	return kd;	
}

//------------------


steady_state::steady_state(my_Storage *r, unsigned long *fl, struct R_pd_opt &p, CProt * pr) :
nff(r), flag(fl), pd(p), r(pr), one_cycle(-1), gen_ss(NULL), step_mtd(0), ogr_out(false), m_gensize(0), method_ogr(OLD_OGR)
{
	trian = &steady_state::trian_ref;					
	nod1_s=NULL;
#ifdef _cimsubst
	cimloads_s=NULL;
#endif
	nod2_s=NULL;
	polin_s=NULL;
	tr_s=NULL;
	//	poin=0;
	matj_s = _matj_s = NULL;
	dclink_s=NULL;dclink_num=0;
	pfacts_s=NULL;
	islands_s=NULL;
	vet_s = NULL;
	sech_s = NULL;
	grline_s = NULL;
	//	area_s=NULL;
	dff = 0.;
//	struct con c={1.,1.,10.,0.05,0.1,2.0,0.50,1.570797,1.2,0.,20,8};
//	consb=c;
	nfil=nzei=nplo=0;jakk=0;blok_out=false;
	memset(&rval,0,sizeof(struct rvalue));
	init_matr=0;
	find_det=false;
}

steady_state::~steady_state()
{
	if(init_matr)
		for(size_t i=0;i<pd.nk;i++)
			pd.may[i].nqy=maty_s[i].nqy-nod2_s;
	rfree(_matj_s);
	rfreegenas();	//rfree(gen_ss);	bagr 05.2012
	//	rge_file *fr=&rg->fr;
	rfree(nod2_s);	
	rfree(nod1_s);
	rfree (tr_s);
	rfree(dclink_s); 
	rfree(pfacts_s);
	rfree(islands_s);
	free(sech_s);
	free(grline_s);
	delete[] vet_s;
 #ifdef _cimsubst
	//rfree(cimloads_s);
	_cimLoad *pc,*pc2;
	vector<_cimLoad::_cimpolin>::iterator itpol;
	for(pc=cimloads_s ; pc ; )
	{
		pc2 = pc->nx;
		for(itpol=pc->loads.begin() ; itpol!=pc->loads.end() ; itpol++)
		{
			if(itpol->prP)
			{
				itpol->prP->Release();
				//		itpol->prP = NULL;
			}
			if(itpol->prQ)
			{
				itpol->prQ->Release();
				//		itpol->prQ = NULL;
			}
		}
		delete pc;
		pc = pc2;
	}
	cimloads_s = NULL;
#endif
	rfree(polin_s);
};




compl steady_state::statx(pnod1 pny,compl *sn,bool *f)
{
	polin* psx = NULL;
	compl ret,rt2;
	int ik=pny->nsx-3;
	if(polin_s!=NULL && ik>=0 && ik<npol)  { psx = &polin_s[ik]; }
	  double df=dff;
  if (islands_s!=NULL && pny->pi!=NULL)
  {
	  df=pny->pi->frec;
  }


#ifdef _cimsubst
	if(pny->pCimLoad)
	{
		//double *r_sx = pny->pCimLoad->resultPolin;
		double *r_sx(NULL);
		double z = pny->uhom > TINY ? (double)pny->v / pny->uhom : 0;
		double z1 = z * z;
		double z2 = z1+z1;
		//double p = sn->real(),q = sn->imag();
		double p(0),q(0);


		vector<_cimLoad::_cimpolin>::iterator it;
		for(it=pny->pCimLoad->loads.begin() ; it!=pny->pCimLoad->loads.end() ; it++)
		{
			r_sx = it->a;
			p += it->sBase.real() * (r_sx[0]+r_sx[1]*z+r_sx[2]*z1);
			q += it->sBase.imag() * (r_sx[3]+r_sx[4]*z+r_sx[5]*z1);

			ret += compl(it->sBase.real()*(r_sx[1]*z+r_sx[2]*z2),it->sBase.imag()*(r_sx[4]*z+r_sx[5]*z2));
			rt2 += compl(it->sBase.real()*(r_sx[2]*z2),it->sBase.imag()*(r_sx[5]*z2));
		}
		p += pny->pCimLoad->sRest.real();
		q += pny->pCimLoad->sRest.imag();
		*sn = compl(p,q);
	}
	else

#endif


		ret = stat_x(pny->nsx, pny->v, pny->uhom, sn, psx, rt2, fwc, df,  pny->current_polin,f);
	//  if(fwc!=0 && pny->kfc<0.)  *sn -=compl(pny->kfc*dff*pny->pg_nom,0.);
	{
		int    nRes = 0;
		int    nFactsIndx = CFACTSs::NO_FACTS;
		compl  sn2;
		compl  rt2;
		compl  retf( 0., 0. );
		int    nKod = 0;
		double dBF = 0.;
		nRes = m_FACTSs.HaveNodeFacts( pny - nod1_s, &nFactsIndx );
		if( ( nRes > 0 ) && ( nFactsIndx != CFACTSs::NO_FACTS ) )
		{
			sn2 = *sn;
			retf = m_FACTSs.Calc( TRUE, nFactsIndx, pny->v, pny->uhom, &sn2, rt2, fwc, dff, pny->kfc, pny->pg_nom, &dBF, &nKod );
			ret += retf;
			/*
			compl dy( 0., 0. );
			dy = pny->nobr->nhp->y;
			pny->nobr->nhp->y = pny->y_ish + compl( 0, dBF * 1E-6 );
			dy -= pny->nobr->nhp->y;
			*/
		};
		ATLASSERT( nRes > 0 );
	}
	return ret;
}
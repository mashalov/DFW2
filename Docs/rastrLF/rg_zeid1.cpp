
#include "stdafx.h"
#include "rg_ss.h"
#define _USE_MATH_DEFINES
#include <math.h>

#define CRR -1000.
//int poin;
//pmaty smat[500];
//double sfl[500];
void steady_state::savetostack(pmaty nt, double d)
{
	smat.push(nt);
	sfl.push(d);
	// smat[poin]=nt;
	// sfl[poin]=d;
	// poin++;
	// ATLASSERT(poin < 500);
}

int steady_state::restorefromstack(pmaty *nt, double *d)
{

	if (!smat.empty())
	{
		*nt = smat.top(); smat.pop();
		*d = sfl.top();  sfl.pop();
		return 1;

	}
	return 0;
	/* if(poin > 0)
	 {
	   poin--;

	  *nt=smat[poin];
	   *d=sfl[poin];
	 return 1;
	 }
	 return 0;
	 */
}

#include <float.h>
void steady_state::printy(int ny, int nby)
{
	pmaty nt, ind1, ind2;
	pnod2  ptn;
	pnod1 pny, pnyj;
	fcprintf(2, " -----------------\n");
	for (ptn = nod2_s; ptn < nod2_s + ny; ptn++)
	{
		int flag = 0;
		double db = 0.;
		pny = ptn->niyp;
		ind1 = ptn->nhp;
		ind2 = (ptn + 1)->nhp;
		for (nt = ind1; nt < ind2; nt++)
		{
			pnyj = nt->nqy->niyp;
			fcprintf(2, " %d - %d  : %.2f %.2f \n", pny->npu, pnyj->npu, nt->y.real(), nt->y.imag());

		}
	}

}






void  steady_state::zeid(int ny, int nby)
{
	pmaty nt, ind1, ind2, ntd;
	pnod2  ptn;
	pnod1 pny, pnk, pn, pnyj;
	compl sn, du, tok, ud, s1, re, yd;
	double q, q1, d;
	int it;
	pnk = nod1_s;
	// printy(ny,nby);

	 //FILE *str = fopen("c:\\tmp\\itp.csv", "a");


	for (pny = nod1_s; pny < nod1_s + ny; pny++)  pny->a.nsp = NULL;

	for (pny = nod1_s; pny < nod1_s + ny; pny++)
	{
		pny->nkg = 0; pny->dv = pny->v; pny->dd = pny->a.delta;
		if (pny->tip == BAZA) { pnk->a.nsp = pny; pnk++; pny->nkg = 1; }
	}
	for (pny = nod1_s; pnk < nod1_s + ny && pny < nod1_s + ny; pny++)
	{
		pn = pny->a.nsp;
		if (pn == NULL) continue;
		ptn = pn->nobr;
		ind1 = ptn->nhp;
		ind2 = (ptn + 1)->nhp;
		for (nt = ind1; nt < ind2; nt++)
		{
			pnyj = nt->nqy->niyp;
			if (pn == pnyj) continue;
			if (pnyj->nkg) continue;
			pnyj->nkg = 1;
			pnk->a.nsp = pnyj;
			pnk++;
		}
	}

	for (ptn = nod2_s; ptn < nod2_s + ny; ptn++)
	{
		int flag = 0;
		double db = 0.;
		pny = ptn->niyp;
		ind1 = ptn->nhp;
		ind2 = (ptn + 1)->nhp;
		for (nt = ind1; nt < ind2; nt++)
		{
			if (nt->nqy == ptn) { ntd = nt; continue; }
			// ЇаRў_аЄ  §- Є 
			if (nt->y.imag() > 0.)
			{
				flag = 1;
				savetostack(nt, nt->y.imag());
				pnyj = nt->nqy->niyp;
				if (fabs(pny->uhom - pnyj->uhom) / pny->uhom > 0.2)
				{
					//if(pny->uhom > pnyj->uhom) db+=nt->y.im*(pny->uhom/pnyj->uhom);
					//else
					db += (nt->y.imag() - CRR) / (pny->uhom / pnyj->uhom);
				}
				else  db += nt->y.imag() - CRR;
				nt->y = compl(nt->y.real(), CRR);
				//		db-=CRR;
			}
		}
		if (flag)
		{
			savetostack(ntd, ntd->y.imag());
			//	ntd->y.im+=db;
			ntd->y += compl(0., db);
		}
	}

	// printy(ny,nby);

	for (it = 0; it < consb.itz; it++)
	{
		for (pny = nod1_s + nby; pny < nod1_s + ny; pny++)
		{
			pn = pny->a.nsp;
			if (pn == NULL) continue;
			sn = pn->sn;
			statx(pn, &sn);
			ptn = pn->nobr;
			ind1 = ptn->nhp;
			ind2 = (ptn + 1)->nhp;
			du = compl(0., 0.);
			for (nt = ind1; nt < ind2; nt++)
			{
				pnyj = nt->nqy->niyp;
				if (pnyj == pn) ntd = nt;
				else // mulla(&du,&(nt->y),&(pnyj->u));
					du += nt->y*pnyj->u;

			}
			//   s1.re=sn.re-pn->sg.re;s1.im=sn.im-pn->sg.im;
			s1 = sn - pn->sg;
			//   pn->s.re=s1.re;pn->s.im=s1.im;q1=0.;
			pn->s = s1;
			q1 = 0.;
			//   double qqq = 0;
			// ud.re=pn->u.re;ud.im=pn->u.im;
			ud = pn->u;
			if (pn->tip == GEN)
			{
				//  tok.re=du.re;tok.im=du.im;
				//    mulla(&tok,&(ntd->y),&(pn->u));
				tok = du + ntd->y*pn->u;

				//    q=s1.im+tok.im*ud.re - tok.re*ud.im;
				q = s1.imag() + tok.imag()*ud.real() - tok.real()*ud.imag();

				//	  qqq= q;

				if (it > 1)
				{
					if (q < pn->qmin - consb.qk && method_ogr != DOP_OGR)
					{
						pn->tip = GEN_MIN;
						q = pn->qmin;
						q1 = q;
					}
					else if (q > pn->qmax + consb.qk && method_ogr != DOP_OGR)
					{
						pn->tip = GEN_MAX;
						q = pn->qmax;
						q1 = q;
					}
				}

				// s1.im=pn->s.im-q;
				s1 = compl(s1.real(), pn->s.imag() - q);

				//   pn->s.im-=q1;
				pn->s -= compl(0., q1);
				//  pn->sg.im=q1;
				pn->sg = compl(pn->sg.real(), q1);

			}
			//   divc(&re,&s1,&ud); re.im=-re.im;
			//   re.re-=du.re;re.im-=du.im;yd.re=ntd->y.re;yd.im=ntd->y.im;
			//   divc(&du,&re,&yd);
			//   fprintf(str, "%8d  %12.2f %12.2f %12.2f %12.2f  \n ", pn->npu, du.real(), du.imag(), pn->u.real(), pn->u.imag());

			du = (conj(s1 / ud) - du) / ntd->y;
			//	fprintf(str, "%8d  %12.4f %12.4f %12.2f %12.2f  \n ", pn->npu, ntd->y.real(), ntd->y.imag(),  du.real(), du.imag() );

			//   pn->u.re=ud.re+cons.rku*(du.re-ud.re);
			//   pn->u.im=ud.im+cons.rku*(du.im-ud.im);
			pn->u = ud + consb.rku*(du - ud);

			if (_isnan(pn->u.real()))
			{
				pn->u = 1;
			}
			//  pn->v=sqrt(pn->u.re*pn->u.re+pn->u.im*pn->u.im);
			pn->v = abs(pn->u);
			//	double vvv = pn->v;
			switch (pn->tip)
			{
			case GEN_MIN:
				//	     if(pn->v >pn->vzd) break;
				if (pn->get_difv() > -consb.v) break;
				q = pn->qmin;
				goto REST;
			case GEN_MAX:
				//	     if(pn->v < pn->vzd) break;
				if (pn->get_difv() < consb.v) break;
				q = pn->qmax;
			REST: //pn->sg.im=0.f;
				pn->sg = compl(pn->sg.real(), 0.);

				//	     pn->s.im+=q;
				pn->s += compl(0, q);
			case GEN:
				d = pn->vzd / pn->v;
				//	     pn->u.re*=d;pn->u.im*=d;
				pn->u *= d;
				pn->v = pn->vzd;
				pn->tip = GEN;
				break;
			default: break;
			}
			//if (pn->npu==1670 || pn->npu == 1676 || pn->npu == 1685 || pn->npu == 1636  )
			//fprintf(str, "%8d %8d %12.4f %12.4f %12.2f %12.2f %12.2f %12.2f \n ", pn->npu, pn->tip,vvv, pn->v, qqq,q,pn->qmin,pn->qmax);

		}
	}



	for (pn = nod1_s; pn < nod1_s + nby; pn++)
	{
		pnod1 pny = pn->a.nsp;
		if (pny == NULL) continue;
		pny->nkg = 1;
		pny->dvr = pny->dd;
	}
	for (pn = nod1_s + nby; pn < nod1_s + ny; pn++)
	{
		pnod1 pny = pn->a.nsp;
		if (pny == NULL) continue;
		pny->nkg = 0;
		pny->dvr = 0;
	}
	double pi2 = M_PI * 2;
	double ik[] = { -2 * M_PI,0, 2 * M_PI };
	int siik = sizeof(ik) / sizeof(double);
	for (pn = nod1_s + nby; pn < nod1_s + ny; pn++)
	{
		pnod1 pny = pn->a.nsp;
		if (pny == NULL) continue;
		ptn = pny->nobr;
		ind1 = ptn->nhp;
		ind2 = (ptn + 1)->nhp;
		double ds = 0;
		int ns = 0;
		for (nt = ind1; nt < ind2; nt++)
		{
			pnyj = nt->nqy->niyp;
			if (pnyj->nkg) {
				ds += pnyj->dvr; ns++;
			}
		}
		if (ns != 0)
		{
			ds /= ns;
			double delta = arg(pny->u);
			double minm = 1e10;
			int iks = 0;
			for (int i = 0; i < siik; i++)
			{
				double dk = abs(delta + ik[i] - ds);
				if (dk < minm)
				{
					minm = dk;
					iks = i;
				}
			}
			pny->dvr = delta + ik[iks];
			pny->nkg = 1;
		}
		else pny->dvr = 0;
		pny->nkg = 1;
	}
	for (pny = nod1_s; pny < nod1_s + ny; pny++)
	{
		if (pny->tconnect == N_SLAVE) continue;
		//  pny->a.delta=atan2(pny->u.im,pny->u.re);
		if (pny->tip == UPR_TRANS)
		{
			d = pny->vzd / pny->v;
			pny->u *= d;
			pny->v = pny->vzd;
		}



		//	pny->a.delta=arg(pny->u);
		pny->a.delta = pny->dvr;
		pny->dv = (pny->v - pny->dv) / pny->dv;
		pny->dd = pny->a.delta - pny->dd;
	}
	// printy(ny,nby);

	while (restorefromstack(&nt, &d))
	{
		//	nt->y.im=d;
		nt->y = compl(nt->y.real(), d);
	}
	//  printy(ny,nby);

	 //fclose(str);

}






void  steady_state::zeid2(ptrdiff_t ny, ptrdiff_t nby)
{
	pnod1 pny, pnk, pn, pnyj;
	compl sn, du, tok, ud, s1, re, yd;
	double q, q1, d;
	int it;
	pnk = nod1_s;
	consb.rku = 1.;

	int USE_Z = 0;
	// Оптимальное упорядочивание узлов для метода Зейделя
	for (pny = nod1_s; pny < nod1_s + ny; pny++)  pny->a.nsp = NULL;

	if (USE_Z) // Вариант 1 = по эквивалентному сопротивлению от балансируещего узла
	{
		typedef multimap<double, pnod1> rk;
		rk keys;
		pair < rk::const_iterator, rk::const_iterator> dpz;

		for (pny = nod1_s, pnk = nod1_s; pny < nod1_s + ny; pny++)
		{
			pny->nkg = 0;
			pny->dv = pny->v;
			pny->dd = pny->a.delta;
			pny->dvr = 1.e99;

			if (pny->tip == BAZA)
			{
				pny->nkg = 1;
				pny->dvr = 0.;
				pny->kt2 = 1;
				keys.insert(make_pair(0., pny));
			}
		}

		for (pny = nod1_s; pny < nod1_s + ny; pny++)
		{
			auto kk = keys.begin();
			if (kk == keys.end()) break;
			pn = kk->second;
			pnk->a.nsp = pn;
			pnk++;
			pn->nkg = 2;
			keys.erase(kk);
			double zt = pn->dvr;
			ptrdiff_t ii = pn - nod1_s;
			for (ptrdiff_t st = ogg[ii].first, en = ogg[ii].second; st < en; st++)
			{
				pvet pv = vet_s + gvec[st];
				compl cij, ci;
				if (pv->sta)continue;
				pnyj = pv->get_opposite(pn);
				if (pnyj->nkg > 1) continue;
				double kt2 = 1;
				double zn = zt + pv->get_ZOE(pn, kt2);
				if (pnyj->nkg == 1)
				{
					if (pnyj->dvr > zn)
					{
						dpz = keys.equal_range(pnyj->dvr);
						for (auto k = dpz.first; k != dpz.second; k++) if (k->second == pnyj)
						{
							keys.erase(k);
							keys.insert(make_pair(zn, pnyj));
							break;
						}
						pnyj->dvr = zn;
						pnyj->kt2 = kt2;

					}
					continue;
				}
				pnyj->nkg = 1;
				pnyj->dvr = zn;
				pnyj->kt2 = kt2;
				keys.insert(make_pair(pnyj->dvr, pnyj));
			}
		}

	}

	else   // Простой алгоритм упорядочения от балансирующего узла
	{

		for (pny = nod1_s, pnk = nod1_s; pny < nod1_s + ny; pny++)
		{
			pny->nkg = 0;
			pny->dv = pny->v;
			pny->dd = pny->a.delta;
			if (pny->tip == BAZA)
			{
				pnk->a.nsp = pny;
				pnk++;
				pny->nkg = 1;
			}
		}

		for (pny = nod1_s; pny < nod1_s + ny; pny++)
		{
			pn = pny->a.nsp;
			if (pn == NULL) continue;
			ptrdiff_t ii = pn - nod1_s;
			for (ptrdiff_t st = ogg[ii].first, en = ogg[ii].second; st < en; st++)
			{
				pvet pv = vet_s + gvec[st];
				compl cij, ci;
				if (pv->sta)continue;
				pnyj = pv->get_opposite(pn);
				if (pnyj->nkg) continue;
				pnyj->nkg = 1;
				pnk->a.nsp = pnyj;
				pnk++;
			}
		}

	}
	// Конец блока упорядочения 

	// Формирование проводимостей для метода Зейделя с удалением отрицательных
	for (pvet pv = vet_s; pv < vet_s + nv; pv++) if (!pv->sta) pv->form_c();

	for (pny = nod1_s; pny < nod1_s + ny; pny++)
	{
		pn = pny->a.nsp;
		if (pn == NULL) continue;
		ptrdiff_t ii = pn - nod1_s;
		compl yii = conj(pn->cysh);
		for (ptrdiff_t st = ogg[ii].first, en = ogg[ii].second; st < en; st++)
		{
			pvet pv = vet_s + gvec[st];
			if (pv->sta)continue;
			yii += pv->get_yii(pn);
		}
		pn->sii = yii;
	}

	// Цикл метода Зейделя

	
	for (it = 0; it < consb.itz; it++)
	{
		double Pmax(0.0);
		int imax(0);

		for (pny = nod1_s + nby; pny < nod1_s + ny; pny++)
		{
			pn = pny->a.nsp;
			if (pn == NULL) continue;
			sn = pn->sn;
			statx(pn, &sn);
			du = compl(0., 0.);

			ptrdiff_t ii = pn - nod1_s;
			for (ptrdiff_t st = ogg[ii].first, en = ogg[ii].second; st < en; st++)
			{
				pvet pv = vet_s + gvec[st];

				if (pv->sta) continue;
				pnyj = pv->get_opposite(pn);
				du += pnyj->u*((pv->pnb == pn) ? pv->sij : pv->sji);
			}
			compl siii = pn->sii;
			if (0) // Добавление нагрузки в диагональ - отключено в текущей версии
			{
				if (sn.real() > 0. && sn.imag() > 0.)
				{
					siii += -conj(sn) / (pn->v*pn->v);
					sn = 0.;
				}
			}
			s1 = sn - pn->sg;
			pn->s = s1;
			q1 = 0.;

			ud = pn->u;
			bool check = true;
			if (pn->tip == GEN)
			{
				tok = du + siii * pn->u;

				q = s1.imag() + tok.imag()*ud.real() - tok.real()*ud.imag();

				if (it > consb.itz_ogr_max) //Контроль ограничений по Qmax начиная с заданной итерации
				{
					if ((it > consb.itz_ogr_max + consb.itz_ogr_min) && q < pn->qmin - consb.qk && method_ogr != DOP_OGR) //Контроль ограничений по Qmin начиная с заданной итерации
					{
						pn->tip = GEN_MIN;
						q = pn->qmin;
						q1 = q;
						check = false;
					}
					else if (q > pn->qmax + consb.qk && method_ogr != DOP_OGR) //Контроль ограничений по Qmax
					{
						pn->tip = GEN_MAX;
						q = pn->qmax;
						q1 = q;
						check = false;
					}
				}

				s1 = compl(s1.real(), pn->s.imag() - q);

				pn->s -= compl(0., q1);
				pn->sg = compl(pn->sg.real(), q1);

			}

			compl stest = conj(du + ud * siii) * ud - s1;

			if (fabs(stest.real()) > fabs(Pmax))
			{
				Pmax = stest.real();
				imax = pn->npu;
			}

			du = (conj(s1 / ud) - du) / siii;
			pn->u = ud + consb.rku*(du - ud);

			if (_isnan(pn->u.real()))
			{
				pn->u = 1;
			}
			pn->v = abs(pn->u);



			if (check)switch (pn->tip) // Контроль восстановления напряжений
			{
			case GEN_MIN:
				if (pn->get_difv() > -consb.v) break;
				q = pn->qmin;
				goto REST;
			case GEN_MAX:
				if (pn->get_difv() < consb.v) break;
				q = pn->qmax;

			REST:
				pn->sg = compl(pn->sg.real(), 0.);
				pn->s += compl(0, q);
			case GEN:
				d = pn->vzd / pn->v;
				pn->u *= d;
				pn->v = pn->vzd;
				pn->tip = GEN;
				break;
			default: break;
			}
		}

		ATLTRACE("\n%g %d", Pmax, imax);
	}
	// Конец цикла метода Зейделя


	// Расчет напряжений в полярных координатах с учетом возможного проворота угла
	for (pn = nod1_s; pn < nod1_s + nby; pn++)
	{
		pnod1 pny = pn->a.nsp;
		if (pny == NULL) continue;
		pny->nkg = 1;
		pny->dvr = pny->dd;
	}
	for (pn = nod1_s + nby; pn < nod1_s + ny; pn++)
	{
		pnod1 pny = pn->a.nsp;
		if (pny == NULL) continue;
		pny->nkg = 0;
		pny->dvr = 0;
	}
	double pi2 = M_PI * 2;
	double ik[] = { -2 * M_PI,0, 2 * M_PI };
	int siik = sizeof(ik) / sizeof(double);
	for (pn = nod1_s + nby; pn < nod1_s + ny; pn++)
	{
		pnod1 pny = pn->a.nsp;
		if (pny == NULL) continue;
		double ds = 0;
		int ns = 0;

		ptrdiff_t ii = pny - nod1_s;
		for (ptrdiff_t st = ogg[ii].first, en = ogg[ii].second; st < en; st++)

		{
			pvet pv = vet_s + gvec[st];
			if (pv->sta)continue;

			pnyj = pv->get_opposite(pny);
			if (pnyj->nkg) {
				ds += pnyj->dvr; ns++;
			}

		}


		if (ns != 0)
		{
			ds /= ns;
			double delta = arg(pny->u);
			double minm = 1e10;
			int iks = 0;
			for (int i = 0; i < siik; i++)
			{
				double dk = abs(delta + ik[i] - ds);
				if (dk < minm)
				{
					minm = dk;
					iks = i;
				}
			}
			pny->dvr = delta + ik[iks];
			pny->nkg = 1;
		}
		else pny->dvr = 0;
		pny->nkg = 1;
	}
	for (pny = nod1_s; pny < nod1_s + ny; pny++)
	{
		if (pny->tconnect == N_SLAVE) continue;
		//  pny->a.delta=atan2(pny->u.im,pny->u.re);
		if (pny->tip == UPR_TRANS)
		{
			d = pny->vzd / pny->v;
			pny->u *= d;
			pny->v = pny->vzd;
		}

		pny->a.delta = pny->dvr;
		pny->dv = (pny->v - pny->dv) / pny->dv;
		pny->dd = pny->a.delta - pny->dd;
	}
}


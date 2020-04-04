#pragma once
#include "klu.h"
#include "klu_version.h"
#include "cs.h"
#include "Header.h"
namespace DFW2
{
	template<typename T>
	class KLUWrapper
	{
	protected:
		unique_ptr<double[]>	pAx,			// ������ ������� �����
								pb;				// ������ ������ �����
		unique_ptr<ptrdiff_t[]> pAi,			// ������ �����
								pAp;			// ������ ��������
		ptrdiff_t m_nMatrixSize;
		ptrdiff_t m_nNonZeroCount;
		ptrdiff_t m_nAnalyzingsCount = 0;
		ptrdiff_t m_nFactorizationsCount = 0;
		ptrdiff_t m_nRefactorizationsCount = 0;
		ptrdiff_t m_nRefactorizationFailures = 0;
		wstring m_strKLUError;

		template <typename T>
		struct doubles_count 
		{
			static const ptrdiff_t count = 1;
		};
		template<>
		struct doubles_count<std::complex<double>>
		{
			static const ptrdiff_t count = 2;
		};

		template <typename T, typename L> class KLUFunctions;


		template<> struct KLUFunctions<double, int>
		{
			static KLU_numeric* TKLU_factor(int* Ap, int* Ai, double* Ax, KLU_symbolic *Symbolic, KLU_common* Common)
			{
				return klu_factor(Ap, Ai, Ax, Symbolic, Common);
			}
			static KLU_symbolic* TKLU_analyze(int nSize, int* Ap, int* Ai, KLU_common* Common)
			{
				return klu_analyze(nSize, Ap, Ai, Common);
			}
			static int TKLU_refactor(int* Ap, int* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_refactor(Ap, Ai, Ax, Symbolic, Numeric, Common);
			}
			static void TKLU_free_numeric(KLU_numeric** Numeric, KLU_common* Common)
			{
				klu_free_numeric(Numeric, Common);
			}
			static void TKLU_free_symbolic(KLU_symbolic** Symbolic, KLU_common* Common)
			{
				klu_free_symbolic(Symbolic, Common);
			}
			static int TKLU_tsolve(KLU_symbolic* Symbolic, KLU_numeric* Numeric, int nSize, int nrhs, double* B, KLU_common* Common)
			{
				return klu_tsolve(Symbolic, Numeric, nSize, nrhs, B, Common);
			}
			static int TKLU_rcond(KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_rcond(Symbolic, Numeric, Common);
			}
		};

		template<> struct KLUFunctions<double, __int64>
		{
			static KLU_numeric* TKLU_factor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic *Symbolic, KLU_common* Common)
			{
				return klu_l_factor(Ap, Ai, Ax, Symbolic, Common);
			}
			static KLU_symbolic* TKLU_analyze(ptrdiff_t nSize, ptrdiff_t* Ap, ptrdiff_t* Ai, KLU_common* Common)
			{
				return klu_l_analyze(nSize, Ap, Ai, Common);
			}
			static ptrdiff_t TKLU_refactor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_l_refactor(Ap, Ai, Ax, Symbolic, Numeric, Common);
			}
			static void TKLU_free_numeric(KLU_numeric** Numeric, KLU_common* Common)
			{
				klu_l_free_numeric(Numeric, Common);
			}
			static void TKLU_free_symbolic(KLU_symbolic** Symbolic, KLU_common* Common)
			{
				klu_l_free_symbolic(Symbolic, Common);
			}
			static ptrdiff_t TKLU_tsolve(KLU_symbolic* Symbolic, KLU_numeric* Numeric, ptrdiff_t nSize, ptrdiff_t nrhs, double* B, KLU_common* Common)
			{
				return klu_l_tsolve(Symbolic, Numeric, nSize, nrhs, B, Common);
			}
			static ptrdiff_t TKLU_rcond(KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_l_rcond(Symbolic, Numeric, Common);
			}
		};

		template<> struct KLUFunctions<std::complex<double>, int>
		{
			static KLU_numeric* TKLU_factor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic *Symbolic, KLU_common* Common)
			{
				return klu_z_factor(Ap, Ai, Ax, Symbolic, Common);
			}
			static KLU_symbolic* TKLU_analyze(ptrdiff_t nSize, ptrdiff_t* Ap, ptrdiff_t* Ai, KLU_common* Common)
			{
				return klu_analyze(nSize, Ap, Ai, Common);
			}
			static int TKLU_refactor(int* Ap, int* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_z_refactor(Ap, Ai, Ax, Symbolic, Numeric, Common);
			}
			static void TKLU_free_numeric(KLU_numeric** Numeric, KLU_common* Common)
			{
				klu_z_free_numeric(Numeric, Common);
			}
			static void TKLU_free_symbolic(KLU_symbolic** Symbolic, KLU_common* Common)
			{
				klu_free_symbolic(Symbolic, Common);
			}
			static int TKLU_tsolve(KLU_symbolic* Symbolic, KLU_numeric* Numeric, int nSize, int nrhs, double* B, KLU_common* Common)
			{
				return klu_z_tsolve(Symbolic, Numeric, nSize, nrhs, B, 0, Common);
			}
			static int TKLU_rcond(KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_l_rcond(Symbolic, Numeric, Common);
			}
		};

		template<> struct KLUFunctions<std::complex<double>, __int64>
		{
			static KLU_numeric* TKLU_factor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic *Symbolic, KLU_common* Common)
			{
				return klu_zl_factor(Ap, Ai, Ax, Symbolic, Common);
			}
			static KLU_symbolic* TKLU_analyze(ptrdiff_t nSize, ptrdiff_t* Ap, ptrdiff_t* Ai, KLU_common* Common)
			{
				return klu_l_analyze(nSize, Ap, Ai, Common);
			}
			static ptrdiff_t TKLU_refactor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_zl_refactor(Ap, Ai, Ax, Symbolic, Numeric, Common);
			}
			static void TKLU_free_numeric(KLU_numeric** Numeric, KLU_common* Common)
			{
				klu_zl_free_numeric(Numeric, Common);
			}
			static void TKLU_free_symbolic(KLU_symbolic** Symbolic, KLU_common* Common)
			{
				klu_l_free_symbolic(Symbolic, Common);
			}
			static ptrdiff_t TKLU_tsolve(KLU_symbolic* Symbolic, KLU_numeric* Numeric, ptrdiff_t nSize, ptrdiff_t nrhs, double* B, KLU_common* Common)
			{
				return klu_zl_tsolve(Symbolic, Numeric, nSize, nrhs, B, 0, Common);
			}
			static ptrdiff_t TKLU_rcond(KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
			{
				return klu_zl_rcond(Symbolic, Numeric, Common);
			}
		};

		template <typename T, typename L>
		class KLUCommonDeleter
		{
			T *m_p;
			KLU_common& Common;
			void CleanUp(KLU_numeric*)
			{
				KLUFunctions<L, ptrdiff_t>::TKLU_free_numeric(&m_p, &Common);
			}
			void CleanUp(KLU_symbolic*)
			{
				KLUFunctions<L, ptrdiff_t>::TKLU_free_symbolic(&m_p, &Common);
			}
		public:
			KLUCommonDeleter(T* p, KLU_common& common) : m_p(p), Common(common) {}
			virtual ~KLUCommonDeleter()
			{
				if (m_p)
					CleanUp(m_p);
			}
			T* GetKLUObject() { return m_p; }

		};

		using KLUSymbolic = KLUCommonDeleter<KLU_symbolic, T>;
		using KLUNumeric = KLUCommonDeleter<KLU_numeric, T>;
		unique_ptr<KLUSymbolic> pSymbolic;
		unique_ptr<KLUNumeric>  pNumeric;
		KLU_common pCommon;

		const _TCHAR* KLUErrorDescription()
		{
			switch (pCommon.status)
			{
			case 0:
				m_strKLUError = CDFW2Messages::m_cszKLUOk;
				break;
			case 1:
				m_strKLUError = CDFW2Messages::m_cszKLUSingular;
				DumpMatrix(true);
				break;
			case -2:
				m_strKLUError = CDFW2Messages::m_cszKLUOutOfMemory;
				break;
			case -3:
				m_strKLUError = CDFW2Messages::m_cszKLUInvalidInput;
				break;
			case -4:
				m_strKLUError = CDFW2Messages::m_cszKLUIntOverflow;
				break;
			default:
				m_strKLUError = Cex(CDFW2Messages::m_cszKLUUnknownError, pCommon.status);
			}
			return m_strKLUError.c_str();
		}
		static const _TCHAR* KLUWrapperName()
		{
			return  _T("KLUWrapper");
		}

	public:
		KLUWrapper()
		{
			KLU_defaults(&pCommon);
		}
		void SetSize(ptrdiff_t nMatrixSize, ptrdiff_t nNonZeroCount)
		{
			m_nMatrixSize = nMatrixSize;
			m_nNonZeroCount = nNonZeroCount;
			pAx = make_unique<double[]>(m_nNonZeroCount * doubles_count<T>::count);			// ����� �������
			pb = make_unique<double[]>(m_nMatrixSize * doubles_count<T>::count);			// ������ ������ �����
			pAi = make_unique<ptrdiff_t[]>(m_nMatrixSize + 1);								// ������ �������
			pAp = make_unique<ptrdiff_t[]>(m_nNonZeroCount);								// ������� �������
			pSymbolic.reset();
			pNumeric.reset();
		}
		inline ptrdiff_t MatrixSize() { return m_nMatrixSize; }
		ptrdiff_t NonZeroCount() { return m_nNonZeroCount; }
		ptrdiff_t AnalyzingsCount() { return m_nAnalyzingsCount; }
		ptrdiff_t FactorizationsCount() { return m_nFactorizationsCount; }
		ptrdiff_t RefactorizationsCount() { return m_nRefactorizationsCount; }
		ptrdiff_t RefactorizationFailuresCount() { return m_nRefactorizationFailures; }
		inline double* Ax() { return pAx.get(); }
		inline double* B() { return pb.get(); }
		inline ptrdiff_t* Ai() { return pAi.get(); }
		inline ptrdiff_t* Ap() { return pAp.get(); }
		KLU_symbolic* Symbolic() { return pSymbolic->GetKLUObject(); }
		KLU_common* Common() { return &pCommon; }
		void Analyze()
		{
			pSymbolic = make_unique<KLUSymbolic>(KLUFunctions<T, ptrdiff_t>::TKLU_analyze(m_nMatrixSize, pAi.get(), pAp.get(), &pCommon), pCommon);
			if (!Analyzed())
				throw dfw2error(Cex(_T("%s::KLU_analyze %s"), KLUWrapperName(), KLUErrorDescription()));
			m_nAnalyzingsCount++;
		}

		void Factor()
		{
			if (!Analyzed())
				Analyze();
			pNumeric = make_unique<KLUNumeric>(KLUFunctions<T, ptrdiff_t>::TKLU_factor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), &pCommon) , pCommon);
			if (!Factored())
				throw dfw2error(Cex(_T("%s::KLU_factor %s"), KLUWrapperName(), KLUErrorDescription()));
			m_nFactorizationsCount++;
		}

		bool TryRefactor()
		{
			if (!Factored())
				throw dfw2error(Cex(_T("%s::KLU_refactor - no numeric to refactor"), KLUWrapperName()));
			if (KLUFunctions<T,ptrdiff_t>::TKLU_refactor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
			{
				m_nRefactorizationsCount++;
				return true;
			}
			else
			{
				m_nRefactorizationFailures++;
				return false;
			}
		}

		void Refactor()
		{
			if (!TryRefactor())
				throw dfw2error(Cex(_T("%s::KLU_refactor %s"), KLUWrapperName(), KLUErrorDescription()));
		}

		void Solve()
		{
			if (!Analyzed())
				Analyze();
			if(!Factored())
				Factor();
			if(!KLUFunctions<T, ptrdiff_t>::TKLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon))
				throw dfw2error(Cex(_T("%s::KLU_tsolve %s"), KLUWrapperName(), KLUErrorDescription()));
		}

		void FactorSolve()
		{
			if (!Analyzed())
				Analyze();
			Factor();
			if (!KLUFunctions<T,ptrdiff_t>::TKLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon))
				throw dfw2error(Cex(_T("%s::KLU_tsolve %s"), KLUWrapperName(), KLUErrorDescription()));
		}

		double Rcond()
		{
			if(!KLUFunctions<T, ptrdiff_t>::TKLU_rcond(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
				throw dfw2error(Cex(_T("%s::KLU_rcond %s"), KLUWrapperName(), KLUErrorDescription()));
			return pCommon.rcond;
		}

		bool Factored()
		{
			return pNumeric != nullptr && pNumeric->GetKLUObject() != nullptr;
		}

		bool Analyzed()
		{
			return pSymbolic != nullptr && pSymbolic->GetKLUObject() != nullptr;
		}

		double FindMaxB(ptrdiff_t& nMaxIndex)
		{
			double bmax(0.0);
			nMaxIndex = -1;
			double *pbb = pb.get();
			if (m_nMatrixSize > 0)
			{
				nMaxIndex = 0;
				bmax = *pbb;
				pbb++;
				for (ptrdiff_t x = 1; x < m_nMatrixSize; x++, pbb++)
				{
					double absb = abs(*pbb);
					if (bmax < absb)
					{
						nMaxIndex = x;
						bmax = absb;
					}
				}
			}
			return bmax;
		}

		// todo - complex version
		void DumpMatrix(bool bAnalyzeLinearDependenies)
		{
			FILE *fmatrix;
			if (!_tfopen_s(&fmatrix, _T("c:\\tmp\\dwfsingularmatrix.mtx"), _T("w+")))
			{
				ptrdiff_t *pAi = Ap();
				double *pAx = Ax();
				ptrdiff_t nRow = 0;
				set<ptrdiff_t> BadNumbers, FullZeros;
				vector<bool> NonZeros;
				vector<bool> Diagonals;
				vector<double> Expanded;

				NonZeros.resize(MatrixSize(), false);
				Diagonals.resize(MatrixSize(), false);
				Expanded.resize(MatrixSize(), 0.0);

				for (ptrdiff_t *pAp = Ai(); pAp < Ai() + MatrixSize(); pAp++, nRow++)
				{
					ptrdiff_t *pAiend = pAi + *(pAp + 1) - *pAp;
					bool bAllZeros = true;
					while (pAi < pAiend)
					{
						_ftprintf_s(fmatrix, _T("%10td %10td     %30g\n"), nRow, *pAi, *pAx);

						if (isnan(*pAx) || isinf(*pAx))
							BadNumbers.insert(nRow);

						if (fabs(*pAx) > 1E-7)
						{
							bAllZeros = false;
							NonZeros[*pAi] = true;

							if (nRow == *pAi)
								Diagonals[nRow] = true;
						}
						pAx++; pAi++;
					}
					if (bAllZeros)
						FullZeros.insert(nRow);
				}

				for (const auto& it : BadNumbers)
					_ftprintf_s(fmatrix, _T("Bad Number in Row: %td\n"), it);

				for (const auto& it : FullZeros)
					_ftprintf_s(fmatrix, _T("Full Zero Row : %td\n"), it);

				for (auto&& it = NonZeros.begin(); it != NonZeros.end(); it++)
					if (!*it)
						_ftprintf_s(fmatrix, _T("Full Zero Column: %td\n"), it - NonZeros.begin());

				for (auto&& it = Diagonals.begin(); it != Diagonals.end(); it++)
					if (!*it)
						_ftprintf_s(fmatrix, _T("Zero Diagonal: %td\n"), it - Diagonals.begin());


				if (!bAnalyzeLinearDependenies)
					return;

				// �������� ���������� ������� ��������� ������ � ������� ����������� ����-������
				// (v1 dot v2)^2 <= norm2(v1) * norm2(v2)

				pAi = Ap(); pAx = Ax(); nRow = 0;
				for (ptrdiff_t *pAp = Ai(); pAp < Ai() + MatrixSize(); pAp++, nRow++)
				{
					fill(Expanded.begin(), Expanded.end(), 0.0);

					ptrdiff_t *pAiend = pAi + *(pAp + 1) - *pAp;
					bool bAllZeros = true;
					double normi = 0.0;

					set < pair<ptrdiff_t, double> > RowI;

					while (pAi < pAiend)
					{
						Expanded[*pAi] = *pAx;
						normi += *pAx * *pAx;

						RowI.insert(make_pair(*pAi, *pAx));

						pAx++; pAi++;
					}

					ptrdiff_t nRows = 0;
					ptrdiff_t *pAis = Ap();
					double *pAxs = Ax();
					for (ptrdiff_t *pAps = Ai(); pAps < Ai() + MatrixSize(); pAps++, nRows++)
					{
						ptrdiff_t *pAiends = pAis + *(pAps + 1) - *pAps;
						double normj = 0.0;
						double inner = 0.0;

						set < pair<ptrdiff_t, double> > RowJ;

						while (pAis < pAiends)
						{
							normj += *pAxs * *pAxs;
							inner += Expanded[*pAis] * *pAxs;

							RowJ.insert(make_pair(*pAis, *pAxs));

							pAis++; pAxs++;
						}
						if (nRow != nRows)
						{
							double Ratio = inner * inner / normj / normi;
							if (fabs(Ratio - 1.0) < 1E-5)
							{
								_ftprintf_s(fmatrix, _T("Linear dependent rows %10td %10td with %g\n"), nRow, nRows, Ratio);
								for (auto & it : RowI)
									_ftprintf_s(fmatrix, _T("%10td %10td     %30g\n"), nRow, it.first, it.second);
								for (auto & it : RowJ)
									_ftprintf_s(fmatrix, _T("%10td %10td     %30g\n"), nRows, it.first, it.second);
							}

						}
					}
				}
				fclose(fmatrix);
			}
		}
	};
}


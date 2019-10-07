#pragma once
#include "klu.h"
#include "klu_version.h"
#include "cs.h"
#include "Header.h"
#include "dfw2exception.h"
namespace DFW2
{
	class KLUWrapperData
	{
	protected:
		unique_ptr<double[]>	pAx,			// данные матрицы якоби
			pb;				// вектор правой части
		unique_ptr<ptrdiff_t[]> pAi,			// номера строк
			pAp;			// номера столбцов
		ptrdiff_t m_nMatrixSize;
		ptrdiff_t m_nNonZeroCount;
		ptrdiff_t m_nAnalyzingsCount = 0;
		ptrdiff_t m_nFactorizationsCount = 0;
		ptrdiff_t m_nRefactorizationsCount = 0;
		wstring m_strKLUError;

		template <typename T>
		class KLUCommonDeleter
		{
			T *m_p;
			KLU_common& Common;
			void CleanUp(KLU_numeric*)
			{
				KLU_free_numeric(&m_p, &Common);
			}
			void CleanUp(KLU_symbolic*)
			{
				KLU_free_symbolic(&m_p, &Common);
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

		using KLUSymbolic = KLUCommonDeleter<KLU_symbolic>;
		using KLUNumeric = KLUCommonDeleter<KLU_numeric>;
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
			return  _T("KLUWrapper::");
		}
		void Factor()
		{
			if (!Analyzed())
				Analyze();
			pNumeric = make_unique<KLUNumeric>(KLU_factor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), &pCommon), pCommon);
			if (!Factored())
				throw dfw2error(Cex(_T("%s::KLU_factor %s"), KLUWrapperName(), KLUErrorDescription()));
			m_nFactorizationsCount++;
		}

	public:
		KLUWrapperData()
		{
			KLU_defaults(&pCommon);
		}
		void SetSize(ptrdiff_t nMatrixSize, ptrdiff_t nNonZeroCount)
		{
			m_nMatrixSize = nMatrixSize;
			m_nNonZeroCount = nNonZeroCount;
			pAx = make_unique<double[]>(m_nNonZeroCount);				// числа матрицы
			pb = make_unique<double[]>(m_nMatrixSize);					// вектор правой части
			pAi = make_unique<ptrdiff_t[]>(m_nMatrixSize + 1);			// строки матрицы
			pAp = make_unique<ptrdiff_t[]>(m_nNonZeroCount);			// столбцы матрицы
			pSymbolic.reset();
			pNumeric.reset();
		}
		inline ptrdiff_t MatrixSize() { return m_nMatrixSize; }
		ptrdiff_t NonZeroCount() { return m_nNonZeroCount; }
		ptrdiff_t AnalyzingsCount() { return m_nAnalyzingsCount; }
		ptrdiff_t FactorizationsCount() { return m_nFactorizationsCount; }
		ptrdiff_t RefactorizationsCount() { return m_nRefactorizationsCount; }
		double* Ax() { return pAx.get(); }
		double* B() { return pb.get(); }
		ptrdiff_t* Ai() { return pAi.get(); }
		ptrdiff_t* Ap() { return pAp.get(); }
		KLU_symbolic* Symbolic() { return pSymbolic->GetKLUObject(); }
		KLU_common* Common() { return &pCommon; }
		void Analyze()
		{
			pSymbolic = make_unique<KLUSymbolic>(KLU_analyze(m_nMatrixSize, pAi.get(), pAp.get(), &pCommon), pCommon);
			if (!Analyzed())
				throw dfw2error(Cex(_T("%s::KLU_analyze %s"), KLUWrapperName(), KLUErrorDescription()));
			m_nAnalyzingsCount++;
		}

		void Refactor()
		{
			if (!Factored())
				throw dfw2error(Cex(_T("%s::KLU_refactor %s"), KLUWrapperName(), _T("no numeric to refactor")));
			if (!KLU_refactor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
				throw dfw2error(Cex(_T("%s::KLU_refactor %s"), KLUWrapperName(), KLUErrorDescription()));
			m_nRefactorizationsCount++;
		}

		void Solve()
		{
			if (!Analyzed())
				Analyze();
			if(!Factored())
				Factor();
			if(!KLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon))
				throw dfw2error(Cex(_T("%s::KLU_tsolve %s"), KLUWrapperName(), KLUErrorDescription()));
		}

		void FactorSolve()
		{
			if (!Analyzed())
				Analyze();
			Factor();
			if (!KLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon))
				throw dfw2error(Cex(_T("%s::KLU_tsolve %s"), KLUWrapperName(), KLUErrorDescription()));
		}

		double Rcond()
		{
			if(!KLU_rcond(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
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
	};
}


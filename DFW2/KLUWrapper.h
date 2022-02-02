#pragma once
#include "Header.h"
#include "klu.h"
#include "klu_version.h"
#include "cs.h"
#include "fstream"
#include "iomanip"
#ifndef _MSC_VER
#include "mm_malloc.h"
#endif
namespace DFW2
{
	template<typename T, std::size_t Align>
	struct aligned_unique_ptr : public std::unique_ptr<T, void(*)(T*)>
	{
		using ptrT = std::unique_ptr<T, void(*)(T*)>;
		aligned_unique_ptr() : ptrT(nullptr, aligned_unique_ptr::destroy) { }
		aligned_unique_ptr(std::size_t len) : ptrT(reinterpret_cast<T*>(_mm_malloc(sizeof(T) * len, Align)), aligned_unique_ptr::destroy)
		{
			const auto p{ ptrT::get() };
			std::fill(p, p + len, T{});
		}
		static void destroy(T* p)
		{
			_mm_free(p);
		}
	};

	template <typename T> class KLUFunctions;

	// Instantiate KLU function wrappers based on matrix element type

	struct KLUMinMaxDiagonals
	{
		std::pair<double, ptrdiff_t> Min, Max;
	};

#ifdef DLONG

	template<> struct KLUFunctions<double>
	{
		static KLU_numeric* TKLU_factor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_common* Common)
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

		static ptrdiff_t TKLU_condest(ptrdiff_t* Ap, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
		{
			return klu_l_condest(Ap, Ax, Symbolic, Numeric, Common);
		}
	};

	template<> struct KLUFunctions<std::complex<double>>
	{
		static KLU_numeric* TKLU_factor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_common* Common)
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
		static ptrdiff_t TKLU_condest(ptrdiff_t* Ap, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
		{
			return klu_zl_condest(Ap, Ax, Symbolic, Numeric, Common);
		}

	};

#else

	template<> struct KLUFunctions<double>
	{
		static KLU_numeric* TKLU_factor(int* Ap, int* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_common* Common)
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
		static int TKLU_condest(int* Ap, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
		{
			return klu_condest(Ap, Ax, Symbolic, Numeric, Common);
		}
	};

	template<> struct KLUFunctions<std::complex<double>>
	{
		static KLU_numeric* TKLU_factor(ptrdiff_t* Ap, ptrdiff_t* Ai, double* Ax, KLU_symbolic* Symbolic, KLU_common* Common)
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
			return klu_z_rcond(Symbolic, Numeric, Common);
		}
		static int TKLU_condest(int* Ap, double* Ax, KLU_symbolic* Symbolic, KLU_numeric* Numeric, KLU_common* Common)
		{
			return klu_z_condest(Ap, Ax, Symbolic, Numeric, Common);
		}
	};

#endif

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

	// Deleters for KLU objects to be wrapped to unique_ptrs

	// T - KLU object: Numeric or Symboilc. D - KLU data-type (double or complex)
	template <typename T, typename D>
	class KLUCommonDeleter
	{
		T* m_p;
		KLU_common& Common;
		void CleanUp(KLU_numeric*)
		{
			KLUFunctions<D>::TKLU_free_numeric(&m_p, &Common);
		}
		void CleanUp(KLU_symbolic*)
		{
			KLUFunctions<D>::TKLU_free_symbolic(&m_p, &Common);
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

	template<typename T>
	class KLUWrapper
	{
	protected:
		using vector_aligned_double = aligned_unique_ptr<double, 32>;

		std::unique_ptr<double[]>	 pAx;		// данные матрицы якоби
		vector_aligned_double		 pb,		// вектор правой части
									 pRefine;	// вектор уточнения


		std::unique_ptr<ptrdiff_t[]> pAi,		// номера строк
								     pAp;		// номера столбцов
		ptrdiff_t m_nMatrixSize = 0;
		ptrdiff_t m_nNonZeroCount = 0;
		ptrdiff_t m_nAnalyzingsCount = 0;
		ptrdiff_t m_nFactorizationsCount = 0;
		ptrdiff_t m_nRefactorizationsCount = 0;
		ptrdiff_t m_nRefactorizationFailures = 0;
		std::string m_strKLUError;

		using KLUSymbolic = KLUCommonDeleter<KLU_symbolic, T>;
		using KLUNumeric = KLUCommonDeleter<KLU_numeric, T>;
		std::unique_ptr<KLUSymbolic> pSymbolic;
		std::unique_ptr<KLUNumeric>  pNumeric;
		KLU_common pCommon;

		csi cs_gatxpy(const cs* A, const double* x, double* y)
		{
			csi p, j, n, * Ap, * Ai;
			double* Ax;
			if (!CS_CSC(A) || !x || !y) return (0);       /* check inputs */
			n = A->n; Ap = A->p; Ai = A->i; Ax = A->x;
			for (j = 0; j < n; j++)
			{
				for (p = Ap[j]; p < Ap[j + 1]; p++)
				{
					y[j] += Ax[p] * x[Ai[p]];
					_ASSERTE(Ai[p] < n);
				}
			}
			return (1);
		}

		const char* KLUErrorDescription()
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
				m_strKLUError = fmt::format(CDFW2Messages::m_cszKLUUnknownError, pCommon.status);
			}
			return m_strKLUError.c_str();
		}
		static const char* KLUWrapperName()
		{
			return "KLUWrapper";
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
			pAx = std::make_unique<double[]>(m_nNonZeroCount * doubles_count<T>::count);			// числа матрицы
			pb = vector_aligned_double(m_nMatrixSize * doubles_count<T>::count);					// вектор правой части

			// вектор уточнения 
			// если уже был получен ранее - обновляем его размерность
			if (pRefine)
				pRefine = vector_aligned_double(3 * m_nMatrixSize * doubles_count<T>::count);
			pAi = std::make_unique<ptrdiff_t[]>(m_nMatrixSize + 1);									// строки матрицы
			pAp = std::make_unique<ptrdiff_t[]>(m_nNonZeroCount);									// столбцы матрицы
			pSymbolic.reset();
			pNumeric.reset();
		}
		inline ptrdiff_t MatrixSize() const { return m_nMatrixSize; }
		ptrdiff_t NonZeroCount() { return m_nNonZeroCount; }
		ptrdiff_t AnalyzingsCount() { return m_nAnalyzingsCount; }
		ptrdiff_t FactorizationsCount() { return m_nFactorizationsCount; }
		ptrdiff_t RefactorizationsCount() { return m_nRefactorizationsCount; }
		ptrdiff_t RefactorizationFailuresCount() { return m_nRefactorizationFailures; }
		inline double* Ax() { return pAx.get(); }
		inline const double* Ax() const { return pAx.get(); }
		inline double* B() { return pb.get(); }
		inline const double* const B() const { return pb.get(); }
		inline ptrdiff_t* Ai() { return pAi.get(); }
		inline ptrdiff_t* Ap() { return pAp.get(); }
		inline const ptrdiff_t* Ai() const { return pAi.get(); }
		inline const ptrdiff_t* Ap() const { return pAp.get(); }
		KLU_symbolic* Symbolic() { return pSymbolic->GetKLUObject(); }
		KLU_common* Common() { return &pCommon; }
		void Analyze()
		{
			pSymbolic = std::make_unique<KLUSymbolic>(KLUFunctions<T>::TKLU_analyze(m_nMatrixSize, pAi.get(), pAp.get(), &pCommon), pCommon);
			if (!Analyzed())
			{
				DumpMatrix(false);
				throw dfw2error(fmt::format("{}::KLU_analyze {}", KLUWrapperName(), KLUErrorDescription()));
			}
			m_nAnalyzingsCount++;
		}

		void Factor()
		{
			if (!Analyzed())
				Analyze();
			pNumeric = std::make_unique<KLUNumeric>(KLUFunctions<T>::TKLU_factor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), &pCommon), pCommon);
			if (!Factored())
				throw dfw2error(fmt::format("{}::KLU_factor {}", KLUWrapperName(), KLUErrorDescription()));
			m_nFactorizationsCount++;
		}

		bool TryRefactor()
		{
			if (!Factored())
				throw dfw2error(fmt::format("{}::KLU_refactor - no numeric to refactor", KLUWrapperName()));
			if (KLUFunctions<T>::TKLU_refactor(pAi.get(), pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
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
				throw dfw2error(fmt::format("{}::KLU_refactor {}", KLUWrapperName(), KLUErrorDescription()));
		}

		void Solve()
		{
			if (!Analyzed())
				Analyze();
			if (!Factored())
				Factor();
			if (!KLUFunctions<T>::TKLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon))
				throw dfw2error(fmt::format("{}::KLU_tsolve {}", KLUWrapperName(), KLUErrorDescription()));
		}

		// Решение СЛУ с итерационным уточнением
		// https://en.wikipedia.org/wiki/Iterative_refinement
		double SolveRefine(double Tolerance = 0.0)
		{
			double* pbEnd{ pb.get() + MatrixSize() };

			// если не был получен - создаем
			if (!pRefine)
				pRefine = vector_aligned_double(3 * m_nMatrixSize * doubles_count<T>::count);

			double* pR1{ pRefine.get() };			// правая часть СЛУ (b)
			double* pR2{ pR1 + MatrixSize() };		// исходное решение СЛУ (x)
			double* pR3{ pR2 + MatrixSize() };		// невязки СЛУ (r)

			double* pbb{ pb.get() };
			// сохраняем правую часть в pR1
			std::copy(pbb, pbEnd, pR1);
			// решаем Ax = b - решение x -> b
			Solve();
			// сохраняем исходное решение в pR2
			std::copy(pbb, pbEnd, pR2);

			double r{ 0.0 };
			ptrdiff_t nMaxIndex{ 0 };

			for (ptrdiff_t m = 0; m < 5; m++)
			{
				pR1 = pRefine.get();
				pR2 = pR1 + MatrixSize();
				pR3 = pR2 + MatrixSize();
				pbb = pb.get();

				// Рассчитываем pR3 = Ax
				Multiply(pbb, pR3);
				// рассчитываем r = b - Ax
				while (pbb < pbEnd)
				{
					*pbb = *pR1 - *pR3;
					pR1++;  pR3++;  pbb++;
				}
				r = (std::max)(FindMaxB(nMaxIndex), r);
				// если невязки меньше чем заданное значение
				if (r < Tolerance)
				{
					// отдаем значение с последней итерации
					std::copy(pR2, pR2 + MatrixSize(), pb.get());
					break;
				}
				// решаем Ac = r
				Solve();
				// рассчитываем x_{m+1} = x_{m} + c_{m}
				pbb = pb.get();
				while (pbb < pbEnd)
				{
					*pbb += *pR2;
					*pR2 = *pbb;
					pbb++; pR2++;
				}
			}

			return r;
		}

		void FactorSolve()
		{
			if (!Analyzed())
				Analyze();
			Factor();
			if (!KLUFunctions<T>::TKLU_tsolve(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), m_nMatrixSize, 1, pb.get(), &pCommon))
				throw dfw2error(fmt::format("{}::KLU_tsolve {}", KLUWrapperName(), KLUErrorDescription()));
		}

		double Condest()
		{
			if (!KLUFunctions<T>::TKLU_condest(pAp.get(), pAx.get(), pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
				throw dfw2error(fmt::format("{}::KLU_condest {}", KLUWrapperName(), KLUErrorDescription()));
			return pCommon.condest;
		}

		double Rcond()
		{
			if (!KLUFunctions<T>::TKLU_rcond(pSymbolic->GetKLUObject(), pNumeric->GetKLUObject(), &pCommon))
				throw dfw2error(fmt::format("{}::KLU_rcond {}", KLUWrapperName(), KLUErrorDescription()));
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
			double bmax{ 0.0 };
			nMaxIndex = -1;
			double* pbb{ pb.get() };
			if (m_nMatrixSize > 0)
			{
				nMaxIndex = 0;
				bmax = std::abs(*pbb);
				pbb++;
				for (ptrdiff_t x = 1; x < m_nMatrixSize; x++, pbb++)
				{
					double absb = std::abs(*pbb);
					if (bmax < absb)
					{
						nMaxIndex = x;
						bmax = absb;
					}
				}
			}
			return bmax;
		}

		csi Multiply(const double* x, double* b)
		{
			// формируем матрицу в виде пригодном для умножения на вектор
			cs Aj;
			Aj.i = Ap();
			Aj.p = Ai();
			Aj.x = Ax();
			Aj.m = Aj.n = MatrixSize();
			Aj.nz = -1;
			std::fill(b, b + Aj.n, 0.0);
			return cs_gatxpy(&Aj, x, b);
		}

		KLUMinMaxDiagonals MinMaxDiagonals() const
		{
			KLUMinMaxDiagonals minmax{ {0.0, -1},{0.0, -1} };
			ptrdiff_t nRow{ 0 };
			const ptrdiff_t* pAi{ Ap() };
			const double* pAx{ Ax() };
			for (const ptrdiff_t* pAp = Ai(); pAp < Ai() + MatrixSize(); pAp++, nRow++)
			{
				const ptrdiff_t* pAiend = pAi + *(pAp + 1) - *pAp;
				while (pAi < pAiend)
				{
					if (nRow == *pAi)
					{
						const double val{ std::abs(*pAx) };
						if (minmax.Min.second < 0 || minmax.Min.first > val)
						{
							minmax.Min.second = nRow;
							minmax.Min.first = val;
						}
						if (minmax.Max.second < 0 || minmax.Max.first < val)
						{
							minmax.Max.second = nRow;
							minmax.Max.first = val;
						}
					}
					pAx++; pAi++;
				}
			}

			return minmax;
		}

		// todo - complex version
		void DumpMatrix(bool bAnalyzeLinearDependenies)
		{
			std::ofstream mts(stringutils::utf8_decode("c:\\tmp\\dwfsingularmatrix.mtx"));
			if (mts.is_open())
			{
				ptrdiff_t* pAi = Ap();
				double* pAx = Ax();
				ptrdiff_t nRow = 0;
				std::set<ptrdiff_t> BadNumbers, FullZeros;
				std::vector<bool> NonZeros;
				std::vector<bool> Diagonals;
				std::vector<double> Expanded;

				NonZeros.resize(MatrixSize(), false);
				Diagonals.resize(MatrixSize(), false);
				Expanded.resize(MatrixSize(), 0.0);

				for (ptrdiff_t* pAp = Ai(); pAp < Ai() + MatrixSize(); pAp++, nRow++)
				{
					ptrdiff_t* pAiend = pAi + *(pAp + 1) - *pAp;
					bool bAllZeros = true;
					while (pAi < pAiend)
					{
						mts << std::setw(10) << nRow << std::setw(10) << *pAi << std::setw(30) << *pAx << std::endl;

						if (isnan(*pAx) || isinf(*pAx))
							BadNumbers.insert(nRow);

						if (std::abs(*pAx) > 1E-7)
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
					mts << "Bad Number in Row:\t" << it << std::endl;

				for (const auto& it : FullZeros)
					mts << "Full Zero Row :\t" << it << std::endl;

				for (auto&& it = NonZeros.begin(); it != NonZeros.end(); it++)
					if (!*it)
						mts << "Full Zero Column:\t" << static_cast<ptrdiff_t>(it - NonZeros.begin()) << std::endl;

				for (auto&& it = Diagonals.begin(); it != Diagonals.end(); it++)
					if (!*it)
						mts << "Zero Diagonal:\t" << static_cast<ptrdiff_t>(it - Diagonals.begin()) << std::endl;

				if (!bAnalyzeLinearDependenies)
					return;

				// пытаемся определить линейно зависимые строки с помощью неравенства Коши-Шварца
				// (v1 dot v2)^2 <= norm2(v1) * norm2(v2)

				pAi = Ap(); pAx = Ax(); nRow = 0;
				for (ptrdiff_t* pAp = Ai(); pAp < Ai() + MatrixSize(); pAp++, nRow++)
				{
					std::fill(Expanded.begin(), Expanded.end(), 0.0);

					ptrdiff_t* pAiend = pAi + *(pAp + 1) - *pAp;
					bool bAllZeros = true;
					double normi = 0.0;

					using PAIRSET = std::set < std::pair<ptrdiff_t, double> >;

					PAIRSET RowI;

					while (pAi < pAiend)
					{
						Expanded[*pAi] = *pAx;
						normi += *pAx * *pAx;

						RowI.insert(std::make_pair(*pAi, *pAx));

						pAx++; pAi++;
					}

					ptrdiff_t nRows = 0;
					ptrdiff_t* pAis = Ap();
					double* pAxs = Ax();
					for (ptrdiff_t* pAps = Ai(); pAps < Ai() + MatrixSize(); pAps++, nRows++)
					{
						ptrdiff_t* pAiends = pAis + *(pAps + 1) - *pAps;
						double normj = 0.0;
						double inner = 0.0;

						PAIRSET RowJ;

						while (pAis < pAiends)
						{
							normj += *pAxs * *pAxs;
							inner += Expanded[*pAis] * *pAxs;

							RowJ.insert(std::make_pair(*pAis, *pAxs));

							pAis++; pAxs++;
						}
						if (nRow != nRows)
						{
							double Ratio = inner * inner / normj / normi;
							if (std::abs(Ratio - 1.0) < 1E-5)
							{
								mts << "Linear dependent rows " << std::setw(10) << nRow << " " << std::setw(10) << nRows << " with " << Ratio << std::endl;
								for (auto& it : RowI)
									mts << std::setw(10) << nRow << std::setw(10) << it.first << std::setw(30) << it.second << std::endl;
								for (auto& it : RowJ)
									mts << std::setw(10) << nRows << std::setw(10) << it.first << std::setw(30) << it.second << std::endl;
							}
						}
					}
				}
			}
		}
	};
}


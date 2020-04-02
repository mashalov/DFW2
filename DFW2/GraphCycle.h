#pragma once
#include "memory"
namespace DFW2
{
	template<class IdNodeType, class IdBranchType = int>
	class GraphCycle
	{
	public:
		class GraphEdgeBase;
		class GraphNodeBase
		{
		public:
			GraphNodeBase *m_BackLinkNode = nullptr;
			GraphEdgeBase *m_BackLinkEdge = nullptr;
			GraphEdgeBase **m_ppEdgesBegin = nullptr;
			GraphEdgeBase **m_ppEdgesEnd = nullptr;
			IdNodeType m_Id;
			ptrdiff_t m_nIndex = 0;
			GraphNodeBase* SetId(IdNodeType Id)
			{
				m_Id = Id;
				return this;
			}

			inline ptrdiff_t Rank() const
			{
				return m_ppEdgesEnd - m_ppEdgesBegin;
			}
		};

		class GraphEdgeBase
		{
		public:
			GraphNodeBase *m_pBegin = nullptr;
			GraphNodeBase *m_pEnd = nullptr;
			IdNodeType m_IdBegin, m_IdEnd;
			IdBranchType m_IdBranch;
			ptrdiff_t m_nIndex = 0;
			bool bPassed = false;
			GraphEdgeBase* SetIds(IdNodeType IdBegin, IdNodeType IdEnd, IdBranchType IdBranch)
			{
				m_IdBegin  = IdBegin;
				m_IdEnd    = IdEnd;
				m_IdBranch = IdBranch;
				return this;
			}
		};

		class GraphCycleEdge
		{
		public:
			GraphEdgeBase *m_pEdge = nullptr;
			bool m_bDirect = true;
			GraphCycleEdge(GraphEdgeBase *pEdge, bool bDirect) : m_pEdge(pEdge), m_bDirect(bDirect) {}
		};

		using CycleType = list<GraphCycleEdge>;
		using CyclesType = list<CycleType>;

	protected:

		struct NodeCompare 
		{
			bool operator() (const GraphNodeBase* lhs, const GraphNodeBase* rhs) const 
			{
				return lhs->m_Id < rhs->m_Id;
			}
		};

		using NodesType = set<GraphNodeBase*, NodeCompare>;
		using EdgesType = vector<GraphEdgeBase*>;

		unique_ptr<GraphEdgeBase*[]> m_EdgesPtrs;

		NodesType m_Nodes;
		EdgesType m_Edges;

		// собирает заданный граф в струтуру, оптимальную для обхода
		void BuildGraph()
		{
			GraphNodeBase psrc;	// временный узел для поиска в сете
			ptrdiff_t nCount(0);
			for (auto&& edge : m_Edges)
			{
				// для каждого ребра находим узлы начала и конца по идентификаторам в сете
				auto& pNodeB = m_Nodes.find(psrc.SetId(edge->m_IdBegin));
				auto& pNodeE = m_Nodes.find(psrc.SetId(edge->m_IdEnd));
				if(pNodeB == m_Nodes.end() || pNodeE == m_Nodes.end())
					throw dfw2error(_T("GraphCycle::BuildGraph - one of the edge's nodes not found"));
				// задаем указатели на узлы начала и конца в ребре
				edge->m_pBegin = *pNodeB;
				edge->m_pEnd = *pNodeE;
				edge->m_nIndex = nCount++;
				// увеличиваем количество ребер в найденных узлах (изначально указатель m_ppEdgesEnd == nullptr)
				// ребра к узлам добавляются дважды: прямые и обратные, так как граф ненаправленный
				(*pNodeB)->m_ppEdgesEnd++;
				(*pNodeE)->m_ppEdgesEnd++;
			}
			// выделяем вектор указателей на указатели на ребра, общий для всех узлов
			m_EdgesPtrs = make_unique<GraphEdgeBase*[]>(m_Edges.size() * 2);
			GraphEdgeBase **ppEdges = m_EdgesPtrs.get();
			

			nCount = 0;
			for (auto&& node : m_Nodes)
			{
				// для каждого узла устанавливаем конечный указатель на ребра в векторе ребер
				// с учетом количества ребер узла
				node->m_ppEdgesEnd = ppEdges + (node->m_ppEdgesEnd - node->m_ppEdgesBegin);
				// начальный указатель устанавливаем на текущее начало вектора
				node->m_ppEdgesBegin = ppEdges;
				// текущее начало вектора смещаем на конечный указатель на ребра
				ppEdges = node->m_ppEdgesEnd;
				// конечный указатель приравниваем к начальному,
				// мы сдвинем его добавляя указатели на ребра
				node->m_ppEdgesEnd = node->m_ppEdgesBegin;
				node->m_nIndex = nCount++;
			}

			for (auto&& edge : m_Edges)
			{
				// добавляем указатель на ребро в конечные указатели узлы начала и конца
				*edge->m_pBegin->m_ppEdgesEnd = edge;
				*edge->m_pEnd->m_ppEdgesEnd = edge;
				// сдвигаем конечный указатели
				edge->m_pBegin->m_ppEdgesEnd++;
				edge->m_pEnd->m_ppEdgesEnd++;
			}
		}

		

	public:

		[[nodiscard]] const GraphNodeBase* GetMaxRankNode() const
		{
			auto it = std::max_element(m_Nodes.begin(), m_Nodes.end(), [](const auto& lhs, const auto& rhs) 
					{
							return lhs->Rank() < rhs->Rank();
				    });
			return it != m_Nodes.end() ? *it : nullptr;
		}

		void AddNode(GraphNodeBase *pNode)
		{
			if (!m_Nodes.insert(pNode).second)
				throw dfw2error(_T("GraphCycle::AddNode - Duplicated node"));
		}

		void AddEdge(GraphEdgeBase *pEdge)
		{
			m_Edges.push_back(pEdge);
		}

		// генерирует набор фундаментальных циклов графа
		// в виде списков ребер с направлениями
		void GenerateCycles(CyclesType& Cycles)
		{
			BuildGraph();
			Cycles.clear();
			stack<GraphNodeBase*> stack;
			stack.push(*m_Nodes.begin());
			while (!stack.empty())
			{
				// обходим граф в режиме DFS
				GraphNodeBase *pCurrent = stack.top();
				stack.pop();
				// для текущего узла обходим ребра
				for (GraphEdgeBase **ppEdge = pCurrent->m_ppEdgesBegin; ppEdge < pCurrent->m_ppEdgesEnd; ppEdge++)
				{
					GraphEdgeBase* pEdge = *ppEdge;
					// если ребро уже прошли в любом направлении - ребро не учитываем
					if (pEdge->bPassed)
						continue;
					// непройденное ребро помечаем как пройденное
					pEdge->bPassed = true;
					// определяем узел на обратном конце ребра относительно текущего
					GraphNodeBase *pOppNode = (*ppEdge)->m_pBegin == pCurrent ? (*ppEdge)->m_pEnd : (*ppEdge)->m_pBegin;
					// если у узла на обратном конце ребра нет указателя на родительский узел
					// он пока не входит в дерево, если есть - этот узел входит в цикл
					if (pOppNode->m_BackLinkNode)
					{
						CycleType CurrentCycle{ GraphCycleEdge(pEdge, true) };
						// отслеживаем узлы цикла путем добавления/поиска в сет
						set<GraphNodeBase*> BackTrack;
						// начинаем обходить цикл в обратном порядке от узлов
						// начала и конца текущего ребра (оно образует цикл, все остальные ребра принадлежат дереву)
						GraphNodeBase *pBackNodeBegin = pEdge->m_pBegin;
						GraphNodeBase *pBackNodeEnd = pEdge->m_pEnd;
						// для того, чтобы определить в каком узле начинается цикл
						// используем поиск LCA - Lowest Common Ancestor
						// ближайший общий предок двух узлов
						GraphNodeBase *pLCA = nullptr;

						// идем по дереву обратно до тех пор, пока не придем к узлу
						// начала или не найдем LCA. Идем с двух концов ребра цикла

						while (pBackNodeBegin || pBackNodeEnd)
						{
							if (pBackNodeBegin)
							{
								// если на пути из узла начала ребра цикла не дошли до начала дерева
								// пробуем вставить в сет текущий узел
								if (!BackTrack.insert(pBackNodeBegin).second)
								{
									// если этот узел уже в сете, значит мы прошли его с другой
									// стороны ребра и он является LCA
									pLCA = pBackNodeBegin;
									break;
								}
								pBackNodeBegin = pBackNodeBegin->m_BackLinkNode;
							}
							if (pBackNodeEnd)
							{
								// такой же шаг делаем из узла конца ребра цикла
								if (!BackTrack.insert(pBackNodeEnd).second)
								{
									pLCA = pBackNodeEnd;
									break;
								}
								pBackNodeEnd = pBackNodeEnd->m_BackLinkNode;
							}
						}

						// определяем ветки цикла от узлов начала и конца ребра цикла до LCA
						pBackNodeBegin = pEdge->m_pBegin;
						while (pBackNodeBegin != pLCA)
						{
							// идем либо до начала дерева (если LCA не найден это и есть LCA), 
							// либо до LCA
							// при возврате к LCA от узла начала ребра цикла
							// направление ребер считаем положительным, если их узел конца совпадает с текущим
							bool bDirection = pBackNodeBegin == pBackNodeBegin->m_BackLinkEdge->m_pEnd;
							CurrentCycle.push_back(GraphCycleEdge(pBackNodeBegin->m_BackLinkEdge, bDirection));
							pBackNodeBegin = pBackNodeBegin->m_BackLinkNode;
						}

						pBackNodeBegin = pEdge->m_pEnd;
						while (pBackNodeBegin != pLCA)
						{
							// при возврате к LCA от узла конца ребра цикла
							// направление ребер считаем положительным если узел их узел начала совпадает с текущим
							bool bDirection = pBackNodeBegin == pBackNodeBegin->m_BackLinkEdge->m_pBegin;
							CurrentCycle.push_back(GraphCycleEdge(pBackNodeBegin->m_BackLinkEdge, bDirection));
							pBackNodeBegin = pBackNodeBegin->m_BackLinkNode;
						}

						Cycles.push_back(CurrentCycle);
						/*
						_tcprintf(_T("\nCycle"));
						for (auto&& it : Cycle)
						{
							_tcprintf(_T("\n%s %d-%d"), it.m_bDirect ? _T("+") : _T("-"), it.m_pEdge->m_IdBegin, it.m_pEdge->m_IdEnd);
						}
						*/
					}
					else
					{
						// узел на обратном конце связи пока не входит в дерево
						// добавляем его в дерево путем указания родительского узла (текущего)
						// и добавляем указатель на ребро, по которому дошли до этого узла
						pOppNode->m_BackLinkNode = pCurrent;
						pOppNode->m_BackLinkEdge = pEdge;
						stack.push(pOppNode);
					}
				}
			}

			if (m_Nodes.size() - m_Edges.size() + Cycles.size() != 1)
				throw dfw2error(_T("GraphCycle::GenerateCycles - Cycles count mismatch"));

		}

		[[nodiscard]] const NodesType& Nodes() const
		{
			return m_Nodes;
		}

		[[nodiscard]] const EdgesType& Edges() const
		{
			return m_Edges;
		}

		void Test()
		{
			using GraphType = GraphCycle<int, int>;
			using NodeType = GraphType::GraphNodeBase;
			using EdgeType = GraphType::GraphEdgeBase;

			int nodes[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
			int edges[] = { 1, 2,
						   2, 3,
						   3, 4,
						   3, 5,
						   4, 6,
						   5, 6,
						   6, 7,
						   7, 8,
						   8, 9,
						   8, 10,
						   9, 11,
						   10, 11,
						   11, 12,
						   12, 13,
						   1, 13
			};

			/*
			int nodes[] = { 1, 2, 3, 4};
			int edges[] = { 1, 2,
							2, 3,
							3, 4,
							1, 4,
							1, 3,
							2, 4
			};
			*/

			/*
			int nodes[] = { 1, 2, 3, 4, 5, 6 };
			int edges[] = { 2, 1,
							2, 3,
							4, 3,
							4, 5,
							6, 5,
							6, 1};
							*/
			unique_ptr<NodeType[]> pGraphNodes = make_unique<NodeType[]>(_countof(nodes));
			unique_ptr<EdgeType[]> pGraphEdges = make_unique<EdgeType[]>(_countof(edges) / 2);
			NodeType *pNode = pGraphNodes.get();
			GraphType gc;

			for (int *p = nodes; p < _countof(nodes) + nodes; p++, pNode++)
				gc.AddNode(pNode->SetId(*p));

			EdgeType *pEdge = pGraphEdges.get();
			for (int *p = edges; p < _countof(edges) + edges; p += 2, pEdge++)
				gc.AddEdge(pEdge->SetIds(*p, *(p + 1), p - edges));

			CyclesType Cycles;

			gc.GenerateCycles(Cycles);
		}
	};
}
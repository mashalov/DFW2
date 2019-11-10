#pragma once
#include "memory"
namespace DFW2
{
	template<class IdType>
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
			IdType m_Id;
			GraphNodeBase* SetId(IdType Id)
			{
				m_Id = Id;
				return this;
			}
		};

		class GraphEdgeBase
		{
		public:
			GraphNodeBase *m_pBegin = nullptr;
			GraphNodeBase *m_pEnd = nullptr;
			IdType m_IdBegin;
			IdType m_IdEnd;
			bool bPassed = false;
			GraphEdgeBase* SetIds(IdType IdBegin, IdType IdEnd)
			{
				m_IdBegin = IdBegin;
				m_IdEnd   = IdEnd;
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

	protected:

		struct NodeCompare 
		{
			bool operator() (const GraphNodeBase* lhs, const GraphNodeBase* rhs) const 
			{
				return lhs->m_Id < rhs->m_Id;
			}
		};

		set<GraphNodeBase*, NodeCompare> m_Nodes;
		vector<GraphEdgeBase*> m_Edges;
		unique_ptr<GraphEdgeBase*[]> m_EdgesPtrs;

		void BuildGraph()
		{
			GraphNodeBase psrc;
			for (auto&& edge : m_Edges)
			{
				auto& pNodeB = m_Nodes.find(psrc.SetId(edge->m_IdBegin));
				auto& pNodeE = m_Nodes.find(psrc.SetId(edge->m_IdEnd));
				if(pNodeB == m_Nodes.end() || pNodeE == m_Nodes.end())
					throw dfw2error(_T("GraphCycle::BuildGraph - one of the edge's nodes not found"));
				edge->m_pBegin = *pNodeB;
				edge->m_pEnd = *pNodeE;
				(*pNodeB)->m_ppEdgesEnd++;
				(*pNodeE)->m_ppEdgesEnd++;
			}
			m_EdgesPtrs = make_unique<GraphEdgeBase*[]>(m_Edges.size() * 2);
			GraphEdgeBase **ppEdges = m_EdgesPtrs.get();
			for (auto&& node : m_Nodes)
			{
				node->m_ppEdgesEnd = ppEdges + (node->m_ppEdgesEnd - node->m_ppEdgesBegin);
				node->m_ppEdgesBegin = ppEdges;
				ppEdges = node->m_ppEdgesEnd;
				node->m_ppEdgesEnd = node->m_ppEdgesBegin;
			}
			for (auto&& edge : m_Edges)
			{
				*edge->m_pBegin->m_ppEdgesEnd = edge;
				*edge->m_pEnd->m_ppEdgesEnd = edge;
				edge->m_pBegin->m_ppEdgesEnd++;
				edge->m_pEnd->m_ppEdgesEnd++;
			}
		}

	public:

		void AddNode(GraphNodeBase *pNode)
		{
			if (!m_Nodes.insert(pNode).second)
				throw dfw2error(_T("GraphCycle::AddNode - Duplicated node"));
		}

		void AddEdge(GraphEdgeBase *pEdge)
		{
			m_Edges.push_back(pEdge);
		}


		void GenerateCycles()
		{
			BuildGraph();
			stack<GraphNodeBase*> stack;
			stack.push(*m_Nodes.begin());
			while (!stack.empty())
			{
				GraphNodeBase *pCurrent = stack.top();
				stack.pop();
				for (GraphEdgeBase **ppEdge = pCurrent->m_ppEdgesBegin; ppEdge < pCurrent->m_ppEdgesEnd; ppEdge++)
				{
					GraphEdgeBase* pEdge = *ppEdge;
					if (pEdge->bPassed)
						continue;
					pEdge->bPassed = true;
					GraphNodeBase *pOppNode = (*ppEdge)->m_pBegin == pCurrent ? (*ppEdge)->m_pEnd : (*ppEdge)->m_pBegin;
					if (pOppNode->m_BackLinkNode)
					{
						
						list<GraphCycleEdge> Cycle{ GraphCycleEdge(pEdge, true) };
						set<GraphNodeBase*> BackTrack;
						GraphNodeBase *pBackNodeBegin = pEdge->m_pBegin;
						GraphNodeBase *pBackNodeEnd = pEdge->m_pEnd;
						GraphNodeBase *pLCA = nullptr;

						while (pBackNodeBegin || pBackNodeEnd)
						{
							if (pBackNodeBegin)
							{
								if (!BackTrack.insert(pBackNodeBegin).second)
								{
									pLCA = pBackNodeBegin;
									break;
								}
								pBackNodeBegin = pBackNodeBegin->m_BackLinkNode;
							}
							if (pBackNodeEnd)
							{
								if (!BackTrack.insert(pBackNodeEnd).second)
								{
									pLCA = pBackNodeEnd;
									break;
								}
								pBackNodeEnd = pBackNodeEnd->m_BackLinkNode;
							}
						}

						pBackNodeBegin = pEdge->m_pBegin;
						while (pBackNodeBegin && pBackNodeBegin != pLCA)
						{
							bool bDirection = pBackNodeBegin->m_BackLinkEdge->m_pBegin != pBackNodeBegin;
							Cycle.push_back(GraphCycleEdge(pBackNodeBegin->m_BackLinkEdge, bDirection));
							pBackNodeBegin = pBackNodeBegin->m_BackLinkNode;
						}

						pBackNodeBegin = pEdge->m_pEnd;
						while (pBackNodeBegin && pBackNodeBegin != pLCA)
						{
							bool bDirection = pBackNodeBegin->m_BackLinkEdge->m_pBegin == pBackNodeBegin;
							Cycle.push_back(GraphCycleEdge(pBackNodeBegin->m_BackLinkEdge, bDirection));
							pBackNodeBegin = pBackNodeBegin->m_BackLinkNode;
						}

						_tcprintf(_T("\nCycle"));
						for (auto&& it : Cycle)
						{
							_tcprintf(_T("\n%s %d-%d"), it.m_bDirect ? _T("+") : _T("-"), it.m_pEdge->m_IdBegin, it.m_pEdge->m_IdEnd);
						}
						
					}
					else
					{
						pOppNode->m_BackLinkNode = pCurrent;
						pOppNode->m_BackLinkEdge = pEdge;
						stack.push(pOppNode);
					}
				}
			}
		}
	};
}
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
			GraphNodeBase *m_BackLink = nullptr;
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
					if (pOppNode->m_BackLink)
					{
						_tcprintf(_T("\nCycle on %d %d"), pEdge->m_IdBegin, pEdge->m_IdEnd);
						GraphNodeBase *pBack = pEdge->m_pBegin->m_BackLink;
						while (pBack)
						{
							_tcprintf(_T("\nback to %d"), pBack->m_Id);
							pBack = pBack->m_BackLink;
						}
						pBack = pEdge->m_pEnd->m_BackLink;
						while (pBack)
						{
							_tcprintf(_T("\nback to %d"), pBack->m_Id);
							pBack = pBack->m_BackLink;
						}
					}
					else
					{
						pOppNode->m_BackLink = pCurrent;
						stack.push(pOppNode);
					}
				}
			}
		}
	};
}
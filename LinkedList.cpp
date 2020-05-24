#include "LinkedList.h"


CLinkedList::CLinkedList(void)
:m_Count(0)
, m_pFirstNode(NULL)
, m_pLastNode(NULL)
{
}
CLinkedList::~CLinkedList(void)
{
	if(m_pFirstNode)
		delete m_pFirstNode;

	if(m_pLastNode)
		delete m_pLastNode;
}
/** Add */
bool CLinkedList::Add(void * pNewElement)
{
	if(!pNewElement)
		return false;

	if(!m_pFirstNode)
	{
		m_pFirstNode = new CNode(pNewElement);
		m_pLastNode = m_pFirstNode;
	}
	else
		m_pLastNode = new CNode(pNewElement, m_pLastNode);

	++m_Count;

	return true;
}
/** Delete */
bool CLinkedList::DeleteFirst()
{
	if(!m_pFirstNode)
		return false;

	CNode * pChildNode = m_pFirstNode->Child();
	delete m_pFirstNode;

	m_pFirstNode = pChildNode;

	if(!m_pFirstNode)
		m_pLastNode = m_pFirstNode;

	--m_Count;

	return true;
}
bool CLinkedList::DeleteLast()
{
	if(!m_pLastNode)
		return false;

	CNode * pParentNode = m_pLastNode->Parent();
	delete m_pLastNode;

	m_pLastNode = pParentNode;

	if(!m_pLastNode)
		m_pFirstNode = m_pLastNode;

	--m_Count;

	return true;
}
bool CLinkedList::Delete(void * pElementToDelete)
{
	if(m_pFirstNode && (m_pFirstNode->Data() == pElementToDelete))
	{
		DeleteFirst();
		return true;
	}

	if(m_pLastNode && (m_pLastNode->Data() == pElementToDelete))
	{
		DeleteLast();
		return true;
	}

	/** Search for Node corresponding to pElementToDelete */
	CNode * pNode = m_pFirstNode->Child();
	while(pNode && (pNode->Data() != pElementToDelete))
		pNode = pNode->Child();

	if(pNode)
	{
		--m_Count;
		delete pNode;
		return true;
	}

	return false;
}
/** Retrieve */
void * CLinkedList::operator [] (unsigned int Index) const
{
	void * pResult = NULL;
	unsigned int Id = 0;

	if(Index < m_Count)
	{
		if(Index == 0)
			pResult = m_pFirstNode->Data();
		else if(Index == (m_Count - 1))
			pResult = m_pLastNode->Data();
		else
		{
			CNode * pNode = NULL;
			if(Index > (m_Count / 2))
			{
				pNode = m_pFirstNode->Child();
				while(pNode && (Id++ != Index))
					pNode = pNode->Child();
			}
			else
			{
				Id = m_Count - 1;
				pNode = m_pLastNode->Parent();
				while(pNode && (Id-- != Index))
					pNode = pNode->Parent();
			}
			pResult = pNode->Data();
		}
	}
	return pResult;
}

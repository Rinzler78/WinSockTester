#pragma once
#include "Stdio.h"

class CNode
{
public:
	CNode(void * pData, CNode * pParentNode = NULL, CNode * pChildNode = NULL)
	{
		m_pData = pData; m_pParentNode = pParentNode; m_pChildNode = pChildNode;
		if(m_pParentNode)
			m_pParentNode->SetChildNode(this);
		if(m_pChildNode)
			m_pChildNode->SetParentNode(this);
	}
	~CNode()
	{	if(m_pParentNode)m_pParentNode->SetChildNode(m_pChildNode);
		if(m_pChildNode)m_pChildNode->SetParentNode(m_pParentNode);
	}
	inline void * Data() const {return m_pData;}
	inline void SetParentNode(CNode * pNewParent){m_pParentNode = pNewParent;}
	inline void SetChildNode(CNode * pNewchild){m_pChildNode = pNewchild;}

	inline CNode * Parent() const {return m_pParentNode;}
	inline CNode * Child() const {return m_pChildNode;}

private:
	void * m_pData;
	CNode * m_pParentNode;
	CNode * m_pChildNode;
};

class CLinkedList
{
public:
	CLinkedList();
	~CLinkedList();

	/** Add */
	bool Add(void *);

	/** Delete */
	bool Delete(void *);
	bool DeleteFirst();
	bool DeleteLast();

	/** Get */
	void * operator [] (unsigned int Index) const;
	inline void * GetFirst() const {return m_pFirstNode->Data();}
	inline void * GetLast() const {return m_pLastNode->Data();}

	inline unsigned int Count() const {return m_Count;}

private:
	unsigned int m_Count;
	CNode * m_pFirstNode;
	CNode * m_pLastNode;
};




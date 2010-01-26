// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// node.h - structures to hold node information
//

#ifndef __NODE_H__
#define __NODE_H__

/**
@file node.h
DNS Cache (internal part of the DNS cache implementation)
@internalComponent	Domain Name Resolver
*/
#include "record.h"

class CDndEngine;
class CDndNode;

// Implement a node list ONLY for CDndNode, not for anyone else
class TDndNodeList : public TDblQue<CDndNode>
	{
	friend class CDndNode;
	// only to initialize the offset in the TDblQue head
	TDndNodeList();

	CDndNode *Find(const TNodeLabel &aLabel);	//< Find the node by label
	void AddNode(CDndNode &aNode);				//< Add node to the list
	void DeleteNode(CDndNode *const aNode);		//< Delete node from the list
	void Print(CDndEngine &aControl);			//< Prints out the contents of the node-list (DEBUG only)
	};


class CDndNode : public CBase
	{
	friend class TDndNodeList;
	friend class CDndRecord;
	// constructors and destructor
protected:
	CDndNode(CDndNode *const aParent, const TInt aLevel, const TNodeLabel &aLabel);
	~CDndNode();
public:
	// Locate (and create path) to a node identified by a domain name, and find (or create) a record matching the type and class
	CDndRecord *FindL(
		const TDesC8 &aName,
		const TBool aFlag,
		const EDnsQType aType,
		const EDnsQClass aClass,
		TDndRecordLRU &aLRU,
		const TTime &aReqTime);

	// Prints out the contents of this node and all its descendants
	void Print(CDndEngine &aControl);

protected:
	// Return TRUE, if node has no children and no records
	inline TBool IsEmpty() const { return iRecordList.IsEmpty() && iChildren.IsEmpty(); }
private:	
	static const TInt iOffset;		//< For the linked list

	// Unconditionally delete a record from the node
	void DeleteRecord(CDndRecord *const aRecord);
	// Delete node path, if it has no records and no child nodes
	void Cleanup();
	// Delete a child from the node (which, must be a child of this node!)
	void DeleteNode(CDndNode *const aNode);

	CDndNode *const iParent;	//< Back link to the parent node (except at root = NULL)
	const TInt iLevel;			//< Depth of the node in the tree
	const TNodeLabel iLabel;	//< Label of the node.
	TDblQueLink iDlink;			//< Siblings chain (nodes on same level)		
	TDndRecordList iRecordList;	//< List of Records with this node's name.
	TDndNodeList iChildren;		//< Children are implemented as a linked list of nodes
	};

#endif

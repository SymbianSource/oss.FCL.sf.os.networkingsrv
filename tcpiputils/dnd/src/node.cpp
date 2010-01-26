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
// node.cpp - structures to hold node information
// CDndNode object acts as a node in the tree that represents the cache for the DNS responses.
// TDndNodeList is a linked list of CDndNode objects.
//

#include "node.h"
#include "engine.h"
#include <networking/dnd_err.h>
#include "inet6log.h"

const TInt CDndNode::iOffset = _FOFF(CDndNode, iDlink);

// CDndNode::CDndNode
// ******************
CDndNode::CDndNode(CDndNode *const aParent, const TInt aLevel, const TNodeLabel &aLabel)
: iParent(aParent), iLevel(aLevel), iLabel(aLabel)
	{
	}

// CDndNode::~CDndNode
// *******************
CDndNode::~CDndNode()
	{
	//
	// Node can NEVER be deleted, if it has children or records!
	//
	__ASSERT_ALWAYS(IsEmpty(), User::Panic(_L("DND"), 0));
	}

// CDndNode::Cleanup
// *****************
/**
// Starting from current node, delete nodes in the path
// if they have no records and no children.
//
// Maintain the "cache invariant": whenever control is outside
// the cache implementation, the cache must not contain any
// nodes that have no child nodes and no records (with exception
// of the root nodes at level 0, iParent == NULL)
*/
void CDndNode::Cleanup()
	{
	CDndNode *current = this;
	while (current->iParent && current->IsEmpty())
		{
		CDndNode *parent = current->iParent;
		parent->DeleteNode(current);
		current = parent;
		}
	}

// CDndNode::DeleteNode
// ********************
void CDndNode::DeleteNode(CDndNode *const aNode)
	{
	iChildren.DeleteNode(aNode);
	}


// CDndNode::DeleteRecord
// **********************
/**
// This method must not be called by anyone else except the
// CDndRecord::Delete().
//
// The call will detach the record instance from this node,
// deletes the record instance, and also activates the
// cleanup, which deletes a path of nodes, if they have
// no records and no children. 
//
// @param	aRecord
//		specifies the record to be deleted. This record must
//		exist and be owned by the node 
*/
void CDndNode::DeleteRecord(CDndRecord *const aRecord)
	{
	iRecordList.Delete(aRecord);
	Cleanup();
	}



// CDndNode::FindL
// ***************
/**
// Split the dotted name into components labels and locate the
// node at the end of the path defined by those labels. The
// search starts from the current node. If necessary, the path
// is created (aFlag != 0).
//
// After the end node of the path is located (or created), a
// matching record is located (or created).
//
// @param	aName	defines the domain name
// @param	aFlag
//		if value is FALSE, only search is performed and a NULL returned if node
//		is not found. If value is TRUE, the necessary path and final node is
//		created, if they don't exist yet.
// @param	aType	the type of the record
// @param	aClass	the class of the record
// @param	aLRU	the LRU list into which the record will belong
//					(only used if record is created by this call).
// @param	aReqTime	the current time (of the query)
// @returns	a pointer to the newly created record
//	@li	NULL, if node does not exist (can only happen, if aFlag == 0!
//	@li	non-NULL pointer to the record matching the aName, aClass and aType
// @leave KErrNoMemory
//		The function leaves for non-recoverable error (out of memory).
//		Can occur only if aFlag != 0.
// @leave KErrDndBadQuery
//		The query name is not valid as DNS domain name.
*/
CDndRecord *CDndNode::FindL(const TDesC8 &aName, const TBool aFlag,
							const EDnsQType aType, const EDnsQClass aClass,
							TDndRecordLRU &aLRU, const TTime &aReqTime)
	{
	CDndNode *current = this;
	TPtrC8 remaining(aName);

	while (remaining.Length() > 0)
		{
		// extract the next name segment
		TPtrC8 currentLabel;
		TInt index = remaining.LocateReverse('.');
		if (index == KErrNotFound)
			{
			currentLabel.Set(remaining);
			index = 0;	// Desired node is reached
			}
		else
			{
			if (index <= 0)
				User::Leave(KErrDndBadQuery);	// a name starting with "." is not valid
			currentLabel.Set(remaining.Mid(index + 1));
			}

		if (currentLabel.Length() == 0)
			User::Leave(KErrDndBadQuery);			// bad name, empty labels "foo..bar" not legal!
		if (currentLabel.Length() > KDnsMaxLabel)	// this should be 63! (DNS hard limit)
			User::Leave(KErrDndBadQuery);

		CDndNode *node = current->iChildren.Find(currentLabel);

		if (node == NULL)
			{
			if (!aFlag)
				return NULL;	// Do not insert!
			node = new CDndNode(current, current->iLevel + 1, currentLabel);
			if (node == NULL)
				{
				current->Cleanup();
				User::Leave(KErrNoMemory);
				}
			current->iChildren.AddNode(*node);
			}
		remaining.Set(remaining.Left(index));
		current = node;
		}

	CDndRecord *record = current->iRecordList.Find(aType, aClass, aReqTime);
	if (!aFlag || record != NULL)
		return record;

	//
	// The looked record does not exist yet, create it here
	//
	record = new CDndRecord(aLRU, *current, aType, aClass);
	if (record == NULL)
		{
		// This may invalidate 'this' pointer, so careful here!
		current->Cleanup();
		User::Leave(KErrNoMemory);
		}
	else
		current->iRecordList.Add(*record);
	return record;
	}

#ifdef _LOG
// CDndNode::Print
// ***************
void CDndNode::Print(CDndEngine &aControl)
	{

	// Just for Unicode, need to "widen" the label!
	TBuf<63> label;
	label.Copy(iLabel);

	aControl.ShowTextf(_L("Node: Level = %d, Label = %S"), iLevel, &label);
 
	iRecordList.Print(aControl);
	iChildren.Print(aControl);
	}
#endif

// TDndNodeList.:TDndNodeList
// **************************
TDndNodeList::TDndNodeList() : TDblQue<CDndNode>(CDndNode::iOffset) 
	{}


// TDndNodeList::Find
// ******************
CDndNode * TDndNodeList::Find(const TNodeLabel &aLabel)
	{
	CDndNode *node;
	TDblQueIter<CDndNode> iter(*this);
	while ((node = iter++) != NULL)
		if (DnsCompareNames(node->iLabel, aLabel))
			return node;

	return NULL;
	}


// TDndNodeList::AddNode
// *********************
void TDndNodeList::AddNode(CDndNode &aNode)
	{
	AddFirst(aNode);
	}

// TDndNodeList::DeleteNode
// ************************
void TDndNodeList::DeleteNode(CDndNode *const aNode)
	{
	aNode->iDlink.Deque();
	delete aNode;
	}

#ifdef _LOG
// TDndNodeList::Print
// *******************
void TDndNodeList::Print(CDndEngine &aControl)
	{
	aControl.ShowText(_L("Node list: "));
	if (IsEmpty())
		aControl.ShowText(_L(" - No nodes"));
	else
		{
		CDndNode *node;
		TDblQueIter<CDndNode> iter(*this);
		while((node = iter++) != NULL)
			node->Print(aControl);
		}
	}
#endif

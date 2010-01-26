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
// sbque.cpp - TCP sequence block queue
//

#include "sbque.h"

EXPORT_C SequenceBlock *SequenceBlockQueue::AddOrdered(TTcpSeqNum aLeft, TTcpSeqNum aRight)
	{
	SequenceBlock *current, *block = NULL;
	SequenceBlockQueueIter iter(*this);

	iter.SetToLast();
	while (current = iter--, current != NULL)
		{
		// Find correct position.
		if (aRight < current->iLeft)
			continue;

		// No overlap? Insert new block here.
		if (aLeft > current->iRight)
			{
			if (block = new SequenceBlock(aLeft, aRight), block == NULL)
				return NULL;
			block->iLink.Enque(&current->iLink);
			iCount++;
			iBytes += (aRight - aLeft);
			break;
			}

		// Already know about this block?
		if (aLeft >= current->iLeft && aRight <= current->iRight)
			{
			block = current;
			break;
			}

		// Our block partially overlaps one or more blocks.
		if (aLeft > current->iLeft)
			aLeft = current->iLeft;
		if (aRight < current->iRight)
			aRight = current->iRight;

		// Delete the previously known overlapping block.
		iBytes -= (current->iRight - current->iLeft);
		current->iLink.Deque();
		delete current;
		iCount--;
		}

	// We haven't found or created a sequence block? It goes first then.
	if (!block)
		{
		block = new SequenceBlock(aLeft, aRight);
		if (block != NULL)
			{
			AddFirst(*block);
			iCount++;
			iBytes += (aRight - aLeft);
			}
		}

	// Return the contiguous block, which the new block is belonged to.
	return block;
	}

EXPORT_C SequenceBlock *SequenceBlockQueue::AddUnordered(TTcpSeqNum aLeft, TTcpSeqNum aRight)
	{
	SequenceBlock *current, *block = NULL;
	SequenceBlockQueueIter iter(*this);

	while (current = iter++, current != NULL)
		{
		// Find overlapping blocks.
		if (aLeft > current->iRight || aRight < current->iLeft)
			continue;

		// Overlapping block exists. Extend our block accordingly.
		if (aLeft > current->iLeft)
			aLeft = current->iLeft;
		if (aRight < current->iRight)
			aRight = current->iRight;

		// Delete the previously known overlapping block.
		iBytes -= (current->iRight - current->iLeft);
		current->iLink.Deque();
		delete current;
		iCount--;
		}

	// Insert new block first in the queue.
	if (block = new SequenceBlock(aLeft, aRight), block != NULL)
		{
		AddFirst(*block);
		iOrdered = !iCount;
		iCount++;
		iBytes += (aRight - aLeft);
		}

	// Return the contiguous block, to which the new block belongs.
	return block;
	}


//
// Find a block that contains the given sequence number
//
EXPORT_C SequenceBlock *SequenceBlockQueue::Find(TTcpSeqNum aSeq)
	{
	SequenceBlock *current;
	SequenceBlockQueueIter iter(*this);

	while (current = iter++, current != NULL)
		{
		// Find correct position.
		if (aSeq >= current->iRight)
			continue;

		// Found?
		if (aSeq >= current->iLeft)
			return current;

		// We can stop here if we know the queue is ordered.
		if (iOrdered)
			break;
		}

	return NULL;
	}

//
// This only makes sense with ordered queues.
//
EXPORT_C TInt SequenceBlockQueue::FindGap(TTcpSeqNum& aLeft, TTcpSeqNum& aRight)
	{
	ASSERT(iOrdered);

	SequenceBlock *current;
	SequenceBlockQueueIter iter(*this);
	TInt skipped = 0;

    while (current = iter++, current != NULL)
        {
        TTcpSeqNum right = current->iRight;
        TTcpSeqNum left = current->iLeft;
        if (aLeft >= right)
            {
            skipped += (right - left);
            continue;
            }

        if (aLeft >= left)
            {
            skipped += (right - aLeft);
            aLeft = right;
            continue;
            }

        aRight = left;
        return skipped;
        }
    
	return -1;
	}


//
// Remove all contiguous blocks with sequence numbers
// less than aLeft.
//
EXPORT_C void SequenceBlockQueue::Prune(TTcpSeqNum aLeft)
	{
	SequenceBlock *current;
	SequenceBlockQueueIter iter(*this);

    while (current = iter++, current != NULL)
        {
        TTcpSeqNum right = current->iRight;
        TTcpSeqNum left = current->iLeft;
        
        // The whole block can go?
        if (aLeft >= right)
            {
            iBytes -= (right - left);
            current->iLink.Deque();
            delete current;
            --iCount;
            continue;
            }

        // Trim left edge of block.
        if (aLeft > left)
            {
            iBytes -= (aLeft - left);
            current->iLeft = aLeft;
            }

        // We can stop here if we know the queue is ordered.
        if (iOrdered)
            return;
        }

	if (iCount <= 1)
		iOrdered = ETrue;
	}


//
// Remove contiguous blocks from the end of the queue
// until at most aCount blocks remain.
//
EXPORT_C void SequenceBlockQueue::Limit(TInt aCount)
	{
	SequenceBlock *current;
	SequenceBlockQueueIter iter(*this);

	iter.SetToLast();
	while (iCount > aCount)
		{
		current = iter--;
		iBytes -= (current->iRight - current->iLeft);
		current->iLink.Deque();
		delete current;
		--iCount;
		}

	if (iCount <= 1)
		iOrdered = ETrue;
	}


//
// Clear the queue.
//
EXPORT_C void SequenceBlockQueue::Clear()
	{
	SequenceBlock *current;
	SequenceBlockQueueIter iter(*this);

	while (current = iter++, current != NULL)
		{
		current->iLink.Deque();
		delete current;
		}
	iCount = 0;
	iBytes = 0;
	iOrdered = ETrue;
	}

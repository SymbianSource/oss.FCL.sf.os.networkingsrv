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
// sbque.h - TCP sequence block queue
// Defines a queue for keeping tabs on contiguous blocks of sequence numbers.
//



/**
 @file sbque.h
 @publishedAll
 @released
*/

#ifndef __SBQUE_H__
#define __SBQUE_H__

#include "tcpseq.h"

/** Sequence Block.
@publishedAll
@released
*/
class SequenceBlock
	{
  	friend class SequenceBlockQueue;

private:

    SequenceBlock(TTcpSeqNum aLeft, TTcpSeqNum aRight) : iLeft(aLeft), iRight(aRight) {}
    TDblQueLink iLink;

public:

    TTcpSeqNum  iLeft;
    TTcpSeqNum  iRight;
	};

/** Sequence Block Queue.
@publishedAll
@released
*/
class SequenceBlockQueue : public TDblQue<SequenceBlock>
	{
 public:
    SequenceBlockQueue() : TDblQue<SequenceBlock>(_FOFF(SequenceBlock, iLink)) { iCount = 0; iOrdered = ETrue; }
    inline ~SequenceBlockQueue() { Clear(); }
    IMPORT_C SequenceBlock *AddOrdered(TTcpSeqNum aLeft, TTcpSeqNum aRight);
    IMPORT_C SequenceBlock *AddUnordered(TTcpSeqNum aLeft, TTcpSeqNum aRight);
    inline SequenceBlock *AddOrdered(const SequenceBlock *aBlock) { return AddOrdered(aBlock->iLeft, aBlock->iRight); }
    inline SequenceBlock *AddUnordered(const SequenceBlock *aBlock) { return AddUnordered(aBlock->iLeft, aBlock->iRight); }
    IMPORT_C SequenceBlock *Find(TTcpSeqNum aSeq);
    IMPORT_C TInt FindGap(TTcpSeqNum& aLeft, TTcpSeqNum& aRight);
    IMPORT_C void Prune(TTcpSeqNum aLeft);
    IMPORT_C void Limit(TInt aCount);
    IMPORT_C void Clear();
    inline TInt Count() const { return iCount; }
    inline TInt ByteCount() const { return iBytes; }

private:
    TInt iCount;
    TInt iBytes;
    TBool iOrdered;
	};

typedef TDblQueIter<SequenceBlock> SequenceBlockQueueIter;

#endif

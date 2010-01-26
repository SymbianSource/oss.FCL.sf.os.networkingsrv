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
// frag.h - IPv6/IPv4 fragment queue
//



/**
 @internalComponent
*/
#ifndef __FRAG_H__
#define __FRAG_H__

#include <es_mbuf.h>
#include "in_fmly.h"	// for Panic codes

class RMBufFrag : public RMBufChain
{
 public:
  //
  // The following methods must be defined in the user implementation:
  //

  // Return offset of this fragment.
  TUint32 Offset() { Panic(EInet6Panic_NotSupported); return 0; }

  // Return the amount of data in this fragment
  TUint32 FragmentLength() { Panic(EInet6Panic_NotSupported); return 0; }

  // Join another fragment to this one. The fragment given in
  // parameter either directly follows this one, or overlaps
  // the tail end of this fragment.
  void Join(RMBufChain& /*aChain*/) { Panic(EInet6Panic_NotSupported); }
};

//
// Fragment queue template. The queue holds a number of fragments
// of type T sorted by offset. T must be be derived from RMBufFrag.
// Each fragment is assumed to contain enough header information
// in order to determine the offset and data length of the fragment.
// Whenever a new fragment is inserted in the queue that overlaps
// or is adjacent to an existing fragment, the T::Join() method is
// called in order to combine the two fragments into a single fragment.
//
//
// In general, defragmentation is complete when the folllowing
// two conditions are met:
//
//   1) RMBufFragQ::Count() == 1
//   2) RMBufFragQ::First() is verifiably a complete packet
//
template<class T>
class RMBufFragQ : public RMBufPktQ
{
 public:
  inline RMBufFragQ() : iCount(0) {}
  inline void Init();
  inline void Free();
  inline void Assign(RMBufFragQ &aQueue);
  inline TBool Remove(T &aChain);
  inline void Append(T &aChain);
  inline void Append(RMBufFragQ &aQueue);
  inline void Prepend(T& aFrag);
  inline T& First() { return (T&)RMBufPktQ::First(); }
  inline T& Last()  { return (T&)RMBufPktQ::Last();  }
  inline TUint Count()    { return iCount; }
  void Add(T& aFrag, TUint32* aOff = 0, TUint32* aLen = 0);

 protected:
  // Compare two fragment offsets. Works for all offsets < 2^31
  // as well as for offsets requiring mod32 arithmetic, such as TCP
  // sequence numbers.
  inline TInt32 Compare(TUint32 aOffset1, TUint32 aOffset2) { return (TInt32)(aOffset1 - aOffset2); }

 private:
  // Just for us
  inline void Insert(RMBufChain& aNew, RMBufChain& aPrev);
  inline void Remove(RMBufChain& aNew, RMBufChain& aPrev);

  // Forbidden methods
  void Assign(RMBufPktQ &aQueue);
  TBool Remove(RMBufChain &aChain);
  void Append(RMBufChain &aChain);
  void Append(RMBufPktQ &aQueue);
  void Prepend(RMBufChain& aChain);

  // Members
  TUint iCount;
};

template<class T>
inline void RMBufFragQ<T>::Init()
{
  RMBufPktQ::Init();
  iCount = 0;
}

template<class T>
inline void RMBufFragQ<T>::Free()
{
  RMBufPktQ::Free();
  iCount = 0;
}

template<class T>
inline void RMBufFragQ<T>::Assign(RMBufFragQ &aQueue)
{
  RMBufPktQ::Assign(aQueue);
  iCount = aQueue.iCount;
}

template<class T>
inline TBool RMBufFragQ<T>::Remove(T &aFrag)
{
  if (RMBufPktQ::Remove(aFrag))
    {
      iCount--;
      return ETrue;
    }
  return EFalse;
}

template<class T>
inline void RMBufFragQ<T>::Append(T &aFrag)
{
  RMBufPktQ::Append(aFrag);
  iCount++;
}

template<class T>
inline void RMBufFragQ<T>::Append(RMBufFragQ &aQueue)
{
  RMBufPktQ::Append(aQueue);
  iCount += aQueue.iCount;
}

template<class T>
inline void RMBufFragQ<T>::Prepend(T& aFrag)
{
  RMBufPktQ::Prepend(aFrag);
  iCount++;
}

template<class T>
inline void RMBufFragQ<T>::Insert(RMBufChain& aNew, RMBufChain& aPrev)
{
  if (aPrev.IsEmpty())
    {
      RMBufPktQ::Prepend(aNew);
    }
  else if (aPrev.Next().IsEmpty())
    {
      RMBufPktQ::Append(aNew);
    }
  else
    {
      RMBufChain tmp;
      tmp.Assign(aNew);
      tmp.Link(aPrev.Next());
      aPrev.Link(tmp);
    }
  iCount++;
}

//
// This form of remove is broken in class RMBufPktQ (observed in ER3).
// The only user is TMBufPktQIter::Remove() and the bug only appears
// when trying to remove the last element of the queue using the
// iterator. In this case, the iLast pointer is not updated, which
// puts the queue into a corrupted state. Any subsequent Append() calls
// will cause bad things to happen.
//
template<class T>
inline void RMBufFragQ<T>::Remove(RMBufChain& aNew, RMBufChain& aPrev)
{
  if (aPrev.IsEmpty())
    {
      RMBufPktQ::Remove(aNew);
    }
  else
    {
      aNew.Assign(aPrev.Next());
      aPrev.Link(aNew.Next());
      aNew.Unlink();
      if (aPrev.Next().IsEmpty())
        iLast = aPrev;
    }
  iCount--;
}

//
// Add a fragment to queue. The routine will try to combine fragments
// where possible.
//
template<class T>
void RMBufFragQ<T>::Add(T& aFrag, TUint32 *aOff, TUint32 *aLen)
{
  __ASSERT_DEBUG(!aFrag.IsEmpty(), User::Panic(_L("RMBufFragQ::Add(): Zero length fragment.\r\n"),0));

  TUint32 off = aFrag.Offset();
  TUint32 len = aFrag.FragmentLength();
  TUint32 curOff, curLen;

  // We can't use TMBufPktQIter because of its buggy Remove() implementation.
  RMBufChain prev, current;
  T tmpFrag;

  current = First();
  while (!current.IsEmpty())
    {
      T& cur = (T&)current;
      curOff = cur.Offset();
      curLen = cur.FragmentLength();

      // Find correct position.
      if (Compare(off, curOff + curLen) > 0)
	{
          prev = current;
          current = prev.Next();
	  continue;
	}

      // No overlap? Insert here.
      if (Compare(off + len, curOff) < 0)
	{
          Insert(aFrag, prev);
	  break;
	}

      // New fragment fully overlaps queued fragment? Discard it.
      if (Compare(off, curOff) >= 0 && Compare(off + len, curOff + curLen) <= 0)
	{
	  aFrag.Free();
	  off = curOff;
	  len = curLen;
	  break;
	}

      // We have found an overlapping queued fragment. Dequeue it into tmpFrag.
      if (prev.IsEmpty())
        {
          Remove(tmpFrag);
          current = First();
        }
      else
        {
          Remove(tmpFrag, prev);
          current = prev.Next();
        }

      // Queued fragment fully overlaps new fragment? Discard it.
      if (Compare(off, curOff) <= 0 && Compare(off + len, curOff + curLen) >= 0)
	{
	  tmpFrag.Free();
	  continue;
	}

      // Partial overlap. Determine correct order and call Join(),
      if (Compare(off, curOff) <= 0)
	{
	  aFrag.Join(tmpFrag);
	  tmpFrag.Free();
	}
      else
	{
	  tmpFrag.Join(aFrag);
	  aFrag.Free();
	  aFrag.Assign(tmpFrag);
	  off = curOff;
	}
      len = aFrag.FragmentLength();
    }


  // Still have a fragment? It goes last then.
  if (!aFrag.IsEmpty())
    Append(aFrag);

  // Return the offset and length of the contiguous block,
  // which the inserted fragment belongs to.
  if (aOff) *aOff = off;
  if (aLen) *aLen = len;
}

#endif

// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ipsecpolparser.cpp - IPSec policy parser main module
//

#include <e32std.h>

#include "ipsecpolparser.h"

// Policies Parsing

// Symbian change - start
#ifdef __VC32__
#pragma warning(disable : 4097) // typedef-name used as synonym for class-name
#endif
// Symbian change - end

EXPORT_C
TPolicyParser::TPolicyParser(const TDesC &aPolicy) : TLex(aPolicy)
    {}

EXPORT_C TInt
TPolicyParser::ParseL(CIpSecurityPiece* aPieceData)
    {
    TInt err(KErrNone);
    iLine = 1;

    CSecurityPolicy* sp =  aPieceData->Policies();
    while (!err && NextToken() == token_string)
        {
        if (iToken.Compare(_L("sa")) == 0)
            {
            err = parse_sa_specL(sp);
            }
        else if (iToken.Compare(_L("ep")) == 0)
            {
            err = parse_ep_specL(sp);
            }
        else
            {
            err = parse_conn2saL(sp);
            }
        }

    if (!err && !Eos())
        {
        // Parsing didn't detect error, but not all parsed!
        err = KErrGeneral;
        }

    if (err)
        {
        if (iMsg.Length() > 0 && iMsg.Length() < 200)
            {
            aPieceData->iErrorInfo.Copy(iMsg);
            }
        }

    return (err);
    }

EXPORT_C TInt
TPolicyParser::BufferAppend(HBufC8*& aPolBfr, const TDesC8& aText)
    {
    TInt err(KErrNone);

    // Make sure that we have enough space for the new text
    TInt spaceLeft = aPolBfr->Des().MaxLength() - aPolBfr->Des().Length();
    if (aText.Length() > spaceLeft)
        {
        // Allocate enough space for the new text + some additional
        // free space so that allocations are not too frequent
        TInt newMaxLength = aPolBfr->Des().MaxLength()
                            + aText.Length()
                            + KPolicyBufferSizeIncrement;
        HBufC8* tempBfr = aPolBfr->ReAlloc(newMaxLength);
        if (tempBfr != NULL)
            {
            aPolBfr = tempBfr;
            }
        else
            {
            return KErrNoMemory;
            }
        }

    aPolBfr->Des().Append(aText);
    return err;
    }

EXPORT_C TInt
TPolicyParser::Write(CSecurityPolicy *aSp,
                     HBufC8*& aPolBfr,
                     TBool aSortingOrder)
    {
    TInt err = WriteSAs(aSp->SAList(), aPolBfr);
    if (err)
        {
        return err;
        }
    
    if (aSortingOrder)
        {
        err = WriteSelectorsInSortingOrder(aSp->SelectorList(),
                                           aPolBfr,
                                           aSortingOrder);
        }
    else
        {
        err = WriteSelectors(aSp->SelectorList(), aPolBfr, aSortingOrder);
        }
        
    return err;
    }

TInt
TPolicyParser::WriteSAs(CSAList* aSAList, HBufC8*& aPolBfr)
    {
    TBuf8<1024> aux;
    TInt err(KErrNone);
    TInt count = aSAList->Count();
    for (TInt i = 0; i < count ; i++)
        {
        TextSA(aSAList->At(i), aux);
        err = BufferAppend(aPolBfr, aux);
        if (err != KErrNone)
            {
            return err;
            }
        }
    return KErrNone;
    }
#ifdef  SYMBIAN_IPSEC_VOIP_SUPPORT    
void
TPolicyParser::TextSA(CPolicySpec* aSA, TDes8& aBuf)
    {
    if (aSA->iSpectype == EPolSpecSA)
        {
        aBuf.Format(_L8("sa "));
        // SA name
        aBuf.Append(aSA->iName->Des());
        aBuf.Append(_L8(" = {\n"));
        TInt maxcount = aSA->iPropList->Count();
        
        
		for(TInt i =0;i<maxcount;i++)
		{
		
		aBuf.Append(_L8("proposal\n{\n"));	
        
        switch (aSA->iPropList->At(i)->iType)
           	{
            	case SADB_SATYPE_AH:
                aBuf.Append(_L8(" ah\n"));
                break;
            case SADB_SATYPE_ESP:
                aBuf.Append(_L8(" esp\n"));
                break;
            default:        //SADB_SATYPE_UNSPEC            
               // aBuf.Append(_L8(" ???")); //Shouldn't happen
               //Now It can happen to indicate null policy ignore it.
                continue; // skip this proposal.
                
            }

        // Encryption Algorithm



        if (aSA->iPropList->At(i)->iEalg != 0)
            {
            // Encryption Alg
            aBuf.AppendFormat(_L8(" encrypt_alg %d\n"), aSA->iPropList->At(i)->iEalg);
            }

        if (aSA->iPropList->At(i)->iEalgLen != 0)
            {
            aBuf.AppendFormat(_L8(" max_encrypt_bits %d\n"), aSA->iPropList->At(i)->iEalgLen);
            }

        // Authentication Algorithm
        if (aSA->iPropList->At(i)->iAalg != 0)
            {
            aBuf.AppendFormat(_L8(" auth_alg %d\n"), aSA->iPropList->At(i)->iAalg);
            }

        if (aSA->iPropList->At(i)->iAalgLen != 0)
            {
            aBuf.AppendFormat(_L8(" max_auth_bits %d\n"), aSA->iPropList->At(i)->iAalgLen);
            }
        if (aSA->iPropList->At(i)->iHard.sadb_lifetime_allocations != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_allocations %d\n"),
            		aSA->iPropList->At(i)->iHard.sadb_lifetime_allocations);
            }

        if (aSA->iPropList->At(i)->iHard.sadb_lifetime_bytes != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_bytes %d\n"),
            		aSA->iPropList->At(i)->iHard.sadb_lifetime_bytes);
            }

        if (aSA->iPropList->At(i)->iHard.sadb_lifetime_addtime != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_addtime %d\n"),
            		aSA->iPropList->At(i)->iHard.sadb_lifetime_addtime);
            }

        if (aSA->iPropList->At(i)->iHard.sadb_lifetime_usetime != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_usetime %d\n"),
            		aSA->iPropList->At(i)->iHard.sadb_lifetime_usetime);
            }

        if (aSA->iPropList->At(i)->iSoft.sadb_lifetime_allocations != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_allocations %d\n"),
            		aSA->iPropList->At(i)->iSoft.sadb_lifetime_allocations);
            }

        if (aSA->iPropList->At(i)->iSoft.sadb_lifetime_bytes != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_bytes %d\n"),
            		aSA->iPropList->At(i)->iSoft.sadb_lifetime_bytes);
            }

        if (aSA->iPropList->At(i)->iSoft.sadb_lifetime_addtime != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_addtime %d\n"),
            		aSA->iPropList->At(i)->iSoft.sadb_lifetime_addtime);
            }

        if (aSA->iPropList->At(i)->iSoft.sadb_lifetime_usetime != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_usetime %d\n"),
            		aSA->iPropList->At(i)->iSoft.sadb_lifetime_usetime);
            }

            aBuf.AppendFormat(_L8("\n}\n"));// End of this Proposal
        	        
	  }
        
        if (aSA->iSpec.iPfs != 0)
            {
            aBuf.Append(_L8(" pfs\n"));
            }

        if (aSA->iRemoteIdentity != NULL)
            {
            aBuf.Append(_L8(" identity_remote "));
            aBuf.Append(aSA->iRemoteIdentity->Des());
            aBuf.Append('\n');
            }

        if (aSA->iLocalIdentity != NULL)
            {
            aBuf.Append(_L8(" identity_local "));
            aBuf.Append(aSA->iLocalIdentity->Des());
            aBuf.Append('\n');
            }

        if (aSA->iSpec.iReplayWindowLength != 0)
            {
            aBuf.AppendFormat(_L8(" replay_win_len %d\n"),
                              aSA->iSpec.iReplayWindowLength);
            }

        if (aSA->iSpec.iMatchProtocol != 0)
            {
            aBuf.Append(_L8(" protocol_specific\n"));
            }

        if (aSA->iSpec.iMatchLocalPort != 0)
            {
            aBuf.Append(_L8(" local_port_specific\n"));
            }

        if (aSA->iSpec.iMatchRemotePort != 0)
            {
            aBuf.Append(_L8(" remote_port_specific\n"));
            }

        if (aSA->iSpec.iMatchProxy != 0)
            {
            aBuf.Append(_L8(" proxy_specific\n"));
            }

        if (aSA->iSpec.iMatchSrc != 0)
            {
            aBuf.Append(_L8(" src_specific\n"));
            }

        if (aSA->iSpec.iMatchLocal != 0)
            {
            aBuf.Append(_L8(" local_specific\n"));
            }

        if (aSA->iSpec.iMatchRemote != 0)
            {
            aBuf.Append(_L8(" remote_specific\n"));
            }

        aBuf.AppendFormat(_L8(" }\n\n"));
        }
    else
        {
        TBuf<39> addr;
        aBuf.Format(_L8("ep "));

        // EndPoint name
        aBuf.Append(aSA->iName->Des());
        aBuf.Append(_L8(" = {"));

        if (aSA->iEpSpec.iIsOptional)
            {
            aBuf.Append(_L8(" ? "));
            }
        aSA->iEpSpec.iEpAddr.OutputWithScope(addr);
        aBuf.Append(addr);
        aBuf.Append(_L8(" }\n\n"));
        }
    }
#else
void
TPolicyParser::TextSA(CPolicySpec* aSA, TDes8& aBuf)
    {
    if (aSA->iSpectype == EPolSpecSA)
        {
        aBuf.Format(_L8("sa "));
        // SA name
        aBuf.Append(aSA->iName->Des());
        aBuf.Append(_L8(" = {\n"));
        switch (aSA->iSpec.iType)
            {
            case SADB_SATYPE_AH:
                aBuf.Append(_L8(" ah\n"));
                break;
            case SADB_SATYPE_ESP:
                aBuf.Append(_L8(" esp\n"));
                break;
            default:        //SADB_SATYPE_UNSPEC
                aBuf.Append(_L8(" ???")); //Shouldn't happen
            }

        // Encryption Algorithm



        if (aSA->iSpec.iEalg != 0)
            {
            // Encryption Alg
            aBuf.AppendFormat(_L8(" encrypt_alg %d\n"), aSA->iSpec.iEalg);
            }

        if (aSA->iSpec.iEalgLen != 0)
            {
            aBuf.AppendFormat(_L8(" max_encrypt_bits %d\n"), aSA->iSpec.iEalgLen);
            }

        // Authentication Algorithm
        if (aSA->iSpec.iAalg != 0)
            {
            aBuf.AppendFormat(_L8(" auth_alg %d\n"), aSA->iSpec.iAalg);
            }

        if (aSA->iSpec.iAalgLen != 0)
            {
            aBuf.AppendFormat(_L8(" max_auth_bits %d\n"), aSA->iSpec.iAalgLen);
            }

        if (aSA->iSpec.iPfs != 0)
            {
            aBuf.Append(_L8(" pfs\n"));
            }

        if (aSA->iRemoteIdentity != NULL)
            {
            aBuf.Append(_L8(" identity_remote "));
            aBuf.Append(aSA->iRemoteIdentity->Des());
            aBuf.Append('\n');
            }

        if (aSA->iLocalIdentity != NULL)
            {
            aBuf.Append(_L8(" identity_local "));
            aBuf.Append(aSA->iLocalIdentity->Des());
            aBuf.Append('\n');
            }

        if (aSA->iSpec.iReplayWindowLength != 0)
            {
            aBuf.AppendFormat(_L8(" replay_win_len %d\n"),
                              aSA->iSpec.iReplayWindowLength);
            }

        if (aSA->iSpec.iMatchProtocol != 0)
            {
            aBuf.Append(_L8(" protocol_specific\n"));
            }

        if (aSA->iSpec.iMatchLocalPort != 0)
            {
            aBuf.Append(_L8(" local_port_specific\n"));
            }

        if (aSA->iSpec.iMatchRemotePort != 0)
            {
            aBuf.Append(_L8(" remote_port_specific\n"));
            }

        if (aSA->iSpec.iMatchProxy != 0)
            {
            aBuf.Append(_L8(" proxy_specific\n"));
            }

        if (aSA->iSpec.iMatchSrc != 0)
            {
            aBuf.Append(_L8(" src_specific\n"));
            }

        if (aSA->iSpec.iMatchLocal != 0)
            {
            aBuf.Append(_L8(" local_specific\n"));
            }

        if (aSA->iSpec.iMatchRemote != 0)
            {
            aBuf.Append(_L8(" remote_specific\n"));
            }

        if (aSA->iSpec.iHard.sadb_lifetime_allocations != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_allocations %d\n"),
                              aSA->iSpec.iHard.sadb_lifetime_allocations);
            }

        if (aSA->iSpec.iHard.sadb_lifetime_bytes != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_bytes %d\n"),
                              aSA->iSpec.iHard.sadb_lifetime_bytes);
            }

        if (aSA->iSpec.iHard.sadb_lifetime_addtime != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_addtime %d\n"),
                              aSA->iSpec.iHard.sadb_lifetime_addtime);
            }

        if (aSA->iSpec.iHard.sadb_lifetime_usetime != 0)
            {
            aBuf.AppendFormat(_L8(" hard_lifetime_usetime %d\n"),
                              aSA->iSpec.iHard.sadb_lifetime_usetime);
            }

        if (aSA->iSpec.iSoft.sadb_lifetime_allocations != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_allocations %d\n"),
                              aSA->iSpec.iSoft.sadb_lifetime_allocations);
            }

        if (aSA->iSpec.iSoft.sadb_lifetime_bytes != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_bytes %d\n"),
                              aSA->iSpec.iSoft.sadb_lifetime_bytes);
            }

        if (aSA->iSpec.iSoft.sadb_lifetime_addtime != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_addtime %d\n"),
                              aSA->iSpec.iSoft.sadb_lifetime_addtime);
            }

        if (aSA->iSpec.iSoft.sadb_lifetime_usetime != 0)
            {
            aBuf.AppendFormat(_L8(" soft_lifetime_usetime %d\n"),
                              aSA->iSpec.iSoft.sadb_lifetime_usetime);
            }
        aBuf.AppendFormat(_L8(" }\n\n"));
        }
    else
        {
        TBuf<39> addr;
        aBuf.Format(_L8("ep "));

        // EndPoint name
        aBuf.Append(aSA->iName->Des());
        aBuf.Append(_L8(" = {"));

        if (aSA->iEpSpec.iIsOptional)
            {
            aBuf.Append(_L8(" ? "));
            }
        aSA->iEpSpec.iEpAddr.OutputWithScope(addr);
        aBuf.Append(addr);
        aBuf.Append(_L8(" }\n\n"));
        }
    }

#endif
TInt
TPolicyParser::WriteSelectors(CSelectorList* aSelList,
                              HBufC8*& aPolBfr,
                              TBool /* aSortingOrder */)
    {
    TBuf8<1024> aux;
    TInt err(KErrNone);
    TInt count(aSelList->Count());
    for (TInt i = 0; i < count; i++)
        {
        aux.Zero();
        CPolicySelector* ps = aSelList->At(i);

        // Bypass the selector, if sequence number is 0xFFFFFFFF.
        // This sequence number indicates that the selector
        // is of type 'bypass/drop_everything_else'
        if (ps->iSequenceNumber == 0xFFFFFFFF)
            {
            continue;
            }
            
        // Convert selector to text format and print it into buffer 
        TextSel(ps, aux, EFalse);
        err = BufferAppend(aPolBfr, aux);
        if (err != KErrNone)
            {
            return err;
            }
        }

    // All selectors have been written
    err = BufferAppend(aPolBfr, (_L8("\n")));
    return (err);
    }

//
//  This function writes the selectors to a file according
//  to the sequence numbers available in the CPolicySelector.
//
//
TInt
TPolicyParser::WriteSelectorsInSortingOrder(
    CSelectorList* aSelList,
    HBufC8*& aPolBfr,
    TBool /* aSortingOrder */)
    {
    TInt err(KErrNone);
    TInt count(aSelList->Count());
    TInt currentSequenceNumber(1);

    // Loop here until all selectors have been written
    TBool found = ETrue;
    while (found)
        {
        found = EFalse;

        // Loop through the selector list and search the
        // the selector corresponding to the current sequence number
        for (TInt i = 0; i < count; i++)
            {
            TBuf8<1024> aux;
            aux.Zero();
            CPolicySelector* ps = aSelList->At(i);
            
            if (ps->iSequenceNumber == currentSequenceNumber)
                {
                // Build a selector output string
                TextSel(ps, aux, ETrue);

                // Write a string to the file
                err = BufferAppend(aPolBfr, aux);
                if (err != KErrNone)
                    {
                    return err;
                    }
                // Prepare for the next selector
                currentSequenceNumber++;
                found = ETrue;
                break;
                }
            }
        }

    // All selectors have been written
    err = BufferAppend(aPolBfr, (_L8("\n")));
    return (err);
    }

//
// Prints the supplied selector into a given buffer in text format
//
//
void
TPolicyParser::TextSel(CPolicySelector* aSel,
                       TDes8& aBuf,
                       TBool aOrdered)
    {
    aBuf.Format(_L8(" "));

    if (aSel->iIsFinal)
        {
        aBuf.Append(_L8(" final "));
        }

    if (aSel->iIsMerge)
        {
        aBuf.Append(_L8(" merge "));
        }

    // NOTE:
    //  This is a kludge to save the global selector definition
    //  when policy is loaded/parsed and then finally cached into 
    //  a list in text format. When combined policy is build 
    //  before sending it to IPSEC6.PRT component, the selector
    //  list is ordered so this definition is then not included
    //  in the policy text that is sent into the protocol component
    if (aSel->iGlobalSelector && !aOrdered)
        {
        aBuf.Append(_L8(" scope:global "));
        }

    switch (aSel->iDirection)
        {
        default:
            break;

        case KPolicySelector_SYMMETRIC:
            break;

        case KPolicySelector_INBOUND:
            aBuf.Append(_L8(" inbound "));
            break;

        case KPolicySelector_OUTBOUND:
            aBuf.Append(_L8(" outbound "));
            break;

        case KPolicySelector_INTERFACE:
            TBuf8<20> name;
            name.Copy(aSel->iInterface);
            aBuf.Append(_L8(" if "));
            aBuf.Append(name);
            aBuf.Append(_L8(" "));
            break;
        }

    // Check if remote address exists and no interface name defined
    if (aSel->iDirection != KPolicySelector_INTERFACE 
        && aSel->iRemote.Family() != KAFUnspec)
        {
        TBuf<39> addr;
        TBuf<39> mask;

        aSel->iRemote.OutputWithScope(addr);
        aSel->iRemoteMask.OutputWithScope(mask);
        
        // Add remote address/mask with scope into the buffer        
        aBuf.Append(_L8(" remote "));
        aBuf.Append(addr);
        aBuf.Append(_L8(" "));
        aBuf.Append(mask);
        }
    else 
        {
        if (aSel->iRemSelEpName != NULL)
            {
            // Remote Endpoint name exists so add it into the buffer
            aBuf.Append(_L8(" remote "));
            aBuf.Append(aSel->iRemSelEpName->Des());
            }
        if (aSel->iRemMaskEpName != NULL)
            {
            aBuf.Append(_L8(" "));
            aBuf.Append(aSel->iRemMaskEpName->Des());
            }
        }

    // Check if local address exists and no interface name defined
    if (aSel->iDirection != KPolicySelector_INTERFACE 
        && aSel->iLocal.Family() != KAFUnspec)
        {
        TBuf<39> addr;
        TBuf<39> mask;
        
        aSel->iLocal.OutputWithScope(addr);
        aSel->iLocalMask.OutputWithScope(mask);

        // Add local address/mask with scope into the buffer        
        aBuf.Append(_L8(" local "));
        aBuf.Append(addr);
        aBuf.Append(_L8(" "));
        aBuf.Append(mask);
        }
    else 
        {
        if (aSel->iLocSelEpName != NULL)
            {
            // Local Endpoint name exists so add it into the buffer
            aBuf.Append(_L8(" local "));
            aBuf.Append(aSel->iLocSelEpName->Des());
            }
        if (aSel->iLocMaskEpName != NULL)
            {
            aBuf.Append(_L8(" "));
            aBuf.Append(aSel->iLocMaskEpName->Des());
            }
        }

    if (aSel->iProtocol != 0)
        {
        aBuf.AppendFormat(_L8(" protocol %d "), aSel->iProtocol);
        }

    if (aSel->iLocal.Port() != 0)
        {
        aBuf.AppendFormat(_L8(" local_port %d "), aSel->iLocal.Port());
        }

    if (aSel->iRemote.Port() != 0)
        {
        aBuf.AppendFormat(_L8(" remote_port %d "), aSel->iRemote.Port());
        }

    if (aSel->iIcmpType != -1)
        {
        aBuf.AppendFormat(_L8(" icmp_type %d "), aSel->iIcmpType);
        }

    if (aSel->iType != -1)
        {
        aBuf.AppendFormat(_L8(" type %d "), aSel->iType);
        }

    if (aSel->iIcmpCode != -1)
        {
        aBuf.AppendFormat(_L8(" icmp_code %d "), aSel->iIcmpCode);
        }

    if (aSel->iDropAction)
        {
        aBuf.Append(_L8(" = drop\n "));
        return ;
        }

    aBuf.Append(_L8(" = { "));

    TSecpolBundleIter iterl(aSel->iBundle);
    CSecpolBundleItem* itemL(NULL);
    while ((itemL = iterl++) != NULL)
        {
        if (itemL->iSpec != NULL)
            aBuf.Append(*itemL->iSpec->iName);
        else
            aBuf.Append(_L8(" tunnel"));

        aBuf.Append(_L8("("));

        if (!itemL->iTunnel.IsUnspecified())
            {
            TBuf<39> addr;
            itemL->iTunnel.OutputWithScope(addr);
            aBuf.Append(addr);
            }
        else if (itemL->iTunnelEpName != NULL)
            {
            aBuf.Append(itemL->iTunnelEpName->Des());
            }
        aBuf.Append(_L8(") "));
        }

    aBuf.Append(_L8(" }\n"));
    }

void
TPolicyParser::Error(TRefByValue<const TDesC> aFmt, ...)
    {
    VA_LIST list;
    VA_START(list, aFmt);
    iMsg.FormatList(aFmt, list);
    iMsg += (_L(" at line "));
    iMsg.AppendNum(iLine);
    };

//
// Skip white space and mark, including comments!
//
void
TPolicyParser::SkipSpaceAndMark()
    {
    TChar ch;
    TInt comment = 0;

    while (!Eos())
        {
        ch = Get();
        if (ch == '\n')
            {
            iLine++;
            comment = 0;
            }
        else if (comment || ch == '#')
            comment = 1;
        else if (!ch.IsSpace())
            {
            UnGet();
            break;
            }
        }
    Mark();
    }

//
//
token_type TPolicyParser::NextToken()
    {
    TChar ch;
    token_type val;

    SkipSpaceAndMark();
    if (Eos())
        {
        val = token_eof;
        }
    else
        {
        ch = Get();
        if (ch == '{')
            val = token_brace_left;
        else if (ch == '}')
            val = token_brace_right;
        else if (ch == '(')
            val = token_par_left;
        else if (ch == ')')
            val = token_par_right;
        else if (ch == '=')
            val = token_equal;
        else if (ch == ',')
            val = token_comma;
        else
            {
            val = token_string;
            while (!Eos())
                {
                ch = Peek();
                if (ch == '{' || ch == '}' ||
                    ch == '(' || ch == ')' ||
                    ch == '=' || ch == '#' || ch.IsSpace())
                    break;
                Inc();
                }
            }
        }
    iToken.Set(MarkedToken());
    SkipSpaceAndMark();
    return (val);
    }
#ifdef  SYMBIAN_IPSEC_VOIP_SUPPORT    
TInt TPolicyParser::validateProposals(CPropList& aPropList)
	{
	TInt err = KErrNone;
	CSecurityProposalSpec* prop;
	TInt validPropCount=0;
	for(TInt i=0;i<aPropList.Count();i++)
		{
		prop = aPropList.At(i);
		if((prop->iType == SADB_SATYPE_UNSPEC) ||
		   ((prop->iType == SADB_SATYPE_AH) && !prop->iAalg) ||
		   ((prop->iType == SADB_SATYPE_ESP) && !prop->iEalg) 
		   ){
		  	   
		   	prop->iType = SADB_SATYPE_UNSPEC;
		   	prop->iEalg  = 0;
		   	prop->iAalg = 0;
		   }
		else
			{
			validPropCount++;
			}
		}
	if(validPropCount ==0)
		{
		err = KErrGeneral;
		}
	 return err;
	}
CSecurityProposalSpec* TPolicyParser::CreateProposalL(CPropList& aPropList)
	{
	CSecurityProposalSpec* prop = new(ELeave) CSecurityProposalSpec;
	prop->iType = SADB_SATYPE_UNSPEC;	
	aPropList.AppendL(prop);
	return prop;
	}

TInt
TPolicyParser::parse_sa_spec_paramsL(CPolicySpec& aSpec)
    {
    TInt sa_type_defined(0);
    TInt err(KErrNone);
    token_type val;
    TBool InProposal=EFalse;
    CSecurityProposalSpec* prop = CreateProposalL(*aSpec.iPropList);
    TInt propCount=1;
    
    while (((val = NextToken()) == token_string) || (val == token_brace_right))
        {
        if (iToken.Compare(_L("proposal")) == 0)
            {
            if (NextToken() == token_brace_left)
                {
                if (propCount != 1)
                	 	 prop = CreateProposalL(*aSpec.iPropList);
                ++propCount;	                                 
                InProposal = ETrue;
                }
            else
                {                
                return (KErrGeneral);
                }            
            }  
        else if (val == token_brace_right)
        			{
        			//TODO : sa_type must be defined
        			if(InProposal)
                        {                
                        InProposal = EFalse;
                        prop = aSpec.iPropList->At(0);
                        }         			
        			else
        				{
        				break;//get out of while loop
        				}
        			}
        else if (iToken.Compare(_L("ah")) == 0)
            {
            if(prop->iType != SADB_SATYPE_UNSPEC)
            	{
            	Error(_L("invalid auth alg %d"), (TUint)prop->iType);
                return (KErrGeneral); 
            	}
            prop->iType  = SADB_SATYPE_AH;            
            }
        else if (iToken.Compare(_L("esp")) == 0)
            {            
            if(prop->iType != SADB_SATYPE_UNSPEC)
            	{
            	Error(_L("invalid encrypt alg %d"), (TUint)prop->iType);
                return (KErrGeneral); 
            	}           
            prop->iType = SADB_SATYPE_ESP;
            }
        else if (iToken.Compare(_L("encrypt_alg")) == 0)
            {
            err = Val(prop->iEalg, EDecimal);
            if ((err != KErrNone) || (prop->iEalg > MAX_EALG_VALUE))
                {
                Error(_L("invalid encrypt alg %d"), (TUint)prop->iEalg);
                return (KErrGeneral);
                }
            }
        else if (iToken.Compare(_L("max_encrypt_bits")) == 0)
            {
            err = Val(prop->iEalgLen, EDecimal);
            if (err != KErrNone)
                {
                Error(_L("invalid encrypt alg key length %d"),
                		prop->iEalgLen);
                return (KErrGeneral);
                }
            }
        else if (iToken.Compare(_L("auth_alg")) == 0)
            {
            err = Val(prop->iAalg, EDecimal);
            if (err != KErrNone)
                {
                Error(_L("invalid auth alg %d"), prop->iAalg);
                return (KErrGeneral);
                }
            }
        else if (iToken.Compare(_L("max_auth_bits")) == 0)
            {
            err = Val(prop->iAalgLen, EDecimal);
            if (err != KErrNone)
                {
                Error(_L("invalid auth alg length %d"), prop->iAalgLen);
                return (KErrGeneral);
                }
            }
        else if ((iToken.Compare(_L("identity")) == 0) ||
                 (iToken.Compare(_L("identity_remote")) == 0))
            {
            if (aSpec.iRemoteIdentity)
                {
                Error(_L("duplicate remote identity"));
                err = KErrGeneral;
                }
            else if ((val = NextToken()) == token_string)
                {
                aSpec.iRemoteIdentity = HBufC8::NewL(iToken.Length() + 1);
                aSpec.iRemoteIdentity->Des().Copy(iToken);
                }
            else
                {
                Error(_L("invalid remote identity value"));
                err = KErrGeneral;
                }
            }
        else if (iToken.Compare(_L("identity_local")) == 0)
            {
            if (aSpec.iLocalIdentity)
                {
                Error(_L("duplicate local identity"));
                err = KErrGeneral;
                }
            else if ((val = NextToken()) == token_string)
                {
                aSpec.iLocalIdentity = HBufC8::NewL(iToken.Length() + 1);
                aSpec.iLocalIdentity->Des().Copy(iToken);
                }
            else
                {
                Error(_L("invalid local identity value"));
                err = KErrGeneral;
                }
            }
        else if (iToken.Compare(_L("pfs")) == 0)
            {
            aSpec.iSpec.iPfs = 1;
            }
        else if (iToken.Compare(_L("connid_specific")) == 0)
            {
            // Only a temporary backward compatibility hack
            aSpec.iSpec.iMatchProtocol = 1;
            aSpec.iSpec.iMatchRemotePort = 1;
            aSpec.iSpec.iMatchLocalPort = 1;
            }
        else if (iToken.Compare(_L("protocol_specific")) == 0)
            {
            aSpec.iSpec.iMatchProtocol = 1;
            }
        else if ((iToken.Compare(_L("src_port_specific")) == 0)
                 || (iToken.Compare(_L("local_port_specific")) == 0))
            {
            aSpec.iSpec.iMatchLocalPort = 1;
            }
        else if ((iToken.Compare(_L("dst_port_specific")) == 0)
                 || (iToken.Compare(_L("remote_port_specific")) == 0))
            {
            aSpec.iSpec.iMatchRemotePort = 1;
            }
        else if (iToken.Compare(_L("proxy_specific")) == 0)
            {
            aSpec.iSpec.iMatchProxy = 1;
            }
        else if (iToken.Compare(_L("src_specific")) == 0)
            {
            aSpec.iSpec.iMatchSrc = 1;
            }
        else if (iToken.Compare(_L("local_specific")) == 0)
            {
            aSpec.iSpec.iMatchLocal = 1;
            }
        else if (iToken.Compare(_L("remote_specific")) == 0)
            {
            aSpec.iSpec.iMatchRemote = 1;
            }
        else if (iToken.Compare(_L("replay_win_len")) == 0)
            {
            err = Val(aSpec.iSpec.iReplayWindowLength, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_allocations")) == 0)
            {
            err = Val(prop->iHard.sadb_lifetime_allocations, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_bytes")) == 0)
            {
            err = Val(prop->iHard.sadb_lifetime_bytes, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_addtime")) == 0)
            {
            err = Val(prop->iHard.sadb_lifetime_addtime, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_usetime")) == 0)
            {
            err = Val(prop->iHard.sadb_lifetime_usetime, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_allocations")) == 0)
            {
            err = Val(prop->iSoft.sadb_lifetime_allocations, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_bytes")) == 0)
            {
            err = Val(prop->iSoft.sadb_lifetime_bytes, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_addtime")) == 0)
            {
            err = Val(prop->iSoft.sadb_lifetime_addtime, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_usetime")) == 0)
            {
            err = Val(prop->iSoft.sadb_lifetime_usetime, EDecimal);
            }
        else
            {
            Error(_L("invalid keyword"));
            return (KErrGeneral);
            }
        if (err != KErrNone)
            {
            Error(_L("invalid numeric value"));
            return (err);
            }
        }

    if (val != token_brace_right)
        {
        Error(_L("right brace not found"));
        return (KErrGeneral);
        }
    err = validateProposals(*aSpec.iPropList);
    

    return (err);
    }
#else
TInt
TPolicyParser::parse_sa_spec_paramsL(CPolicySpec& aSpec)
    {
    TInt sa_type_defined(0);
    TInt err(KErrNone);
    token_type val;

    while ((val = NextToken()) == token_string)
        {
        if (iToken.Compare(_L("ah")) == 0)
            {
            sa_type_defined++;
            aSpec.iSpec.iType = SADB_SATYPE_AH;
            }
        else if (iToken.Compare(_L("esp")) == 0)
            {
            sa_type_defined++;
            aSpec.iSpec.iType = SADB_SATYPE_ESP;
            }
        else if (iToken.Compare(_L("encrypt_alg")) == 0)
            {
            err = Val(aSpec.iSpec.iEalg, EDecimal);
            if ((err != KErrNone) || (aSpec.iSpec.iEalg > MAX_EALG_VALUE))
                {
                Error(_L("invalid encrypt alg %d"), (TUint)aSpec.iSpec.iEalg);
                return (KErrGeneral);
                }
            }
        else if (iToken.Compare(_L("max_encrypt_bits")) == 0)
            {
            err = Val(aSpec.iSpec.iEalgLen, EDecimal);
            if (err != KErrNone)
                {
                Error(_L("invalid encrypt alg key length %d"),
                      aSpec.iSpec.iEalgLen);
                return (KErrGeneral);
                }
            }
        else if (iToken.Compare(_L("auth_alg")) == 0)
            {
            err = Val(aSpec.iSpec.iAalg, EDecimal);
            if (err != KErrNone)
                {
                Error(_L("invalid auth alg %d"), aSpec.iSpec.iAalg);
                return (KErrGeneral);
                }
            }
        else if (iToken.Compare(_L("max_auth_bits")) == 0)
            {
            err = Val(aSpec.iSpec.iAalgLen, EDecimal);
            if (err != KErrNone)
                {
                Error(_L("invalid auth alg length %d"), aSpec.iSpec.iAalgLen);
                return (KErrGeneral);
                }
            }
        else if ((iToken.Compare(_L("identity")) == 0) ||
                 (iToken.Compare(_L("identity_remote")) == 0))
            {
            if (aSpec.iRemoteIdentity)
                {
                Error(_L("duplicate remote identity"));
                err = KErrGeneral;
                }
            else if ((val = NextToken()) == token_string)
                {
                aSpec.iRemoteIdentity = HBufC8::NewL(iToken.Length() + 1);
                aSpec.iRemoteIdentity->Des().Copy(iToken);
                }
            else
                {
                Error(_L("invalid remote identity value"));
                err = KErrGeneral;
                }
            }
        else if (iToken.Compare(_L("identity_local")) == 0)
            {
            if (aSpec.iLocalIdentity)
                {
                Error(_L("duplicate local identity"));
                err = KErrGeneral;
                }
            else if ((val = NextToken()) == token_string)
                {
                aSpec.iLocalIdentity = HBufC8::NewL(iToken.Length() + 1);
                aSpec.iLocalIdentity->Des().Copy(iToken);
                }
            else
                {
                Error(_L("invalid local identity value"));
                err = KErrGeneral;
                }
            }
        else if (iToken.Compare(_L("pfs")) == 0)
            {
            aSpec.iSpec.iPfs = 1;
            }
        else if (iToken.Compare(_L("connid_specific")) == 0)
            {
            // Only a temporary backward compatibility hack
            aSpec.iSpec.iMatchProtocol = 1;
            aSpec.iSpec.iMatchRemotePort = 1;
            aSpec.iSpec.iMatchLocalPort = 1;
            }
        else if (iToken.Compare(_L("protocol_specific")) == 0)
            {
            aSpec.iSpec.iMatchProtocol = 1;
            }
        else if ((iToken.Compare(_L("src_port_specific")) == 0)
                 || (iToken.Compare(_L("local_port_specific")) == 0))
            {
            aSpec.iSpec.iMatchLocalPort = 1;
            }
        else if ((iToken.Compare(_L("dst_port_specific")) == 0)
                 || (iToken.Compare(_L("remote_port_specific")) == 0))
            {
            aSpec.iSpec.iMatchRemotePort = 1;
            }
        else if (iToken.Compare(_L("proxy_specific")) == 0)
            {
            aSpec.iSpec.iMatchProxy = 1;
            }
        else if (iToken.Compare(_L("src_specific")) == 0)
            {
            aSpec.iSpec.iMatchSrc = 1;
            }
        else if (iToken.Compare(_L("local_specific")) == 0)
            {
            aSpec.iSpec.iMatchLocal = 1;
            }
        else if (iToken.Compare(_L("remote_specific")) == 0)
            {
            aSpec.iSpec.iMatchRemote = 1;
            }
        else if (iToken.Compare(_L("replay_win_len")) == 0)
            {
            err = Val(aSpec.iSpec.iReplayWindowLength, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_allocations")) == 0)
            {
            err = Val(aSpec.iSpec.iHard.sadb_lifetime_allocations, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_bytes")) == 0)
            {
            err = Val(aSpec.iSpec.iHard.sadb_lifetime_bytes, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_addtime")) == 0)
            {
            err = Val(aSpec.iSpec.iHard.sadb_lifetime_addtime, EDecimal);
            }
        else if (iToken.Compare(_L("hard_lifetime_usetime")) == 0)
            {
            err = Val(aSpec.iSpec.iHard.sadb_lifetime_usetime, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_allocations")) == 0)
            {
            err = Val(aSpec.iSpec.iSoft.sadb_lifetime_allocations, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_bytes")) == 0)
            {
            err = Val(aSpec.iSpec.iSoft.sadb_lifetime_bytes, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_addtime")) == 0)
            {
            err = Val(aSpec.iSpec.iSoft.sadb_lifetime_addtime, EDecimal);
            }
        else if (iToken.Compare(_L("soft_lifetime_usetime")) == 0)
            {
            err = Val(aSpec.iSpec.iSoft.sadb_lifetime_usetime, EDecimal);
            }
        else
            {
            Error(_L("invalid keyword"));
            return (KErrGeneral);
            }
        if (err != KErrNone)
            {
            Error(_L("invalid numeric value"));
            return (err);
            }
        }

    if (val != token_brace_right)
        {
        Error(_L("right brace not found"));
        return (KErrGeneral);
        }
    else if (sa_type_defined < 1)
        {
        Error(_L("sa type not defined for sa"));
        return (KErrGeneral);
        }
    else if (sa_type_defined > 1)
        {
        Error(_L("sa type defined times for sa"));
        return (KErrGeneral);
        }
    else if ((aSpec.iSpec.iType == SADB_SATYPE_AH) && !aSpec.iSpec.iAalg)
        {
        Error(_L("auth alg not defined for sa"));
        return (KErrGeneral);
        }
    else if ((aSpec.iSpec.iType == SADB_SATYPE_ESP) && !aSpec.iSpec.iEalg)
        {
        Error(_L("encrypt alg not defined for sa"));
        return (KErrGeneral);
        }
    else if ((aSpec.iSpec.iType == SADB_SATYPE_UNSPEC) &&
             (aSpec.iSpec.iEalg || aSpec.iSpec.iAalg))
        {
        Error(_L("null SA cannot have any algorithms"));
        return (KErrGeneral);
        }

    return (KErrNone);
    }

#endif
TInt
TPolicyParser::parse_sa_specL(CSecurityPolicy* aSp)
    {
    TInt err(KErrNone);
    CPolicySpec* spec(NULL);

    if (NextToken() != token_string)
        {
        Error(_L("Syntax error"));
        err = KErrGeneral;
        }
    else
        {
        spec = CPolicySpec::NewL(iToken);
        aSp->Add(spec);

        if (NextToken() != token_equal || NextToken() != token_brace_left)
            {
            Error(_L("Syntax error"));
            err = KErrGeneral;
            }
        else
            {
            err = parse_sa_spec_paramsL(*spec);
            }
        }

    return (err);
    }

TInt
TPolicyParser::parse_sa_spec_listL(TSecpolBundle& aBundle,
                                   CSecurityPolicy* aSp)
    {
    CSecpolBundleItem* item(NULL);
    CPolicySpec* spec(NULL);
    token_type val;
    TInt err(KErrNone);

    while ((val = NextToken()) == token_string)
        {
        // Find the SA transform specification from the given policy
        HBufC8 * hbuf = HBufC8::NewL(iToken.Length());
        hbuf->Des().Copy(iToken);
        spec = aSp->FindSpec(hbuf->Des());
        delete hbuf;
        hbuf = NULL;
        
        // A temporary(?) special kludge: if the keyword is 'tunnel'
        // assume this is a plain tunnel specification, without any
        // IPsec processing
        
        // NOTE: 
        //  This works only when the SA specification name is not 'tunnel
        //  ('tunnel' should be illegal name for SA specification to
        //   avoid confusion)
        if (!spec && iToken.Compare(_L("tunnel")))
            {
            Error(_L("sa or plain tunnel not defined"));
            err = KErrGeneral;
            break;
            }

        // Allocate memory for new bundle item
        item = new (ELeave) CSecpolBundleItem;
        CleanupStack::PushL(item);

        // Init bundle item by using the SA transform template found        
        item->iSpec = spec;
        
        // Read next token
        val = NextToken();
        
        // Check that '(' found
        if (val != token_par_left)
            {
            // Remove bundle item from the CleanupStack and set error code
            CleanupStack::PopAndDestroy();
            Error(_L("missing left parenthesis"));
            err = KErrGeneral;
            break;
            }

        // Read next token
        val = NextToken();
        
        // Check if tunnel specification is set
        if (val == token_string)
            {
            // Tunnel entry found so determine if name or plain address
            if (aSp->SearchForEPNameL(iToken) == KErrNone)
                {
                // Tunnel name is set so copy it
                item->iTunnelEpName = HBufC8::NewL(iToken.Length());
                item->iTunnelEpName->Des().Copy(iToken);
                //Search for the SA transform CPolicySpec based on the 
                //remote end point name
                CPolicySpec* specEp(NULL);
                HBufC8 * hbuf = HBufC8::NewL(iToken.Length());
                hbuf->Des().Copy(iToken);
                specEp = aSp->FindSpec(hbuf->Des());
                delete hbuf;
                hbuf = NULL;
                  
                //Set the tunnel address from the above SA transform
                item->iTunnel.SetAddress(specEp->iEpSpec.iEpAddr.Address());                    
                }
            else
                {
                // Tunnel address is set so use it    
                err = item->iTunnel.Input(iToken);
                if (err)
                    {
                    // Remove bundle item from the CleanupStack
                    CleanupStack::PopAndDestroy();
                    Error(_L("Invalid IP address"));
                    break;
                    }
                }

            // Read next token
            val = NextToken();
            }
        
        // Check that ')' terminates the definition correctly    
        if (val != token_par_right)
            {
            // Remove bundle item from the CleanupStack and set error code
            CleanupStack::PopAndDestroy();
            Error(_L("missing right parenthesis"));
            err = KErrGeneral;
            break;
            }

        // Remove bundle item from the CleanupStack and add it into the list
        CleanupStack::Pop();
        aBundle.AddLast(*item);
        }

    // Check that terminating '}' is found
    if (!err && val != token_brace_right)
        {
        Error(_L("missing right brace"));
        err = KErrGeneral;
        }

    return (err);
    }

TInt
TPolicyParser::parse_ip_addr_and_maskL(
    TInetAddr& addr,
    TInetAddr& mask,
    HBufC8*& aSelEpName,
    HBufC8*& aMaskEpName,
    CSecurityPolicy* aSecPol)
    {
    TInt err(KErrNone);
    if (NextToken() != token_string)
        {
        Error(_L("ip address not found"));
        return (KErrGeneral);
        }

    if (aSecPol->SearchForEPNameL(iToken) == KErrNone)
        {
        aSelEpName = HBufC8::NewL(iToken.Length());
        aSelEpName->Des().Copy(iToken);
        }
    else
        {
        err = addr.Input(iToken);
        if (err != 0)
            {
            Error(_L("invalid ip address "));
            return (err);
            }
        }

    if (NextToken() != token_string)
        {
        Error(_L("address mask not found"));
        return (KErrGeneral);
        }

    if (aSecPol->SearchForEPNameL(iToken) == KErrNone)
        {
        aMaskEpName = HBufC8::NewL(iToken.Length());
        aMaskEpName->Des().Copy(iToken);
        }
    else
        {
        err = mask.Input(iToken);
        if (err != 0)
            {
            Error(_L("invalid address mask "));
            return (err);
            }
        }

    return (KErrNone);
    }

//
// Parse the endpoint name entry
//
//
TInt
TPolicyParser::parse_ep_specL(CSecurityPolicy* aSp)
    {
    TInt err(KErrNone);
    CPolicySpec* spec(NULL);

    if (NextToken() != token_string)
        {
        Error(_L("Syntax error"));
        err = KErrGeneral;
        }
    else
        {
        spec = CPolicySpec::NewL(iToken, EPolSpecEP);
        aSp->Add(spec);

        if (NextToken() != token_equal || NextToken() != token_brace_left)
            {
            Error(_L("Syntax error"));
            err = KErrGeneral;
            }
        else
            {
            err = parse_ep_spec_paramsL(*spec);
            }
        }

    return (err);
    }

//
// Parse the endpoint name parameters
//
TInt
TPolicyParser::parse_ep_spec_paramsL(CPolicySpec &aSpec)
    {
    TInt err(KErrNone);
    token_type val;

    while ((val = NextToken()) == token_string)
        {
        if (iToken.Compare(_L("?")) == 0)
            {
            aSpec.iEpSpec.iIsOptional = ETrue;
            }
        else
            {
            err = aSpec.iEpSpec.iEpAddr.Input(iToken);
            if (err != 0)
                {
                Error(_L("invalid ip address "));
                return (err);
                }
            }
        }

    if (val != token_brace_right)
        {
        Error(_L("right brace not found"));
        err = KErrGeneral;
        }

    return (err);
    }

TInt
TPolicyParser::parse_conn2saL(CSecurityPolicy* aSp)
    {
    CPolicySelector* csa(NULL);
    TInt err(KErrNone);
    token_type val;
    TUint port(0);

    csa = CPolicySelector::NewL();
    aSp->Add(csa);

    do
        {
        if ((iToken.Compare(_L("dst")) == 0)
            || (iToken.Compare(_L("remote")) == 0))
            {
            err = parse_ip_addr_and_maskL(csa->iRemote,
                                          csa->iRemoteMask,
                                          csa->iRemSelEpName,
                                          csa->iRemMaskEpName,
                                          aSp);
            }
        else if ((iToken.Compare(_L("src")) == 0)
                 || (iToken.Compare(_L("local")) == 0))
            {
            err = parse_ip_addr_and_maskL(csa->iLocal,
                                          csa->iLocalMask,
                                          csa->iLocSelEpName,
                                          csa->iLocMaskEpName,
                                          aSp);
            }
        else if (iToken.Compare(_L("outbound")) == 0)
            {
            if (csa->iDirection != KPolicySelector_SYMMETRIC)
                {
                Error(_L("Only one inbound or outbound allowed"));
                return (KErrGeneral);
                }
            csa->iDirection = KPolicySelector_OUTBOUND;
            }
        else if (iToken.Compare(_L("inbound")) == 0)
            {
            if (csa->iDirection != KPolicySelector_SYMMETRIC)
                {
                Error(_L("Only one inbound or outbound allowed"));
                return (KErrGeneral);
                }
            csa->iDirection = KPolicySelector_INBOUND;
            }
        else if (iToken.Compare(_L("user_id")) == 0)
            {
            ;   // Needs to be examined, TIdentity? -- msa
            }
        else if (iToken.Compare(_L("protocol")) == 0)
            {
            err = Val(csa->iProtocol);
            }
        else if ((iToken.Compare(_L("src_port")) == 0) ||
                 (iToken.Compare(_L("local_port")) == 0))
            {
            err = Val(port);
            csa->iLocal.SetPort(port);
            }
        else if ((iToken.Compare(_L("dst_port")) == 0) ||
                 (iToken.Compare(_L("remote_port")) == 0))
            {
            err = Val(port);
            csa->iRemote.SetPort(port);
            }
        else if (iToken.Compare(_L("icmp_type")) == 0)
            {
            err = Val(csa->iIcmpType);
            }
        else if (iToken.Compare(_L("type")) == 0)
            {
            err = Val(csa->iType);
            }
        else if (iToken.Compare(_L("icmp_code")) == 0)
            {
            err = Val(csa->iIcmpCode);
            }
        else if (iToken.Compare(_L("if")) == 0)
            {
            if (NextToken() != token_string)
                {
                Error(_L("Invalid interface specifier"));
                err = KErrGeneral;
                }
            csa->iInterface.Append(iToken);
            csa->iDirection = KPolicySelector_INTERFACE;
            }
        else if (iToken.Compare(_L("scope:global")) == 0)
            {
            csa->iGlobalSelector = ETrue;
            }
        else if (iToken.Compare(_L("final")) == 0 )
            {
            csa->iIsFinal = ETrue;
            }
        else if (iToken.Compare(_L("merge")) == 0 )
            {
            csa->iIsMerge = ETrue;
            }
        else
            {
            Error(_L("invalid keyword "));
            return (KErrGeneral);
            }

        if (err != KErrNone)
            {
            // iMsg already contains an error text
            if (iMsg.Length() != 0)
                {
                return err;
                }
            Error(_L("Error = %d"), err);
            return err;
            }
        }
    while ((val = NextToken()) == token_string);

    if (val != token_equal )
        {
        Error(_L("Syntax error"));
        err = KErrGeneral;
        }
    else if (NextToken() == token_brace_left)
        {
        err = parse_sa_spec_listL(csa->iBundle, aSp);
        }
    else if (iToken.Compare(_L("drop")) == 0)
        {
        csa->iDropAction = ETrue;
        }
    else
        {
        Error(_L("Syntax error"));
        err = KErrGeneral;
        }
    return (err);
    }

//
// Keys Parsing
//
EXPORT_C CKeysData::CKeysData()
    {}

EXPORT_C CKeysData::CKeysData(CKeysData* aKey)
    {
    sa_type = aKey->sa_type;
    spi = aKey->spi;
    encr_alg = aKey->encr_alg;
    auth_alg = aKey->auth_alg;
    direction = aKey->direction;
    lifetime_bytes = aKey->lifetime_bytes;
    lifetime_sec = aKey->lifetime_sec;
    src_addr = aKey->src_addr;        // Include port
    dst_addr = aKey->dst_addr;        // Include port
    protocol = aKey->protocol;
    auth_key = aKey->auth_key;
    encr_key = aKey->encr_key;
    }

//
//  CKeysDataArray
//
CKeysDataArray::CKeysDataArray(TInt aGranularity) :
        CArrayFixFlat<class CKeysData *>(aGranularity)
    {}

EXPORT_C CKeysDataArray* CKeysDataArray::NewL(TInt aGranularity)
    {
    CKeysDataArray* self = new (ELeave) CKeysDataArray(aGranularity);
    self->Construct(aGranularity);
    return self;
    }

EXPORT_C void CKeysDataArray::Construct(TInt /* aGranularity */)
    {}

CKeysDataArray::CKeysDataArray(CKeysDataArray* aData) :
        CArrayFixFlat<class CKeysData *>(aData->Count())
    {}

EXPORT_C CKeysDataArray* CKeysDataArray::NewL(CKeysDataArray* aData)
    {
    CKeysDataArray* self = new (ELeave) CKeysDataArray(aData);
    CleanupStack::PushL(self);
    self->ConstructL(aData);
    CleanupStack::Pop();
    return self;
    }

EXPORT_C void CKeysDataArray::ConstructL(CKeysDataArray* aData)
    {
    CopyL(aData);
    }

EXPORT_C CKeysDataArray::~CKeysDataArray()
    {
    Empty();
    }

// Construct this from the data in aData
EXPORT_C void CKeysDataArray::CopyL(CKeysDataArray* aData)
    {
    CKeysData* key_data(NULL);
    for (TInt i = 0; i < aData->Count(); i++)
        {
        key_data = new (ELeave) CKeysData(aData->At(i));
        CleanupStack::PushL(key_data);
        AppendL(key_data);
        CleanupStack::Pop();
        }
    }

EXPORT_C void
CKeysDataArray::Empty()
    {
    for (TInt i = 0; i < Count(); i++)
        {
        delete At(i);
        }

    Reset();
    }

//
// TKeyParser
//
EXPORT_C
TKeyParser::TKeyParser(const TDesC &aStr) : TLex(aStr)
    {
    iFirst = 1;
    }

EXPORT_C TInt
TKeyParser::ParseL(CKeysDataArray *aKeys)
    {
    TInt err(KErrNone);

    while (!err)
        {
        // Skip until first token in line
        while (iFirst == 0)
            NextToken();

        if (iFirst < 0)
            break;

        NextToken();
        if ((iToken.Compare(_L("pfkey_add")) == 0))
            {
            TInt val(0);
            CKeysData* keyData(NULL);
            for (int i = 0; !err && iFirst == 0; ++i)
                {
                switch (i)
                    {
                        // sa type: 1=AH, 2=ESP
                    case 0:
                        keyData = new (ELeave) CKeysData;
                        err = Val(val);
                        if (val == 1)
                            keyData->sa_type = SADB_SATYPE_AH;
                        else if (val == 2)
                            keyData->sa_type = SADB_SATYPE_ESP;
                        else
                            err = KErrGeneral;
                        break;

                        // spi: 1..MAX_UINT32
                    case 1:
                        err = Val(keyData->spi);
                        break;

                        // Pass encryption alg numbers as is
                    case 2:
                        err = Val(keyData->encr_alg, EDecimal);
                        break;

                        // Pass authentication alg numbers as is
                    case 3:
                        err = Val(keyData->auth_alg, EDecimal);
                        break;

                        // direction: 4 = inbound, 8 = outbound                        
                    case 4:
                        err = Val(keyData->direction);
                        // Not used, direction is implicit by the src/dst pair
                        if (keyData->direction & ~(PFKEY_INI_INBOUND 
                                                   | PFKEY_INI_OUTBOUND))
                            err = KErrGeneral;
                        break;

                        // lifetime as bytes: 0 = not used, 
                        //                    1..MAX_UINT32=max sa lifetime                        
                    case 5:     
                        err = Val(keyData->lifetime_bytes);
                        break;

                        // lifetime as seconds: 0 = not used, 
                        //                      1..MAX_UINT32=max sa lifetime
                    case 6:     
                        err = Val(keyData->lifetime_sec);
                        break;

                        // src ip addr: in a.b.c.d format                        
                    case 7:     
                        NextToken();
                        err = keyData->src_addr.Input(iToken);
                        break;

                        // dst ip addr: in a.b.c.d format
                    case 8:     
                        NextToken();
                        err = keyData->dst_addr.Input(iToken);
                        break;

                        // protocol: 0 = sa NOT protocol specific,                        
                        //           1 = ICMP, 4 = IPIP, 6 = TCP, 17 = UDP
                    case 9:     
                        err = Val(val);
                        keyData->protocol = (TUint8)val;
                        break;

                        // local port: 0 = sa NOT src port specific,
                        //             1..MAX_UINT16 = src port for which sa 
                        //             dedicated
                    case 10:    
                        err = Val(val);
                        keyData->src_addr.SetPort(val);
                        break;

                        // remote port: 0 = sa NOT dst port specific,
                        //              1..MAX_UINT16 = dst port for which 
                        //              sa dedicated
                    case 11:    
                        err = Val(val);
                        keyData->dst_addr.SetPort(val);
                        break;

                        // authentication key:  as hex string WITHOUT leading 0x,
                        //               two hex digits for every 8 bits of key,
                        //               HMAC-MD5: 128 bit = 16 byte key,
                        //               HMAC-SHA1: 160 bit = 20 byte key
                    case 12:
                        NextToken();
                        if (iToken != _L("0"))
                            {
                            // 0 is No key assigned    
                            keyData->auth_key.Copy(iToken);
                            }
                        break;

                        // encryption key: as hex string WITHOUT leading 0x,                        
                        //             two hex digits for every 8 bits of key,
                        //             DES-CBC: 64 bit = 8 byte key,
                        //             DES-EDE3-CBC: 192 bit = 24 byte key
                    case 13:    
                        NextToken();
                        if (iToken != _L("0"))    //0 is No key assigned
                            keyData->encr_key.Copy(iToken);
                        break;

                    default:
                        NextToken();
                        err = KErrKeyParser;
                        break;
                    } // switch
                SkipSpaceAndMark();
                } // for

            if (err == KErrNone && keyData)
                {
                CleanupStack::PushL(keyData);
                aKeys->AppendL(keyData);
                CleanupStack::Pop();
                }
            else
                {
                delete keyData;
                keyData = NULL;
                }
            }   // if
        }   // while

    return (err);
    }

EXPORT_C TInt 
TKeyParser::Write(CKeysDataArray *aKeys, RFile &aFile)
    {
    TBuf8<500> text;
    TInt err(KErrNone);
    TInt count = aKeys->Count();
    for (TInt i = 0; i < count ; i++)
        {
        TextPFKey(aKeys->At(i), text);
        err = aFile.Write(text);
        if (err != KErrNone)
            break;
        }
    return (err);
    }

void 
TKeyParser::TextPFKey(CKeysData *aKey, TDes8 &aElem)
    {
    TBuf<39> addr;
    TBuf8<39> addr8;

    aElem.Format(_L8("pfkey_add "));

    if (aKey->sa_type == SADB_SATYPE_AH)
        aElem.AppendFormat(_L8("%d "), 1);
    else
        aElem.AppendFormat(_L8("%d "), 2);

    aElem.AppendFormat(_L8("%d "), aKey->spi);

    // Algorithms
    aElem.AppendFormat(_L8("%d "), aKey->encr_alg);
    aElem.AppendFormat(_L8("%d "), aKey->auth_alg);


    aElem.AppendFormat(_L8("%d "), aKey->direction);

    aElem.AppendFormat(_L8("%d "), aKey->lifetime_bytes);
    aElem.AppendFormat(_L8("%d "), aKey->lifetime_sec);

    // Addresses
    aKey->src_addr.OutputWithScope(addr);

    addr8.Copy(addr);
    aElem.AppendFormat(addr8);
    aElem.AppendFormat(_L8(" "));
    aKey->dst_addr.OutputWithScope(addr);

    addr8.Copy(addr);
    aElem.AppendFormat(addr8);
    aElem.AppendFormat(_L8(" "));
    aElem.AppendFormat(_L8("%d "), aKey->protocol);

    // Ports
    aElem.AppendFormat(_L8("%d "), aKey->src_addr.Port());
    aElem.AppendFormat(_L8("%d "), aKey->dst_addr.Port());

    // Keys
    if (aKey->auth_key.Length() != 0)
        aElem.Append(aKey->auth_key);
    else
        aElem.Append(_L8("0"));

    aElem.Append(_L8(" "));

    if (aKey->encr_key.Length() != 0)
        aElem.Append(aKey->encr_key);
    else
        aElem.Append(_L8("0"));

    aElem.Append(_L8("\n"));
    }

//
// Skip white space and mark, including comments!
//
TInt 
TKeyParser::SkipSpaceAndMark()
    {
    TChar ch;
    TInt comment = 0;
    TInt newline = 0;

    while (!Eos())
        {
        ch = Get();
        if (ch == '\n')
            {
            comment = 0;
            newline = 1;
            }
        else if (comment || ch == '#')
            comment = 1;
        else if (!ch.IsSpace())
            {
            UnGet();
            break;
            }
        }
    Mark();
    return newline;
    }

//
// Extract Next token and return
//
void 
TKeyParser::NextToken()
    {
    if (SkipSpaceAndMark())
        iFirst = 1;     // New line!

    if (Eos())
        {
        iFirst = -1;
        return ;
        }

    while (!Eos())
        {
        TChar ch = Peek();
        if (ch == '#' || ch.IsSpace())
            break;
        Inc();
        }
    iToken.Set(MarkedToken());
    iFirst = SkipSpaceAndMark();
    }

TUint8 
TKeyParser::HexVal(TUint8 c)
    {
    if (c >= 'a' && c <= 'f')
        return (TUint8)(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
        return (TUint8)(c - 'A');
    else if (c >= '0' && c <= '9')
        return (TUint8)(c - '0');
    else
        return 0;
    }

TPtrC8 
TKeyParser::DeHex(const TDesC &aStr)
    {
    const TUint8* s = (TUint8 *)aStr.Ptr();
    TUint8* d = (TUint8 *)iHex.Ptr();
    TInt i = aStr.Length();
    TUint8 d1 = 0;
    TUint8 d2 = 0;

    while (i > 0)
        {
        d1 = TKeyParser::HexVal(*s++);
        d2 = i > 1 ? TKeyParser::HexVal(*s++) : (TUint8)0;
        i -= 2;
        *d++ = (TUint8)(d1 * 16 + d2);
        }

    iHex.SetLength(d - iHex.Ptr());
    return iHex;
    }

//
// Parses an security configuration file
// 
EXPORT_C 
TIpSecParser::TIpSecParser(const TDesC &aDes) : TLex(aDes)
    {}

EXPORT_C TInt 
TIpSecParser::ParseL(CIpSecurityPiece *aPiece_data)
    {
    return DoParseL(aPiece_data, ETrue);
    }

EXPORT_C TInt 
TIpSecParser::ParseAndIgnoreIKEL(CIpSecurityPiece *aPiece_data)
    {
    return DoParseL(aPiece_data, EFalse);
    }

TInt 
TIpSecParser::DoParseL(CIpSecurityPiece *aPiece_data,
                            TBool /* aIncludeIKE */)
    {
    TPtrC token(NULL, 0);
    TInt ret(0);

    if (!CheckVersion())
        return KErrNotSupported;    // Invalid file or version

    while (!Eos())
        {
        token.Set(NextToken());
        if (token.Compare(_L("[INFO]")) == 0)
            {
            ParseInfoL(aPiece_data);
            }
        else if (token.Compare(_L("[POLICY]")) == 0)
            {
            ret = ParsePoliciesL(aPiece_data);
            if (ret != KErrNone)
                return ret;
            }
        else if (token.Compare(_L("[KEYS]")) == 0)
            {
            ret = ParseKeysL(aPiece_data->Keys());
            if (ret != KErrNone)
                return ret;
            }
        else
            {
            // Unknown Tag Ignored
            NextTag();
            }
        }

    return (KErrNone);
    }

TBool 
TIpSecParser::CheckVersion()
    {
    TPtrC token(NULL, 0);
    TLex version_num;

    token.Set(NextToken());
    if (token.Compare(_L("SECURITY_FILE_VERSION:")) == 0)
        {
        version_num = NextToken();
        if (version_num.Val(iVersion) != KErrNone)
            return EFalse;
        if ((iVersion < FIRST_SEC_PARSER_VERSION) ||
            (iVersion > SEC_PARSER_VERSION))
            return EFalse;
        }
    else
        return EFalse;

    return ETrue;

    }

void 
TIpSecParser::ParseInfoL(CIpSecurityPiece *aPiece_data)
    {
    HBufC *buf = HBufC::NewL(MAX_INFO_SIZE);
    TPtr ptr = buf->Des();
    TChar ch = Get();
    TInt i(0);

    CleanupStack::PushL(buf);

    ch = Get();
    while (((ch == ' ') || (ch == '\n')) && (!Eos()))
        {
        ch = Get();
        }

    while ((ch != '[') && (!Eos()) && i < MAX_INFO_SIZE)
        {
        ptr.Append(ch);
        i++;
        ch = Get();
        }

    if (i == MAX_INFO_SIZE) //The rest is ignored
        {
        ch = Get();
        while ( (ch != '[') && (!Eos()) )
            ch = Get();
        }

    if (ch == '[')
        {
        UnGet();    // the '['
        if (ptr.Length() > 0)   //If empty no \n
            ptr.SetLength(ptr.Length() - 1);    //eliminates the \n at the end
        }

    aPiece_data->SetInfoL(ptr);
    CleanupStack::PopAndDestroy();
    }

TInt 
TIpSecParser::ParsePoliciesL(CIpSecurityPiece *aPieceData)
    {
    TInt err;
    TInt pos = Remainder().Find(_L("[KEYS]"));
    if (pos == KErrNotFound)
        {
        pos = Remainder().Find(_L("["));   //The segment is until the next tag or Eos()
        }
    if (pos != KErrNotFound)
        {
        TPtr pol_ptr((TUint16 *)Remainder().Ptr(), pos, pos);    //Until the next section
        TPolicyParser parser(pol_ptr);
        err = parser.ParseL(aPieceData);
        Assign(Remainder().Ptr() + pos);    //rest of the text to parse
        }
    else
        {
        TPolicyParser parser(Remainder());
        err = parser.ParseL(aPieceData);
        }
    return (err);
    }

TInt 
TIpSecParser::ParseKeysL(CKeysDataArray *aKeys)
    {
    TInt err;
    //The segment is until the next tag or Eos()
    TInt pos = Remainder().Find(_L("["));
    if (pos != KErrNotFound)
        {
        // Until the next section
        TPtr key_ptr((TUint16 *)Remainder().Ptr(), pos, pos);
        TKeyParser parser(key_ptr);
        err = parser.ParseL(aKeys);

        // Rest of the text to parse
        Assign(Remainder().Ptr() + pos);
        }
    else
        {
        // No more tags
        TKeyParser parser(Remainder());
        err = parser.ParseL(aKeys);
        }

    return (err);
    }

void 
TIpSecParser::NextTag()
    {
    while (!Eos())
        if (Get() == '[' )
            {
            // Next tag found
            UnGet();
            return ;
            }
    }

// Puts the security file data into string format to be saved to the
// caller's buffer.
EXPORT_C TInt 
TIpSecParser::Write(CIpSecurityPiece* aPiece_data,
                    HBufC8*& aPolBfr)
    {
    TInt err(KErrNone);

    err = WriteVersion(aPolBfr);
    if (err != KErrNone)
        return err;

    err = WriteInfo(aPiece_data, aPolBfr);
    if (err != KErrNone)
        return err;

    err = WritePolicies(aPiece_data, aPolBfr);
    if (err != KErrNone)
        return err;

    return (err);
    }

TInt 
TIpSecParser::WriteVersion(HBufC8*& aPolBfr)
    {
    TBuf8<32> buf;
    buf.Format(_L8("SECURITY_FILE_VERSION: %d\n"), SEC_PARSER_VERSION);
    return TPolicyParser::BufferAppend(aPolBfr, buf);
    }

TInt 
TIpSecParser::WriteInfo(CIpSecurityPiece *aPiece_data, HBufC8*& aPolBfr)
    {
    TInt err;

    TBuf8<MAX_INFO_SIZE> buf = _L8("[INFO]\n");
    err = TPolicyParser::BufferAppend(aPolBfr, buf);
    if (err != KErrNone)
        return err;

    buf.Copy(aPiece_data->Info()->Des());
    err = TPolicyParser::BufferAppend(aPolBfr, buf);
    if (err != KErrNone)
        return err;
    return TPolicyParser::BufferAppend(aPolBfr, (_L8("\n")));

    }

TInt 
TIpSecParser::WritePolicies(CIpSecurityPiece *aPiece_data, HBufC8*& aPolBfr)
    {
    TBuf8<10> buf = _L8("[POLICY]\n");
    TInt err = TPolicyParser::BufferAppend(aPolBfr, buf);
    if (err != KErrNone)
        return err;
    return TPolicyParser::Write(aPiece_data->Policies(), aPolBfr);
    }

//
// CIpSecurityPiece
//
EXPORT_C void 
CIpSecurityPiece::ConstructL(TInt aSize)
    {
    iInfo = HBufC::NewL(aSize);
    iPolicies = new (ELeave) CSecurityPolicy();
    iPolicies->ConstructL();
    iKeys = CKeysDataArray::NewL(1);
    }

EXPORT_C void 
CIpSecurityPiece::SetInfoL(const TDesC &aDes)
    {
    if (aDes.Length() > iInfo->Des().MaxLength())
        {
        // ReAllocs if needed
        iInfo = iInfo->ReAllocL(aDes.Length());
        }

    iInfo->Des().Copy(aDes);
    }

EXPORT_C 
CIpSecurityPiece::~CIpSecurityPiece()
    {
    delete iInfo;
    delete iPolicies;
    delete iKeys;
    }

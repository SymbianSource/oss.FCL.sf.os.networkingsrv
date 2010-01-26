// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32base.h>
#include "T_HTTPOB.H"


THTTPMessage::THTTPMessage():iVers(_L8("HTTP/1.1"))
	{
	iFieldsNumb= 0;
	iCur= 0;
	}
	
void THTTPField::Get(TDes8& aBuf)	
	{	
	aBuf.Copy(iFld);	
	aBuf.Append(_L(": "));	
	aBuf.Append(iVal);	
	aBuf.Append(_L("\r\n"));	
	}	

void THTTPField::Field(TDes8& aBuf)	
	{	
	aBuf.Copy(iFld);	
	}
	
void THTTPField::Value(TDes8& aBuf)	
	{	
	aBuf.Copy(iVal);
	}

TInt THTTPMessage::ParseMessage( TDesC8& aMes)	
	{	
	TBuf8<512> tk;	
	iCur=0;	
	NextToken(aMes,tk);	
	if(tk.Compare(iVers)) return -1;	
	NextToken(aMes,tk);//status	
	iStat.Copy(tk);	
	NextToken(aMes,tk);//comment	
	iPhrase.Copy(tk);	
	TBuf8<2048> val;	
	iFieldsNumb=0;	
	while(!NextMessHeader(aMes,tk,val))	
		iFlds[iFieldsNumb++].Set(tk,val);	

	return 0;	
	}

void THTTPMessage::AddHeaderField(const TDesC8& aField,const TDesC8& aVal)
	{
	iFlds[iFieldsNumb++].Set(aField,aVal);
	}
void THTTPMessage::GetHeader(TDes8& aBuf)
	{
	aBuf.Copy(iMeth);
	aBuf.Append(' ');
	aBuf.Append(iURI);
	aBuf.Append(' ');
	aBuf.Append(iVers);
	aBuf.Append(_L("\r\n"));
	TBuf8 <256> tmp;
	for(TInt i=0;i<iFieldsNumb;i++)
		{
		iFlds[i].Get(tmp);
		aBuf.Append(tmp);
		}
	aBuf.Append(_L("\r\n"));
	}
TInt THTTPMessage::FindField(const TDesC8& aTag, TDes8& aVal)
	{
	TBuf8<256> tmp;
	TInt find=0;
	for(TInt i=0;i<iFieldsNumb;i++)
		{
		iFlds[i].Field(tmp);
		if(!aTag.Compare(tmp)) 
			{
			find=1;
			iFlds[i].Value(aVal);
		//	aVal.Copy(tmp);
			break; //in the case of muliple fields same tag need special treatment
			}
		}
	return find;
	}

TInt THTTPMessage::NextMessHeader(TDesC8& aSrc,TDes8& aField,TDes8& aVal)
	{//!!simplification won't work if not <field>: <value> forme
	NextToken(aSrc,aField);
	if (!aField.Length()) return -1;
	aField.SetLength(aField.Length()-1);// to cut the ":"
	Content(aSrc,aVal);
	return 0;
	}
TInt THTTPMessage::NextToken(TDesC8& aSrc, TDes8& aTok)
	{
	aTok.Zero();
	if (aSrc[iCur]==' ' && aSrc.Length()>iCur) iCur++;
	while(aSrc.Length()>(iCur+3) && aSrc[iCur]!=' ' && aSrc[iCur]!='\r' )
		aTok.Append(aSrc[iCur++]);
	if(aSrc[iCur]=='\r') iCur++;
	if(aSrc[iCur]=='\n') iCur++;

	return 0;
	}

TInt THTTPMessage::Content(TDesC8& aSrc, TDes8& aCont)
	{
	aCont.Zero();
	if (aSrc[iCur]==' ' && aSrc.Length()>iCur) iCur++;
	while(aSrc[iCur]!='\r' && aSrc.Length()>(iCur+3))
		aCont.Append(aSrc[iCur++]);
	if(aSrc[iCur]=='\r') iCur++;
	if(aSrc[iCur]=='\n') iCur++;
	return 0;
	}



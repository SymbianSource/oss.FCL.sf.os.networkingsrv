// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if (!defined __TEHTTPPACKET_H__)
#define __TEHTTPPACKET_H__

const TInt KNumberOfHTTPField = 10;

class THTTPField
	{
public:
	void Set(const TDesC8& aF,const TDesC8& aV){iFld=aF;iVal=aV;};
	void Get(TDes8& aBuf);
	void Field(TDes8& aBuf);
	void Value(TDes8& aBuf);
private:
	TBuf8<64> iFld;
	TBuf8<512> iVal;
	};

class THTTPMessage
	{
public:
	THTTPMessage();
	void Method(const TDesC8& aMet){iMeth=aMet;};
	void URI(const TDesC8& aUri){iURI=aUri;};
	void RespStatus( TDes8& aStat){aStat=iStat;};
	void ReasonPhrase( TDes8& aP){aP=iPhrase;};
	TInt ParseMessage(const TDesC8& aMes);
public:
	void AddHeaderField(const TDesC8& aField,const TDesC8& aVal= TPtrC8(0,0));
	void GetHeader(TDes8& aBuf);
	TBool FindField(const TDesC8& aTag, TDes8& aVal);
private:
	TInt NextMessHeader(const TDesC8& aSrc,TDes8& aField,TDes8& aVal);
	TInt NextToken(const TDesC8& aSrc, TDes8& aTok);
	TInt Content(const TDesC8& aSrc, TDes8& aCont);
private:
	const TBuf8<8> iVers;
	TBuf8<16> iMeth;
	TBuf8<128> iURI;
	TBuf8<128> iPhrase;
	TBuf8<8> iStat;
	TInt iFieldsNumb;
	TInt iCur;
	THTTPField iFlds[KNumberOfHTTPField];
	};

#endif

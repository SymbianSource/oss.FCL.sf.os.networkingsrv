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
// ws_mes.cpp - http server message handler
//

#include "ws_mes.h"

//********************************************** HTTP MESSAGE HEADER ***************************************************

CHttpMessageHeader::CHttpMessageHeader()
{

}

CHttpMessageHeader::~CHttpMessageHeader()
{
	DestroyAllHeaders();
}


void CHttpMessageHeader::DestroyAllHeaders()
{
	//Destroying General-Header fields

	delete	iCacheControl;
	delete	iConnection;
	delete	iDate;
	delete	iPragma;
	delete	iTrailer;
	delete	iTransferEncoding;
	delete	iUpgrade;
	delete	iVia;
	delete	iWarning;
	
	// Destroying Entity-Header fields
	delete	iAllow;
	delete	iContentEncoding;
	delete	iContentLanguage;
	delete	iContentLength;
	delete	iContentLocation;	
	delete	iContentMD5;
	delete	iContentRange;
	delete	iContentType;
	delete	iExpires;
	delete	iLastModified;
	
	// Destroying Request-header fields
	delete	iAccept;
	delete	iAcceptCharset;
	delete  iAcceptRanges;
	delete	iAcceptEncoding;
	delete	iAcceptLanguage;
	delete	iAuthorization;
	delete	iExcept;
	delete	iFrom;
	delete	iHost;
	delete	iIfMatch;
	delete	iIfModifiedSince;
	delete  iIfNoneMatch;
	delete	iIfUnModifiedSince;
	delete  iIfRange;
	delete	iMaxForwards;
	delete	iProxyAuthorization;
	delete	iRange;
	delete	iReferer;
	delete	iTE;
	delete	iUserAgent;

	//Destroying Response-Header fields
	delete	iAges;
	delete	iETag;
	delete	iLocation;
	delete	iProxyAuthenticate;
	delete	iRetryAfter;
	delete	iServer;
	delete	iVary;
	delete	iWWWAuthenticate;

	delete	 iExtensionHeader;


	delete iRequestMessageBody;

	iCacheControl = 0;
	iConnection = 0;
	iDate = 0;
	iPragma = 0;
	iTrailer = 0;
	iTransferEncoding = 0;
	iUpgrade = 0;
	iVia = 0;
	iWarning = 0;

	iAllow = 0;
	iContentEncoding = 0;
	iContentLanguage = 0;
	iContentLength = 0;
	iContentLocation = 0;	
	iContentMD5 = 0;
	iContentRange = 0;
	iContentType = 0;
	iExpires = 0;
	iLastModified = 0;

	iAccept = 0;
	iAcceptCharset = 0;
	iAcceptRanges = 0;
	iAcceptEncoding = 0;
	iAcceptLanguage = 0;
	iAuthorization = 0;
	iExcept = 0;
	iFrom = 0;
	iHost = 0;
	iIfMatch = 0;
	iIfModifiedSince = 0;
	iIfNoneMatch = 0;
	iIfUnModifiedSince = 0;
	iIfRange = 0;
	iMaxForwards = 0;
	iProxyAuthorization = 0;
	iRange = 0;
	iReferer = 0;
	iTE = 0;
	iUserAgent = 0;

	iAges = 0;
	iETag = 0;
	iLocation = 0;
	iProxyAuthenticate = 0;
	iRetryAfter = 0;
	iServer = 0;
	iVary = 0;
	iWWWAuthenticate = 0;

 	iExtensionHeader = 0;

	iRequestMessageBody = 0;

}


//******************************************* Request-Header functions ***********************************************


TInt CHttpMessageHeader::CheckAccept(const TDesC& aContentType) const
// In this implementation the quality values are ignored.
{
	TInt err = KErrNone;
// I must check if there is q = 0 values in the Accept field.
	if (iAccept != 0)
	{
		err =iAccept->Find(_L("*/*"));
		if (err == KErrNotFound)
		{
			err = iAccept->Find(aContentType);
			if (err == KErrNotFound)
			{
				TInt pos = aContentType.Locate('/');
				TBuf<256> aux = aContentType.Left(pos);
				aux.Append(_L("*"));
				err = iAccept->Find(aux);
			}
		}
		
		if (err > KErrNotFound)
			err = KErrNone;

	}

	return err;
}

TInt CHttpMessageHeader::CheckAuthorization() const
{
	if (iAuthorization == 0) return KErrAccessDenied;
	else return KErrNone;
}


//****************************************** Response-Header functions *******************************************

void CHttpMessageHeader::SetAcceptRangesL()
// This header will change in future version. In this version we don't allow ranges.
// Set the Accept-Ranges field.
{
	delete iAcceptRanges;
	iAcceptRanges = _L("bytes").AllocL();

}

void CHttpMessageHeader::SetAcceptEncodingL()
// This header will change in future version. In this version we don't allow any encoding.
// Set the Accpet-Encodinfg field.
{
	delete iAcceptEncoding;
	iAcceptEncoding = _L(" ").AllocL();
}

void CHttpMessageHeader::SetServerL()
// Sets the Sever field.
{
	delete iServer;
	iServer = SERVER_NAME.AllocL();
}


//****************************************** Entity-Header functions *******************************************

void CHttpMessageHeader::SetAllowL(TMethodType aMethod)
// Note: This header should change in the future. The argument it has to be an array of Method types.
// Set the methods allowed in the Allow field.
{
	delete iAllow;

	switch(aMethod)
	{
	case EAll:
		iAllow = _L("GET, HEAD, PUT, POST, OPTIONS, TRACE, DELETE").AllocL();
		break;
	case EGet:
		iAllow = _L("GET").AllocL();
		break;
	case EHead:
		iAllow = _L("HEAD").AllocL();
		break;
	case EPut:
		iAllow = _L("PUT").AllocL();
		break;
	case EPost:
		iAllow = _L("POST").AllocL();
		break;
	case EOptions:
		iAllow = _L("OPTIONS").AllocL();
		break;
	case ETrace:
		iAllow = _L("TRACE").AllocL();
		break;
	case EDelete:
		iAllow = _L("DELETE").AllocL();
		break;
	case ENoneMethod:
	default:
		iAllow = _L(" ").AllocL();
		break;
	}
}


void CHttpMessageHeader::SetContentLengthL(TUint aSize)
// Set the content-length field.
{
	TBuf<MAX_CL_LEN> aux;
	
	aux.Num(aSize);
	delete iContentLength;
	iContentLength = aux.AllocL();


}
void CHttpMessageHeader::SetContentRangeL(TInt aFirstPos,TInt aLastPos,TInt aSize)
{
	TBuf<MAX_HEADER_LEN> aux;
	if ((aFirstPos < aSize) || (aFirstPos <= aLastPos))
	{
		aux.Format(_L("bytes %d - %d/%d"),aFirstPos,aLastPos,aSize);
	}
	else
		aux = _L("*");
	
	delete iContentRange;
	iContentRange = aux.AllocL();
}

void CHttpMessageHeader::SetETagL(const TInt64& aETag)
// Set the ETag field. (We can use the time-stamp as unique identifier, it is enough to idetify uniquely any file
// (we ha a resolution of microseconds))
{
	TBuf<MAX_ETAG_LEN> auxtag;
#ifdef I64LOW
	auxtag.AppendFormat(_L("\"%x%x\""),I64HIGH(aETag),I64LOW(aETag));
#else
	auxtag.AppendFormat(_L("\"%x%x\""),aETag.High(),aETag.Low());
#endif
	delete iETag;
	iETag = auxtag.AllocL();
}

void CHttpMessageHeader::SetLastModifiedL(const TTime& aDate)
// Set the Last-Modified field
{
	TBuf<MAX_HEADER_LEN> StringDate;

	aDate.FormatL(StringDate,(_L("%F%*E, %D %*N %Y %H:%T:%S GMT")));
	
	delete iLastModified;
	iLastModified  = StringDate.AllocL();

}

void CHttpMessageHeader::SetRangeValuesL(TInt& aStart,TInt& aEnd)
// This functions initializes aStart and aEnd with the values of the actual range.
{
	ASSERT(iRange != NULL);

	TBuf<MAX_HEADER_LEN> range,token;
	range = iRange->Des();
	TLex lex(range);
	TLex lex2;


	lex.SkipSpaceAndMark();
	lex.SkipCharacters();
	token = lex.MarkedToken();
	
	if (token.Find(_L("bytes=")) == 0)
	{
		lex.UnGetToMark();
		lex.Inc(6); // Skip 'bytes='
	}
	else if (token.Compare(_L("bytes")) == 0)
	{
		lex.SkipSpace(); // Skip all the spaces until '='.
		lex.Inc(); //Skip '='.
	}
	else
		lex.UnGetToMark();
	
	if(!lex.Eos() && (lex.Peek() == ',')) lex.Inc();
	lex.SkipSpaceAndMark();
	while (!lex.Eos() && lex.Peek()!=',')
	{
		lex.Inc();
	}
	lex2 = lex.MarkedToken();
	lex2.SkipSpace();
	lex2.Val(aStart);

	lex2.SkipSpace();
	lex2.Inc(); // Skip '-'
	lex2.SkipSpace();
	lex2.Val(aEnd);
	delete iRange;
	if (!lex.Eos())
		iRange = lex.Remainder().AllocL();
	else
		iRange = 0;
}

//**************************************** This function is used in CheckRange function. ****************************
static TInt ByteRangeSpec(TLex& aLex,TInt aSize)
{
	TInt err,firstbyte,lastbyte;

	aLex.SkipSpace();
	err = aLex.Val(firstbyte);
	if (err == KErrNone && firstbyte >= 0)
	{
		aLex.SkipSpace();
		if (!aLex.Eos() && aLex.Get()=='-')
		{
			aLex.SkipSpace();
			if (!aLex.Eos() && aLex.Peek().IsDigit())
			{
				aLex.SkipSpace();
				err = aLex.Val(lastbyte);
				if (err == KErrNone && lastbyte >= 0)
				{
					aLex.SkipSpace();
					if (!aLex.Eos() && aLex.Get() != ',') err = KErrArgument;
					else
					{
						if (firstbyte > lastbyte) err = KErrNotSatisfiable;
						if (firstbyte >=  aSize) err = KErrNotSatisfiable;
					}
				}
				else
					err = KErrArgument;
			}
			else
			{
				if (!aLex.Eos() && aLex.Get() != ',') err = KErrArgument;
			}
		}
		else
			err = KErrArgument;
	}
	else
		err = KErrArgument;

	return err;

}
//*************************************************************************************************************************

TInt CHttpMessageHeader::CheckRange(TInt aSize)
// Check the Range field. If it is syntantically invalid, the field is deleted, then it will be ignored.(see RFC 2616)
// If the range is not satisfiable then an error is returned.
// Errors:
// + KErrNotSatisfiable => Range Not Satisfiable;
// + KErrArgument => Range field is IGNORED !
// + KErrNone => Everything is ok.
{
	TInt err = KErrNone;
	TBool NotSatisfiable = FALSE;

	if (iRange != 0)
	{
		TInt firstbyte;
		TBuf<256> auxbuffer = iRange->Des();
		TLex lex(auxbuffer);
		TBuf<MAX_RANGE_TOKEN> aux;

		while (!lex.Eos() && (err == KErrNone || err == KErrNotSatisfiable))
		{
			lex.SkipSpaceAndMark();
			if (lex.Peek().IsDigit())
			{
				err = lex.Val(firstbyte);
				if (err == KErrNone)
				{
					lex.UnGetToMark();
					err = ByteRangeSpec(lex,aSize);
				}
			}
			else if (lex.Peek()=='-')  // Suffix entry
			{
				err = lex.Val(firstbyte);
				if (firstbyte == 0) err = KErrNotSatisfiable;
				lex.SkipSpace();
				if (!lex.Eos() && lex.Get() != ',') err = KErrArgument;
			}
			else
			{
				lex.SkipCharacters();
				if (lex.TokenLength() <= MAX_RANGE_TOKEN)
				{
						aux = lex.MarkedToken();
						
						if (aux.Find(_L("bytes=")) == 0)
						{
							lex.UnGetToMark();
							lex.Inc(6);
							err = ByteRangeSpec(lex,aSize);
						}
						else if (aux.Find(_L("bytes")) == 0)
						{
							lex.SkipSpace();
							TChar auxchar = lex.Get();
							if (auxchar == '=')
							{
								err = ByteRangeSpec(lex,aSize);
							}
							else
								err = KErrArgument;
						}
						else
							err = KErrArgument;

				}
				else
					err = KErrArgument;
			}	
			lex.SkipSpace();
			NotSatisfiable = (err == KErrNotSatisfiable);
		}
		
		if (err == KErrArgument)
		{
			delete iRange;
			iRange = 0;
			err = KErrNone;
		}
		else
		{
			if (NotSatisfiable) err = KErrNotSatisfiable;
		}
	}

	return err;
}

TBool CHttpMessageHeader::ExistsIfRange() const
{
	return (iIfRange != 0);
}

TBool CHttpMessageHeader::ExistsRange() const
{
	return (iRange != 0);
}

TInt CHttpMessageHeader::GetContentLength()
{
	TInt result = 0;
	TInt err = KErrNone;
	
	if (iContentLength != 0)
	{
		TBuf<MAX_HEADER_LEN> aux;
		aux = iContentLength->Des();
		TLex convert(aux);
		convert.SkipSpace();
		if ((err = convert.Val(result)) == KErrNone) return result;
	}
	else
		err = KErrNotFound;
	

	return err;
}

TBool CHttpMessageHeader::IfMatch(const TInt64& aEntityTag) const
// This function is used to know if one of Entity Tags in IfMatch matches whith the entity tag of the current entity.
// This version doesn't care about weak or strong entity tags !!!!
{
	if (iIfMatch == 0) return TRUE; //Optional field. If it doesn't exist then by default TRUE.
	else
		if (iIfMatch->Find(_L("*")) >= KErrNone) return TRUE;
		else
		{
			TBuf<18> aux;
#ifdef I64LOW
			aux.Format(_L("\"%x%x\""),I64HIGH(aEntityTag),I64LOW(aEntityTag));
#else
			aux.Format(_L("\"%x%x\""),aEntityTag.High(),aEntityTag.Low());
#endif
			return iIfMatch->Find(aux) > KErrNotFound;
		}
}

TBool CHttpMessageHeader::IfModSince(const TTime& aDate)
// This function is used to check if the URI has been modified since the last request.
{
	TTime auxdate;

	if (iIfModifiedSince == 0) return TRUE;
	else
	{
		TLex lex(iIfModifiedSince->Des());
		//Skip the day of the week.
		lex.Mark();
		lex.SkipCharacters();
		lex.Mark();
		
		TPtrC aux = lex.RemainderFromMark();
			
		if (iIfModifiedSince->Find(_L(" GMT")) != KErrNotFound)
		{
			if (auxdate.Parse(aux.Mid(0,aux.Length() - 4)) < KErrNone)
				return TRUE; //Bad format.(Ignore the field)
		}
		else
		{
			if (auxdate.Parse(aux) < KErrNone)
				return TRUE ; //Bad format.(Ignore the field)
		}
		// This conversion is needed because the time-stamp in the file has microsecond
		// and the time in the HTTP message hasn't !!!
		// We have to compare from seconds.

		TInt64 tadate,tauxdate;
		tadate = aDate.Int64()/1000000;
		tauxdate = auxdate.Int64()/1000000;

		return (tadate > tauxdate);
	}

}

TBool CHttpMessageHeader::IfNoneMatch(const TInt64& aEntityTag)
// This function is used to know if one of Entity Tags in IfNoneMatch matches whith the entity tag of the current entity.
// This version doesn't care about weak or strong entity tags !!!!
{
	if (iIfNoneMatch == 0) return TRUE; //Optional field. If it doesn't exist then by default TRUE.
	else
		if (iIfNoneMatch->Find(_L("*")) >= KErrNone) return FALSE;
		else
		{
				TBuf<18> aux;
#ifdef I64LOW
				aux.Format(_L("\"%x%x\""),I64HIGH(aEntityTag),I64LOW(aEntityTag));
#else
				aux.Format(_L("\"%x%x\""),aEntityTag.High(),aEntityTag.Low());
#endif
				if (iIfNoneMatch->Find(aux) == KErrNotFound)
				{
					delete iIfModifiedSince;
					iIfModifiedSince = 0;
					return TRUE;
				}
				else
					return FALSE;
		}
}

TBool CHttpMessageHeader::IfUnModSince(const TTime& aDate)
{
	TTime auxdate;
	if (iIfUnModifiedSince == 0) return TRUE;
	else
	{
		TLex lex(iIfUnModifiedSince->Des());
		//Skip the day of the week.
		lex.Mark();
		lex.SkipCharacters();
		lex.Mark();
		
		TPtrC aux = lex.RemainderFromMark();
			
		if (iIfUnModifiedSince->Find(_L(" GMT")) != KErrNotFound)
		{
			if (auxdate.Parse(aux.Mid(0,aux.Length() - 4)) < KErrNone) return TRUE; //Bad format.(Ignore the field)
		}
		else
		{
			if (auxdate.Parse(aux) < KErrNone) return TRUE ; //Bad format.(Ignore the field)
		}
		// This conversion is needed because the time-stamp in the file has microsecond
		// and the time in the HTTP message hasn't !!!
		// We have to compare from seconds.
		TInt64 tadate,tauxdate;
		tadate = aDate.Int64()/1000000;
		tauxdate = (auxdate.Int64()/1000000);

		return (tadate <= tauxdate);
	}
}

TBool CHttpMessageHeader::IfRange(const TInt64& aEtag, const TTime& aDate)
{
	
	if (iIfRange == 0)
		return TRUE;
	else
	{
		if (iIfRange->Locate('"') > KErrNotFound)
		{
			TBuf<18> auxbuffer;
#ifdef I64LOW
			auxbuffer.Format(_L("\"%x%x\""),I64HIGH(aEtag),I64LOW(aEtag));
#else
			auxbuffer.Format(_L("\"%x%x\""),aEtag.High(),aEtag.Low());
#endif
			return (iIfRange->Compare(auxbuffer) == 0);
		}
		else
		{
			TTime auxdate;
			TLex lex(iIfRange->Des());
			//Skip the day of the week.
			lex.Mark();
			lex.SkipCharacters();
			lex.Mark();
			
			TPtrC aux = lex.RemainderFromMark();
				
			if (iIfRange->Find(_L(" GMT")) != KErrNotFound)
				auxdate.Parse(aux.Mid(0,aux.Length() - 4));
			else
				auxdate.Parse(aux);

			return (aDate <= auxdate);
		}
	}

}

TBool CHttpMessageHeader::IsMultiRange() const
{
	if (iRange == 0)
		return FALSE;
	else
		return (iRange->Locate(',') > KErrNotFound);
}

TBool CHttpMessageHeader::MoreRanges() const
{
	return (iRange != 0);
}

//****************************************** General-Header functions *******************************************

void CHttpMessageHeader::SetDateL()
// Set the Date.
{
	TTime time;
	TBuf<MAX_HEADER_LEN> StringDate;

	time.UniversalTime();
	time.FormatL(StringDate,(_L("%F%*E, %D %*N %Y %H:%T:%S GMT")));
	
	delete iDate;
	iDate = StringDate.AllocL();

}

//********************************************* END HTTP MESSAGE HEADER ************************************************



//********************************************** HTTP MESSAGE **********************************************************

CHttpMessage::CHttpMessage()
{
}

CHttpMessage::~CHttpMessage()
{
	DestroyRequestLine();
	DestroyResponseLine();
}



//******************************* Local functions ************************************

static void IncUntilChar(TLex& aLex, const TChar& aChar)
{
	while (!aLex.Eos() && (aLex.Peek()!= aChar))
	{
		aLex.Inc();
	}
}

//**************************************************************************************

//***************************** Private Functions **************************************
void CHttpMessage::ChangeHexL()
{
	TPtr aux = iURI->Des();
	TBuf<256> aux2;
	TChar auxchar;
	TUint code;

	TLex lex(aux);

	while (!lex.Eos() && (aux2.Length() < aux2.MaxLength()))
	{
		auxchar = lex.Get();
		if (auxchar == '%')
		{
			if ((!lex.Eos()) && (lex.Peek().IsHexDigit()))
			{
				lex.Inc();
				if ((!lex.Eos()) && (lex.Peek().IsHexDigit()))
				{
					lex.Inc();
					if ((!lex.Eos()) && (lex.Peek().IsHexDigit()))
					{
						lex.Inc(-2);
						lex.Mark();
						lex.Inc(2);
						TBuf<2> auxbuffer = lex.MarkedToken();
						TLex auxlex(auxbuffer);
						auxlex.Val(code,EHex);
						auxchar = code;
					}
					else
					{
						lex.Inc(-2);
						lex.Val(code,EHex);
						auxchar = code;
					}
				}
				else
					lex.Inc(-1);
			}

		}
		aux2.Append(auxchar);
	}

	delete iURI;
	iURI = aux2.AllocL();
}

void CHttpMessage::ChangeSlash()
// This funxtion changes the slash ´/´ to ´\´ in the iURI.
{
	TPtr aux = iURI->Des();
	TInt pos;

	TLex lex(aux);

	while (!lex.Eos())
	{
		if (lex.Peek() == '/')
		{
			pos = lex.Offset();
			aux.Replace(pos,1,_L("\\"));
		}
		lex.Inc();
	}
}

TInt CHttpMessage::CheckVersion()
{
	TInt err = KErrBadName,major,minor;

	if (iVersion->Find(_L("HTTP/")) == 0)
	{
		TLex lex(iVersion->Des());
		lex.Inc(5);
		if ((lex.Val(major) >= KErrNone) && (major >= 0))
		{
			if (lex.Peek() == '.')
			{
				lex.Inc();
				if ((lex.Val(minor) >= KErrNone) && (minor >= 0))
				{
					if (lex.Eos())
					{
						if (((major == 0) && (minor == 9)) ||
							((major == 1)))							
							err = KErrNone;
						else  // (major>1)
							err = KErrNotSupported;
					}
					
				}
			}
		}
	}

	return err;
}

void CHttpMessage::DestroyRequestLine()
{
	// destroying request-line fields
	delete iMethod;
	delete iURI;
	delete iVersion;
	
	iMethod = 0;
	iURI = 0;
	iVersion = 0;

}

void CHttpMessage::DestroyResponseLine()
{

	// destroying response-line fields
	delete iStatusCode;
	delete iReasonPhrase;

	iStatusCode = 0;
	iReasonPhrase = 0;

}

//***************************************************************************************************************


TInt CHttpMessage::CheckHost() const
{
	ASSERT(iVersion != NULL);
	
	if ((iVersion->CompareF(_L("HTTP/1.1")) == 0))
	{
		if (iHost == 0) return KErrNotFound;	
	}

	return KErrNone;
}

TInt CHttpMessage::CheckRequestLine()
{
	TInt err = KErrNone;
	TMethodType method;
	if ((iMethod == 0) ||
	    (iURI == 0)    ||
	    (iVersion == 0))
	{
			err = KErrBadName; //Bad Request.
	}
	else
	{
		method = GetMethod();
		if (method != ENoneMethod)
		{
			err = CheckURI();
			if (err == KErrNone)
			{
				err = CheckVersion();
				if (err == KErrNone)
				{
					if (((iVersion->Compare(_L("HTTP/0.9")) == 0) && (method != EGet)) ||
						((iVersion->Compare(_L("HTTP/1.0")) == 0) && (method != EGet && method != EHead && method != EPost)))
					{					
						err = KErrBadName; // Bad Request.
					}
				}
			}
		}
		else
			err = KErrNotSupported; // Method unknown => Not Supported.
	}

	return err;
}

TInt CHttpMessage::CheckURI() const
// Check if the request is safe. (For example. The Server mustn't allow this URI /www.epocserver.fi/../../etc/passwd)
{	
	TParse parse;
	TInt err = KErrNone;
	TBuf<MAX_URI_LEN> resource;

	ASSERT(iURI != NULL);

	if (iURI->Length() > MAX_URI_LEN)
		err = KErrArgument;
	else
	{
		GetAbsPath(resource);
		resource.Insert(0,_L("\\"));
		err = parse.Set(resource,NULL,NULL);
		if (err == KErrNone)
		{
			if ((parse.IsWild()) && (GetMethod() != EOptions)) //|| (resource.Find(_L("\\..")) != KErrNotFound))
				err = KErrBadName;	
		}
		else
			err = KErrBadName;
	}	

	return err;
	
}

void CHttpMessage::CleanMessage()
// Clean the fields.
{
	
	DestroyResponseLine();

	//This code must go in a function from CHTTMessageHeader !!!!!
	delete iDate;
	iDate = 0;
	delete iContentRange;
	iContentRange = 0;
	delete iContentType;
	iContentType = 0;
	delete iContentLength;
	iContentLength = 0;
	delete iAcceptEncoding;
	iAcceptEncoding = 0;
	delete iLastModified;
	iLastModified = 0;
	delete iETag;
	iETag = 0;

}

void CHttpMessage::CleanFullMessage()
// Clean the fields.
{
	DestroyRequestLine();
	DestroyResponseLine();
	DestroyAllHeaders();

}

TBool CHttpMessage::CloseConnection() const
// This function is used to know if we have a persistent connection.
{
	if (iConnection !=0)
		if (iConnection->CompareF(_L("close")) == 0)
			return TRUE;
		else
			if (iVersion && (iVersion->CompareF(HTTP_VER) == 0))
				return FALSE;
			else // If it is not a HTTP 1.1 message we ignore the header. Always close connection !
				return TRUE;
	else
		return TRUE;
}

TBool CHttpMessage::DefaultResource() const
{
	ASSERT(iURI != NULL);

	TParse parse;
	TInt pos;

	pos = iURI->Locate('?');

	if (pos == KErrNotFound)
		parse.Set(*iURI,NULL,NULL);
	else if (pos > KErrNotFound)
		parse.Set(iURI->Left(pos),NULL,NULL);
	else //Something has gone wrong. (This function must be called after check the URI)
		return FALSE;

	return !parse.NamePresent();
}

void CHttpMessage::GetAbsPath(TDes& aAbsPath) const
{
	ASSERT(iURI != NULL);

	TInt pos;

	if (IsAbsoluteURI())
	{
		// AbsoluteURI : "http:""//" host [":"port] [abs_path [ "?" query ]]
		aAbsPath = iURI->Mid(7);
		// aAbsPath has : host [":"port] [abs_path [ "?" query ]]
		pos = aAbsPath.Locate('\\');
		aAbsPath.Copy(aAbsPath.Mid(pos + 1));
	}
	else
		aAbsPath.Copy(iURI->Mid(1));

	pos = aAbsPath.Locate('?');
	if (pos > KErrNone) aAbsPath = aAbsPath.Left(pos);

}

void CHttpMessage::GetCGICommandArgumentsL(CCommandArg& aComArg) const
{
	ASSERT(iURI != NULL);

	TInt pos,argc =0;
	TBuf<256> parameters;
	if ((pos = iURI->Locate('?')) >KErrNotFound)
	{
		if (iURI->Mid(pos +1).Locate('=') == KErrNotFound)
		{
			parameters = iURI->Mid(pos +1);
			
			TLex lex(parameters);
			
			while (!lex.Eos())
			{
				lex.Mark();
				IncUntilChar(lex,'+');
				aComArg.iArgv[argc] = lex.MarkedToken().AllocL();
				lex.Inc();
				argc++;
			}
		
		}
	}

	aComArg.iArgc = argc;

}


TInt CHttpMessage::GetCGIQueryString(TDes& aQueryString)
{
	ASSERT(iURI != NULL);
	TInt pos;

	aQueryString = iURI->Des();

	pos = aQueryString.Locate('?');
	if (pos >KErrNotFound)
	{
		aQueryString = iURI->Mid(pos +1);
		pos = KErrNone;
	}
	else
		aQueryString = _L("");
	
	return pos;
}

TPtr CHttpMessage::GetCGINameL()
// Function which gets the CGI Name.
{
	HBufC* aux;
	
	aux = HBufC::NewL(GetURI().Length() + 4);
	aux->Des().Copy(GetURI());

	TParse parse;
	TInt pos;

	parse.Set(aux->Des(),NULL,NULL);

	if (parse.ExtPresent())
	{
		pos = aux->Locate('.') + 1;
		aux->Des().Replace(pos,3,_L("dll"));
	}
	else
	{
		aux->Des().Append(_L(".dll"));
	}

	return aux->Des();

}

void CHttpMessage::GetHost(TDes& aHost) const
{
	if (iHost !=0)
		 aHost = *iHost;
	else
		 aHost = _L("");
}

TMethodType CHttpMessage::GetMethod() const
{
	TMethodType result = ENoneMethod;
	if (iMethod !=0)
	{
		if (iMethod->CompareF(_L("GET")) == 0) result = EGet;
		else if (iMethod->CompareF(_L("HEAD")) == 0) result = EHead;
		else if (iMethod->CompareF(_L("OPTIONS")) == 0) result = EOptions;
		else if (iMethod->CompareF(_L("POST")) == 0) result = EPost;
		else if (iMethod->CompareF(_L("PUT")) == 0) result = EPut;
		else if (iMethod->CompareF(_L("DELETE")) == 0) result = EDelete;
		else if (iMethod->CompareF(_L("TRACE")) == 0) result = ETrace;
	}
	
	return result;
}

void CHttpMessage::GetMethod(TDes& aMethod) const
{
	if (iMethod !=0)
		aMethod = *iMethod;
}

TPtrC CHttpMessage::GetURI()
// Function which gets the URI.
{
	ASSERT(iURI != NULL);

	TInt pos;
	TPtr aux = iURI->Des();

	// Check if we have an absolute_URI or an abs_path		
	// This Server allow resources to differ by the requested host.
	pos = aux.FindF(_L("http:\\"));
	if (pos > KErrNotFound)
	{
		aux = iURI->Mid(pos + 7);
		pos = aux.Locate('\\');
		aux = aux.Mid(pos);
	}	
	
	pos = aux.Locate('?');
	if (pos > KErrNotFound)  aux = aux.Left(pos);
	
	return aux;
}

void CHttpMessage::InsertDefaultResourceL(const TDesC& aDefaultResource)
{
	ASSERT(iURI != NULL);

	TInt pos;

	pos = iURI->Locate('?');

	iURI = iURI->ReAllocL(iURI->Length() + aDefaultResource.Length());	
	if (pos == KErrNotFound) // We expect a valid path in URI !
	{
		iURI->Des().Append(aDefaultResource);
	}
	else
	{	
		iURI->Des().Insert(pos, aDefaultResource);
	}
}

TBool CHttpMessage::IsAbsoluteURI() const
{
	ASSERT(iURI != NULL);
	return (iURI->Find(_L("http:\\")) == 0);
}

TBool CHttpMessage::IsQuery() const
{
	ASSERT(iURI != NULL);
	return (iURI->Locate('?') > KErrNotFound);
}

void CHttpMessage::MakeRawMessage(TDes8& aMessage)
// Make a message from a CHttpMessage which it suitable to be sended by a Socket.
{
	ASSERT(iStatusCode != NULL);
	ASSERT(iReasonPhrase != NULL);
	ASSERT(iVersion != NULL);

#ifdef _UNICODE
	aMessage.Copy(iVersion->Des());
#else
	aMessage = *((HBufC8*)iVersion);
#endif
	aMessage.Append(' ');
	aMessage.Append(*iStatusCode);
	aMessage.Append(' ');
	// Reason Phrase
	aMessage.Append(*iReasonPhrase);
	aMessage.Append(' ');
	aMessage.Append(_L("\r\n"));

	if (iVersion->Compare(_L("HTTP/0.9")) !=0)
	{
		//Make Response Header
		if (iDate!= 0)
		{
			aMessage.Append(_L("Date: "));
			aMessage.Append(*iDate);
			aMessage.Append(_L("\r\n"));

		}
		if (iServer!= 0)
		{
			aMessage.Append(_L("Server: "));
			aMessage.Append(*iServer);
			aMessage.Append(_L("\r\n"));

		}

		if (iContentRange!= 0)
		{
			aMessage.Append(_L("Content-Range: "));
			aMessage.Append(*iContentRange);
			aMessage.Append(_L("\r\n"));

		}
		if (iContentType!= 0)
		{
			aMessage.Append(_L("Content-Type: "));
			aMessage.Append(*iContentType);
			aMessage.Append(_L("\r\n"));

		}
		if (iContentLength != 0)
		{
			aMessage.Append(_L("Content-Length: "));
			aMessage.Append(*iContentLength);
			aMessage.Append(_L("\r\n"));

		}
		if (iAllow != 0)
		{
			aMessage.Append(_L("Allow: "));
			aMessage.Append(*iAllow);
			aMessage.Append(_L("\r\n"));
		}
		if (iAcceptRanges!= 0)
		{
			aMessage.Append(_L("Accept-Ranges: "));
			aMessage.Append(*iAcceptRanges);
			aMessage.Append(_L("\r\n"));
		}
		if (iAcceptEncoding!= 0)
		{
			aMessage.Append(_L("Accept-Encoding: "));
			aMessage.Append(*iAcceptEncoding);
			aMessage.Append(_L("\r\n"));
		}
		
		if (iLastModified != 0)
		{
			aMessage.Append(_L("Last-Modified: "));
			aMessage.Append(*iLastModified);
			aMessage.Append(_L("\r\n"));

		}
		if (iRetryAfter != 0)
		{
			aMessage.Append(_L("Retry-After: "));
			aMessage.Append(*iRetryAfter);
			aMessage.Append(_L("\r\n"));

		}
		if (iVersion->Compare(_L("HTTP/1.0")) !=0)
		{
			if (iETag!= 0)
			{
				aMessage.Append(_L("ETag: "));
				aMessage.Append(*iETag);
				aMessage.Append(_L("\r\n"));
			}
		}
	}
	
	aMessage.Append(_L("\r\n")); // End of the response header.

}


void CHttpMessage::SetCurrentVersionL()
{
	delete iVersion;
	iVersion = HTTP_VER.AllocL();
}

void CHttpMessage::SetDefaultPathL(const TDesC& aDefaultPath)
{
	ASSERT(iURI != NULL);
	iURI = iURI->ReAllocL(iURI->Length() + aDefaultPath.Length());
	iURI->Des().Insert(0,aDefaultPath);
}

void CHttpMessage::SetHeaderLineL(HBufC* aHeaderLine)
{
	TLex lex(aHeaderLine->Des());

	lex.Mark();

	// This skips all the characters until it founds a ':' character.
	while ((lex.Peek() != ':') && (!lex.Eos()))
	{
		lex.Inc();
	}	
	if (!lex.Eos()) lex.Inc();

	TPtrC field = lex.MarkedToken();
	lex.SkipSpaceAndMark();
	TPtrC datafield = lex.RemainderFromMark() ;

	
	//Entity-header fields
	if (field.Find(_L("Allow:"))==0)
	{
		delete iAllow;
		iAllow = datafield.AllocL();
	}
	else if (field.Find(_L("Content-Base:"))==0)
	{
		delete iContentBase;
		iContentBase = datafield.AllocL();
	}
	else if (field.Find(_L("Content-Encoding:"))==0)
	{	
		delete iContentEncoding;	
		iContentEncoding = datafield.AllocL();	
	}
	else if (field.Find(_L("Content-Language:"))==0)
	{
		delete iContentLanguage;
		iContentLanguage = datafield.AllocL();
	}
	else if (field.Find(_L("Content-Length:"))==0)
	{
		delete iContentLength;
		iContentLength = datafield.AllocL();
	}
	else if (field.Find(_L("Content-Location:"))==0)
	{
		delete iContentLocation;
		iContentLocation = datafield.AllocL();
	}
	else if (field.Find(_L("Content-MD5:"))==0)
	{
		delete iContentMD5;
		iContentMD5 = datafield.AllocL();
	}
	else if (field.Find(_L("Content-Range:"))==0)
	{
		delete iContentRange;
		iContentRange = datafield.AllocL();
	}
	else if (field.Find(_L("Content-Type:"))==0)
	{
		delete iContentType;
		iContentType = datafield.AllocL();
	}
	else if (field.Find(_L("Last-Modified:"))==0)
	{
		delete iLastModified;
		iLastModified = datafield.AllocL();
	}
	else if (field.Find(_L("Expires:"))==0)
	{
		delete iExpires;
		iExpires = datafield.AllocL();
	}
	// General-header fields
	else if (field.Find(_L("Cache-Control:"))==0)
	{
		delete iCacheControl;
		iCacheControl = datafield.AllocL();
	}
	else if (field.Find(_L("Connection:"))!=KErrNotFound)
	{
		delete iConnection;
		iConnection = datafield.AllocL();
	}
	else if (field.Find(_L("Date:"))==0)
	{
		delete iDate;
		iDate = datafield.AllocL();
	}
	else if (field.Find(_L("Pragma:"))==0)
	{
		delete iPragma;
		iPragma = datafield.AllocL();
	}
	else if (field.Find(_L("Trailer:"))==0)
	{
		delete iTrailer;
		iTrailer = datafield.AllocL();
	}
	else if (field.Find(_L("TransferEncoding:"))==0)
	{
		delete iTransferEncoding;
		iTransferEncoding = datafield.AllocL();
	}
	else if (field.Find(_L("Upgrade:"))==0)
	{
		delete iUpgrade;
		iUpgrade = datafield.AllocL();
	}
	else if (field.Find(_L("Via:"))==0)
	{
		delete iVia;
		iVia = datafield.AllocL();
	}
	else if (field.Find(_L("Warning:"))==0)
	{
		delete iWarning;
		iWarning = datafield.AllocL();
	}
	//Request-header fields
	else if (field.Find(_L("Accept:"))==0)				
	{
		delete iAccept;
		iAccept = datafield.AllocL();
	}
	else if (field.Find(_L("Accept-Charset:"))==0)
	{
		delete iAcceptCharset;
		iAcceptCharset = datafield.AllocL();
	}
	else if (field.Find(_L("Accept-Encoding:"))==0)
	{
		delete iAcceptEncoding;
		iAcceptEncoding = datafield.AllocL();
	}
	else if (field.Find(_L("Accept-Language:"))==0)
	{
		delete iAcceptLanguage;
		iAcceptLanguage = datafield.AllocL();
	}
	else if (field.Find(_L("Authorization:"))==0)
	{	
		delete iAuthorization;	
		iAuthorization = datafield.AllocL();
	}
	else if (field.Find(_L("Except:"))==0)
	{
		delete iExcept;
		iExcept = datafield.AllocL();
	}
	else if (field.Find(_L("From:"))==0)
	{	
		delete iFrom;
		iFrom = datafield.AllocL();
	}
	else if (field.Find(_L("Host:"))==0)
	{
		delete iHost;
		iHost = datafield.AllocL();
	}
	else if (field.Find(_L("If-Match:"))==0)
	{
		delete iIfMatch;
		iIfMatch = datafield.AllocL();
	}
	else if (field.Find(_L("If-Modified-Since:"))==0)
	{
		delete iIfModifiedSince;
		iIfModifiedSince = datafield.AllocL();
	}
	else if (field.Find(_L("If-None-Match:"))==0)
	{
		delete iIfNoneMatch;
		iIfNoneMatch = datafield.AllocL();
	}
	else if (field.Find(_L("If-Range:"))==0)
	{
		delete iIfRange;
		iIfRange = datafield.AllocL();
	}
	else if (field.Find(_L("If-Unmodified-Since:"))==0)
	{
		delete iIfUnModifiedSince;
		iIfUnModifiedSince = datafield.AllocL();
	}
	else if (field.Find(_L("Max-Forwards:"))==0)
	{
		delete iMaxForwards;
		iMaxForwards = datafield.AllocL();
	}
	else if (field.Find(_L("Proxy-Authorization:"))==0)
	{
		delete iProxyAuthorization;
		iProxyAuthorization = datafield.AllocL();
	}
	else if (field.Find(_L("Range:"))==0)
	{
		delete iRange;
		iRange = datafield.AllocL();
	}
	else if (field.Find(_L("Referer:"))==0)				
	{
		delete iReferer;
		iReferer = datafield.AllocL();
	}
	else if (field.Find(_L("TE:"))==0)	
	{
		delete iTE;
		iTE = datafield.AllocL();
	}
	else if (field.Find(_L("User-Agent:"))==0)			
	{	
		delete iUserAgent;
		iUserAgent = datafield.AllocL();
	}
	// I think I should generate an error instead to initialise the fields !! (or just ignore them)
	//Response-Header fields
	else if (field.Find(_L("Accept-Ranges:"))==0)		
	{
		delete iAcceptRanges;
		iAcceptRanges = datafield.AllocL();
	}
	else if (field.Find(_L("Ages:"))==0)
	{
		delete iAges;
		iAges = datafield.AllocL();
	}
	else if (field.Find(_L("ETag:"))==0)
	{
		delete iETag;
		iETag = datafield.AllocL();
	}
	else if (field.Find(_L("Location:"))==0)
	{
		delete iLocation;
		iLocation = datafield.AllocL();
	}
	else if (field.Find(_L("Proxy-Authenticate:"))==0)
	{
		delete iProxyAuthenticate;
		iProxyAuthenticate = datafield.AllocL();
	}
	else if (field.Find(_L("Retry-After:"))==0)
	{
		delete iRetryAfter;
		iRetryAfter= datafield.AllocL();
	}
	else if (field.Find(_L("Server:"))==0)	
	{
		delete iServer;
		iServer = datafield.AllocL();
	}
	else if (field.Find(_L("Vary:"))==0)
	{
		delete iVary;
		iVary = datafield.AllocL();
	}
	else if (field.Find(_L("WWW-Authenticate:"))==0)		
	{
		delete iWWWAuthenticate;
		iWWWAuthenticate = datafield.AllocL();
	}
	else if (field.Find(_L("Extension-Header:"))==0) //????????????????????
	{
		delete iExtensionHeader;
		iExtensionHeader = datafield.AllocL();
	}
	//else
		//Ignore the header;

}

void CHttpMessage::SetMessageL(const HBufC8* aMessage)
// This functions sets a CHttpMessage from a buffer.
{
	ASSERT(aMessage != NULL);
	TInt pos;

	pos=aMessage->Find(_L8("\r\n"));
	if (pos > 0)
	{	
#ifdef _UNICODE
		HBufC* aux = HBufC::NewL(pos);
        aux->Des().Copy(aMessage->Left(pos));
#else     
		HBufC* aux = (HBufC*)(aMessage->Left(pos).AllocL());
#endif        
		TPtrC8 remained = aMessage->Mid(pos + 2);
		SetRequestLineL(aux);
		
		pos=remained.Find(_L8("\r\n"));
		if (pos > 0)
		{
			delete aux;
			aux = 0;
#ifdef _UNICODE
			aux = HBufC::NewL(pos);
			aux->Des().Copy(remained.Left(pos));
#else     
			aux = (HBufC*)(remained.Left(pos).AllocL());
#endif
			remained.Set(remained.Mid(pos + 2));
			while (pos > 0)
			{
				SetHeaderLineL(aux);
				pos=remained.Find(_L8("\r\n"));
				if (pos >0)
				{
				delete aux;
				aux = 0;
#ifdef _UNICODE
			    aux = HBufC::NewL(pos);
				aux->Des().Copy(remained.Left(pos));
#else     
				aux = (HBufC*)(remained.Left(pos).AllocL());
#endif
				remained.Set(remained.Mid(pos + 2));
				}
			}
		}
		delete aux;

	}
	// if pos < 0 => ERROR !!!!!!!

}

void CHttpMessage::SetRequestLineL(HBufC* aRequestLine)
// This function sets the Request fields from a buffer.
{
	TLex lex(aRequestLine->Des());
	
	if(!lex.Eos())
	{
		lex.Mark();
		lex.SkipCharacters();
		delete iMethod;
		iMethod = lex.MarkedToken().AllocL();
		if(!lex.Eos())
		{
			lex.SkipSpaceAndMark();
			lex.SkipCharacters();
			delete iURI;
			iURI = lex.MarkedToken().AllocL();
			ChangeHexL();
			ChangeSlash();
			if(!lex.Eos())
			{
				lex.SkipSpaceAndMark();
				lex.SkipCharacters();
				delete iVersion;
				iVersion = lex.MarkedToken().AllocL();
			}
		}
	}
}	

void CHttpMessage::SetStatusLineL(TUint aCode)
// This function sets the status code and the Reason phrase.
{
	delete iStatusCode;  // just in case.
	delete iReasonPhrase;  // just in case.
	
	if ((iVersion == 0) ||
		((iVersion->Compare(_L("HTTP/0.9"))!=0) &&
		 (iVersion->Compare(_L("HTTP/1.0"))!=0) &&
		 (iVersion->Compare(_L("HTTP/1.1"))!=0)))
	{	
		SetCurrentVersionL();
	}

	TBuf<MAX_SC_LEN> aux;
	aux.Num(aCode);
	iStatusCode = aux.AllocL();
	
	switch(aCode)
	{
	case SC_CONTINUE:
		iReasonPhrase = RP_CONTINUE.AllocL();
		break;
	case SC_OK:
		iReasonPhrase= RP_OK.AllocL();
		break;
	case SC_CREATED:
		iReasonPhrase= RP_CREATED.AllocL();
		break;
	case SC_NO_CONTENT:
		iReasonPhrase = RP_NO_CONTENT.AllocL();
		break;
	case SC_PARTIAL_CONTENT:
		iReasonPhrase = RP_PARTIAL_CONTENT.AllocL();
		break;
	case SC_MOVED_TEMPORARILY:
		iReasonPhrase= RP_MOVED_TEMPORARILY.AllocL();
		break;
	case SC_NOT_MODIFIED:
		iReasonPhrase= RP_NOT_MODIFIED.AllocL();
		break;
	case SC_BAD_REQUEST:
		iReasonPhrase= RP_BAD_REQUEST.AllocL();
		break;
	case SC_UNAUTHORIZED:
		iReasonPhrase= RP_UNAUTHORIZED.AllocL();
		break;
	case SC_NOT_FOUND:
		iReasonPhrase= RP_NOT_FOUND.AllocL();
		break;
	case SC_NOT_ACCEPTABLE:
		iReasonPhrase= RP_NOT_ACCEPTABLE.AllocL();
		break;
	case SC_REQUEST_TIMEOUT:
		iReasonPhrase= RP_REQUEST_TIMEOUT.AllocL();
		break;
	case SC_CONFLICT:
		iReasonPhrase= RP_CONFLICT.AllocL();
		break;
	case SC_LENGTH_REQUIERED:
		iReasonPhrase = RP_LENGTH_REQUIERED.AllocL();
		break;
	case SC_PRECONDITION_FAILED:
		iReasonPhrase = RP_PRECONDITION_FAILED.AllocL();
		break;
	case SC_REQUEST_URI_TOO_LONG:
		iReasonPhrase = RP_REQUEST_URI_TOO_LONG.AllocL();
		break;
	case SC_RANGE_NOT_SATISFIABLE:
		iReasonPhrase = RP_RANGE_NOT_SATISFIABLE.AllocL();
		break;
	case SC_INTERNAL_SERVER_ERROR:
		iReasonPhrase = RP_INTERNAL_SERVER_ERROR.AllocL();
		break;
	case SC_NOT_IMPLEMENTED:
		iReasonPhrase = RP_NOT_IMPLEMENTED.AllocL();
		break;
	case SC_SERVICE_UNAVAILABLE:
		iReasonPhrase = RP_SERVICE_UNAVAILABLE.AllocL();
		break;
	case SC_HTTP_VERSION_NOT_SUPPORTED:
		iReasonPhrase = RP_HTTP_VERSION_NOT_SUPPORTED.AllocL();
		break;
	default:
		iReasonPhrase= RP_NOT_IMPLEMENTED.AllocL();
		break;
	}
}

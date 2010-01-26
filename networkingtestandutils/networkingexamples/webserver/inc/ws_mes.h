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
// ws_mes.h - http server message handler
//



/**
 @internalComponent
*/
#ifndef __WS_MES
#define __WS_MES

#include <f32file.h>

#include "ws_cgi.h"

// Define

#define KErrNotSatisfiable -999

#define MAX_CL_LEN		12  // Content-Length max length. (12 decimal digits => 999.999.999.999 bytes)
#define MAX_ETAG_LEN	18  // ETag max length ( 16 hex digits + 2 '"' characters.
#define MAX_HEADER_LEN  256	
#define MAX_URI_LEN			256
#define MAX_SC_LEN		3	// All Error Codes have only 3 digits.


#define SC_INFORMATIONAL_MAX 199 // MAX. of 1xx codes.
#define MAX_RANGE_TOKEN 80 // It is used in the CheckRange function.

#define HTTP_VER _L("HTTP/1.1")
#define SERVER_NAME _L("Epoc-WS/1.1")

// Content-types
#define DEFAULT_CONTENT_TYPE _L("application/octet-stream")
#define HTTP_CONTENT_TYPE _L("message/http")
#define HTML_CONTENT_TYPE _L("text/html")

//Status Codes and Reply Phrases
// Status code 100 - 199

#define SC_CONTINUE 100
#define RP_CONTINUE _L("Continue")

// Status code 200 - 299

#define SC_OK 200
#define RP_OK _L("OK")

#define SC_CREATED 201
#define RP_CREATED _L("Created")

#define SC_NO_CONTENT 204
#define RP_NO_CONTENT _L("Not Content")

#define SC_PARTIAL_CONTENT 206
#define RP_PARTIAL_CONTENT _L("Partial Content")


// Status code 300 - 399

#define SC_MOVED_TEMPORARILY 302
#define RP_MOVED_TEMPORARILY _L("Moved Temporarily")


#define SC_NOT_MODIFIED 304
#define RP_NOT_MODIFIED _L("Not modified")

// Status code 400 - 499 (Client Error)

#define SC_BAD_REQUEST 400
#define RP_BAD_REQUEST _L("Bad Request")

#define SC_UNAUTHORIZED 401
#define RP_UNAUTHORIZED _L("Unauthorized")

#define SC_NOT_FOUND 404
#define RP_NOT_FOUND _L("Not Found")

#define SC_NOT_ACCEPTABLE 406
#define RP_NOT_ACCEPTABLE _L("Not Acceptable")

#define SC_REQUEST_TIMEOUT 408
#define RP_REQUEST_TIMEOUT _L("Request Timeout")

#define SC_CONFLICT 409
#define RP_CONFLICT _L("Conflict")

#define SC_LENGTH_REQUIERED 411
#define RP_LENGTH_REQUIERED _L("Length Requiered")

#define SC_PRECONDITION_FAILED 412
#define RP_PRECONDITION_FAILED _L("Precondtion Failed")

#define SC_REQUEST_URI_TOO_LONG 414
#define RP_REQUEST_URI_TOO_LONG _L("Request-URI too long")

#define SC_RANGE_NOT_SATISFIABLE 416
#define RP_RANGE_NOT_SATISFIABLE _L("Range Not Satisfiable")


// Status code from 500 - .... (Server Error)

#define SC_INTERNAL_SERVER_ERROR 500
#define RP_INTERNAL_SERVER_ERROR _L("Server Internal Error")

#define SC_NOT_IMPLEMENTED 501
#define RP_NOT_IMPLEMENTED _L("Not Implemented")

#define  SC_SERVICE_UNAVAILABLE 503
#define  RP_SERVICE_UNAVAILABLE _L("Server Unavalaible")

#define  SC_HTTP_VERSION_NOT_SUPPORTED 505
#define  RP_HTTP_VERSION_NOT_SUPPORTED _L("HTTP Version Not Supported")

// End Status Codes and Reply Phrases

// Error Messages
// Code 100 - 199

// Code 200 - 299
#define SC_OK_MES _L("<html><body><h1> 200 OK Epoc WebServer 1.0 </h1></body></html>")
#define SC_CREATED_MES _L("<html><body><h1> 201 CREATED Epoc WebServer 1.0 </h1></body></html>")

// Code 300 - 399

// Code 400 - 499
#define SC_BAD_REQUEST_MES	_L("<html><body><h1> 400 BAD REQUEST Epoc WebServer 1.0 </html></body></h1>")
#define SC_UNAUTHORIZED_MES	_L("<html><body><h1> 401 UNAUTHORIZED Epoc WebServer 1.0 </html></body></h1>")
#define SC_NOT_FOUND_MES	_L("<html><body><h1> 404 NOT FOUND Epoc WebServer 1.0 </html></body></h1>")
#define SC_NOT_ACCEPTABLE_MES _L("<html><body><h1> 406 NOT ACCEPTABLE Epoc WebServer 1.0 </html></body></h1>")
#define SC_REQUEST_TIMEOUT_MES _L("<html><body><h1> 408 REQUEST TIMEOUT Epoc WebServer 1.0 </html></body></h1>")
#define SC_CONFLICT_MES		_L("<html><body><h1> 409 CONFLICT Epoc WebServer 1.0 </html></body></h1>")
#define SC_LENGTH_REQUIERED_MES _L("<html><body><h1> 411 LENGTH REQUIERED Epoc WebServer 1.0 </html></body></h1>")
#define SC_PRECONDITION_FAILED_MES _L("<html><body><h1> 412 PRECONDITION FAILED Epoc WebServer 1.0 </html></body></h1>")
#define SC_REQUEST_URI_TOO_LONG_MES _L("<html><body><h1> 414 REQUEST-URI TOO LONG Epoc WebServer 1.0 </html></body></h1>")
#define SC_RANGE_NOT_SATISFIABLE_MES _L("<html><body><h1> 416 RANGE NOT SATISFIABLE Epoc WebServer 1.0 </html></body></h1>")

// Code 500 - ...
#define SC_INTERNAL_SERVER_ERROR_MES _L("<html><body><h1> 500 INTERNAL SERVER ERROR Epoc WebServer 1.0 </html></body></h1>")
#define SC_NOT_IMPLEMENTED_MES	_L("<html><body><h1> 501 NOT IMPLEMENTED Epoc WebServer 1.0 </html></body></h1>")
#define  SC_SERVICE_UNAVAILABLE_MES _L("<html><body><h1> 503 SERVER UNAVAILABLE Epoc WebServer 1.0 </html></body></h1>")
#define SC_UNEXPECTED_MES	_L("<html><body><h1> UNEXPECTED ERROR Epoc WebServer 1.0 </html></body></h1>")
#define SC_HTTP_VERSION_NOT_SUPPORTED_MES _L("<html><body><h1> HTTP VERSION NOT SUPPORTED Epoc WebServer 1.0 </html></body></h1>")

// End Error Messages.

//End Define


//***********************************************************************************************************************


enum TMethodType
{
	ENoneMethod,EGet,EHead,EOptions,EPost,EPut,EDelete,ETrace,EAll
};

class CHttpMessageHeader
{
public:
	CHttpMessageHeader();
	virtual ~CHttpMessageHeader();
public: //General-Header fields.
	void SetDateL();
public: // Entity-header fields
	TInt CheckRange(TInt aSize);
	TBool ExistsIfRange() const;
	TBool ExistsRange() const;
	TInt GetContentLength();
	TBool IfMatch(const TInt64& aEntityTag) const;
	TBool IfModSince(const TTime& aDate);
	TBool IfNoneMatch(const TInt64& aEntityTag);
	TBool IfUnModSince(const TTime& aDate);
	TBool IfRange(const TInt64& aEtag, const TTime& aDate);
	TBool IsMultiRange() const;
	TBool MoreRanges() const; // It is equal to ExistsRanges !!!!
	void SetAllowL(TMethodType aMethod);
	void SetContentLengthL(TUint aSize);
	void SetContentRangeL(TInt aFirstPos,TInt aLastPos,TInt aSize);
	void SetContentTypeL(TUint aType);
	void SetLastModifiedL(const TTime& aDate);
	void SetETagL(const TInt64& aETag);
	void SetRangeValuesL(TInt& aStart,TInt& aEnd);
public: // Response-header fields
	void SetAcceptRangesL();
	void SetAcceptEncodingL();
	void SetServerL();
public: //Request-header
	TInt CheckAccept(const TDesC& aContentType) const;
	TInt CheckAuthorization() const;
protected:
	void DestroyAllHeaders();
public: // Declaration of the HTTP Message fields.
	// General-header fields

	HBufC*	iCacheControl;
	HBufC*	iConnection;
	HBufC*	iDate;
	HBufC*	iPragma;
	HBufC*	iTrailer;
	HBufC*	iTransferEncoding;
	HBufC*	iUpgrade;
	HBufC*	iVia;
	HBufC*	iWarning;

	// Entity-header fields

	HBufC*	iAllow;
	HBufC*	iContentBase; //Not in the RFC !? It is in the book !
	HBufC*	iContentEncoding;
	HBufC*	iContentLanguage;
	HBufC*	iContentLength;
	HBufC*	iContentLocation;
	HBufC*	iContentMD5;
	HBufC*	iContentRange;
	HBufC*	iContentType;
	HBufC*	iExpires;
	HBufC*	iLastModified;


	// Request-header fields

	HBufC*	iAccept;
	HBufC*	iAcceptCharset;
	HBufC*	iAcceptEncoding;
	HBufC*	iAcceptLanguage;
	HBufC*	iAuthorization;
	HBufC*	iExcept;
	HBufC*	iFrom;
	HBufC*	iHost;
	HBufC*	iIfMatch;
	HBufC*	iIfModifiedSince;
	HBufC*	iIfNoneMatch;
	HBufC*	iIfRange;
	HBufC*	iIfUnModifiedSince;
	HBufC*	iMaxForwards;
	HBufC*	iProxyAuthorization;
	HBufC*	iRange;
	HBufC*	iReferer;
	HBufC*	iTE;
	HBufC*	iUserAgent;

	//Response-Header fields

	HBufC*	iAcceptRanges;
	HBufC*	iAges;
	HBufC*	iETag;
	HBufC*	iLocation;
	HBufC*	iProxyAuthenticate;
	HBufC*	iRetryAfter;
	HBufC*	iServer;
	HBufC*	iVary;
	HBufC*	iWWWAuthenticate;
	HBufC*	iExtensionHeader;

	//Request Message Body
	HBufC*	iRequestMessageBody;

};

class CHttpMessage : public CHttpMessageHeader
{
public:
	CHttpMessage();
	~CHttpMessage();
public:
	
	TInt CheckHost() const;
	TInt CheckRequestLine();
	TInt CheckURI() const;
	void CleanFullMessage();
	void CleanMessage();
	TBool CloseConnection() const;
	TBool DefaultResource() const;
	void GetAbsPath(TDes& aAbsPath) const;
	void GetCGICommandArgumentsL(CCommandArg& aComArg) const;
	TInt GetCGIQueryString(TDes& aQueryString);
	TPtr GetCGINameL();
	void GetHost(TDes& aHost) const;
	void GetMethod(TDes& aMethod) const;
	TMethodType GetMethod() const;
	TPtrC GetURI();
	void InsertDefaultResourceL(const TDesC& aDefaultResource);
	TBool IsQuery() const;
	TBool IsAbsoluteURI() const;
	void MakeRawMessage(TDes8& aMessage);
	void SetCurrentVersionL();
	void SetDefaultPathL(const TDesC& aDefaultPath);
	void SetHeaderLineL(HBufC* aHeaderLine);
	void SetMessageL(const HBufC8* aMessage);
	void SetStatusLineL(TUint aCode);
private:
	void ChangeHexL();
	void ChangeSlash();
	TInt CheckVersion();
	void DestroyRequestLine();
	void DestroyResponseLine();
	void SetRequestLineL(HBufC* aRequestLine);
public:
	// Request-Line fields
	HBufC*	iMethod;	
	HBufC*	iURI;
	HBufC*	iVersion;	//Also in the Response-Line
	
	// Status-Line fileds;
	HBufC*	iStatusCode;
	HBufC*	iReasonPhrase;

};

#endif

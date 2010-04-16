//
//    readme2.txt - http server development notes
//
// portions Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
////////////////////////////////////////////////////////////////

/* EPOC - WebServer. (WebServer condiotional complaint HTTP/1.1) 					  */			   	          */
/* The Webserver works fine with the epoc browser, Internet Explorer 4.01 and Netscape Communicator 4.06. */
/* Functionality level.											  */					
/* 24 - 1 - 2000 											  */

General Issues
==============

Chunked encoding is not supported.
Accept headers are not supported.
Authentication is not supported.

Persistent connections are supported.

There is no interface for the log file.
There is no interface for the config file.
There is no interface for the mime-types file(which associates an extension with a mime type).

GET Method
==========

Unconditional and Conditional (If headers) requests are supported.
Query requests are supported, but not fully tested.
Range requests are supported, but multi-range request are not supported.

HEAD Method
===========

Unconditional and Conditional (If headers) requests are supported.
Query requests are supported, but not fully tested.
Range and multi-range requests are supported and tested

POST Method
===========

Parse headers requests are supported, but are not fully tested.
Non-parse headers requests are supported, but are NOT tested.

OPTIONS Method
==============

Supported and partially tested.

TRACE Method
============

Supported and partially tested.

PUT Method
==========

Supported and partially tested.


DELETE Method
=============

Supported and partially tested.

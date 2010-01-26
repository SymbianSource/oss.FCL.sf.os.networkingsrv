/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file VJTest.h
*/
#if (!defined __VJ_TEST_H__)
#define __VJ_TEST_H__
#include <test/testexecutestepbase.h>
#include <in_sock.h>
#include <es_sock.h>
#include "TE_VJCompStepBase.h"
#include "TE_VJComp.h"

/*
Used to test CVJCompressor class 
*/
__DEFINE_TEST_CLASS(CVJCompressTest)

/*
Used to test CVJDeCompressor class 
*/
__DEFINE_TEST_CLASS(CVJDecompressTest)

/*
Used to test CVJDeCompressor class 
*/
__DEFINE_TEST_CLASS(CVJDecompressTypeErrorTest)

/*
Used to test CVJDeCompressor class 
*/
__DEFINE_TEST_CLASS(CVJandRefDecompressTest)


#endif //__VJ_TEST_H

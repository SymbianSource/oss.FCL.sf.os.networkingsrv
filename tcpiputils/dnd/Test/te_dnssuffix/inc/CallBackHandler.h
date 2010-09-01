/*
 * CallBackHandler.h
 *
 *  Created on: Apr 15, 2010
 *      Author: sakpatil
 */

#ifndef CALLBACKHANDLER_H_
#define CALLBACKHANDLER_H_

class MCallBackHandler
    {
public:
    virtual void HandleCallBackL(TInt aError) = 0;
    };

#endif /* CALLBACKHANDLER_H_ */

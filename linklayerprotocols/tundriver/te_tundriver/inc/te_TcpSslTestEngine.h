/*
 * TcpTestEngine.h
 *
 *  Created on: Jan 28, 2010
 *      Author: GUNAGI
 */

#ifndef TE_TCPSSLTESTENGINE_H_
#define TE_TCPSSLTESTENGINE_H_

#include <e32cons.h>
#include <c32comm.h>
#include <in_sock.h>
#include <es_sock.h> 
#include <f32file.h> 
#include <commdbconnpref.h>

/** Number of chars allowed for address & page settings */
const TInt KSettingFieldWidth = 128;

/** Global variables */
// File system sesion path
// IP file where the system IP is mentioned
//_LIT8(hostIpAddress, "10.225.164.117");
//const TInt KPortNum = 80;

_LIT8(hostIpAddress, "10.1.1.1");
const TInt KPortNum = 80;

/** Connection settings to access a server  */
struct TConnectSettings
    {
    /** Server address (as text) */
    TBuf<KSettingFieldWidth> iAddress;
    /** Server port */
    TInt iPortNum;
    /** Web page to get from the server */
    TBuf8<KSettingFieldWidth> iPage;
    };

/**
    Manages connection to a SSL web server.
*/
class CTcpSslTestEngine : public CActive
    {
public:
/**
Allocates and constructs a new engine.
@return New object
*/
    static CTcpSslTestEngine *NewL();
/**
Destructor.
*/
    ~CTcpSslTestEngine();
/**
Initiates the connection to a server and the transaction

    @param aAddress Server address (e.g. www.symbian.com or dotted decimal format)

    @param aPortNum Server port for secure web (e.g. 443)

    @param aPage Web page to get from the server. The leading `/` should be included, e.g. /webpage.html.

    @param aCipherSuite Cipher suites that client will tell server it supports (decimal). This 
    should be in decimal, with 2 characters, ie for suites 3,7,8,9, this field would be 03070809.
    By entering a single `0` for this field, the SSL default cipher suites will be passed.

    @param aCipher Cipher suite that server is expected to use (decimal). This is compared with 
    actual cipher for pass/fail. If this field is 0, no comparisons with the actual cipher 
    suite used will be made.

*/
    void ConnectL( const TConnectSettings& aConnectSettings );
/**
Sets the console to write messages to

@param aConsole The console
*/
    void SetConsole( CConsoleBase& aConsole );

/**
Sets the (opened) file to write server response to

@param aOutputFile The file
*/
    void SetOutputFile( RFile& aOutputFile );


    void Initialize();
    
    /**
Tests if the connection is in progress.

@return True if in progress else false
*/
    TBool InUse();

    
private:
    /** Engine states */
    enum TStates  
        {
        /** IP connection initiated */
        ESocketConnected,
        /** Setting the ciphers for a secure connection */
        ESettingCiphers,
        /** Secure socket request initiated */
        ESecureConnected,
        /** Send get page request to server */
        EGetRequestSent,
        /** Server has responded to request */
        EDataReceived,
        /** Connection closed down */
        EConnectionClosed
        };
    
private:
    /** Constructor. */
    CTcpSslTestEngine();    
    /** Second phase constructor. */
    void ConstructL();

    // Methods from CActive
    /** Previous state has completed. */
    void RunL();
    /** Cancel request */
    void DoCancel();
    /** Handles a leave occurring in RunL(). */
    TInt RunError( TInt aError );
    
    // Handle particular engine states
    /** Attempts secure connection. */
    void MakeConnectionL();
    /** Sends page request to server */ 
    void MakePageRequestL();
    /** Start getting server's response to the request */
    void GetServerResponseL();
    /** Finish getting server's response */
    void ReadServerResponseL();
    /** Handle connection completed */
    void ConnectionClosed();

private:
    // Sockets objects
    /** The socket server */
    RSocketServ iSocketServ;
    /** Socket to make connection on */
    RSocket iSocket;
    /** For resolving DNS addresses */
    RHostResolver iHostResolver;
    /** Server address */
    TInetAddr iInetAddr;

    // Connection parameters
    const TConnectSettings* iConnectSettings;

    // Transfer buffers and counters
    /** Data sent buffer */
    TPtr8 iSndBuffer;
    /** #bytes sent */
    TSockXfrLength iBytesSent;
    /** Data received buffer */
    TPtr8 iRcvBuffer;
    /** #bytes received */
    TInt iTotalBytesRead;

    /** For retries, after a delay */
    RTimer iTimer;

    /** Output console */
    CConsoleBase *iConsole;
    /** Output file */
    RFile* iOutputFile;

    // Flags and state
    /** True if the transation completed successfully */
    TBool       iSuccess;
    /** True if success on first attempt */
    TBool   iFirstRunFlag;
    /** True if connection is in progress */
    TBool   iInUse;
    /** Engine state (a TStates value) */
    TInt    iRunState;
    
    /** Counter for stopping making connections */
    TInt iCounter;
    
    RFs iFs;
    };


#endif /* TE_TCPSSLTESTENGINE_H_ */

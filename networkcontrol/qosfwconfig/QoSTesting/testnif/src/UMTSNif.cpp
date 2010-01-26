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
//

#include "qos_if.h"
#include "UMTSNif.h"
#include <NIFMBUF.H>

#include "TestIf.h"
#include <f32file.h>
#include <s32file.h>

#include "log-r6.h"

#include <in6_if.h>     // KSoIface*, KIf*
#include <eui_addr.h>   // TE64Addr

const TUint KUmtsIfaceFeatures    = KIfIsPointToPoint | 0;
const TUint KUmtsDefaultFrameSize = 1500;
const TUint KUmtsSpeedMetric      = 115200;

CUmtsNif::CUmtsNif(CUmtsNifLink* aUmtsNifLink) : CNifIfBase(*aUmtsNifLink)
    {
    LOG(Log::Printf(_L("<%s> CUmtsNif::CUmtsNif - start "), 
                    iNifUniqueName.PtrZ()));    

    iLinkLayer = aUmtsNifLink;

    RFs fs;
    RFile file;
    fs.Connect();
    
    if(file.Open(fs, 
                 KTestSetDefaultQoSFail, 
                 EFileShareAny|EFileWrite|EFileStreamText) == KErrNone)
        {
        iControlOption = KSetDefaultQoSFail;
        fs.Delete(KTestSetDefaultQoSFail);
        }
    
    file.Close();    
    fs.Close();
    
    LOG(Log::Printf(_L("<%s> CUmtsNif::CUmtsNif - end "),
                    iNifUniqueName.PtrZ()));    
    }

CUmtsNif::~CUmtsNif()
    {    
    LOG(Log::Printf(_L("<%s> CUmtsNif::~CUmtsNif - Nif going down "), 
                    iNifUniqueName.PtrZ()));    
    
    TNetworkParameters networkParameters;

    networkParameters.iNetworkEventCode = KNetworkInterfaceDown;
    TPckg<TNetworkParameters> event(networkParameters);
    
    if(EventsOn()) 
        {
        ASSERT(iEventHandler != NULL);
        // Notify the upper layer
        this->EventHandler().Event((CProtocolBase*)this,
                                                   KNetworkStatusEvent,
                                                   event);    
        }

    delete iContextManager;    

    iDownFlag = ETrue;

    // CNifIfLink object can be destroyed before 
    // the CNifIfBase objects are destroyed.
    if (iLinkLayer)
        {
        iLinkLayer->NifDown();
        iLink.Deque();
        }

    LOG(Log::Printf(_L("<%s> Nif destroyed"),iNifUniqueName.PtrZ()));
    }

/*
 *  Second phase constructor
 */
void CUmtsNif::ConstructL(CUmtsNifLink *aLinkLayer,const TDesC& aNetworkName)
    {
    _LIT(KUmtsName, "testnif.");    
    _LIT(colon,":");
    iNetworkName = aNetworkName;

    TUint32 iapId = 0;
    TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(IAP);
    columnName.Append(TChar(KSlashChar));
    columnName.Append(TPtrC(COMMDB_ID));
    TInt ret = iNotify->ReadInt(columnName, iapId);
    User::LeaveIfError(ret);
    
    iIapId = iapId;

    iEventsOn = FALSE;
    iDefaultQoSSet = FALSE;

    iNifUniqueName = KUmtsName;
    iNifUniqueName.AppendNum((TInt)iIapId);
    iNifUniqueName.Append(colon);
    iNifUniqueName.Append(iNetworkName);

    iLinkLayer = aLinkLayer;
    iNifController = aLinkLayer->Controller();
    
    iNetwork = NULL;        

    iContextManager = new (ELeave) CNifContextManager();
    iContextManager->ConstructL(this);

    LOG(Log::Printf(_L("<%s> New Nif constructed"),iNifUniqueName.PtrZ()));

    }

// Called when linklayer is destroyed
void CUmtsNif::LinkLayerDown()
    {
    iLinkLayer = NULL;
    iLink.Deque();
    }

void CUmtsNif::BindL(TAny* aId)
    {
    if (iNetwork) 
        {
        User::Leave(KErrInUse);
        }

    iNetwork = (CProtocolBase*) aId;
    }

void CUmtsNif::Info(TNifIfInfo& aInfo) const
    {
    FillInInfo(aInfo);
    aInfo.iName = iNifUniqueName;    
    
    if(iNetworkName.CompareF(KDescIp6) == 0)
        {
        aInfo.iProtocolSupported = KProtocolInet6Ip; 
        }
    else if(iNetworkName.CompareF(KDescIp) == 0)
        {
        aInfo.iProtocolSupported = KProtocolInetIp; 
        }
    }

void CUmtsNif::FillInInfo(TNifIfInfo& aInfo)
    {
    aInfo.iVersion = TVersion(KUmtsNifMajorVersionNumber, 
                              KUmtsNifMinorVersionNumber, 
                              KUmtsNifVersionNumber);
    aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
    }

TInt CUmtsNif::State() 
    {
    return 0; 
    }

/* 
 *  Control interface: For Stack and GUQoS -module 
 */
TInt CUmtsNif::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny*)
    {               
    if (aLevel == KSOLInterface)
        {
        switch (aName)
            { 
            // WCDMA specific options  START ->
            // Set QoS of the Nif to be created at startup
            case KNifSetDefaultQoS: 
                {
                if(iControlOption == KSetDefaultQoSFail)
                    {
                    return KErrTest;
                    }
                else
                    {
                    return SetDefaultQoS(aOption);
                    }
                }
            // Creates a new secondary context
            case KContextCreate:
                {
                if(iControlOption == KContextCreateFail)
                    {
                    return KErrTest;
                    }
                else
                    {
                    return CreateSecondaryContext(aOption);
                    }
                }
            // Deletes the Context with given id.
            case KContextDelete:
                {
                return DeleteContext(aOption);
                }
            // Activates the Context with given id.
            case KContextActivate:    
                if(iControlOption == KContextActivateFail)
                    {
                    return KErrTest;
                    }
                else
                    {
                    return ActivateContext(aOption);
                    }
            // Commits changes on an active context to the network
            case KContextModifyActive:
                {
                if(iControlOption == KContextModifyActiveFail)
                    {
                    return KErrTest;
                    }
                else
                    {
                    return ModifyActive(aOption);
                    }
                }
            // Sets the QoS of a context
            case KContextQoSSet:
                {
                if(iControlOption == KContextQoSSetFail)
                    {
                    return KErrTest;
                    }
                else
                    {
                    return SetContextQoS(aOption);
                    }
                }
            // Traffic Flow Template modification: 
            // add filter/remove filter/delete TFT
            case KContextTFTModify:
                {
                if(iControlOption == KContextTFTModifyFail)
                    {
                    return KErrTest;
                    }
                else
                    {
                    return ContextTFTModify(aOption);
                    }
                }
            // <- END WCDMA options 

            // New communication etc. related options 
            // designed for UMTSNif START ->
            case KRegisterEventHandler :
                {
                TEvent *eventHandlerWrapperPointer = (TEvent *)aOption.Ptr();
                iEventHandler = (MNifEvent *)
                                    eventHandlerWrapperPointer->iEvent;
                if(iEventHandler != NULL)
                    {
                    LOG(Log::Printf(
                        _L("<%s> Registered and event handler <%d>"),
                        iNifUniqueName.PtrZ(),
                        iEventHandler));
                    return KErrNone; // Registration OK
                    }
                else 
                    {
                    LOG(Log::Printf(
                        _L("<%s> Event handler registration failed"),
                        iNifUniqueName.PtrZ()));
                    return KErrArgument; // Null pointer registration
                    }
                }        
            case KContextSetEvents :
                {
                if(iEventHandler == NULL) // No event handler registered
                    {
                    return KErrGeneral;
                    }
                
                TBool *eventsOnPtr = (TBool *)aOption.Ptr();
                iEventsOn = *eventsOnPtr;

                // If the Nif has already active contexts, 
                // send a KPrimaryContextCreated event
                if(iEventsOn && ContextManager()->ContextCount() > 0)
                    {
                    ContextManager()->SendPrimaryContextEvent();
                    }

                if(iEventsOn)
                    {
                    LOG(Log::Printf(_L("<%s> Events set ON"),
                        iNifUniqueName.PtrZ()));
                    }
                else
                    {
                    LOG(Log::Printf(_L("<%s> Events set OFF"),
                        iNifUniqueName.PtrZ()));
                    }
                return KErrNone; // Events set On/Off
                }
            // Get required plug-in 
            case KSoIfControllerPlugIn: 
                {
                TSoIfControllerInfo& opt = *(TSoIfControllerInfo*)
                                               aOption.Ptr();
                
                RFs fs;
                RFile file;
                fs.Connect();
                if(file.Open(fs, 
                             KUseTestModule, 
                             EFileShareAny|EFileWrite|EFileStreamText) 
                             == KErrNone)
                    {
                    _LIT(KUmtsPlugInName, "testmodule");
                    opt.iPlugIn = KUmtsPlugInName;
                    opt.iProtocolId = 361;
                    file.Close();
                    }
                else if(file.Open(fs, 
                                  KUseTestModuleNonExist, 
                                  EFileShareAny|EFileWrite|EFileStreamText) 
                                  == KErrNone)
                    {
                    _LIT(KUmtsPlugInName, "whatever");
                    opt.iPlugIn = KUmtsPlugInName;
                    opt.iProtocolId = 369;
                    file.Close();
                    }
                // no module            
                else if(file.Open(fs, 
                                  KUseNoModule, 
                                  EFileShareAny|EFileWrite|EFileStreamText) 
                                  == KErrNone)
                    {
                    file.Close();
                    fs.Close();
                    // return nonzero value for not loading .prt by qos 
                    return KErrNotFound;
                    }
                else // normal case
                    {                
                    _LIT(KUmtsPlugInName, "guqos");
                    opt.iPlugIn = KUmtsPlugInName;
                    opt.iProtocolId = 360;
                    }
                fs.Close();

                LOG(Log::Printf(
                    _L("<%s> Plug-in info requested"),
                    iNifUniqueName.PtrZ()));
                LOG(Log::Printf(
                    _L("\tPlug-in name:\t\t<%s>"),opt.iPlugIn.PtrZ()));
                LOG(Log::Printf(
                    _L("\tPlug-in protocolID:\t<%d>"),opt.iProtocolId));
                
                return KErrNone;
                }
            // Option to get IAP- and network IDs
            case KSoIfGetConnectionInfo:
                {
                TSoIfConnectionInfo& opt = *(TSoIfConnectionInfo*)
                                                aOption.Ptr();
                
                TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(IAP);
                columnName.Append(TChar(KSlashChar));
                columnName.Append(TPtrC(COMMDB_ID));
                TInt ret = iNotify->ReadInt(columnName, opt.iIAPId);
                if(ret != KErrNone)
                    {
                    return KErrNotSupported;
                    }

                // ** Add fetch for network ID once it exists in CommDb **
                
                columnName.Copy(TPtrC(IAP));
                columnName.Append(KSlashChar);
                columnName.Append(TPtrC(IAP_NETWORK));
                TInt err = KErrNone;
                if ((err = iNotify->ReadInt(columnName, opt.iNetworkId)) 
                        != KErrNone)
                        {
                        return err;
                        }
                
                LOG(Log::Printf(
                    _L("<%s> Network info requested"),
                    iNifUniqueName.PtrZ()));
                LOG(Log::Printf(
                    _L("\tIAP id:\t<%d>"),opt.iIAPId));
                LOG(Log::Printf(
                    _L("\tNetwork id:\t<%d>"),opt.iNetworkId));
                
                return KErrNone;
                
                }
            // <- END New communication etc. related options 
            // designed for UMTSNif

            // standard configuration stuff below:
            // Just added static data to get things running. Replace once 
            // it can be configured for real
            case KSoIfHardwareAddr:
                {
                return KErrNotSupported;
                }
            case KSoIfConfig:
                {
                // iNetworkName
                // This should be replaced once the IP-address is actually 
                // received from GPDS
                TSoInetIfConfig& opt = *(TSoInetIfConfig*)aOption.Ptr();
                
                // Hybrid IPv4 / IPv6 
                if (opt.iFamily!=KAfInet && opt.iFamily!=KAfInet6)
                    {
                    return KErrNotSupported;
                    }
                else if(iNetworkName.CompareF(KDescIp6) == 0 && 
                        opt.iFamily == KAfInet)
                    {
                    return KErrNotSupported;
                    }
                else if(iNetworkName.CompareF(KDescIp) == 0 && 
                        opt.iFamily == KAfInet6)
                    {
                    return KErrNotSupported;
                    }                

                if (opt.iFamily == KAfInet ) 
                    {
                    TInetAddr::Cast(opt.iConfig.iAddress).SetAddress(
                        28870678);
                    TInetAddr::Cast(opt.iConfig.iNetMask).SetAddress(0);
                    TInetAddr::Cast(opt.iConfig.iBrdAddr).SetAddress(
                        28870678);
                    TInetAddr::Cast(opt.iConfig.iDefGate).SetAddress(
                        28870678); //2887067833
                    TInetAddr::Cast(opt.iConfig.iNameSer1).SetAddress(0);
                    TInetAddr::Cast(opt.iConfig.iNameSer2).SetAddress(0);

                    LOG(Log::Printf(
                        _L("<%s> SoIfConfig requested, return IPv4 info"),
                        iNifUniqueName.PtrZ()));
                    }
                else 
                    {
                    if ((TUint)aOption.MaxLength() < 
                        sizeof (TSoInet6IfConfig))
                        {
                        return KErrArgument;
                        }

                    TSoInet6IfConfig& opt = *(TSoInet6IfConfig*)
                                                aOption.Ptr();
                    if (opt.iFamily != KAfInet6)
                        {
                        return KErrNotSupported;
                        }

                    TEui64Addr* ifId = (TEui64Addr*)&opt.iLocalId;

                    TE64Addr iLocalIfId;

                    const TUint8 constantId[8] = { 0x00, 0x00, 0x00, 0x00, 
                                                   0x00, 0x00, 0x00, 0x63 };
                    iLocalIfId.SetAddr(constantId, sizeof (constantId));
                    ifId->Init();
                    ifId->SetAddress(iLocalIfId);
                
                    ifId = (TEui64Addr*)&opt.iRemoteId;
                    TE64Addr iRemoteIfId;
                    iRemoteIfId.SetAddrRandomNZButNot(iLocalIfId);
                    ifId->Init();
                    ifId->SetAddress(iRemoteIfId);

                    opt.idPaddingBits = 0;

                    LOG(Log::Printf(
                        _L("<%s> SoIfConfig requested, return IPv6 info"),
                        iNifUniqueName.PtrZ()));
                    }

                return KErrNone;
                }
            case KSoIfInfo:
                {
                if(iNetworkName.CompareF(KDescIp6) == 0)
                    {
                    return KErrNotSupported;
                    }
                
                TSoIfInfo& opt = *(TSoIfInfo*)aOption.Ptr();
                opt.iName = iNifUniqueName; 
                
                opt.iFeatures = KUmtsIfaceFeatures | KIfIsDialup;
                opt.iMtu = KUmtsDefaultFrameSize;
                opt.iSpeedMetric = KUmtsSpeedMetric;
                
                LOG(Log::Printf(
                    _L("<%s> SoIfInfo requested:"),iNifUniqueName.PtrZ()));
                LOG(Log::Printf(_L("\tName:\t<%s> "),opt.iName.PtrZ()));
                LOG(Log::Printf(_L("\tMTU:\t<%d> "),opt.iMtu));
                
                return KErrNone;
                }            
            case KSoIfInfo6:
                {
                if(iNetworkName.CompareF(KDescIp) == 0)
                    {
                    return KErrNotSupported;
                    }

                TSoIfInfo6& opt  = *(TSoIfInfo6*)aOption.Ptr();
                opt.iName        = iNifUniqueName; 
                
                opt.iFeatures    = KUmtsIfaceFeatures | 
                                   KIfCanMulticast    | 
                                   KIfIsDialup;
                opt.iMtu         = KUmtsDefaultFrameSize;
                opt.iSpeedMetric = KUmtsSpeedMetric;
                opt.iRMtu        = KUmtsDefaultFrameSize;

                LOG(Log::Printf(
                    _L("<%s> SoIfInfo6 requested:"),iNifUniqueName.PtrZ()));
                LOG(Log::Printf(_L("\tName:\t<%s> "),opt.iName.PtrZ()));
                LOG(Log::Printf(_L("\tMTU:\t<%d> "),opt.iMtu));

                return KErrNone;
                }
            // Test Interface cases:
            case KTest:
                {
                return KErrNone;
                }
            case KSetQoSFail:
                {
                iControlOption = KSetQoSFail;
                return KErrNone;
                }
            case KNetworkContextDelete:
                {
                if(iContextId < 0)
                    {
                    LOG(Log::Printf(
                        _L("ContextId: %d. Aborting TestAPI initiated \
KNetworkContextDelete"), iContextId));
                    return KErrNotFound;
                    }

                iContextManager->Context(
                    iContextId)->NetworkInitiatedDeletion();
                return KErrNone;
                }
            case KDropContext:
                {
                DropContext();
                return KErrNone;
                }
            case KReNegotiateFail:
                {
                iControlOption = KReNegotiateFail;
                return KErrNone;
                }
            case KDowngrade:
            case KNotifySecondaryCreated:
            case KSetDefaultQoSFail:
            case KContextCreateFail:
            case KContextActivateFail:
            case KContextModifyActiveFail:
            case KContextQoSSetFail:
            case KContextTFTModifyFail:
            case KContextCreateFailAsync:
            case KContextActivateFailAsync:
            case KContextModifyActiveFailAsync:
            case KContextQoSSetFailAsync:
            case KContextTFTModifyFailAsync:
            case KPrintRel99:
            case KResetMessage:     // To reset the message values back 
            case KNetworkDowngrade: // To downgrade qos values by network
            case KNetworkUpgrade:   // To downgrade qos values by network
            case KContextActivateRejectSBLP: // To create the SBLP rejection error code
            case KNetworkUnsupportedIms: // To create the IMS unsupported network scenerio
            case KNetworkDowngradeR5: // To downgrade the umts r5 parameters
            case KNetworkUnsupportedUmtsR5: // To create the IMS unsupported network scenerio
                {
                iControlOption = aName;
                return KErrNone;
                }
            default:
                break;
            } 
        return KErrNotSupported;
        }
    return KErrNotSupported;
    }

void CUmtsNif::SecondaryContextCreated()
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Open(fs, KTestFile1, EFileStreamText);
    if(err == KErrNotFound)
        {
        file.Create(fs, KTestFile1, EFileStreamText);
        }
    else
        {
        file.Create(fs, KTestFile2, EFileStreamText);
        }
    file.Close();
    fs.Close();
    }

TUint CUmtsNif::ControlOption()
    {
    return iControlOption;
    }

void CUmtsNif::DropContext()
    {
    TContextParameters *contextDeletedEvent = NULL;
    TRAPD(err, contextDeletedEvent = new (ELeave) TContextParameters);
    if (err)
        {
        return;
        }
    contextDeletedEvent->iTFTOperationCode = 0;
    contextDeletedEvent->iContextType = ESecondaryContext;
    contextDeletedEvent->iReasonCode = KErrNone;
    contextDeletedEvent->iContextInfo.iContextId = iContextId;

    TPckg<TContextParameters> event(*contextDeletedEvent);
    
    EventHandler().Event((CProtocolBase*)this,KContextDeleteEvent,event);
    delete contextDeletedEvent;
    contextDeletedEvent=NULL;
    }

// Checks the context id of the packet and 
// forwards it to the correct sender object
TInt CUmtsNif::Send(RMBufChain& aPacket, TAny*)
    {
    RMBufPacket packet;
    packet.Assign(aPacket);
    RMBufPktInfo* info = packet.Unpack();
    
    TSockAddr destination = info->iDstAddr;
    // iDstAddr.Port contains the context id 
    TUint contextId = destination.Port(); 

    packet.Pack(); 
    
    // Send the packet on the correct channel    
    iContextManager->Context((TInt8)contextId)->Send(aPacket);
    
    // Context should set this return value to != 1 if flow is blocked
    return iContextManager->Context((TInt8)contextId)->FlowOn(); 
    }

void CUmtsNif::ReceivePacket(RMBufChain& aPacket)
    {
    if (iNetwork) 
        {
        iNetwork->Process(aPacket, (CProtocolBase*)this);
        }
    else 
        {
        aPacket.Free();
        }
    }

TInt CUmtsNif::SetDefaultQoS(TDes8& aOption) 
    {
    // If already up, should return PrimaryCreatedEvent 
    TContextParameters& parameters = *(TContextParameters *)aOption.Ptr();
    parameters.iContextConfig.GetUMTSQoSReq(iDefaultQoS);
    iDefaultQoSSet = TRUE;

    return KErrNone;
    }

/*
 *  Method to start a new primary context:
 *   - Open context
 *   - Set config
 *   - Set QoS
 *  Used when creating a new Nif
 */
TInt CUmtsNif::CreateStandAlonePrimaryContext() 
    {
    TInt ret = 0;

    LOG(Log::Printf(_L("<%s> Starting to create a new primary PDP context"),
        iNifUniqueName.PtrZ()));

    TContextParameters *parameters = NULL;

    TRAPD(err, parameters = new (ELeave) TContextParameters);
    if (err)
        {
        return KErrNoMemory;
        }

    // Fetch configuration from CommDb START ->
    // PDP-type

    // Using only the IfNetworks info 
    // Convert PDP-type
    if(iNetworkName.CompareF(KDescIp6) == 0) 
        {
        parameters->iContextConfig.SetPdpType(
            (RPacketContext::TProtocolType)RPacketContext::EPdpTypeIPv6);
        }
    // Convert PDP-type
    else if(iNetworkName.CompareF(KDescIp) == 0)
        {
        parameters->iContextConfig.SetPdpType(
            (RPacketContext::TProtocolType)RPacketContext::EPdpTypeIPv4);
        }

    TBuf8<KCommsDbSvrMaxFieldLength+1>     stringValue;
    TBuf<KCommsDbSvrMaxColumnNameLength+1> columnBuffer =
                                               TPtrC(OUTGOING_WCDMA);

    columnBuffer.Append(TChar(KSlashChar));
    columnBuffer.Append(TPtrC(GPRS_PDP_ADDRESS));
    ret = iNotify->ReadDes(columnBuffer, stringValue);
    parameters->iContextConfig.SetPdpAddress(stringValue);

    columnBuffer=TPtrC(OUTGOING_WCDMA);
    columnBuffer.Append(TChar(KSlashChar));
    columnBuffer.Append(TPtrC(GPRS_APN));

    ret = iNotify->ReadDes(columnBuffer, stringValue);
    parameters->iContextConfig.SetAccessPointName(stringValue);

    // Anonymous Access Required
    TBool anonymous;
    columnBuffer=TPtrC(OUTGOING_WCDMA);
    columnBuffer.Append(TChar(KSlashChar));
    columnBuffer.Append(TPtrC(GPRS_ANONYMOUS_ACCESS));

    ret = iNotify->ReadBool(columnBuffer, anonymous);

    // Pdp compression
    // Header
    TBool headerCompression;
    columnBuffer=TPtrC(OUTGOING_WCDMA);
    columnBuffer.Append(TChar(KSlashChar));
    columnBuffer.Append(TPtrC(GPRS_HEADER_COMPRESSION));
    ret = iNotify->ReadBool(columnBuffer, headerCompression);

    // Data
    TBool dataCompression;
    columnBuffer=TPtrC(OUTGOING_WCDMA);
    columnBuffer.Append(TChar(KSlashChar));
    columnBuffer.Append(TPtrC(GPRS_DATA_COMPRESSION));
    ret = iNotify->ReadBool(columnBuffer, dataCompression);

    // Join data & header
    TUint compression = 0;
    if(dataCompression)
        {
        compression |= RPacketContext::KPdpDataCompression;
        }
    if(headerCompression) 
        {
        compression |= RPacketContext::KPdpHeaderCompression;
        }
    parameters->iContextConfig.SetPdpCompression(compression);

    if(!iDefaultQoSSet)
        {
        LOG(Log::Printf(
            _L("<%s> Primary context creation: No default QoS set -> \
Fetching QoS from CommDb"),iNifUniqueName.PtrZ()));            

        // Fetch QoS-parameters from CommDb
        // For now, just set to some values.
        RPacketQoS::TQoSR5Requested *QoS = NULL;
        TRAP(err, QoS = new (ELeave) RPacketQoS::TQoSR5Requested());
        if (err)
            {
            delete parameters;
            return KErrNoMemory;
            }
        QoS->iReqTrafficClass      = RPacketQoS::ETrafficClassStreaming;
        QoS->iMinTrafficClass      = RPacketQoS::ETrafficClassBackground;
        QoS->iReqDeliveryOrderReqd = RPacketQoS::EDeliveryOrderUnspecified;
        QoS->iMinDeliveryOrderReqd = RPacketQoS::EDeliveryOrderUnspecified;

        QoS->iReqDeliverErroneousSDU = 
            RPacketQoS::EErroneousSDUDeliveryRequired;
        QoS->iMinDeliverErroneousSDU = 
            RPacketQoS::EErroneousSDUDeliveryRequired;

        QoS->iReqMaxSDUSize                      = 1500;
        QoS->iMinAcceptableMaxSDUSize            = 1200;
        QoS->iReqMaxRate.iUplinkRate             = 10;
        QoS->iReqMaxRate.iDownlinkRate           = 10;
        QoS->iMinAcceptableMaxRate.iUplinkRate   = 20;
        QoS->iMinAcceptableMaxRate.iDownlinkRate = 20;

        QoS->iReqBER           = RPacketQoS::EBERFivePerHundred;
        QoS->iMaxBER           = RPacketQoS::EBERFivePerHundred;
        QoS->iReqSDUErrorRatio = RPacketQoS::ESDUErrorRatioOnePerTen;
        QoS->iMaxSDUErrorRatio  = RPacketQoS::ESDUErrorRatioOnePerTen;

        QoS->iReqTrafficHandlingPriority = RPacketQoS::ETrafficPriority2;
        QoS->iMinTrafficHandlingPriority = RPacketQoS::ETrafficPriority3;

        QoS->iReqTransferDelay                = 15;
        QoS->iMaxTransferDelay                = 15;
        QoS->iReqGuaranteedRate.iUplinkRate   = 50;
        QoS->iReqGuaranteedRate.iDownlinkRate = 20;
        QoS->iMinGuaranteedRate.iUplinkRate   = 0;
        QoS->iMinGuaranteedRate.iDownlinkRate = 0;
        parameters->iContextConfig.SetUMTSQoSReq(*QoS);
        delete QoS;
        }
    else // GUQoS has set the default QoS
        {
        parameters->iContextConfig.SetUMTSQoSReq(iDefaultQoS);
        }
    // <- END Fetch configuration from CommDb 

    TContextId cid = 0;

    ret = iContextManager->CreateContext(cid);
    if(ret == 0)    // Context create ok
        {            
        parameters->iContextInfo.iContextId = cid;
        iContextManager->Context(cid)->IssueRequest(parameters,
            KStartupPrimaryContextCreation);
        }
    else            // Context creation failed
        {
        LOG(Log::Printf(
            _L("<%s> Primary context creation failed: error code <%d>"),
            iNifUniqueName.PtrZ(),ret));

        // We should signal agent here. Connection failed.
        iContextManager->Delete(cid);

        LOG(Log::Printf(
            _L("<%s> Sending NegotiationFailed signal to Nifman"),
            this->Name().PtrZ()));
        
        // Causes this CNifIfBase to be deleted
        Notify()->NegotiationFailed(this,KErrNone); 
        delete parameters;
        return cid;
        }

    delete parameters;
    return KErrNone;
    }

//
// GPRS/UMTS-specific signalling operations
//
TInt CUmtsNif::CreateSecondaryContext(TDes8& aOption) 
    {
    LOG(Log::Printf(_L("<%s> Open New Secondary Context"),
        iNifUniqueName.PtrZ()));

    TInt OperationStatus;
    TContextId cid = 0;
    TInt       ret = 0;

    TContextParameters& parameters = *(TContextParameters *)aOption.Ptr();

    ret = iContextManager->CreateContext(cid);

    if(ret == 0)    // Context created ok
        {
        parameters.iContextInfo.iContextId = cid;
        OperationStatus = KErrNone;            
        OperationStatus = iContextManager->Context(cid)->IssueRequest(
            &parameters,KContextCreate);

        // Store the contextId so it can be used with testAPI. 
        // Normally we get id from guqos, but with testAPI we 
        // need it straight from CUmtsNif.
        iContextId = cid;
        }
    else            // Context creation failed
        {            
        OperationStatus = ret;
        }

    parameters.iReasonCode = OperationStatus;
    return KErrNone;
    }

TInt CUmtsNif::DeleteContext(TDes8& aOption)
    {
    TInt OperationStatus;

    TContextParameters& parameters = *(TContextParameters * )aOption.Ptr();
    TInt8 cid = (TInt8)parameters.iContextInfo.iContextId;    

    LOG(Log::Printf(_L("<%s>[Context %d] Delete called"),
        iNifUniqueName.PtrZ(),cid ));

    TInt ret = iContextManager->Delete(cid);

    if(ret == 0)    // Delete issued OK
        {            
        parameters.iContextInfo.iContextId = cid;
        OperationStatus = KErrNone;                    
        }
    else            // Context deletion failed
        {
        OperationStatus = cid;
        LOG(Log::Printf(_L("<%s> Issued delete failed, reason <%d>"),
            iNifUniqueName.PtrZ(),ret));
        }
    parameters.iReasonCode = OperationStatus;
    return KErrNone;
    }

TInt CUmtsNif::LastContextDown()
    {    
    LOG(Log::Printf(_L("<%s> Last context deleted. Indicating upper layer"),
        iNifUniqueName.PtrZ()));

    iDownFlag = ETrue;

    // CNifIfLink object can be destroyed before 
    // the CNifIfBase objects are destroyed.
    if (iLinkLayer)
        {
        iLinkLayer->NifDown();
        }
    return KErrNone;
    }

//
// Returns true if the Nif is down
//
TBool CUmtsNif::Down()
    {
    return iDownFlag; 
    }

TInt CUmtsNif::ActivateContext(TDes8& aOption)
    {
    TContextParameters& parameters = *(TContextParameters * )aOption.Ptr();
    TInt OperationStatus;

    TInt8 cid = (TInt8)parameters.iContextInfo.iContextId;

    OperationStatus = iContextManager->Context(cid)->IssueRequest(
                          &parameters,KContextActivate);

    parameters.iContextInfo.iContextId = cid; 
    parameters.iReasonCode = OperationStatus;

    return 0;
    }

TInt CUmtsNif::SetContextQoS(TDes8& aOption) 
    {
    TInt OperationStatus;
    TContextParameters& parameters = *(TContextParameters *)aOption.Ptr();
    TContextInfo context = parameters.iContextInfo;

    TInt8 cid = (TInt8)context.iContextId;    

    OperationStatus = iContextManager->Context(cid)->IssueRequest(
        &parameters,KContextQoSSet); 

    parameters.iContextInfo.iContextId = cid;
    parameters.iReasonCode = OperationStatus;

    return KErrNone;
    }

TInt CUmtsNif::ContextTFTModify(TDes8& aOption) 
    {
    TInt OperationStatus;

    TContextParameters& parameters = *(TContextParameters *)aOption.Ptr();
    TContextInfo context = parameters.iContextInfo;
    TInt8 cid = (TInt8)context.iContextId;    

    OperationStatus = iContextManager->Context(cid)->IssueRequest(
        &parameters,KContextTFTModify);

    parameters.iContextInfo.iContextId = cid;
    parameters.iReasonCode = OperationStatus;

    return KErrNone;
    }

TInt CUmtsNif::ModifyActive(TDes8& aOption) 
    {
    TInt OperationStatus;
    TContextParameters& parameters = *(TContextParameters *)aOption.Ptr();
    TContextInfo context = parameters.iContextInfo;

    TInt8 cid = (TInt8)context.iContextId;    

    OperationStatus = iContextManager->Context(cid)->IssueRequest(
                          &parameters,KContextModifyActive);

    parameters.iContextInfo.iContextId = cid;
    parameters.iReasonCode = OperationStatus;

    return KErrNone;
    }

CProtocolBase *CUmtsNif::Network() const
    {
    return iNetwork;    
    }

TInt CUmtsNif::Notification(TAgentToNifEventType /*aEvent*/, void* /*aInfo*/)
    {
    //aEvent;
    return KErrNone;
    }

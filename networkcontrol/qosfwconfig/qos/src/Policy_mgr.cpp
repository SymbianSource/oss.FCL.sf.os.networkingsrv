// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <es_ini.h>
#include <f32file.h>

#include "policy_mgr.h"
#include "qos_prot.h"
#include "policy_sap.h"
#include "qoserr.h"

CQoSPolicyMgr::CQoSPolicyMgr()
	{
	LOG(Log::Printf(_L("new\tqos Policy Manager[%u] size=%d"), (TInt)this, sizeof(CQoSPolicyMgr)));
	TUint i;
	for (i = 0; i < KPolicyTableSize; i++)
		{
		iPolicies[i].SetOffset(_FOFF(CSelectorBase, iNext));
		}
	iFiles.SetOffset(_FOFF(TPolicyFile, iLink));
	}


CQoSPolicyMgr* CQoSPolicyMgr::NewL(const TDesC& aFileName)
	{
	CQoSPolicyMgr* mgr = new (ELeave) CQoSPolicyMgr();
	CleanupStack::PushL(mgr);
	mgr->ConstructL(aFileName);
	CleanupStack::Pop();
	return mgr;
	}


CQoSPolicyMgr::~CQoSPolicyMgr()
	{
	LOG(Log::Printf(_L("~\tqos Policy Manager[%u] destruct -- start"), (TInt)this, sizeof(CQoSPolicyMgr)));
	TDblQueIter<TPolicyFile> iter(iFiles);
	TPolicyFile* file;
	while ((file=iter++)!=NULL)
		{
		file->iLink.Deque();
		LOG(Log::Printf(_L("~\tqos TPolicyFile[%u] '%S' deleting"), (TInt)file, &file->iName));
		delete file;
		}
	Flush();
	LOG(Log::Printf(_L("~\tqos Policy Manager[%u] destruct -- done"), (TInt)this, sizeof(CQoSPolicyMgr)));
	}

void CQoSPolicyMgr::ConstructL(const TDesC& aFileName)
	{
	TRAPD(err, LoadFileL(aFileName));
	if( err == KErrNotFound)
		{
		TRAP(err, LoadFileL(KQosPoliciesIniFileC));
		LOG(Log::Printf(_L("\tqos Policy Manager[%u] Policyfile [%S] loaded [error code = %d]"), 
			(TInt)this, &KQosPoliciesIniFileC, err));
		}
	else
		{
		LOG(Log::Printf(_L("\tqos Policy Manager[%u] Policyfile [%S] loaded [error code = %d]"), 
			(TInt)this, &aFileName, err));
		}
	}

CPolicySelector* CQoSPolicyMgr::AddPolicyL(TPfqosMessage &aMsg)
	{
	CPolicySelector* sel = new (ELeave) CPolicySelector(aMsg);
	sel->SetQoSParameters(aMsg.iFlowSpec);
	AddPolicy(*sel);
	return sel;
	}

void CQoSPolicyMgr::AddModulePolicyL(TPfqosMessage &aMsg, TUint aOwner)
	{
	if (aMsg.iNumModules == 0)
		{
		User::Leave(KErrArgument);
		}

	CModuleSelector *mod = (CModuleSelector*)ExactMatch(aMsg, EPfqosModulespecPolicy);
	if (mod)
		{
		User::Leave(KErrAlreadyExists);
		}

	mod = new (ELeave) CModuleSelector(aMsg);
	mod->iOwner = aOwner;
	CleanupStack::PushL(mod);
	TSglQueIter<TPfqosModule> iter(aMsg.iModuleList);
	TPfqosModule *item;

	while ((item = iter++) != NULL)
		{
		CModuleSpec* mspec = CModuleSpec::NewL(*item);
		mod->AddModuleSpec(*mspec);
		}
	CleanupStack::Pop();
	AddPolicy(*mod);
	}

void CQoSPolicyMgr::AddExtensionPolicyL(TPfqosMessage &aMsg, TUint aOwner)
	{
	if (aMsg.iExtensions.IsEmpty())
		{
		return;
		}
	CExtensionPolicy* sel = new (ELeave) CExtensionPolicy(aMsg.iBase, 
							  aMsg.iSelector, aMsg.iSrcAddr, aMsg.iDstAddr, 
							  EPfqosExtensionPolicy);
	LOG(Log::Printf(_L("new\tqos selector[%u] EPfqosExtensionPolicy(%d) size=%d"), (TInt)sel, sel->iType, sizeof(CExtensionPolicy)));
	sel->iOwner = aOwner;
	CleanupStack::PushL(sel);

	TSglQueIter<CPfqosPolicyData> iter(aMsg.iExtensions);
	CPfqosPolicyData *data;
	while ((data = iter++) != NULL)
		{
		sel->AddExtensionL(data->Data());
		}

	CleanupStack::Pop();
	AddPolicy(*sel);
	}

CSelectorBase* CQoSPolicyMgr::FindPolicy(const CFlowContext* iContext, 
					 TUint aType, const TUidType& aUid, 
					 TUint32 aIapId, TUint aPriority, const TDesC& aName)
	{
	CSelectorBase *sel = ExactMatch(iContext->LocalAddr(), 
									iContext->RemoteAddr(), 
									iContext->Protocol(),
									(TUint16)iContext->LocalPort(), 
									(TUint16)iContext->RemotePort(), aType, 
									aUid, aIapId, aPriority, aName);
	if (sel == NULL)
		{
		sel = Match(iContext->LocalAddr().Ip6Address(), 
				 iContext->RemoteAddr().Ip6Address(), 
				 iContext->Protocol(), iContext->LocalPort(), 
				 iContext->RemotePort(), aType, aUid, aIapId, 
				 aPriority, aName);
		}
#ifdef _LOG
	if (sel)
		{
		Log::Printf(_L("\tqos Policy Manager[%u] FindPolicy type=%d found selector[%u]"), (TInt)this, aType, (TInt)sel);
		DumpSelector(*sel);	
		}
	else
		Log::Printf(_L("\tqos Policy Manager[%u] FindPolicy type=%d not found"), (TInt)this, aType);
#endif
	return sel;
	}


CSelectorBase* CQoSPolicyMgr::Match(const TIp6Addr &aLocal, 
					const TIp6Addr &aRemote, TUint aProtocol, 
					TUint aLocalPort, TUint aRemotePort, TUint aType, 
					const TUidType& aUid, TUint32 aIapId, TUint aPriority, 
					const TDesC& aName)
	{
	TPolicyQueIter iter(iPolicies[HashKey(aType)]);
	CSelectorBase* s;
	TInetAddr src, dst;

	src.SetAddress(aLocal);
	dst.SetAddress(aRemote);

	while ((s = iter++) != NULL)
		{
		if ((s->iType == aType && s->iPriority == aPriority) && 
			(s->iDst.IsUnspecified() || dst.Match(s->iDst, s->iDstMask)) &&
			(s->iSrc.IsUnspecified() || src.Match(s->iSrc, s->iSrcMask)) &&
			(s->iProtocol == 0 || s->iProtocol == aProtocol) &&
			(s->iDst.Port() == 0 || (aRemotePort >= s->iDst.Port() && 
				aRemotePort <= s->iDstPortMax)) &&
			(s->iSrc.Port() == 0 || (aLocalPort >= s->iSrc.Port() && 
				aLocalPort <= s->iSrcPortMax)) &&
			((s->iUid.UidType()[0].iUid == 0 && s->iUid.UidType()[1].iUid == 0
				&& s->iUid.UidType()[2].iUid == 0) ||
			s->iUid.UidType() == aUid) && (s->iIapId == 0 || 
			s->iIapId == aIapId) && (s->iName.Length() == 0 || 
			aName.Compare(s->iName) == 0))
			{
#if 0	//?? this log disabled now
			TBuf<60> tmpMask;
			TBuf<60> tmpAddr;
				
			Log::Printf(_L("\tqos Match() selector Name: %S, Proto: %d, type: %d, IapId: %d, Priority: %d"),
				&s->iName, s->iProtocol, s->iType, 
				s->iIapId, s->iPriority);
			s->iSrc.Output(tmpAddr);
			s->iSrcMask.Output(tmpMask);
			Log::Printf(_L("\tLocal  { %S, port: %d mask: %S }"), &tmpAddr, s->iSrc.Port(), &tmpMask); 
			s->iDst.Output(tmpAddr);
			s->iDstMask.Output(tmpMask);			
			Log::Printf(_L("\tRemote { %S, port: %d mask: %S }"), &tmpAddr, s->iDst.Port(), &tmpMask);
			Log::Printf(_L("\t UID { 0x%x 0x%x 0x%x } "), 
				s->iUid.UidType()[0].iUid,
				s->iUid.UidType()[1].iUid,
				s->iUid.UidType()[2].iUid
				);
#endif
			return s;
			}
		}

	return NULL;
	}


CSelectorBase* CQoSPolicyMgr::ExactMatch(const TInetAddr &aLocal, 
					 const TInetAddr &aRemote, TUint aProtocol,
					 TUint iSrcPortMax, TUint iDstPortMax, TUint aType, 
					 const TUidType& aUid, TUint32 aIapId, TUint aPriority, 
					 const TDesC& aName)
	{
	TPolicyQueIter iter(iPolicies[HashKey(aType)]);
	CSelectorBase* sel;

	while((sel = iter++) != NULL)
		{
#if 0	//?? this log disabled now 
		TBuf<60> local;
		TBuf<60> remote;
		aLocal.Output( local );
		aRemote.Output( remote );

		Log::Printf(_L("\t<---------------- EXACT - Matching the following supplied values --"));
		Log::Printf(_L("\taLocal: %S, aRemote: %S"),&local, &remote);
		Log::Printf(_L("\taProtocol: %d, iSrcPortMax: %d, iDstPortMax: %d"), aProtocol, iSrcPortMax, iDstPortMax);
		Log::Printf(_L("\taType: %d, aIapId: %d, aPriority: %d, aName: %S, aUid: %d %d %d"), 
			aType, aIapId, aPriority, &aName,aUid[0].iUid,aUid[1].iUid,aUid[2].iUid);		
		 
		Log::Printf(_L("\t------ against "));

		TBuf<60> local_mask;
		TBuf<60> remote_mask;

		sel->iSrc.Output( local);
		sel->iSrcMask.Output( local_mask );
		sel->iDst.Output( remote);
		sel->iDstMask.Output( remote_mask );

		Log::Printf(_L("\tiSrc: %S, iSrcMask: %S, iDst: %S, iDstMask: %S"),	&local, &local_mask, &remote, &remote_mask);
		Log::Printf(_L("\tiProtocol: %d, iSrcPortMax: %d, iDstPortMax: %d"), sel->iProtocol, sel->iSrcPortMax, sel->iDstPortMax);
		Log::Printf(_L("\tiType: %d, iIapId: %d, iPriority: %d, iName: %S, iUid: %d %d %d"), 
			sel->iType, sel->iIapId, sel->iPriority, &sel->iName,
			sel->iUid.UidType()[0].iUid, sel->iUid.UidType()[1].iUid,sel->iUid.UidType()[2].iUid);
		Log::Printf(_L("\t--------------------------------------------------------------->"));
		Log::Printf(_L("\t<--- Following condition(s) FAILS ---"));
		if (!(sel->iType == aType && sel->iPriority == aPriority))
			{
			Log::Printf(_L("\t\tsel->iType == aType && sel->iPriority == aPriority"));
			}
		if (!(aRemote.Match(sel->iDst, sel->iDstMask)))
			{
			Log::Printf(_L("\t\taRemote.Match(sel->iDst, sel->iDstMask"));
			}
		if (!(aLocal.Match(sel->iSrc, sel->iSrcMask)))
			{
			Log::Printf(_L("\t\taLocal.Match(sel->iSrc, sel->iSrcMask"));
			}
		if (!(sel->iProtocol == aProtocol))
			{
			Log::Printf(_L("\t\tsel->iProtocol == aProtocol"));
			}
		if (!(sel->iDst.Port() == aRemote.Port()))
			{
			Log::Printf(_L("\t\tsel->iDst.Port() == aRemote.Port"));
			}
		if (!(sel->iSrc.Port() == aLocal.Port()))
			{
			Log::Printf(_L("\t\tsel->iSrc.Port() == aLocal.Port"));
			}
		if (!(sel->iSrcPortMax == iSrcPortMax))
			{
			Log::Printf(_L("\t\tsel->iSrcPortMax == iSrcPortMax"));
			}
		if (!(sel->iDstPortMax == iDstPortMax))
			{
			Log::Printf(_L("\t\tsel->iDstPortMax == iDstPortMax"));
			}
		if (!(sel->iUid.UidType() == aUid))
			{
			Log::Printf(_L("\t\tsel->iUid.UidType() == aUid"));
			}
		if (!(sel->iIapId == aIapId))
			{
			Log::Printf(_L("\t\tsel->iIapId == aIapId"));
			}
		if (aName.Compare(sel->iName))
			{
			Log::Printf(_L("\t\taName.Compare(sel->iName)"));
			}
		Log::Printf(_L("\t------------------------------------>"));
		Log::Printf(_L(""));
#endif

		// if uid is not presented in the policy file (i.e, if sel->iUid == 0)
		// then don't compare the uid but compare all the other values of the 
		// selector else compare all the other values and along with uid value
		if ((sel->iUid.UidType() == TUid::Uid(0)))
			{
			if ((sel->iType == aType && sel->iPriority == aPriority) && 
				(aRemote.Match(sel->iDst, sel->iDstMask)) && 
				(aLocal.Match(sel->iSrc, sel->iSrcMask)) && 
				(sel->iProtocol == aProtocol) && 
				(sel->iDst.Port() == aRemote.Port()) && 
				(sel->iSrc.Port() == aLocal.Port()) && 
				(sel->iSrcPortMax == iSrcPortMax) && 
				(sel->iDstPortMax == iDstPortMax) &&
				(sel->iIapId == aIapId) && aName.Compare(sel->iName) == 0)
				{
#if 0	//?? this log disabled now 
				Log::Printf(_L("\tCQoSPolicyMgr::ExactMatch - there is a match! Following is the @@@@@@@@@@@@@ \n"));

				sel->iSrc.Output(local);
				sel->iDst.Output(remote);
				sel->iSrcMask.Output(local_mask);
				sel->iDstMask.Output(remote_mask);			
					
				Log::Printf(_L("\tCQoSPolicyMgr::ExactMatch() selector Name: %S, Proto: %d, type: %d, IapId: %d, Priority: %d"),
					&sel->iName, sel->iProtocol, sel->iType, sel->iIapId, sel->iPriority);
				Log::Printf(_L("\tLocal  { %S, port: %d mask: %S }"), &local, sel->iSrc.Port(), &local_mask); 
				Log::Printf(_L("\tRemote { %S, port: %d mask: %S }"), &remote, sel->iDst.Port(), &remote_mask);
				Log::Printf(_L("\t UID [0x%x 0x%x 0x%x] "), 
					sel->iUid.UidType()[0].iUid,
					sel->iUid.UidType()[1].iUid,
					sel->iUid.UidType()[2].iUid );
#endif
				return sel;
				}
			}
		else
			{
			if ((sel->iType == aType && sel->iPriority == aPriority) && 
				(aRemote.Match(sel->iDst, sel->iDstMask)) && 
				(aLocal.Match(sel->iSrc, sel->iSrcMask)) && 
				(sel->iProtocol == aProtocol) && 
				(sel->iDst.Port() == aRemote.Port()) && 
				(sel->iSrc.Port() == aLocal.Port()) && 
				(sel->iSrcPortMax == iSrcPortMax) && 
				(sel->iDstPortMax == iDstPortMax) &&
				(sel->iUid.UidType() == aUid) &&	 
				(sel->iIapId == aIapId) && aName.Compare(sel->iName) == 0)
				{
#if 0	//?? this log disabled now 
				Log::Printf(_L("\tCQoSPolicyMgr::ExactMatch - there is a match! Following is the @@@@@@@@@@@@@ \n"));

				sel->iSrc.Output(local);
				sel->iDst.Output(remote);
				sel->iSrcMask.Output(local_mask);
				sel->iDstMask.Output(remote_mask);
					
				Log::Printf(_L("\tCQoSPolicyMgr::ExactMatch() selector Name: %S, Proto: %d, type: %d, IapId: %d, Priority: %d"),
					&sel->iName, sel->iProtocol, sel->iType, sel->iIapId, sel->iPriority);
				Log::Printf(_L("\tLocal  { %S, port: %d mask: %S }"), &local, sel->iSrc.Port(), &local_mask); 
				Log::Printf(_L("\tRemote { %S, port: %d mask: %S }"), &remote, sel->iDst.Port(), &remote_mask);
				Log::Printf(_L("\t { UID [0x%x 0x%x 0x%x]"), 
					sel->iUid.UidType()[0].iUid,
					sel->iUid.UidType()[1].iUid,
					sel->iUid.UidType()[2].iUid);
#endif
				return sel;
				}
			}
		if ((sel->iType == aType && sel->iPriority == aPriority) && 
			(aRemote.Match(sel->iDst, sel->iDstMask)) && 
			(aLocal.Match(sel->iSrc, sel->iSrcMask)) && 
			(sel->iProtocol == aProtocol) && 
			(sel->iDst.Port() == aRemote.Port()) && 
			(sel->iSrc.Port() == aLocal.Port()) && 
			(sel->iSrcPortMax == iSrcPortMax) && 
			(sel->iDstPortMax == iDstPortMax) &&
			(sel->iUid.UidType() == aUid) &&	 
			(sel->iIapId == aIapId) && aName.Compare(sel->iName) == 0)
			{
#if 0	//?? this log disabled now 
			Log::Printf(_L("\tCQoSPolicyMgr::ExactMatch Following is the ***********@@@@@ 6\n"));

			sel->iSrc.Output(local);
			sel->iDst.Output(remote);
			sel->iSrcMask.Output(local_mask);
			sel->iDstMask.Output(remote_mask);

			Log::Printf(_L("\tCQoSPolicyMgr::ExactMatch() selector Name: %S, Proto: %d, type: %d, IapId: %d, Priority: %d"),
				&sel->iName, sel->iProtocol, 
				sel->iType, sel->iIapId, sel->iPriority);
			Log::Printf(_L("\tLocal  { %S, port: %d mask: %S }"), &local, sel->iSrc.Port(), &local_mask); 
			Log::Printf(_L("\tRemote { %S, port: %d mask: %S }"), &remote, sel->iDst.Port(), &remote_mask);				
			Log::Printf(_L("\t UID [0x%x 0x%x 0x%x ]"), 
				sel->iUid.UidType()[0].iUid,
				sel->iUid.UidType()[1].iUid, 
				sel->iUid.UidType()[2].iUid);
#endif
			return sel;
			}
		}
	return NULL;
	}

CSelectorBase* CQoSPolicyMgr::ExactMatch(TPfqosMessage& aMsg, 
										 TUint aPolicyType, 
										 TUint aPriority, 
										 const TDesC& aName)
	{
	TCheckedUid uid;
	aMsg.iSelector.GetUid(uid);
	CSelectorBase* sel = ExactMatch(*aMsg.iSrcAddr.iAddr, 
									*aMsg.iDstAddr.iAddr, 
									aMsg.iSelector.iExt->protocol, 
									aMsg.iSrcAddr.iExt->pfqos_port_max, 
									aMsg.iDstAddr.iExt->pfqos_port_max, 
									aPolicyType, uid.UidType(), 
									aMsg.iSelector.iExt->iap_id, 
									aPriority, aName);
	return sel;
	}


//
// Flush deletes all policies from database.
//
void CQoSPolicyMgr::Flush()
	{
	// ??? what happens to flows and other objects that still reference
	// ??? these selectors?
	for (TUint i = 0; i < KPolicyTableSize; i++)
		{
		TPolicyQueIter iter(iPolicies[i]);
		CSelectorBase *sel;
		while ((sel = iter++) != NULL)
			{
			sel->iNext.Deque();
			LOG(Log::Printf(_L("\tqos selector[%u] type=%d being deleted"), (TInt)sel, sel->iType));
			LOG(DumpSelector(*sel));	
			delete sel;
			}
		}
	}


TInt CQoSPolicyMgr::CreatePolicies(TPolicyParser& aParser, TUint aOwner)
	{
	TQoSPolicyQueueIter iter(aParser.Policies());
	CExtensionPolicy* policy;

	while ((policy = iter++) != NULL)
		{
		switch (policy->Type())
			{
			case EPfqosFlowspecPolicy:
				CreateFlowSpecPolicy(*policy, aOwner);
				break;
		
			case EPfqosModulespecPolicy:
				CreateModuleSpecPolicy(*policy, aOwner);
				break;
		
			default:
				policy->iNext.Deque();
				policy->iOwner = aOwner;
				AddPolicy(*policy);
				break;		
			}
		}
	return KErrNone;
	}


// Create a flowspec policy from a generic policy
TInt CQoSPolicyMgr::CreateFlowSpecPolicy(CExtensionPolicy& aPolicy, 
										 TUint aOwner)
	{
	CPolicySelector* flowPolicy = new CPolicySelector(aPolicy);
	if (flowPolicy == NULL)
		{
		return KErrNoMemory;
		}
	flowPolicy->iOwner = aOwner;

	//??? Why is iter used when there is no loop? Why not just
	//??? test if extensionqueue is non-empty and take the first?
	//??? What would it mean if there were more than one extesion?
	TExtensionQueueIter iter(aPolicy.Extensions());
	CExtension *extension;
	extension = iter++;
	TQoSParameters qos_parameters;
	if (extension)
		{
		qos_parameters.Copy(*extension);
		}

	flowPolicy->SetQoSParameters(qos_parameters);
	AddPolicy(*flowPolicy);
	return KErrNone;
	}


// Create a modulespec policy from a generic policy
TInt CQoSPolicyMgr::CreateModuleSpecPolicy(CExtensionPolicy& aPolicy, 
										   TUint aOwner)
	{
	CModuleSelector* modulePolicy = new CModuleSelector(aPolicy);
	if (modulePolicy == NULL)
		{
		return KErrNoMemory;
		}
	modulePolicy->iOwner = aOwner;

	TExtensionQueueIter iter(aPolicy.Extensions());
	CExtension *extension;

	// Iterate through extension list
	while ((extension = iter++) != NULL)
		{
		TVariableBase* var;
		CModuleSpec* spec=NULL;
		
		_LIT(KDescName, "module");
		var = extension->FindVariable(KDescName);
		if (var && var->Type() == KPfqosTypeString)
			{
			TRAPD(err, spec = CModuleSpec::NewL(((
				TStringVariable*)var)->Value(), extension->Name()));
			if (err != KErrNone)
				{
				delete modulePolicy;
				return err;
				}
			}
		else
			{
			continue;
			}
		_LIT(KDescId, "id");
		var = extension->FindVariable(KDescId);
		if (var && var->Type() == KPfqosTypeInteger)
			{
			spec->SetProtocolId(((TIntVariable*)var)->Value());
			}
		else
			{
			delete spec;
			continue;
			}
		_LIT(KDescFlags, "flags");
		var = extension->FindVariable(KDescFlags);
		if (var && var->Type() == KPfqosTypeInteger)
			{
			spec->SetFlags(((TIntVariable*)var)->Value());
			}
		else
			{
			delete spec;
			continue;
			}
		//??? What is this copying exactly? There doesn't seem to be
		//??? anything added to the CModuleSpec extensions in above?
		//??? And if something is copied, then for what purpose? They
		//??? would still be left in CModuleSpec which is added to the
		//??? policy selector anyway?
		TRAPD(err, extension->CopyL(*spec->PolicyData()));
		if (err != KErrNone)
			{
			LOG(Log::Printf(_L("extension->CopyL error: %d"), err));
			}
		modulePolicy->AddModuleSpec(*spec);
		}
	AddPolicy(*modulePolicy);
	return KErrNone;
	}


TInt CQoSPolicyMgr::DeletePolicies(TPolicyParser& aParser)
	{
	TQoSPolicyQueueIter iter(aParser.Policies());
	CExtensionPolicy* policy;

	while ((policy = iter++) != NULL)
		{
		CSelectorBase* selector = ExactMatch(policy->iSrc, policy->iDst, 
											 policy->iProtocol, 
											 policy->iSrcPortMax, 
											 policy->iDstPortMax, 
											 policy->iType, 
											 policy->iUid.UidType(), 
											 policy->iPriority);
		if (selector)
			{
			selector->iNext.Deque();
			LOG(Log::Printf(_L("\tqos Policy Manager[%u] DeletePolicies -- deleting selector:")));
			LOG(DumpSelector(*selector));	
			delete selector;
			}
		else
			{
			LOG(Log::Printf(_L("\tqos Policy Manager[%u] DeletePolicies -- didn't find match for:")));
			LOG(DumpSelector(*policy));
			}
		}
	return KErrNone;
	}

TInt CQoSPolicyMgr::LoadFileL(const TDesC& aFile)
	{
	TPolicyFile* policyfile = FindPolicyFile(aFile);

	if (policyfile)
		{
		User::Leave(KErrAlreadyExists);
		}
	TAutoClose<RFs> fs;
	User::LeaveIfError(fs.iObj.Connect());
	fs.PushL();
	TAutoClose<RFile> file;
	User::LeaveIfError(file.iObj.Open(fs.iObj, aFile, EFileRead));
	file.PushL();
	TInt size;
	User::LeaveIfError(file.iObj.Size(size));


	HBufC* file_data = HBufC::NewL(size);
	CleanupStack::PushL(file_data);

		{
#ifdef _UNICODE
		// Note: because file is narrow and enviroment is wide
		// this could just use the file_data as temp buffer and
		// "widen" the data inside that buffer.
		HBufC8* tmp_buf = HBufC8::NewL(size);
		CleanupStack::PushL(tmp_buf);
		TPtr8 tmp_ptr(tmp_buf->Des());
		User::LeaveIfError(file.iObj.Read(tmp_ptr));
		file_data->Des().Copy(tmp_ptr);
		CleanupStack::Pop();
		delete tmp_buf;
#else
		TPtr ptr(file_data->Des());
		User::LeaveIfError(file.iObj.Read(ptr));
#endif
		}

	policyfile = new (ELeave) TPolicyFile(aFile);
	//coverity[leave_without_push]
	LOG(Log::Printf(_L("new\tqos TPolicyFile[%u] '%S' size=%d"), (TInt)policyfile, &policyfile->iName, sizeof(TPolicyFile)));
	iFiles.AddLast(*policyfile);

	// Read QoS policies into stack from config file.
	TPolicyParser parser(*file_data);
	// ParseL must be trapped, because the TPolicyParser destructor must be executed.
	// (This is a temp hack, the callers seem to ignore the return value, and expect
	// a leave...)
	TRAPD(ret, ret = parser.ParseL());

	if (ret == KErrNone)
		{
		CreatePolicies(parser, (TUint)policyfile);
		}
	else
		{
		LOG(Log::Printf(_L("\tqos Policy Manager[%u] LoadFile [%S]: Error [%d] in policy file"), (TUint)this, &aFile, ret));
		}

	file.Pop();
	fs.Pop();
	CleanupStack::Pop();
	delete file_data;
	return ret;
	}


TInt CQoSPolicyMgr::UnloadFile(const TDesC& aFile, CProtocolQoS& aProtocol)
	{
	TPolicyFile* policyfile = FindPolicyFile(aFile);
	if (!policyfile)
		{
		return KErrNotFound;
		}
	DoCleanUp(aProtocol, (TUint)policyfile);
	policyfile->iLink.Deque();
	LOG(Log::Printf(_L("~\tqos TPolicyFile[%u] '%S' unloaded, deleting"), (TInt)policyfile, &policyfile->iName, sizeof(TPolicyFile)));
	delete policyfile;
	return KErrNone;
	}

void CQoSPolicyMgr::AddPolicy(CSelectorBase &aPolicy)
	{
#ifdef _LOG
	Log::Printf(_L("\tqos selector[%u] type=%d owner=%d AddPolicy"), (TInt)&aPolicy, aPolicy.iType, aPolicy.iOwner);
	Log::Printf(_L("\t<------------- POLICY DATABASE  --------------------------------"));
	DumpSelector(aPolicy);
	Log::Printf(_L("\t--------------------------------------------------------------->"));
#endif
	iPolicies[HashKey(aPolicy.iType)].AddLast(aPolicy);
	}

void CQoSPolicyMgr::DoCleanUp(CProtocolQoS& aProtocol, TUint aOwner)
	{
	for (TUint i = 0; i < KPolicyTableSize; i++)
		{
		TPolicyQueIter iter(iPolicies[i]);
		CSelectorBase *sel;
		
		while ((sel = iter++) != NULL)
			{
			// CQoSPolicyMgr::DoCleanUp() may be called by 
			// RQoSPolicy::UnloadPolicyFile(), by removing the 
			// KPfqosOptionDynamic test, policy that was loaded 
			// from a file may now be deleted too. 
				
			if (sel->iOwner == aOwner)
				{
				LOG(Log::Printf(_L("\tCQoSPolicyMgr::DoCleanup() Deleting policy object owned by %d"), sel->iOwner));
				LOG(DumpSelector(*sel));
				switch (sel->iType)
					{
				case EPfqosFlowspecPolicy:
					sel->iNext.Deque();
					aProtocol.Release((CPolicySelector*)sel);
					LOG(Log::Printf(_L("\tqos selector[%u] type=%d being deleted"), (TInt)sel, sel->iType));
					delete sel;
					break;
				
				case EPfqosModulespecPolicy:
				default:
					sel->iNext.Deque();
					LOG(Log::Printf(_L("\tqos selector[%u] type=%d being deleted"), (TInt)sel, sel->iType));
					delete sel;
					break;				
					}
				}
			}
		}
	}

TPolicyFile* CQoSPolicyMgr::FindPolicyFile(const TDesC& aName)
	{
	TDblQueIter<TPolicyFile> iter(iFiles);
	TPolicyFile* file;
	while((file=iter++)!=NULL)
		{
		if (file->iName.Compare(aName) == 0)
			{
			return file;
			}
		}
	return NULL;
	}


//
// Combine 'application priority' flowspec with 'default priority' flowspec.
//
TInt TPolicyCombine::CombineDefault(const TQoSParameters& aDefault, 
										  TQoSParameters& aSpec)
	{
	// Uplink
	if (aSpec.GetUpLinkDelay() == 0 && aDefault.GetUpLinkDelay() > 0)
		{
		aSpec.SetUpLinkDelay(aDefault.GetUpLinkDelay());
		}

	if (aSpec.GetUpLinkMaximumPacketSize() == 0 && 
		aDefault.GetUpLinkMaximumPacketSize() > 0)
		{
		aSpec.SetUpLinkMaximumPacketSize(
			aDefault.GetUpLinkMaximumPacketSize());
		}

	if (aSpec.GetUpLinkAveragePacketSize()   == 0 && 
		aDefault.GetUpLinkAveragePacketSize() > 0)
		{
		aSpec.SetUpLinkAveragePacketSize(
			aDefault.GetUpLinkAveragePacketSize());
		}

	if (aSpec.GetUpLinkPriority()	== 0xffff && 
		aDefault.GetUpLinkPriority() != 0xffff)
		{
		aSpec.SetUpLinkPriority(aDefault.GetUpLinkPriority());
		}

	if (aSpec.GetUpLinkMaximumBurstSize()   == 0 && 
		aDefault.GetUpLinkMaximumBurstSize() > 0)
		{
		aSpec.SetUpLinkMaximumBurstSize(aDefault.GetUpLinkMaximumBurstSize());
		}

	if (aSpec.GetUplinkBandwidth() == 0 && aDefault.GetUplinkBandwidth() > 0)
		{
		aSpec.SetUplinkBandwidth(aDefault.GetUplinkBandwidth());
		}

	// Downlink
	if (aSpec.GetDownLinkDelay() == 0 && aDefault.GetDownLinkDelay() > 0)
		{
		aSpec.SetDownLinkDelay(aDefault.GetDownLinkDelay());
		}

	if (aSpec.GetDownLinkMaximumPacketSize()   == 0 && 
		aDefault.GetDownLinkMaximumPacketSize() > 0)
		{
		aSpec.SetDownLinkMaximumPacketSize(
			aDefault.GetDownLinkMaximumPacketSize());
		}

	if (aSpec.GetDownLinkAveragePacketSize()   == 0 && 
		aDefault.GetDownLinkAveragePacketSize() > 0)
		{
		aSpec.SetDownLinkAveragePacketSize(
			aDefault.GetDownLinkAveragePacketSize());
		}

	if (aSpec.GetDownLinkPriority()	== 0xffff && 
		aDefault.GetDownLinkPriority() != 0xffff)
		{
		aSpec.SetDownLinkPriority(aDefault.GetDownLinkPriority());
		}

	if (aSpec.GetDownLinkMaximumBurstSize()   == 0 && 
		aDefault.GetDownLinkMaximumBurstSize() > 0)
		{
		aSpec.SetDownLinkMaximumBurstSize(
			aDefault.GetDownLinkMaximumBurstSize());
		}

	if (aSpec.GetDownlinkBandwidth()   == 0 && 
		aDefault.GetDownlinkBandwidth() > 0)
		{
		aSpec.SetDownlinkBandwidth(aDefault.GetDownlinkBandwidth());
		}

	return KErrNone;
	}


//
// Combine 'application priority' flowspec with 'override priority' flowspec.
//
TInt TPolicyCombine::CombineOverride(const TQoSParameters& aOverride, 
										   TQoSParameters& aSpec)
	{
	TQoSParameters temp = aSpec;

	// Uplink
	if (aOverride.GetUpLinkDelay() > 0)
		{
		if (aOverride.GetUpLinkDelay() < aSpec.GetUpLinkDelay())
			{
			aSpec.SetUpLinkDelay(aOverride.GetUpLinkDelay());
			}
		}

	if (aOverride.GetUpLinkMaximumPacketSize() > 0)
		{
		if (aOverride.GetUpLinkMaximumPacketSize() < 
			aSpec.GetUpLinkMaximumPacketSize())
			{
			aSpec.SetUpLinkMaximumPacketSize(
				aOverride.GetUpLinkMaximumPacketSize());
			}
		}

	if (aOverride.GetUpLinkAveragePacketSize() > 0)
		{
		if (aOverride.GetUpLinkAveragePacketSize() != 
			aSpec.GetUpLinkAveragePacketSize())
			{
			aSpec.SetUpLinkAveragePacketSize(
				aOverride.GetUpLinkAveragePacketSize());
			}
		}

	if (aOverride.GetUpLinkPriority() != 0xffff)
		{
		if (aOverride.GetUpLinkPriority() != aSpec.GetUpLinkPriority())
			{
			aSpec.SetUpLinkPriority(aOverride.GetUpLinkPriority());
			}
		}

	if (aOverride.GetUpLinkMaximumBurstSize() > 0)
		{
		if (aOverride.GetUpLinkMaximumBurstSize() < 
			aSpec.GetUpLinkMaximumBurstSize())
			{
			aSpec.SetUpLinkMaximumBurstSize(
				aOverride.GetUpLinkMaximumBurstSize());
			}
		}

	if (aOverride.GetUplinkBandwidth() > 0)
		{
		if (aOverride.GetUplinkBandwidth() < aSpec.GetUplinkBandwidth())
			{
			aSpec.SetUplinkBandwidth(aOverride.GetUplinkBandwidth());
			}
		}

	// Downlink
	if (aOverride.GetDownLinkDelay() > 0)
		{
		if (aOverride.GetDownLinkDelay() < aSpec.GetDownLinkDelay())
			{
			aSpec.SetDownLinkDelay(aOverride.GetDownLinkDelay());
			}
		}

	if (aOverride.GetDownLinkMaximumPacketSize() > 0)
		{
		if (aOverride.GetDownLinkMaximumPacketSize() < 
			aSpec.GetDownLinkMaximumPacketSize())
			{
			aSpec.SetDownLinkMaximumPacketSize(
				aOverride.GetDownLinkMaximumPacketSize());
			}
		}

	if (aOverride.GetDownLinkAveragePacketSize() > 0)
		{
		if (aOverride.GetDownLinkAveragePacketSize() != 
			aSpec.GetDownLinkAveragePacketSize())
			{
			aSpec.SetDownLinkAveragePacketSize(
				aOverride.GetDownLinkAveragePacketSize());
			}
		}

	if (aOverride.GetDownLinkPriority() != 0xffff)
		{
		if (aOverride.GetDownLinkPriority() != aSpec.GetDownLinkPriority())
			{
			aSpec.SetDownLinkPriority(aOverride.GetDownLinkPriority());
			}
		}

	if (aOverride.GetDownLinkMaximumBurstSize() > 0)
		{
		if (aOverride.GetDownLinkMaximumBurstSize() < 
			aSpec.GetDownLinkMaximumBurstSize())
			{
			aSpec.SetDownLinkMaximumBurstSize(
				aOverride.GetDownLinkMaximumBurstSize());
			}
		}

	if (aOverride.GetDownlinkBandwidth() > 0)
		{
		if (aOverride.GetDownlinkBandwidth() < aSpec.GetDownlinkBandwidth())
			{
			aSpec.SetDownlinkBandwidth(aOverride.GetDownlinkBandwidth());
			}
		}

	if (!(temp == aSpec))
		{
		return EQoSDowngradeForced;
		}
	else
		{
		return KErrNone;
		}
	}


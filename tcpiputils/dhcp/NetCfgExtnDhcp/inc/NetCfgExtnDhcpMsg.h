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
// NetCfgExtDhcpMsg.h
// DHCPv4/v6 specific control & IoCtl commands & structures
// 
//

/**
 @internalComponent
*/

#if !defined (__NET_CFG_EXTN_DHCP_MSG_H__)
#define __NET_CFG_EXTN_DHCP_MSG_H__

#include <e32base.h>

//-- for some DHCPv4 constants and data structures definition
#include <comms-infras/es_config.h>
#include <nifman.h>


/**
    Specific options for getting raw DHCP data
    @internalComponent
*/
const TUint KConnGetDhcp4RawOptionData = KConnGetDhcpRawOptionData;    //-- DHCPv4 specific alias
const TUint KConnGetDhcp6RawOptionData = KConnGetDhcpRawOptionData;    //-- DHCPv6 specific alias


/**
    Just a DHCPv4 specific alias, actually is defined in es_config.h
    @internalComponent
*/
typedef TDhcpRawOptionDataPckg TDhcp4RawOptionDataPckg;


/**
    TDhcp6RawOptionDataPckg - provide simple interface for dealing with DHCPv6 raw option data.
    IPv4 version is defined in es_config.h
    @internalComponent
*/
class TDhcp6RawOptionDataPckg : public TPckgDes<TUint16>
	{
public:
 	 /** @param aDes User buffer to hold data fetched. */
     TDhcp6RawOptionDataPckg(TDes8& aDes)  :
		TPckgDes<TUint16>(aDes)
		{
		}

	 /**
  	  SetOpCode. Make sure the buffer is at least 8 bytes long.
  	  @param aOpCode Desired DHCP Raw option.
	 */
	 inline void SetOpCode(TUint16 aOpCode)
        {
		const TUint opCodeSize = sizeof(aOpCode);
        TBuf8<opCodeSize> buf;
		buf.Copy(reinterpret_cast<const TUint8*>(&aOpCode), opCodeSize);
        iDes->Replace(0,opCodeSize,buf);
		}

	inline TUint16 OpCode()
		{
		return *(reinterpret_cast<const TUint16*>(iDes->Ptr()));
		}
	};

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
class TDhcp6RawOptionMultipleDataPckg :  public TPckgDes<TUint16>
/**
 * For use when an application wants to access multiple raw parameter options.
 * The descriptor contains a list of OpCodes. After the RConnection::Ioctl() call has 
 * completed, the descriptor will contain the number of parameters in the first byte 
 * followed by parameter opcode, length and data for each parameter option.
 *	
 *	@code
 *     Message	
 *	     -------------------------------   
 *		|	|	|	|		|
 *		|op1|op2|-	|		|
 *		 -------------------------------
 * @endcode
 * @publishedPartner
 * @released
*/
	{
public:
	TDhcp6RawOptionMultipleDataPckg(TDes8& aDes)  :
		TPckgDes<TUint16>(aDes)
	//Constructor for the class. Pushing Buffer containing number
	//of OpCode in iDes.  
		{
		//Setting data length of descriptor as NULL
		iDes->SetLength(NULL);
		}

	void AddRawOptionCodeL(const TUint16 aOpCode)
	/**
	 * Sets parameters one at a time.
	 * The only thing the application provides is the OpCode.
	 * @param aOpCode The OpCode supplied by the user.
	*/
		{
		//  ---------------------
		// |      |		|..........................
		// |OP1   |	OP2	|
		// |      |     |.......................
		// ----------------------
		//Data will be appended at the end of the buffer. Length of buffer will indicate
		//number of opcodes as every thing is 1 byte
		iDes->Append(aOpCode);	
		}
	
	TInt GetRawParameterData(const TUint16 aOpCode ,TPtrC8& aDes)
	   	/**
	   	 * Returns the data portion corresponding to the supplied opcode.  
		 * It's only meaningful to call this after an Ioctl() call.
		 *
	   	 * @code
	   	 *  ---------------------------------------------------
	   	 * |No. of |      |Data  |      |       |Data  |       |
	   	 * |opcodes| OP1  |Length| Data |  OP2  |Length| Data  |
	   	 * |       |      |      |      |       |      |       |
	   	 *  --------------------------------------------------- 
	   	 * @endcode
	   	 *
	   	 * @param aOpCode The opcode passed by the user.
	   	 * @param aDes On return, it contains the message opcode data.
	   	 * @return KErrNone if aOpCode is found, else KErrNotFound 
	   	*/
		{
		TUint16 opCodeIterated=0;
		TUint16 opCodeIterator= 2;
		const TUint16 sizeOfOpcodeAndDataBytes = 3;
		const TUint16 sizeOfLengthByte = 2;
				
		//extracting number of opcodes. First byte of Descriptor will contain number of bytes
		TUint16 numberOfOpcodes  = *(iDes->Ptr());
		//taking pointer to base location of aDes.
		const TUint8* desBasePtr=iDes->Ptr();
		
		//While loop true conditions are opCodeFound if Particular opcode is extracted or All opcode in 
		//descriptor are iterated and OpCode is not found.
		while( opCodeIterated < numberOfOpcodes)
			{
			//If opcode is found then come out of loop and TPtr will be passed in the descriptor. Pointer is returned to the 
			//start byte of data for that particular Opcode with length of the data.
 			if(aOpCode==*(desBasePtr+opCodeIterator))
				{
				TInt dataLength = *(desBasePtr+(opCodeIterator+sizeOfLengthByte));
  				aDes.Set(reinterpret_cast<const TUint8*>(desBasePtr+opCodeIterator+sizeOfOpcodeAndDataBytes),dataLength);	
				return KErrNone;
				}
		
				//Opcode iterator is iterated in such a way that it will always point to opcode location.
				opCodeIterator += *(desBasePtr+(opCodeIterator+sizeOfLengthByte))+4;
				opCodeIterated++;
			}//while loop.
		return KErrNotFound;
		}
	
	TUint16 NumOfOptionRetrieved()
	/**
	 * Gets the number of opcode data buffers.
	 * It's only meaningful to call this after an Ioctl() call.
	 * @return The number of opcode data buffers.
	 */
		{
		return *(iDes->Ptr());
		}
	};
#endif //SYMBIAN_TCPIPDHCP_UPDATE
#endif

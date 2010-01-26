/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Implementation of the Netcards.exe adapter selection application.
* This version writes settings to an ethernet.ini file only.  The perl
* script "configchange.pl" can be used to run this version of Netcards.exe
* and extract the ethernet settings from ethernet.ini and place them in the
* correct setup files for EKA1 (ethermac.dat and etherdriver.dat) or EKA2 (epoc.ini).
* there is a bug in the rpcasync.h windows header file
* where the forward declaration for type _RPC_ASYNC_STATE
* type is locally scoped inside an extern C block, rather
* than outside it, so we have turned this warning off, as
* we can;t change the header file... 
* 
*
*/



/**
 @file NETCARDS.C 
 @internalTechnology
 @file
*/
#pragma warning (disable : 4115)
#include <windows.h>
#pragma warning (default : 4115)
#include <stdio.h>
#include <malloc.h>
#include <conio.h>


#define EPOC_INI_FILE			"ethernet.ini"

#define EPOC_INI_FILE_TEMP		"__temp__ethernet__.ini"

#define	ETHER_NIF_ENTRY			"ETHER_NIF"
#define	ETHER_MAC_ENTRY			"ETHER_MAC"
#define	ETHER_SPEED_ENTRY		"ETHER_SPEED"

#define MAX_VALUE		80
#define MAX_LINE		100
#define MAX_OID_DATA	256

#define OID_802_3_CURRENT_ADDRESS		   		0x01010102


char    AdapterList[10][1024];

// replace or write new 'value' for 'entry' in epoc.ini file
// returns 0 if ok, negative value if sth wrong
int replace_in_inifile(char * entry, char* value, BOOL valUnicode );


int main(int argc, char* argv[])
{

	// Packet.lib variables:
	LPADAPTER  lpAdapter = 0;
	PPACKET_OID_DATA pOidData = malloc( sizeof(PACKET_OID_DATA) + MAX_OID_DATA );
	NetType *type = malloc(sizeof(NetType));
	UINT speed_Mbps = 0;

	BOOL isWinNT = FALSE;
	int        i;

	DWORD dwVersion;
	DWORD dwWindowsMajorVersion;


	//unicode strings (winnt)
	WCHAR		AdapterName[8192]; // string that contains a list of the network adapters
	WCHAR		*temp,*temp1;

	//ascii strings (win95)
	char		AdapterNamea[8192]; // string that contains a list of the network adapters
	char		*tempa,*temp1a;


	int			AdapterNum=0,Open;
	unsigned long		AdapterLength;


	FILE * inifile;

	char		speed_value[10];
	char		*MAC_value = malloc( 13*sizeof(char) );
	char		*temp2 = malloc( 13*sizeof(char) );

	//interfaceArg specifies the interface # passed fron configchange.pl
	//set interfaceArg to argv[1] if arguments are passed
	int interfaceArg = 0;
	if ( argc>1 && argv[1] )
	{
		interfaceArg = atoi(argv[1]);
	}

	// obtain the name of the adapters installed on this machine
	//printf("Adapters installed:\n");
	i=0;

	// the data returned by PacketGetAdapterNames is different in Win95 and in WinNT.
	// We have to check the os on which we are running
	dwVersion=GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
	{  // Windows NT
		isWinNT = TRUE;
		AdapterLength=sizeof(AdapterName);
		PacketGetAdapterNames((char*)AdapterName,&AdapterLength);
		temp=AdapterName;
		temp1=AdapterName;
		while ((*temp!='\0')||(*(temp-1)!='\0'))
		{
			if (*temp=='\0')
			{
				memcpy(AdapterList[i],temp1,(temp-temp1)*2);
				temp1=temp+1;
				i++;
			}

		temp++;
		}

		AdapterNum=i;
		// we want to automate the adaptor selection process
		// but if there are more than one to choose from, we can't
		// so
		if (AdapterNum>1)
		{
			for (i=0;i<AdapterNum;i++)
			{
				wprintf(L"\n%d- %s\n",i+1,AdapterList[i]);
   			}
			printf("\n");
  		}
	}
	else	//windows 95
	{
		AdapterLength=sizeof(AdapterNamea);
		PacketGetAdapterNames(AdapterNamea,&AdapterLength);
		tempa=AdapterNamea;
		temp1a=AdapterNamea;

		while ((*tempa!='\0')||(*(tempa-1)!='\0'))
		{
			if (*tempa=='\0')
			{
				memcpy(AdapterList[i],temp1a,tempa-temp1a);
				temp1a=tempa+1;
				i++;
			}
			tempa++;
		}

		AdapterNum=i;
		// we want to automate the adaptor selection process
		// but if there are more than one to choose from, we can't
		// so
		if (AdapterNum>1)
		{
			printf("Adapters installed:\n");
			for (i=0;i<AdapterNum;i++)
			{
				printf("\n%d- %s\n",i+1,AdapterList[i]);
   			}
			printf("\n");
  		}
	}

	// we want to automate the adaptor selection process
	// but if there are more than one to choose from, we can't
	// so
	//
	
	if (AdapterNum>1)
	{	
		if ( (interfaceArg>AdapterNum)||(interfaceArg<1) )
		{
			do
			{
				printf("Select the number of the adapter to use : ");scanf("%d",&Open);
				if (Open>AdapterNum) printf("\nThe number must be smaller than %d",AdapterNum);
			} while (Open>AdapterNum);
		}
		else
		{
			Open = interfaceArg;
		}
	}
	else
	{
		Open = AdapterNum;
 	}

	lpAdapter = PacketOpenAdapter(AdapterList[Open-1]);


	MAC_value[0] = '\0';
	temp2[0] = '\0';
	speed_value[0] = '\0';

	if( NULL != lpAdapter)
	{

		if ( TRUE == PacketGetNetType (lpAdapter, type) )
		{
			speed_Mbps = type->LinkSpeed / 1000000; // LinkSpeed is in bits per second
		}
		else
			printf("Could not read Ethernet card's speed\n");

		if ( type->LinkType != NdisMedium802_3)
			printf("NOT Ethernet802.3 card.\nNetwork Interface not supported\n");
		else
		{
			pOidData->Oid = OID_802_3_CURRENT_ADDRESS;

			pOidData->Length = MAX_OID_DATA;


			if ( TRUE == PacketRequest(lpAdapter, FALSE , pOidData) )
			{
				// get info obtained
//				printf("Physical address read: ");
	/*			printf("%x %x %x %x %x %x\n", pOidData->Data[0], pOidData->Data[1],
					pOidData->Data[2],pOidData->Data[3],
					pOidData->Data[4],pOidData->Data[5]
					);*/

				pOidData->Data[0] += 2; // changing address from global to local
				for( i=0; i<6; i++ )
				{
					strcpy( temp2, MAC_value);
					if( pOidData->Data[i] > 15 )
						// has 2 hex digits
						sprintf( MAC_value, "%s%x", temp2, pOidData->Data[i]);
					else
						sprintf( MAC_value, "%s0%x", temp2, pOidData->Data[i]);
				}
			}
			else
				printf("Failed to read physical address of Ethernet card\n");
		}

		free(pOidData);
		free(type);

		PacketCloseAdapter( lpAdapter );
	}
	else
	{
		// lpAdapter NULL
		printf("Problem with opening adapter (packet.lib issue)\n");
		return (1);
	}

	inifile = fopen(EPOC_INI_FILE, "a"); // to create if does exist
	if ( NULL != inifile )
		fclose(inifile);
	else
	{
		printf("Can't create or access %s.\n\n", EPOC_INI_FILE);
		return 0;
	}

	if ( 0 == replace_in_inifile( ETHER_NIF_ENTRY, AdapterList[Open-1], isWinNT ) )
		printf( "Netcards using adapter %d\n", Open );
	else
	{
		return 0;
	}


	if ( 0 != replace_in_inifile( ETHER_MAC_ENTRY, MAC_value, FALSE ) )
	{
		printf("Couldn't write MAC address to %s file\n", EPOC_INI_FILE);
		return (1);
	}


	if( 0 != speed_Mbps )
		sprintf( speed_value, "%dMbps", speed_Mbps);


	if ( 0 != replace_in_inifile( ETHER_SPEED_ENTRY, speed_value, FALSE ) )
	{
		printf("Couldn't write speed value to %s file\n", EPOC_INI_FILE);
		return (1);
	}

	//printf("Netcards has written settings to %s.\n\n", EPOC_INI_FILE);

	free(MAC_value);
	free(temp2);
	return (0);
}


int replace_in_inifile(char * entry_str, char* value, BOOL valUnicode)
{

	int err = 0; // 0 - ok, negative sth wrong

	int replaced = 0;
	int len = strlen(entry_str);

	FILE *	file;
	FILE *	tmp_file;

	char*  s = malloc(MAX_LINE);
	char *line = malloc(MAX_LINE);

	if ( NULL == (tmp_file = fopen(EPOC_INI_FILE_TEMP, "w")) )
	{
      printf( "Could not create '%s'\n", EPOC_INI_FILE_TEMP );
	  return -1;
	}

	if ( NULL == (file  = fopen(EPOC_INI_FILE, "r+")) )
	{
		fclose( tmp_file );
		remove( EPOC_INI_FILE_TEMP  );
		printf( "Could not open '%s'\n", EPOC_INI_FILE );
		return -1;
	}

	rewind(file);


	while( fgets(line, MAX_LINE, file) != NULL)
    {
		if (sscanf( line, "%s", s ) > 0) // to trim blank chars
		{
			s[len] = '\0';
			if( 0 == strcmp(entry_str, s))
			{

				fprintf(tmp_file, "%s=", entry_str);

				if( valUnicode == TRUE )
					fwprintf(tmp_file, L"%s\n", value);
				else
					fprintf(tmp_file, "%s\n", value);

				replaced = 1;
			}
			else
				if( EOF == fputs(line, tmp_file) )
				{
					err = -1;
					break;
				}
		}
	}

	free(line);
	free(s);

	if( (0 == replaced) && (0 == err) )
	{
		// no entry encountered - add new
		if( 0 != fseek( tmp_file, 0, SEEK_END ) )
					err = -1;

		fprintf( tmp_file, "\n%s=", entry_str);
		if ( valUnicode )
			fwprintf( tmp_file, L"%s\n", value);
		else
			fprintf( tmp_file, "%s\n", value);
	}


	if ( 0 != fclose(file ) )
	{
      printf( "Could not close %s file\n", EPOC_INI_FILE );
	  return -1;
	}

	if ( 0 != fclose( tmp_file ) )
	{
      printf( "Could not close %s file\n", EPOC_INI_FILE_TEMP );
	  return -1;
	}


	if( remove( EPOC_INI_FILE  ) == -1 )
	{
		printf( "Could not overwrite %s file\n", EPOC_INI_FILE );
		return -1;
	}

	if( rename( EPOC_INI_FILE_TEMP, EPOC_INI_FILE ) != 0 )
	{
		printf( "\nCould not rename '%s' to '%s'\n", EPOC_INI_FILE_TEMP, EPOC_INI_FILE );
		return -1;
	}

	return 0;

}


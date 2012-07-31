
/*
 * Copyright 2006-2012, Ubiquiti Networks, Inc.
 * Author: Kestutis Barkauskas <gpl@ubnt.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stddef.h>

#include <abz/typedefs.h>
#include <abz/error.h>
#include <debug/log.h>

#include <tinysnmp/tinysnmp.h>
#include <tinysnmp/agent/module.h>
#include <tinysnmp/agent/odb.h>
#include <tinysnmp/agent/ifcache.h>

#include "ieee802dot11.h"
#include "iwlib.h"

/****************************************************************************
 *                                Defines                                    *
 ****************************************************************************/
//#define DISPLAYWIEXT                        // display wireless ext info
#define TABLE_SIZE   1
//#define MINLOADFREQ 15                    // min reload frequency in seconds
#define MINLOADFREQ 5                       // min reload frequency in seconds      // for testing
#define PROC_NET_DEV      "/proc/net/dev"
#define PROC_NET_WIRELESS "/proc/net/wireless"

#ifndef UCHAR
typedef unsigned char UCHAR;
#endif

/****************************************************************************
 *                            Private Functions                              *
 ****************************************************************************/
static void loadTables();
static void loadWiExt ( int, char *, struct wireless_info * );
static void load80211Structs ( int, char *, struct wireless_info * );
static void initStructs();

// Wireless Extensions Specific Functions
static void loadWiExtTo80211Structs ( int, char *, struct wireless_info * );
static void displayWiExt ( struct wireless_info );

// Linked List Functions
static void addList ( char *, char *, int );
static void initLists();                    // initialize all the linked lists
static void flushLists();                   // flush all the linked lists
static void flushList ( char * );           // flush a single linked list

// Utility Functions
static int  openSocket ( void );
static int  mWatt2dbm ( int );
static char *htob ( char * );
static int  hasChanged ( char *, int );

/****************************************************************************
 *                            Private Variables                              *
 ****************************************************************************/
static unsigned long lastLoad = 0;          // ET in secs at last table load

static struct avNode *lastNode, *newNode, *np;

/*1.1.1. */
enum
{
    DOT11STATIONID = 1,
    DOT11MEDIUMOCCUPANCYLIMIT,
    DOT11CFPOLLABLE,
    DOT11CFPPERIOD,
    DOT11CFPMAXDURATION,
    DOT11AUTHENTICATIONRESPONSETIMEOUT,
    DOT11PRIVACYOPTIONIMPLEMENTED,
    DOT11POWERMANAGEMENTMODE,
    DOT11DESIREDSSID,
    DOT11DESIREDBSSTYPE,
    DOT11OPERATIONALRATESET,
    DOT11BEACONPERIOD,
    DOT11DTIMPERIOD,
    DOT11ASSOCIATIONRESPONSETIMEOUT,
    DOT11DISASSOCIATEREASON,
    DOT11DISASSOCIATESTATION,
    DOT11DEAUTHENTICATEREASON,
    DOT11DEAUTHENTICATESTATION,
    DOT11AUTHENTICATEFAILSTATUS,
    DOT11AUTHENTICATEFAILSTATION,
};

/*1.2.1. */
enum {
    DOT11AUTHENTICATIONALGORITHM		= 2,
    DOT11AUTHENTICATIONALGORITHMSENABLE,
};

/*1.3.1. */
enum {
    DOT11WEPDEFAULTKEYVALUE	= 2,
};

/*1.4.1. */
enum {
    DOT11WEPKEYMAPPINGADDRESS = 2,
    DOT11WEPKEYMAPPINGWEPON,
    DOT11WEPKEYMAPPINGVALUE,
    DOT11WEPKEYMAPPINGSTATUS,
};

/*1.5.1. */
enum {
    DOT11PRIVACYINVOKED		= 1,
    DOT11WEPDEFAULTKEYID,
    DOT11WEPKEYMAPPINGLENGTH,
    DOT11EXCLUDEUNENCRYPTED,
    DOT11WEPICVERRORCOUNT,
    DOT11WEPEXCLUDEDCOUNT,
};

/*2.1.1. */
enum {
    DOT11MACADDRESS		= 1,
    DOT11RTSTHRESHOLD,
    DOT11SHORTRETRYLIMIT,
    DOT11LONGRETRYLIMIT,
    DOT11FRAGMENTATIONTHRESHOLD,
    DOT11MAXTRANSMITMSDULIFETIME,
    DOT11MAXRECEIVELIFETIME,
    DOT11MANUFACTURERID,
    DOT11PRODUCTID,
};

/*2.2.1. */
enum {
    DOT11TRANSMITTEDFRAGMENTCOUNT	= 1,
    DOT11MULTICASTTRANSMITTEDFRAMECOUNT,
    DOT11FAILEDCOUNT,
    DOT11RETRYCOUNT,
    DOT11MULTIPLERETRYCOUNT,
    DOT11FRAMEDUPLICATECOUNT,
    DOT11RTSSUCCESSCOUNT,
    DOT11RTSFAILURECOUNT,
    DOT11ACKFAILURECOUNT,
    DOT11RECEIVEDFRAGMENTCOUNT,
    DOT11MULTICASTRECEIVEDFRAMECOUNT,
    DOT11FCSERRORCOUNT,
    DOT11TRANSMITTEDFRAMECOUNT,
    DOT11WEPUNDECRYPTABLECOUNT,
};

/*2.3.1. */
enum {
    DOT11ADDRESS	= 2,
    DOT11GROUPADDRESSESSTATUS,
};

/*3.1. */
enum {
    DOT11RESOURCETYPEIDNAME	= 1,
};

/*3.1.2.1. */
enum {
    DOT11MANUFACTUREROUI	= 1,
    DOT11MANUFACTURERNAME,
    DOT11MANUFACTURERPRODUCTNAME,
    DOT11MANUFACTURERPRODUCTVERSION,
};

/*4.1.1. */
enum {
    DOT11PHYTYPE	= 1,
    DOT11CURRENTREGDOMAIN,
    DOT11TEMPTYPE,
};

/*4.2.1. */
enum {
    DOT11CURRENTTXANTENNA	= 1,
    DOT11DIVERSITYSUPPORT,
    DOT11CURRENTRXANTENNA,
};

/*4.3.1. */
enum {
    DOT11NUMBERSUPPORTEDPOWERLEVELS	= 1,
    DOT11TXPOWERLEVEL1,
    DOT11TXPOWERLEVEL2,
    DOT11TXPOWERLEVEL3,
    DOT11TXPOWERLEVEL4,
    DOT11TXPOWERLEVEL5,
    DOT11TXPOWERLEVEL6,
    DOT11TXPOWERLEVEL7,
    DOT11TXPOWERLEVEL8,
    DOT11CURRENTTXPOWERLEVEL,
};

/*4.4.1. */
enum {
    DOT11HOPTIME	= 1,
    DOT11CURRENTCHANNELNUMBER,
    DOT11MAXDWELLTIME,
    DOT11CURRENTDWELLTIME,
    DOT11CURRENTSET,
    DOT11CURRENTPATTERN,
    DOT11CURRENTINDEX,
};

/*4.5.1. */
enum {
    DOT11CURRENTCHANNEL	= 1,
    DOT11CCAMODESUPPORTED,
    DOT11CURRENTCCAMODE,
    DOT11EDTHRESHOLD,
};

/*4.6.1. */
enum {
    DOT11CCAWATCHDOGTIMERMAX	= 1,
    DOT11CCAWATCHDOGCOUNTMAX,
    DOT11CCAWATCHDOGTIMERMIN,
    DOT11CCAWATCHDOGCOUNTMIN,
};

/*4.7.1. */
enum {
    DOT11REGDOMAINSSUPPORTVALUE	= 2,
};

/*4.8.1. */
enum {
    DOT11SUPPORTEDTXANTENNA	= 2,
    DOT11SUPPORTEDRXANTENNA,
    DOT11DIVERSITYSELECTIONRX,
};

/*4.9.1. */
enum {
    DOT11SUPPORTEDDATARATESTXVALUE	= 2,
};

/*4.10.1. */
enum {
    DOT11SUPPORTEDDATARATESRXVALUE	= 2,
};

static int update (struct odb **odb,const uint32_t *oid,uint8_t type,void *data)
{
    snmp_value_t value;

    value.type = type;

    switch (type)
    {
    case BER_INTEGER:
	value.data.INTEGER = *(int32_t *) data;
	break;
    case BER_Counter32:
	value.data.Counter32 = *(uint32_t *) data;
	break;
    case BER_Gauge32:
	value.data.Gauge32 = *(uint32_t *) data;
	break;
    case BER_TimeTicks:
	value.data.TimeTicks = *(uint32_t *) data;
	break;
    case BER_Counter64:
	value.data.Counter64 = *(uint64_t *) data;
	break;
    case BER_OID:
	value.data.OID = (uint32_t *) data;
	break;
    case BER_OCTET_STRING:
	value.data.OCTET_STRING = *(octet_string_t *) data;
	break;
    default:
	abz_set_error ("invalid type (0x%02x) specified",type);
	return (-1);
    }

    return (odb_add (odb,oid,&value));
}

/*Readers only*/
static int update_ieee802dot11(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    return -1;
}

static int update_dot11StationConfigTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 1, 1, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11AuthenticationAlgorithmsTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 1, 2, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11WEPDefaultKeysTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 1, 3, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11WEPKeyMappingsTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 1, 4, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PrivacyTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 1, 5, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11OperationTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 2, 1, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11CountersTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 2, 2, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11GroupAddressesTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 2, 3, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11ResourceInfoTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[10] = { 9, 42, 840, 10036, 3, 1, 2, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PhyOperationTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 1, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PhyAntennaTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 2, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PhyTxPowerTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 3, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PhyFHSSTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 4, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PhyDSSSTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 5, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11PhyIRTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 6, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11RegDomainsSupportedTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 7, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11AntennasListTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 8, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11SupportedDataRatesTxTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 9, 1, entry, index };
    return (update (odb,table,type,data));
}

static int update_dot11SupportedDataRatesRxTable(struct odb **odb,uint32_t entry,uint32_t index,uint8_t type,void *data)
{
    const uint32_t table[9] = { 8, 42, 840, 10036, 4, 10, 1, entry, index };
    return (update (odb,table,type,data));
}

static int dot11_update (struct odb **odb)
{
    int32_t index;
    octet_string_t str;

    loadTables();

    for ( np = LIST_FIRST ( &scList ); np != NULL; np = LIST_NEXT ( np, nodes ))
    {
	sc = ( struct scTbl_data * ) np->data;
	index = sc->ifIndex;
	if ( sc->haveStationID )
	{
	    str.buf = sc->stationID;
	    str.len = strlen (str.buf);
	    update_dot11StationConfigTable (odb, DOT11STATIONID, index, BER_OCTET_STRING, &str);
	}
	if ( sc->haveMediumOccupancyLimit )
	    update_dot11StationConfigTable (odb, DOT11MEDIUMOCCUPANCYLIMIT, index, BER_INTEGER, &sc->mediumOccupancyLimit);
	if ( sc->haveCFPPollable )
	    update_dot11StationConfigTable (odb, DOT11CFPOLLABLE, index, BER_INTEGER, &sc->CFPPollable);

	if ( sc->haveCFPPeriod )
	    update_dot11StationConfigTable (odb, DOT11CFPPERIOD, index, BER_INTEGER, &sc->CFPPeriod);
	if ( sc->haveMaxDuration )
	    update_dot11StationConfigTable (odb, DOT11CFPMAXDURATION, index, BER_INTEGER, &sc->maxDuration);
	if ( sc->haveAuthenticationResponseTimeOut )
	    update_dot11StationConfigTable (odb, DOT11AUTHENTICATIONRESPONSETIMEOUT, index, BER_INTEGER, &sc->authenticationResponseTimeOut);
	if ( sc->havePrivacyOptionImplemented )
	    update_dot11StationConfigTable (odb, DOT11PRIVACYOPTIONIMPLEMENTED, index, BER_INTEGER, &sc->privacyOptionImplemented);
	if ( sc->havePowerManagementMode )
	    update_dot11StationConfigTable (odb, DOT11POWERMANAGEMENTMODE, index, BER_INTEGER, &sc->powerManagementMode);
	if ( sc->haveDesiredSSID )
	{
	    str.buf = sc->desiredSSID;
	    str.len = strlen (str.buf);
	    update_dot11StationConfigTable (odb, DOT11DESIREDSSID, index, BER_OCTET_STRING, &str);
	}
	if ( sc->haveDesiredBSSType )
	    update_dot11StationConfigTable (odb, DOT11DESIREDBSSTYPE, index, BER_INTEGER, &sc->desiredBSSType);
	if ( sc->haveOperationalRateSet )
	{
	    str.buf = sc->operationalRateSet;
	    str.len = strlen (str.buf);
	    update_dot11StationConfigTable (odb, DOT11OPERATIONALRATESET, index, BER_OCTET_STRING, &str);
	}
	if ( sc->haveBeaconPeriod )
	    update_dot11StationConfigTable (odb, DOT11BEACONPERIOD, index, BER_INTEGER, &sc->beaconPeriod);
	if ( sc->haveDTIMPeriod )
	    update_dot11StationConfigTable (odb, DOT11DTIMPERIOD, index, BER_INTEGER, &sc->DTIMPeriod);
	if ( sc->haveAssociationResponseTimeOut )
	    update_dot11StationConfigTable (odb, DOT11ASSOCIATIONRESPONSETIMEOUT, index, BER_INTEGER, &sc->associationResponseTimeOut);
	if ( sc->haveDisAssociationReason )
	    update_dot11StationConfigTable (odb, DOT11DISASSOCIATEREASON, index, BER_INTEGER, &sc->disAssociationReason);
	if ( sc->haveDisAssociationStation )
	{
	    str.buf = sc->disAssociationStation;
	    str.len = strlen (str.buf);
	    update_dot11StationConfigTable (odb, DOT11DISASSOCIATESTATION, index, BER_OCTET_STRING, &str);
	}
	if ( sc->haveDeAuthenticationReason )
	    update_dot11StationConfigTable (odb, DOT11DEAUTHENTICATEREASON, index, BER_INTEGER, &sc->deAuthenticationReason);
	if ( sc->haveDeAuthenticationStation )
	{
	    str.buf = sc->deAuthenticationStation;
	    str.len = strlen (str.buf);
	    update_dot11StationConfigTable (odb, DOT11DEAUTHENTICATESTATION, index, BER_OCTET_STRING, &str);
	}
	if ( sc->haveAuthenticateFailStatus )
	    update_dot11StationConfigTable (odb, DOT11AUTHENTICATEFAILSTATUS, index, BER_INTEGER, &sc->authenticateFailStatus);
	if ( sc->haveAuthenticateFailStation )
	{
	    str.buf = sc->authenticateFailStation;
	    str.len = strlen (str.buf);
	    update_dot11StationConfigTable (odb, DOT11AUTHENTICATEFAILSTATION, index, BER_OCTET_STRING, &str);
	}
    }

    for ( np = LIST_FIRST ( &opList ); np != NULL; np = LIST_NEXT ( np, nodes )) {
	op = ( struct opTbl_data * ) np->data;
	index = op->ifIndex;

	if (op->haveMACAddress) {
	    str.buf = op->MACAddress;
	    str.len = strlen (str.buf);
	    update_dot11OperationTable(odb, DOT11MACADDRESS, index, BER_OCTET_STRING, &str);
	}
	if ( op->haveRTSThreshold )
	    update_dot11OperationTable(odb, DOT11RTSTHRESHOLD, index, BER_INTEGER, &op->RTSThreshold);
	if ( op->haveShortRetryLimit )
	    update_dot11OperationTable(odb, DOT11SHORTRETRYLIMIT, index, BER_INTEGER, &op->shortRetryLimit);
	if ( op->haveLongRetryLimit )
	    update_dot11OperationTable(odb, DOT11LONGRETRYLIMIT, index, BER_INTEGER, &op->longRetryLimit);
	if ( op->haveFragmentationThreshold )
	    update_dot11OperationTable(odb, DOT11FRAGMENTATIONTHRESHOLD, index, BER_INTEGER, &op->fragmentationThreshold);
	if ( op->haveMaxTransmitMSDULifetime )
	    update_dot11OperationTable(odb, DOT11MAXTRANSMITMSDULIFETIME, index, BER_INTEGER, &op->maxTransmitMSDULifetime);
	if ( op->haveMaxReceiveLifetime )
	    update_dot11OperationTable(odb, DOT11MAXRECEIVELIFETIME, index, BER_INTEGER, &op->maxReceiveLifetime);
	if ( op->haveManufacturerID ) {
	    str.buf = op->manufacturerID;
	    str.len = strlen (str.buf);
	    update_dot11OperationTable(odb, DOT11MANUFACTURERID, index, BER_OCTET_STRING, &str);
	}
	if ( op->haveProductID ) {
	    str.buf = op->productID;
	    str.len = strlen (str.buf);
	    update_dot11OperationTable(odb, DOT11PRODUCTID, index, BER_OCTET_STRING, &str);
	}
    }

    for ( np = LIST_FIRST ( &coList ); np != NULL; np = LIST_NEXT ( np, nodes )) {
	co = ( struct coTbl_data * ) np->data;
	index = co->ifIndex;

	if ( co->haveTransmittedFragmentCount )
	    update_dot11CountersTable(odb, DOT11TRANSMITTEDFRAGMENTCOUNT, index, BER_INTEGER, &co->transmittedFragmentCount);
	if ( co->haveMulticastTransmittedFrameCount )
	    update_dot11CountersTable(odb, DOT11MULTICASTTRANSMITTEDFRAMECOUNT, index, BER_INTEGER, &co->multicastTransmittedFrameCount);
	if ( co->haveFailedCount )
	    update_dot11CountersTable(odb, DOT11FAILEDCOUNT, index, BER_Counter32, &co->failedCount);
	if ( co->haveRetryCount )
	    update_dot11CountersTable(odb, DOT11RETRYCOUNT, index, BER_Counter32, &co->retryCount);
	if ( co->haveMultipleRetryCount )
	    update_dot11CountersTable(odb, DOT11MULTIPLERETRYCOUNT, index, BER_INTEGER, &co->multipleRetryCount);
	if ( co->haveFrameDuplicateCount )
	    update_dot11CountersTable(odb, DOT11FRAMEDUPLICATECOUNT, index, BER_INTEGER, &co->frameDuplicateCount);
	if ( co->haveRTSSuccessCount )
	    update_dot11CountersTable(odb, DOT11RTSSUCCESSCOUNT, index, BER_INTEGER, &co->RTSSuccessCount);
	if ( co->haveRTSFailureCount )
	    update_dot11CountersTable(odb, DOT11RTSFAILURECOUNT, index, BER_INTEGER, &co->RTSFailureCount);
	if ( co->haveACKFailureCount )
	    update_dot11CountersTable(odb, DOT11ACKFAILURECOUNT, index, BER_INTEGER, &co->ACKFailureCount);
	if ( co->haveReceivedFragmentCount )
	    update_dot11CountersTable(odb, DOT11RECEIVEDFRAGMENTCOUNT, index, BER_INTEGER, &co->receivedFragmentCount);
	if ( co->haveMulticastReceivedFrameCount )
	    update_dot11CountersTable(odb, DOT11MULTICASTRECEIVEDFRAMECOUNT, index, BER_INTEGER, &co->multicastReceivedFrameCount);
	if ( co->haveFCSErrorCount )
	    update_dot11CountersTable(odb, DOT11FCSERRORCOUNT, index, BER_INTEGER, &co->FCSErrorCount);
	if ( co->haveTransmittedFrameCount )
	    update_dot11CountersTable(odb, DOT11TRANSMITTEDFRAMECOUNT, index, BER_INTEGER, &co->transmittedFrameCount);
	if ( co->haveWEPUndecryptableCount )
	    update_dot11CountersTable(odb, DOT11WEPUNDECRYPTABLECOUNT, index, BER_Counter32, &co->WEPUndecryptableCount);
    }

    for ( np = LIST_FIRST ( &poList ); np != NULL; np = LIST_NEXT ( np, nodes ))
    {
	po = ( struct poTbl_data * ) np->data;
	index = po->ifIndex;
	if ( po->havePHYType )
	    update_dot11PhyOperationTable(odb, DOT11PHYTYPE, index, BER_INTEGER, &po->PHYType);
          if ( po->haveCurrentRegDomain )
	    update_dot11PhyOperationTable(odb, DOT11CURRENTREGDOMAIN, index, BER_INTEGER, &po->currentRegDomain);
	  if ( po->haveTempType )
	    update_dot11PhyOperationTable(odb, DOT11TEMPTYPE, index, BER_INTEGER, &po->tempType);
    }

    for ( np = LIST_FIRST ( &pdList ); np != NULL; np = LIST_NEXT ( np, nodes ))
    {
	pd = ( struct pdTbl_data * ) np->data;
	index = pd->ifIndex;
	if ( pd->haveCurrentChannel )
	    update_dot11PhyDSSSTable(odb, DOT11CURRENTCHANNEL, index, BER_INTEGER, &pd->currentChannel);
	if ( pd->haveCCAModeSupported )
	    update_dot11PhyDSSSTable(odb, DOT11CCAMODESUPPORTED, index, BER_INTEGER, &pd->CCAModeSupported);
	if ( pd->haveCurrentCCAMode )
	    update_dot11PhyDSSSTable(odb, DOT11CURRENTCCAMODE, index, BER_INTEGER, &pd->currentCCAMode);
	if ( pd->haveEDThreshold )
	    update_dot11PhyDSSSTable(odb, DOT11EDTHRESHOLD, index, BER_INTEGER, &pd->EDThreshold);
    }


    for ( np = LIST_FIRST ( &riList ); np != NULL; np = LIST_NEXT ( np, nodes ))
    {
	ri = ( struct riTbl_data * ) np->data;
	index = ri->ifIndex;
	if ( ri->haveManufacturerOUI )
	{
	    str.buf = ri->manufacturerOUI;
	    str.len = strlen (str.buf);
	    update_dot11ResourceInfoTable (odb, DOT11MANUFACTUREROUI, index, BER_OCTET_STRING, &str);
	}
	if ( ri->haveManufacturerName )
	{
	    str.buf = ri->manufacturerName;
	    str.len = strlen (str.buf);
	    update_dot11ResourceInfoTable (odb, DOT11MANUFACTURERNAME, index, BER_OCTET_STRING, &str);
	}
	if ( ri->haveManufacturerProductName )
	{
	    str.buf = ri->manufacturerProductName;
	    str.len = strlen (str.buf);
	    update_dot11ResourceInfoTable (odb, DOT11MANUFACTURERPRODUCTNAME, index, BER_OCTET_STRING, &str);
	}
	if ( ri->haveManufacturerProductVersion )
	{
	    str.buf = ri->manufacturerProductVersion;
	    str.len = strlen (str.buf);
	    update_dot11ResourceInfoTable (odb, DOT11MANUFACTURERPRODUCTVERSION, index, BER_OCTET_STRING, &str);
	}
    }

    return (0);
}

// iso(1).member-body(2).us(840).ieee802dot11(10036).objects"
static const uint32_t dot11Objects[7] = { 3, 42, 840, 10036 };
/* iso(1).member-body(2).us(840).ieee802dot11(10036).dot11MIB */
static const uint32_t dot11MIB[7] = { 3, 42, 840, 10036 };


struct module module =
{
    .name	= "ieee802dot11",
    .descr	= "The MIB module for IEEE 802.11 entities.",
    .mod_oid	= dot11Objects,
    .con_oid	= dot11MIB,
    .parse	= NULL,
    .open	= NULL,
    .update	= dot11_update,
    .close	= NULL, //flushLists,
};

/****************************************************************************
 *                                                                           *
 *                      loadTables() - Load the Tables                       *
 *                                                                           *
 ****************************************************************************/
static void loadTables()
{
    int skfd;                               // generic raw socket desc
    struct iwreq wrq;                       // ioctl request structure
    //struct ifreq ifr;
    struct timeval et;                      // elapsed time
    struct wireless_info info;              // workarea for wireless ioctl information
    FILE *fp;
    char  bfr[1024], ifName[1024];
    char *s, *t;

    gettimeofday ( &et, ( struct timezone * ) 0 );  // get time-of-day
    if ( et.tv_sec < lastLoad + MINLOADFREQ )       // only reload so often
	return;
    lastLoad = et.tv_sec;

    skfd = openSocket();                            // open socket
    if ( skfd < 0 ) {
		log_printf (LOG_ERROR, "SNMP ieee802dot11.loadTables() - %s\n", "socket open failure" );
		return;
    }

    flushLists();

    // find interfaces in /proc/net/dev and find the wireless interfaces
    fp = fopen ( PROC_NET_DEV, "r" );
    if ( fp ) {
	while ( fgets ( bfr, sizeof ( bfr ), fp )) {
	    if ( strstr ( bfr, ":" )) {
		s = bfr; t = ifName;
		while ( isspace ( *s ))                     // discard white space
		    *s++;
		while ( *s != ':' )                         // get interface name
		    *t++ = *s++;
		*t = '\0';

		// verify as a wireless device
		memset (( char * ) &info, 0, sizeof ( struct wireless_info ));
		strncpy ( wrq.ifr_name, ifName, IFNAMSIZ );
		if ( ioctl ( skfd, SIOCGIWNAME, &wrq ) >= 0 ) {
		    initStructs();
		    loadWiExt( skfd, ifName, &info );
		    displayWiExt ( info );
		    load80211Structs ( skfd, ifName, &info );
		}
	    }
	}
	fclose ( fp );
    }

    close ( skfd );
}

/****************************************************************************
 *                                                                           *
 *              load80211Structs() - load the 802.11 structures              *
 *                                                                           *
 ****************************************************************************/
static void
load80211Structs ( int skfd, char *ifName, struct wireless_info *wi )
{
    int rc, ifIndex = 0;
    struct ifreq ifr;
    char  MACAddress [ MACADDR_LEN + 1 ];

    strcpy ( ifr.ifr_name, ifName );
    rc = ioctl ( skfd, SIOCGIFHWADDR, &ifr );
    if ( rc >= 0 ) {

	sprintf ( MACAddress, "%02X:%02X:%02X:%02X:%02X:%02X\0",
		 ( UCHAR ) ifr.ifr_hwaddr.sa_data[0], ( UCHAR ) ifr.ifr_hwaddr.sa_data[1],
		 ( UCHAR ) ifr.ifr_hwaddr.sa_data[2], ( UCHAR ) ifr.ifr_hwaddr.sa_data[3],
		 ( UCHAR ) ifr.ifr_hwaddr.sa_data[4], ( UCHAR ) ifr.ifr_hwaddr.sa_data[5] );

	nSc.haveStationID = 1;
	strcpy  ( nSc.stationID, MACAddress );
	nOp.haveMACAddress = 1;
	strcpy  ( nOp.MACAddress, MACAddress );
	nRi.haveManufacturerOUI = 1;
	strncpy ( nRi.manufacturerOUI, MACAddress, MAN_OUI_LEN );

	ifIndex = ifcache_get_ifindex ( ifName );
	if ( !ifIndex ) {
	    log_printf (LOG_WARNING, "SNMP %s - %s %s\n",
		    "ieee802dot11.load80211Structs()", ifName, "has no ifIndex\n" );
	    return;
	}

	loadWiExtTo80211Structs ( ifIndex, ifName, wi );

	if ( hasChanged (( char * ) &nSc, sizeof ( nSc ))) {
	    nSc.ifIndex = ifIndex;
	    sprintf ( nSc.UID, "%04d\0", nSc.ifIndex );
	    strcpy ( nSc.ifName, ifName );
	    addList (( char * ) &scList, ( char * ) &nSc, sizeof ( nSc ));
	}

	if ( hasChanged (( char * ) &nPr, sizeof ( nPr ))) {
	    nPr.ifIndex = ifIndex;
	    sprintf ( nPr.UID, "%04d\0", nPr.ifIndex );
	    strcpy ( nPr.ifName, ifName );
	    addList (( char * ) &prList, ( char * ) &nPr, sizeof ( nPr ));
	}

	if ( hasChanged (( char * ) &nOp, sizeof ( nOp ))) {
	    nOp.ifIndex = ifIndex;
	    sprintf ( nOp.UID, "%04d\0", nOp.ifIndex );
	    strcpy ( nOp.ifName, ifName );
	    addList (( char * ) &opList, ( char * ) &nOp, sizeof ( nOp ));
	}

	if ( hasChanged (( char * ) &nCo, sizeof ( nCo ))) {
	    nCo.ifIndex = ifIndex;
	    sprintf ( nCo.UID, "%04d\0", nCo.ifIndex );
	    strcpy ( nCo.ifName, ifName );
	    addList (( char * ) &coList, ( char * ) &nCo, sizeof ( nCo ));
	}

	if ( hasChanged (( char * ) &nRi, sizeof ( nRi ))) {
	    nRi.ifIndex = ifIndex;
	    sprintf ( nRi.UID, "%04d\0", nRi.ifIndex );
	    strcpy ( nRi.ifName, ifName );
	    addList (( char * ) &riList, ( char * ) &nRi, sizeof ( nRi ));
	}

	if ( hasChanged (( char * ) &nPo, sizeof ( nPo ))) {
	    nPo.ifIndex = ifIndex;
	    sprintf ( nPo.UID, "%04d\0", nPo.ifIndex );
	    strcpy ( nPo.ifName, ifName );
	    addList (( char * ) &poList, ( char * ) &nPo, sizeof ( nPo ));
	}

	if ( hasChanged (( char * ) &nPa, sizeof ( nPa ))) {
	    nPa.ifIndex = ifIndex;
	    sprintf ( nPa.UID, "%04d\0", nPa.ifIndex );
	    strcpy ( nPa.ifName, ifName );
	    addList (( char * ) &paList, ( char * ) &nPa, sizeof ( nPa ));
	}

	if ( hasChanged (( char * ) &nPt, sizeof ( nPt ))) {
	    nPt.ifIndex = ifIndex;
	    sprintf ( nPt.UID, "%04d\0", nPt.ifIndex );
	    strcpy ( nPt.ifName, ifName );
	    addList (( char * ) &ptList, ( char * ) &nPt, sizeof ( nPt ));
	}

	if ( hasChanged (( char * ) &nPf, sizeof ( nPf ))) {
	    nPf.ifIndex = ifIndex;
	    sprintf ( nPf.UID, "%04d\0", nPf.ifIndex );
	    strcpy ( nPf.ifName, ifName );
	    addList (( char * ) &pfList, ( char * ) &nPf, sizeof ( nPf ));
	}

	if ( hasChanged (( char * ) &nPd, sizeof ( nPd ))) {
	    nPd.ifIndex = ifIndex;
	    sprintf ( nPd.UID, "%04d\0", nPd.ifIndex );
	    strcpy ( nPd.ifName, ifName );
	    addList (( char * ) &pdList, ( char * ) &nPd, sizeof ( nPd ));
	}

	if ( hasChanged (( char * ) &nPi, sizeof ( nPi ))) {
	    nPi.ifIndex = ifIndex;
	    sprintf ( nPi.UID, "%04d\0", nPi.ifIndex );
	    strcpy ( nPi.ifName, ifName );
	    addList (( char * ) &piList, ( char * ) &nPi, sizeof ( nPi ));
	}
    }
}

/* UBNT get FW version from this file */
#define FW_VERSION "/usr/lib/version"

/****************************************************************************
 *                                                                           *
 *                     initStructs() - initialize structures                 *
 *                                                                           *
 ****************************************************************************/
static void initStructs()
{
    int i;
    FILE *f;

    // 802.11 MIB Stuctures
    memset (( char * ) &nSc, 0, sizeof ( nSc ));  memset (( char * ) &nAa, 0, sizeof ( nAa ));
    memset (( char * ) &nDf, 0, sizeof ( nDf ));  memset (( char * ) &nKm, 0, sizeof ( nKm ));
    memset (( char * ) &nPr, 0, sizeof ( nPr ));  memset (( char * ) &nOp, 0, sizeof ( nOp ));
    memset (( char * ) &nCo, 0, sizeof ( nCo ));  memset (( char * ) &nGa, 0, sizeof ( nGa ));
    memset (( char * ) &nRi, 0, sizeof ( nRi ));  memset (( char * ) &nPo, 0, sizeof ( nPo ));
    memset (( char * ) &nPa, 0, sizeof ( nPa ));  memset (( char * ) &nPt, 0, sizeof ( nPt ));
    memset (( char * ) &nPf, 0, sizeof ( nPf ));  memset (( char * ) &nPd, 0, sizeof ( nPd ));
    memset (( char * ) &nPi, 0, sizeof ( nPi ));  memset (( char * ) &nRd, 0, sizeof ( nRd ));
    memset (( char * ) &nAl, 0, sizeof ( nAl ));  memset (( char * ) &nRt, 0, sizeof ( nRt ));
    memset (( char * ) &nRr, 0, sizeof ( nRr ));

    // Wireless Extensions
    wepCurrentKey = 0;
    haveWepCurrentKey = 0;
    for ( i = 0; i < MAX_WEP_KEYS; i++ ) {
	wep[i].len = 0;
	wep[i].key[0] = '\0';
	wep[i].haveKey = 0;
    }

    {
	    nOp.haveProductID = 0;
	    nRi.haveManufacturerProductName = 0;
	    FILE *f = fopen("/etc/board.info", "r");
	    char buf[80], product[80] = "", productID[80] = "";
	    char manufac[80] = "Ubiquiti Networks, Inc.";
	    if (f)
	    {
		    while (fgets(buf, sizeof(buf), f))
		    {
			    if (!nOp.haveProductID && sscanf(buf, "board.sysid=%79s", productID) == 1)
			    {
				    nOp.haveProductID = 1;
			    }
			    if (!nRi.haveManufacturerProductName && sscanf(buf, "board.name=%79[^\n]", product) == 1)
			    {
				    nRi.haveManufacturerProductName = 1;
			    }
		    }
		    fclose(f);
	    }
	    snprintf(nOp.productID, sizeof(nOp.productID), productID);
	    snprintf(nRi.manufacturerProductName, sizeof(nRi.manufacturerProductName), product);
	    nRi.haveManufacturerName = 1;
	    snprintf(nRi.manufacturerName, sizeof(nRi.manufacturerName), manufac);
    }
    if (f = fopen(FW_VERSION, "r")) {
	    if (fgets(nRi.manufacturerProductVersion, sizeof(nRi.manufacturerProductVersion), f))
		{
			nRi.manufacturerProductVersion[strlen(nRi.manufacturerProductVersion) - 1] = '\0';
			nRi.haveManufacturerProductVersion = 1;
		}
	    fclose(f);
    }
}

/****************************************************************************
 *                                                                           *
 *                Wireless Extensions Specific Functions                     *
 *                                                                           *
 ****************************************************************************/
/****************************************************************************
 *                                                                           *
 * loadWiExtTo80211Structs() - load wireless extensions to 802.11 structures *
 *                                                                           *
 ****************************************************************************/
static void
loadWiExtTo80211Structs ( int ifIndex, char *ifName, struct wireless_info *wi )
{
    int i, j = 0;

    // dot11Smt Group
    // dot11StationConfigTable
    nSc.havePrivacyOptionImplemented = 1;
    nSc.privacyOptionImplemented = 1;           // assume we support WEP

    if ( wi->has_power ) {
	nSc.havePowerManagementMode = 1;
	nSc.powerManagementMode = 1;              // assume power is active
	if ( !wi->power.disabled &&
	    wi->power.flags & IW_POWER_MIN )
	    nSc.powerManagementMode = 2;            // power save mode
    }

    if ( wi->has_essid && strlen ( wi->essid )) {
	nSc.haveDesiredSSID = 1;
	strcpy ( nSc.desiredSSID, wi->essid );
    }

    if ( wi->has_mode ) {
	nSc.haveDesiredBSSType = 1;
	if ( wi->mode == IW_MODE_ADHOC )
	    nSc.desiredBSSType = 2;         // independent
	else if ( wi->has_ap_addr )
	    nSc.desiredBSSType = 1;         // infrastructure
	else
	    nSc.desiredBSSType = 3;         // any
    }

    if ( wi->has_range ) {
	for ( i = 0; i < wi->range.num_bitrates && j < 126; i++ ) {
	    nSc.haveOperationalRateSet = 1;
	    nSc.operationalRateSet[j++] = ( char ) ( wi->range.bitrate[i] / 500000L );
	}
    }

    // dot11AuthenticationAlgorithmsTable
    nAa.haveAuthenticationAlgorithm = 1;           // it's a rule to always have
    nAa.haveAuthenticationAlgorithmsEnable = 1;    //    'open' supported
    nAa.ifIndex = ifIndex;
    nAa.authenticationAlgorithmsIndex = 1;            // index number one
    nAa.authenticationAlgorithm = 1;                  // 1 => open key
    sprintf ( nAa.UID, "%04d%04d\0", nAa.ifIndex, nAa.authenticationAlgorithmsIndex );
    nAa.authenticationAlgorithmsEnable = 1;           // enabled by default
    if ( ( wi->has_key                        ) &&
	( wi->key_size  != 0                 ) &&
	!( wi->key_flags & IW_ENCODE_DISABLED ))
	nAa.authenticationAlgorithmsEnable = 2;
    addList (( char * ) &aaList, ( char * ) &nAa, sizeof ( nAa ));

    nAa.haveAuthenticationAlgorithm = 1;           // I'm gonna assume we always support WEP
    nAa.haveAuthenticationAlgorithmsEnable = 1;
    nAa.ifIndex = ifIndex;
    nAa.authenticationAlgorithmsIndex = 2;            // index number 2
    nAa.authenticationAlgorithm = 2;                  // 2 => shared key
    sprintf ( nAa.UID, "%04d%04d\0", nAa.ifIndex, nAa.authenticationAlgorithmsIndex );
    nAa.authenticationAlgorithmsEnable = 2;
    if ( ( wi->has_key                        ) &&
	( wi->key_size  != 0                 ) &&
	!( wi->key_flags & IW_ENCODE_DISABLED ))
	nAa.authenticationAlgorithmsEnable = 1;         // disabled by default
    addList (( char * ) &aaList, ( char * ) &nAa, sizeof ( nAa ));

    //dot11WEPDefaultKeysTable
    if ( wi->has_range ) {
	for ( i = 0; i < MAX_WEP_KEYS; i++ ) {
	    nDf.haveWEPDefaultKeyValue = 1;
	    nDf.ifIndex = ifIndex;
	    nDf.WEPDefaultKeyIndex = i + 1;               // index number
	    sprintf ( nDf.UID, "%04d%04d\0", nDf.ifIndex, nDf.WEPDefaultKeyIndex );
	    if ( wep[i].haveKey )
		strcpy ( nDf.WEPDefaultKeyValue, "*****" );
	    else
		nDf.WEPDefaultKeyValue[0] = '\0';
	    addList (( char * ) &dfList, ( char * ) &nDf, sizeof ( nDf ));
	}
    }

    // dot11PrivacyTable
    nPr.havePrivacyInvoked = 1;
    nPr.privacyInvoked = 2;                   // 2 => FALSE
    nPr.haveWEPDefaultKeyID = 1;
    nPr.WEPDefaultKeyID = 0;
    nPr.haveExcludeUnencrypted = 1;
    nPr.excludeUnencrypted = 2;               // 2 => FALSE
    if ( wi->has_range ) {
	if ( ( wi->key_size != 0 ) &&
	    !( wi->key_flags & IW_ENCODE_DISABLED )) {
	    nPr.privacyInvoked = 1;
	    if ( wi->key_flags & IW_ENCODE_RESTRICTED )
		nPr.excludeUnencrypted = 1;
	    nPr.WEPDefaultKeyID = wepCurrentKey;
	}
    }

    // dot11Mac Group
    // dot11OperationTable
    if ( wi->has_rts ) {
	nOp.haveRTSThreshold = 1;
	nOp.RTSThreshold = wi->rts.value;
    } else if (wi->has_range) {
	nOp.haveRTSThreshold = 1;
	nOp.RTSThreshold = wi->range.max_rts;
	}

    if ( wi->has_frag && wi->frag.value ) {
	nOp.haveFragmentationThreshold = 1;
	nOp.fragmentationThreshold = wi->frag.value;
    }

    // dot11Phy Group
    // dot11PhyOperationTable
    if ( strstr ( wi->name, "IEEE 802.11-FS"      )) nPo.PHYType = 1;   // So what if I
    if ( strstr ( wi->name, "IEEE 802.11b"      )) nPo.PHYType = 2;   // made up a couple?
    if ( strstr ( wi->name, "IEEE 802.11-IR"      )) nPo.PHYType = 3;
    if ( strstr ( wi->name, "IEEE 802.11a"    )) nPo.PHYType = 4;   // 802.11a
    if ( strstr ( wi->name, "IEEE 802.11g" )) nPo.PHYType = 5;   // 802.11g
    if ( strstr ( wi->name, "IEEE 802.11T"   )) nPo.PHYType = 6;   // Atheros TURBO mode
    if ( strstr ( wi->name, "IEEE 802.11Ta"   )) nPo.PHYType = 7;   // Atheros TURBO mode
    if ( strstr ( wi->name, "IEEE 802.11Tg"   )) nPo.PHYType = 8;   // Atheros TURBO mode
    if ( nPo.PHYType ) nPo.havePHYType = 1;

    // dot11PhyDSSSTable
	// XXX
    if ( wi->has_range ) { // && wi->freq <= ( double ) 2483000000 ) {  // DSSS frequencies only
	for ( i = 0; i < wi->range.num_frequency; i++ ) {
	    if ((( double ) ( wi->range.freq[i].e * 10 ) * ( double ) wi->range.freq[i].m ) == wi->freq ) {
		nPd.haveCurrentChannel = 1;
		nPd.currentChannel = wi->range.freq[i].i;
	    }
	}
    }

    // dot11SupportedDataRatesTxTable
    if ( wi->has_range ) {
	for ( i = 0; i < wi->range.num_bitrates; i++ ) {
	    nRt.ifIndex = ifIndex;
	    nRt.supportedDataRatesTxIndex = i + 1;
	    nRt.supportedDataRatesTxValue = wi->range.bitrate[i] / 500000L;
	    nRt.haveSupportedDataRatesTxValue = 1;
	    sprintf ( nRt.UID, "%04d%04d\0", nRt.ifIndex, nRt.supportedDataRatesTxIndex );
	    strcpy ( nRt.ifName, ifName );
	    addList (( char * ) &rtList, ( char * ) &nRt, sizeof ( nRt ));
	}
    }

    // dot11SupportedDataRatesRxTable
    if ( wi->has_range ) {
	for ( i = 0; i < wi->range.num_bitrates; i++ ) {
	    nRr.ifIndex = ifIndex;
	    nRr.supportedDataRatesRxIndex = i + 1;
	    nRr.supportedDataRatesRxValue = wi->range.bitrate[i] / 500000L;
	    nRr.haveSupportedDataRatesRxValue = 1;
	    sprintf ( nRr.UID, "%04d%04d\0", nRr.ifIndex, nRr.supportedDataRatesRxIndex );
	    strcpy ( nRr.ifName, ifName );
	    addList (( char * ) &rrList, ( char * ) &nRr, sizeof ( nRr ));
	}
    }

	/* stats table */
	if (wi->has_stats) {
			nCo.haveFailedCount = 1;
			nCo.failedCount = wi->stats.discard.nwid
					+ wi->stats.discard.fragment 
					+ wi->stats.discard.misc;
			nCo.haveRetryCount = 1;
			nCo.retryCount = wi->stats.discard.retries;
			nCo.haveWEPUndecryptableCount = 1;
			nCo.WEPUndecryptableCount = wi->stats.discard.code;
	}
}

/****************************************************************************
 *                                                                           *
 *      loadWiExt() - load wireless extensions structures;                   *
 *                    use ioctl calls and read /proc/net/wireless            *
 *                                                                           *
 ****************************************************************************/
static void loadWiExt ( int skfd, char *ifname, struct wireless_info *wi )
{
    struct iwreq wrq;                       // ioctl request structure
    FILE *fp;
    char  bfr[1024];
    char  buffer[sizeof ( iwrange ) * 2]; /* Large enough */
    char *s, *t;
    int i, j;

    strncpy ( wrq.ifr_name, ifname, IFNAMSIZ );

    /* Get wireless name */
    if ( ioctl ( skfd, SIOCGIWNAME, &wrq ) >= 0 ) {
	strncpy ( wi->name, wrq.u.name, IFNAMSIZ );
	wi->name[IFNAMSIZ] = '\0';
    }

    /* Get ranges */    // NOTE: some version checking in iwlib.c
    memset ( buffer, 0, sizeof ( buffer ));
    wrq.u.data.pointer = ( caddr_t ) &buffer;
    wrq.u.data.length = sizeof ( buffer );
    wrq.u.data.flags = 0;
    if ( ioctl ( skfd, SIOCGIWRANGE, &wrq ) >= 0 ) {
	memcpy (( char * ) &wi->range, buffer, sizeof ( iwrange ));
	wi->has_range = 1;
    }

    /* Get network ID */
    if ( ioctl ( skfd, SIOCGIWNWID, &wrq ) >= 0 ) {
	memcpy ( &wi->nwid, &wrq.u.nwid, sizeof ( iwparam ));
	wi->has_nwid = 1;
    }

    /* Get frequency / channel */         // THIS NUMBER LOOKS FUNNY
    if ( ioctl ( skfd, SIOCGIWFREQ, &wrq ) >= 0 ) {
	wi->freq = ( double ) wrq.u.freq.m;
	wi->has_freq = 1;
	while (wrq.u.freq.e--)
	    wi->freq *= 10;
    }

    /* Get sensitivity */
    if ( ioctl ( skfd, SIOCGIWSENS, &wrq ) >= 0 ) {
	wi->has_sens = 1;
	memcpy ( &wi->sens, &wrq.u.sens, sizeof ( iwparam ));
    }

    /* Get encryption information */
    wrq.u.data.pointer = ( caddr_t ) &wi->key;
    wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
    wrq.u.data.flags = 0;
    if ( ioctl ( skfd, SIOCGIWENCODE, &wrq ) >= 0 ) {
	wi->has_key = 1;
	wi->key_size = wrq.u.data.length;
	wi->key_flags = wrq.u.data.flags;
	wepCurrentKey = wrq.u.data.flags & IW_ENCODE_INDEX;
    }

    for ( i = 0; i < wi->range.max_encoding_tokens; i++ ) {
	wrq.u.data.pointer = ( caddr_t ) &wi->key;
	wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	wrq.u.data.flags = i;
	if ( ioctl ( skfd, SIOCGIWENCODE, &wrq ) >= 0 ) {
	    if ( ( wrq.u.data.length != 0 ) &&
		!( wrq.u.data.flags & IW_ENCODE_DISABLED )) {
		wep[i].len = wrq.u.data.length;
		wep[i].haveKey = 1;
		t = wep[i].key;
		for ( j = 0; j < wrq.u.data.length; j++ ) {
		    if (( j & 0x1 ) == 0 && j != 0 )
			strcpy ( t++, "-");
		    sprintf ( t, "%.2X", wi->key[j] );
		    t += 2;
		}
		t = '\0';
	    }
	}
    }

    /* Get ESSID */
    wrq.u.essid.pointer = ( caddr_t ) &wi->essid;
    wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
    wrq.u.essid.flags = 0;
    if ( ioctl ( skfd, SIOCGIWESSID, &wrq ) >= 0 ) {
	wi->has_essid = 1;
	wi->essid_on = wrq.u.data.flags;
    }

    /* Get AP address */
    if ( ioctl ( skfd, SIOCGIWAP, &wrq ) >= 0 ) {
	wi->has_ap_addr = 1;
	memcpy ( &wi->ap_addr, &wrq.u.ap_addr, sizeof ( sockaddr ));
    }

    /* Get NickName */
    wrq.u.essid.pointer = ( caddr_t ) &wi->nickname;
    wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
    wrq.u.essid.flags = 0;
    if ( ioctl ( skfd, SIOCGIWNICKN, &wrq ) >= 0 ) {
	if ( wrq.u.data.length > 1 )
	    wi->has_nickname = 1;
    }

    /* Get bit rate */
    if ( ioctl ( skfd, SIOCGIWRATE, &wrq ) >= 0 ) {
	wi->has_bitrate = 1;
	memcpy ( &wi->bitrate, &wrq.u.bitrate, sizeof ( iwparam ));
    }

    /* Get RTS threshold */
    if ( ioctl ( skfd, SIOCGIWRTS, &wrq ) >= 0 ) {
	wi->has_rts = 1;
	memcpy ( &wi->rts, &wrq.u.rts, sizeof ( iwparam ));
    }

    /* Get fragmentation threshold */
    if ( ioctl ( skfd, SIOCGIWFRAG, &wrq ) >= 0 ) {
	wi->has_frag = 1;
	memcpy ( &wi->frag, &wrq.u.frag, sizeof ( iwparam ));
    }

    /* Get operation mode */
    if ( ioctl ( skfd, SIOCGIWMODE, &wrq ) >= 0 ) {
	wi->mode = wrq.u.mode;
	if ( wi->mode < IW_NUM_OPER_MODE && wi->mode >= 0 )
	    wi->has_mode = 1;
    }

    /* Get Power Management settings */                 // #if WIRELESS_EXT > 9
    wrq.u.power.flags = 0;
    if ( ioctl ( skfd, SIOCGIWPOWER, &wrq ) >= 0 ) {
	wi->has_power = 1;
	memcpy ( &wi->power, &wrq.u.power, sizeof ( iwparam ));
    }

    /* Get retry limit/lifetime */                      // #if WIRELESS_EXT > 10
    if ( ioctl ( skfd, SIOCGIWRETRY, &wrq ) >= 0 ) {
	wi->has_retry = 1;
	memcpy ( &wi->retry, &wrq.u.retry, sizeof ( iwparam ));
    }

    /* Get stats */                                     // #if WIRELESS_EXT > 11
    wrq.u.data.pointer = ( caddr_t ) &wi->stats;
    wrq.u.data.length = 0;
    wrq.u.data.flags = 1;   /* Clear updated flag */
    if ( ioctl ( skfd, SIOCGIWSTATS, &wrq ) < 0 )
	wi->has_stats = 1;

    if ( !wi->has_stats ) {                        // no ioctl support, go to file
	fp = fopen ( PROC_NET_WIRELESS, "r" );
	if ( fp ) {
	    while ( fgets ( bfr, sizeof ( bfr ), fp )) {
		bfr [ sizeof ( bfr ) - 1 ] = '\0';        // no buffer overruns here!
		strtok (( char * ) &bfr, "\n" );          // '\n' => '\0'
		if ( strstr ( bfr, ifname ) && strstr ( bfr, ":" )) {
		    wi->has_stats = 1;
		    s = bfr;
		    s = strchr ( s, ':' ); s++;             /* Skip ethX:   */
		    s = strtok ( s, " " );                  /* ' ' => '\0'  */
		    sscanf ( s, "%X", &wi->stats.status ); // status

		    s = strtok ( NULL, " " );               // link quality
		    if ( strchr ( s, '.' ) != NULL )
			wi->stats.qual.updated |= 1;
		    sscanf ( s, "%d", &wi->stats.qual.qual );

		    s = strtok ( NULL, " " );               // signal level
		    if ( strchr ( s,'.' ) != NULL )
			wi->stats.qual.updated |= 2;
		    sscanf ( s, "%d", &wi->stats.qual.level );

		    s = strtok ( NULL, " " );               // noise level
		    if ( strchr ( s, '.' ) != NULL )
			wi->stats.qual.updated += 4;
		    sscanf ( s, "%d", &wi->stats.qual.noise );

		    s = strtok ( NULL, " " ); sscanf ( s, "%d", &wi->stats.discard.nwid     );
		    s = strtok ( NULL, " " ); sscanf ( s, "%d", &wi->stats.discard.code     );
		    s = strtok ( NULL, " " ); sscanf ( s, "%d", &wi->stats.discard.fragment );
		    s = strtok ( NULL, " " ); sscanf ( s, "%d", &wi->stats.discard.retries  );
		    s = strtok ( NULL, " " ); sscanf ( s, "%d", &wi->stats.discard.misc     );
		    s = strtok ( NULL, " " ); sscanf ( s, "%d", &wi->stats.miss.beacon      );
		}
	    }
	    fclose ( fp );
	}
    }
}

/****************************************************************************
 *                                                                           *
 *       displayWiExt() - show what I got from Wireless Extensions           *
 *                                                                           *
 ****************************************************************************/
static void displayWiExt ( struct wireless_info info )
{
#ifdef DISPLAYWIEXT
    int i;
    char title[] = "displayWiExt() -";

    printf ( "========================================\n" );
    printf ( "===> Wireless Extension IOCTL calls <===\n" );
    printf ( "========================================\n" );

    if ( strlen ( info.name ))
	printf ( "%s name: %s\n", "SIOCGIWNAME", info.name );
    else
	printf ( "%s\n", "no info.name support" );

    if ( info.has_nickname == 1 )
	printf ( "%s nickname: %s\n", "SIOCGIWNICKN", info.nickname );
    else
	printf ( "%s %s\n", "SIOCGIWNICKN", " ===> no info.nickname support" );

    if ( info.has_essid )
	printf ( "%s essid_on: %d essid: %s\n", "SIOCGIWESSID", info.essid_on, info.essid );
    else
	printf ( "%s %s\n", "SIOCGIWESSID", " ===> no info.essid support" );

    if ( info.has_range ) {
	printf ( "%s throughput: %d\n",           "SIOCGIWRANGE", info.range.throughput );
	printf ( "%s min_nwid: %d\n",             "SIOCGIWRANGE", info.range.min_nwid  );
	printf ( "%s max_nwid: %d\n",             "SIOCGIWRANGE", info.range.max_nwid  );
	printf ( "%s sensitivity: %d\n",          "SIOCGIWRANGE", info.range.sensitivity );
	printf ( "%s num_bitrates: %d\n",         "SIOCGIWRANGE", info.range.num_bitrates );
	for ( i = 0; i < info.range.num_bitrates; i++ )
	    printf ( "%s bitrate[%d]: %d\n",        "SIOCGIWRANGE", i, info.range.bitrate[i]  );
	printf ( "%s min_rts: %d\n",              "SIOCGIWRANGE", info.range.min_rts );
	printf ( "%s max_rts: %d\n",              "SIOCGIWRANGE", info.range.max_rts );
	printf ( "%s min_frag: %d\n",             "SIOCGIWRANGE", info.range.min_frag );
	printf ( "%s max_frag: %d\n",             "SIOCGIWRANGE", info.range.max_frag );
	printf ( "%s min_pmp: %d\n",              "SIOCGIWRANGE", info.range.min_pmp );
	printf ( "%s max_pmp: %d\n",              "SIOCGIWRANGE", info.range.max_pmp );
	printf ( "%s min_pmt: %d\n",              "SIOCGIWRANGE", info.range.min_pmt );
	printf ( "%s max_pmt: %d\n",              "SIOCGIWRANGE", info.range.max_pmt );
	printf ( "%s pmp_flags: %d\n",            "SIOCGIWRANGE", info.range.pmp_flags );
	printf ( "%s pmt_flags: %d\n",            "SIOCGIWRANGE", info.range.pmt_flags );
	printf ( "%s pm_capa: %d\n",              "SIOCGIWRANGE", info.range.pm_capa );
	printf ( "%s num_encoding_sizes: %d\n",   "SIOCGIWRANGE", info.range.num_encoding_sizes );
	for ( i = 0; i < info.range.num_encoding_sizes; i++ )
	    printf ( "%s encoding_size[%d]: %d\n",  "SIOCGIWRANGE", i, info.range.encoding_size[i]  );
	printf ( "%s max_encoding_tokens: %d\n",  "SIOCGIWRANGE", info.range.max_encoding_tokens );
	//  printf ( "%s encoding_login_index: %d\n", "SIOCGIWRANGE", info.range.encoding_login_index );
	printf ( "%s txpower_capa: %d\n",         "SIOCGIWRANGE", info.range.txpower_capa );
	printf ( "%s num_txpower: %d dBm\n",      "SIOCGIWRANGE", info.range.num_txpower );
	for ( i = 0; i < info.range.num_txpower; i++ )
	    printf ( "%s txpower[%d]: %d\n",        "SIOCGIWRANGE", i, info.range.txpower[i]  );
	printf ( "%s we_version_compiled: %d\n",  "SIOCGIWRANGE", info.range.we_version_compiled );
	printf ( "%s we_version_source: %d\n",    "SIOCGIWRANGE", info.range.we_version_source );
	printf ( "%s retry_capa: %d\n",           "SIOCGIWRANGE", info.range.retry_capa );
	printf ( "%s retry_flags: %d\n",          "SIOCGIWRANGE", info.range.retry_flags );
	printf ( "%s r_time_flags: %d\n",         "SIOCGIWRANGE", info.range.r_time_flags );
	printf ( "%s min_retry: %d\n",            "SIOCGIWRANGE", info.range.min_retry );
	printf ( "%s max_retry: %d\n",            "SIOCGIWRANGE", info.range.max_retry );
	printf ( "%s min_r_time: %d\n",           "SIOCGIWRANGE", info.range.min_r_time );
	printf ( "%s max_r_time: %d\n",           "SIOCGIWRANGE", info.range.max_r_time );
	printf ( "%s num_channels: %d\n",         "SIOCGIWRANGE", info.range.num_channels );
	printf ( "%s num_frequency: %d\n",        "SIOCGIWRANGE", info.range.num_frequency );
	for ( i = 0; i < info.range.num_frequency; i++ )
	    printf ( "%s freq[%d].i: %d freq[%d].e: %d freq[%d].m: %d\n", "SIOCGIWRANGE",
		    i, info.range.freq[i].i, i, info.range.freq[i].e, i, info.range.freq[i].m );
    }
    else
	printf ( "%s %s\n", "SIOCGIWRANGE", " ===> no info.range support" );

    if ( info.has_nwid )
	printf ( "%s nwid - disabled: %d value: %X\n", "SIOCGIWNWID", info.nwid.disabled, info.nwid.value );
    else
	printf ( "%s %s\n", "SIOCGIWNWID", " ===> no info.nwid support" );

    if ( info.has_freq ) {
	//  printf ( "%s freq: %g\n", "SIOCGIWFREQ", info.freq / GIGA );
	printf ( "%s freq: %g\n", "SIOCGIWFREQ", info.freq );
    }
    else
	printf ( "%s %s\n", "SIOCGIWFREQ", " ===> no info.freq support" );

    if ( info.has_sens )
	printf ( "%s sens: %d\n", "SIOCGIWSENS", info.sens );
    else
	printf ( "%s %s\n", "SIOCGIWSENS", " ===> no info.sens support" );

    if ( info.has_key ) {
	printf ( "%s key_size: %d key_flags: %d wepCurrentKey: %d\n",
		"SIOCGIWENCODE", info.key_size, info.key_flags, wepCurrentKey );
	printf ( "%s MODE: %d DISABLED: %d INDEX: %d OPEN: %d RESTRICTED: %d NOKEY: %d TEMP: %d\n",
		"SIOCGIWENCODE",                           info.key_flags & IW_ENCODE_MODE,
		info.key_flags & IW_ENCODE_DISABLED ? 1:0, info.key_flags & IW_ENCODE_INDEX,
		info.key_flags & IW_ENCODE_OPEN     ? 1:0, info.key_flags & IW_ENCODE_RESTRICTED ? 1:0,
		info.key_flags & IW_ENCODE_NOKEY    ? 1:0, info.key_flags & IW_ENCODE_TEMP       ? 1:0 );
    }
    else
	printf ( "%s %s\n", "SIOCGIWENCODE", " ===> no info.key support" );

    for ( i = 0; i < MAX_WEP_KEYS; i++ ) {
	if ( wep[i].haveKey )
	    printf ( "%s wep[%d].len: %d wep[%d].key: %s\n",
		    "SIOCGIWENCODE", i, wep[i].len, i, wep[i].key );
    }

    if ( info.has_ap_addr )
	printf ( "%s ap_addr.sa_data: %02X:%02X:%02X:%02X:%02X:%02X ap_addr.sa_family: %d\n",
		"SIOCGIWAP",  ( UCHAR ) info.ap_addr.sa_data[0], ( UCHAR ) info.ap_addr.sa_data[1],
		( UCHAR ) info.ap_addr.sa_data[2], ( UCHAR ) info.ap_addr.sa_data[3],
		( UCHAR ) info.ap_addr.sa_data[4], ( UCHAR ) info.ap_addr.sa_data[5],
		info.ap_addr.sa_family );
    else
	printf ( "%s %s\n", "SIOCGIWAP", " ===> no ap_addr information" );

    if ( info.has_bitrate )
	printf ( "%s bitrate: %d value: %d fixed: %d disabled: %d flags: %d\n",
		"SIOCGIWRATE", info.bitrate, info.bitrate.value, info.bitrate.fixed,
		info.bitrate.disabled, info.bitrate.flags );
    else
	printf ( "%s %s\n", "SIOCGIWRATE", " ===> no info.bitrate support" );

    if ( info.has_rts )
	printf ( "%s rts: %d\n", "SIOCGIWRTS", info.rts );
    else
	printf ( "%s %s\n", "SIOCGIWRTS", " ===> no info.rts support" );

    if ( info.has_frag )
	printf ( "%s frag: %d\n", "SIOCGIWFRAG", info.frag );
    else
	printf ( "%s %s\n", "SIOCGIWFRAG", " ===> no info.frag support" );

    if ( info.has_mode )
	printf ( "%s mode: %d\n", "SIOCGIWMODE", info.mode );
    else
	printf ( "%s %s\n", "SIOCGIWMODE", " ===> no info.mode support" );

    if ( info.has_power ) {
	printf ( "%s power: %d\n", "SIOCGIWPOWER", info.power );
	printf ( "%s disabled: %d MIN: %d MAX: %d TIMEOUT: %d RELATIVE: %d\n",
		"SIOCGIWPOWER",
		info.power.disabled                  ? 1:0,
		info.power.flags & IW_POWER_MIN      ? 1:0,
		info.power.flags & IW_POWER_MAX      ? 1:0,
		info.power.flags & IW_POWER_TIMEOUT  ? 1:0,
		info.power.flags & IW_POWER_RELATIVE ? 1:0 );
	printf ( "%s UNICAST: %d MULTICAST: %d ALL: %d FORCE: %d REPEATER: %d\n",
		"SIOCGIWPOWER",
		info.power.flags & IW_POWER_UNICAST_R   ? 1:0,
		info.power.flags & IW_POWER_MULTICAST_R ? 1:0,
		info.power.flags & IW_POWER_ALL_R       ? 1:0,
		info.power.flags & IW_POWER_FORCE_S     ? 1:0,
		info.power.flags & IW_POWER_REPEATER    ? 1:0 );
    }
    else
	printf ( "%s %s\n", "SIOCGIWPOWER", " ===> no info.power support" );

    if ( info.has_retry )
	printf ( "%s retry: %d\n", "SIOCGIWRETRY", info.retry );
    else
	printf ( "%s %s\n", "SIOCGIWRETRY", " ===> no info.retry support" );

    if ( info.has_stats ) {
	printf ( "%s status: %d\n",           "SIOCGIWSTATS", info.stats.status           );
	printf ( "%s qual.level: %d\n",       "SIOCGIWSTATS", info.stats.qual.level       );
	printf ( "%s qual.noise: %d\n",       "SIOCGIWSTATS", info.stats.qual.noise       );
	printf ( "%s qual.qual: %d\n",        "SIOCGIWSTATS", info.stats.qual.qual        );
	printf ( "%s qual.updated: %d\n",     "SIOCGIWSTATS", info.stats.qual.updated     );
	printf ( "%s discard.code: %d\n",     "SIOCGIWSTATS", info.stats.discard.code     );
	printf ( "%s discard.fragment: %d\n", "SIOCGIWSTATS", info.stats.discard.fragment );
	printf ( "%s discard.misc: %d\n",     "SIOCGIWSTATS", info.stats.discard.misc     );
	printf ( "%s discard.nwid: %d\n",     "SIOCGIWSTATS", info.stats.discard.nwid     );
	printf ( "%s discard.retries: %d\n",  "SIOCGIWSTATS", info.stats.discard.retries  );
	printf ( "%s miss.beacon: %d\n",      "SIOCGIWSTATS", info.stats.miss.beacon      );
    }
    else
	printf ( "%s %s\n", "SIOCGIWSTATS", " ===> no info.stats support" );

    if ( info.txpower.flags & IW_TXPOW_MWATT )
	printf ( "%s txpower1: %d dBm disabled: %d fixed: %d flags: %d\n", "SIOCGIWRANGE",
		mWatt2dbm ( info.txpower.value ), info.txpower.disabled, info.txpower.fixed, info.txpower.flags);
    else
	printf ( "%s txpower2: %d dBm disabled: %d fixed: %d flags: %d\n", "SIOCGIWRANGE", info.txpower.value, info.txpower.disabled, info.txpower.fixed, info.txpower.flags );

    if ( info.has_range )
	if ( info.sens.value < 0 )
	    printf ( "%s sens: %d dBm\n", "SIOCGIWRANGE", info.sens.value );
	else
	    printf ( "%s sens: %d/%d\n", "SIOCGIWRANGE", info.sens.value, info.range.sensitivity );

    if ( info.has_range && ( info.stats.qual.level != 0 ))
	if ( info.stats.qual.level > info.range.max_qual.level )
	    /* Statistics are in dBm (absolute power measurement) */
	    printf ( "%s Quality: %d/%d Signal level: %d dBm Noise level: %d dBm\n",
		    "SIOCGIWRANGE",
		    info.stats.qual.qual, info.range.max_qual.qual,
		    info.stats.qual.level - 0x100,
		    info.stats.qual.noise - 0x100 );
	else
	    printf (  "%s Quality: %d/%d Signal level: %d/%d Noise level: %d/%d",
		    "SIOCGIWRANGE",
		    info.stats.qual.qual,  info.range.max_qual.qual,
		    info.stats.qual.level, info.range.max_qual.level,
		    info.stats.qual.noise, info.range.max_qual.noise );

#endif // #ifdef DISPLAYWIEXT
}

/****************************************************************************
 *                                                                           *
 *                        Linked List Functions                              *
 *                                                                           *
 ****************************************************************************/
/****************************************************************************
 *                                                                           *
 *                addList() - add an entry to a linked list                  *
 *                                                                           *
 ****************************************************************************/
static void
addList ( char *l, char *data, int len  )
{
    char uid[256];
    LIST_HEAD ( , avNode ) *list;

    // NOTE: this assumes the UID is at the begining of the
    //       data structure and that UIDs are strings

    list = ( LIST_HEAD ( , avNode ) * ) l;            // NOTE: don't know how to get
    strcpy ( uid, data );                             //  rid of compiler warning on
    //  LISTHEAD typecast
    // create a new node and the data that goes in it
    newNode = malloc ( sizeof ( struct avNode ));
    newNode->data = malloc ( len );
    memcpy ( newNode->data, data, len );

    // this deals with an empty list
    if ( LIST_EMPTY ( list )) {
	LIST_INSERT_HEAD ( list, newNode, nodes );
	return;
    }

    // this deals with UIDs that match
    for ( np = LIST_FIRST ( list ); np != NULL; np = LIST_NEXT ( np, nodes )) {
	if ( strncmp ( uid, np->data, strlen ( uid )) == 0 ) {                      // found matching UID
	    LIST_INSERT_AFTER ( np, newNode, nodes );
	    if ( np->data )
		free ( np->data );
	    LIST_REMOVE ( np, nodes );
	    free ( np );
	    return;
	}
    }

    // this deals with inserting a new UID in the list
    for ( np = LIST_FIRST ( list ); np != NULL; np = LIST_NEXT ( np, nodes )) {
	lastNode = np;
	if ( strncmp ( np->data, uid, strlen ( uid )) > 0 ) {                       // old ID > new ID AND
	    LIST_INSERT_BEFORE ( np, newNode, nodes );
	    return;
	}
    }

    // this deals with a UID that needs to go on the end of the list
    LIST_INSERT_AFTER ( lastNode, newNode, nodes );

    return;
}

/****************************************************************************
 *                                                                           *
 *              initLists() - initialize all the linked lists                *
 *                                                                           *
 ****************************************************************************/
static void initLists()
{
    LIST_INIT ( &scList );  LIST_INIT ( &aaList );  LIST_INIT ( &dfList );
    LIST_INIT ( &kmList );  LIST_INIT ( &prList );
    LIST_INIT ( &opList );  LIST_INIT ( &coList );
    LIST_INIT ( &gaList );  LIST_INIT ( &riList );  LIST_INIT ( &poList );
    LIST_INIT ( &paList );  LIST_INIT ( &ptList );  LIST_INIT ( &pfList );
    LIST_INIT ( &pdList );  LIST_INIT ( &piList );  LIST_INIT ( &rdList );
    LIST_INIT ( &alList );  LIST_INIT ( &rtList );  LIST_INIT ( &rrList );
}
/****************************************************************************
 *                                                                           *
 *                 flushLists() - flush all linked lists                     *
 *                                                                           *
 ****************************************************************************/
static void flushLists()
{
    flushList (( char * ) &scList );  flushList (( char * ) &aaList );
    flushList (( char * ) &dfList );  flushList (( char * ) &kmList );
    flushList (( char * ) &prList );
    flushList (( char * ) &opList );  flushList (( char * ) &coList );
    flushList (( char * ) &gaList );  flushList (( char * ) &riList );
    flushList (( char * ) &poList );  flushList (( char * ) &paList );
    flushList (( char * ) &ptList );  flushList (( char * ) &pfList );
    flushList (( char * ) &pdList );  flushList (( char * ) &piList );
    flushList (( char * ) &rdList );  flushList (( char * ) &alList );
    flushList (( char * ) &rtList );  flushList (( char * ) &rrList );
}

/****************************************************************************
 *                                                                           *
 *                   flushList() - flush a linked list                       *
 *                                                                           *
 ****************************************************************************/
static void flushList ( char *l )
{
    LIST_HEAD ( , avNode ) *list;

    list = ( LIST_HEAD ( , avNode ) * ) l;    // NOTE: don't know how to get
    while ( !LIST_EMPTY ( list )) {           //  rid of compiler warning on
	np = LIST_FIRST ( list );               //  LISTHEAD typecast
	if ( np->data )
	    free ( np->data );
	LIST_REMOVE ( np, nodes );
	free ( np );
    }
}

/****************************************************************************
 *                                                                           *
 *                            Utility Functions                              *
 *                                                                           *
 ****************************************************************************/
/****************************************************************************
 *                                                                           *
 *        The following two routines were taken directly from iwlib.c        *
 *                                                                           *
 ****************************************************************************/
/*
 * Open a socket.
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */
static int openSocket ( void )
{
    static const int families[] = {
	AF_INET, AF_IPX, AF_AX25, AF_APPLETALK
    };
    unsigned int  i;
    int   sock;

    /*
     * Now pick any (exisiting) useful socket family for generic queries
     * Note : don't open all the socket, only returns when one matches,
     * all protocols might not be valid.
     * Workaround by Jim Kaba <jkaba@sarnoff.com>
     * Note : in 99% of the case, we will just open the inet_sock.
     * The remaining 1% case are not fully correct...
     */

    /* Try all families we support */
    for(i = 0; i < sizeof(families)/sizeof(int); ++i) {
	/* Try to open the socket, if success returns it */
	sock = socket(families[i], SOCK_DGRAM, 0);
	if(sock >= 0)
	    return sock;
    }

    return -1;
}

/*------------------------------------------------------------------*/
/*
 * Convert a value in milliWatt to a value in dBm.
 */
static int mWatt2dbm ( int in )
{
#ifdef WE_NOLIBM
    /* Version without libm : slower */
    double  fin = (double) in;
    int   res = 0;

    /* Split integral and floating part to avoid accumulating rounding errors */
    while(fin > 10.0)
    {
	res += 10;
	fin /= 10.0;
    }
    while(fin > 1.000001) /* Eliminate rounding errors, take ceil */
    {
	res += 1;
	fin /= LOG10_MAGIC;
    }
    return(res);
#else /* WE_NOLIBM */
    /* Version with libm : faster */
    return((int) (ceil(10.0 * log10((double) in))));
#endif  /* WE_NOLIBM */
}

/****************************************************************************
 *                                                                           *
 *                 htob - converts hex string to binary                      *
 *                                                                           *
 ****************************************************************************/
static char *htob ( char *s )
{
    char nibl, *byt;
    static char bin[20];

    byt = bin;

    while ((nibl = *s++) && nibl != ' ') {    /* While not end of string. */
	nibl -= ( nibl > '9') ?  ('A' - 10): '0';
	*byt = nibl << 4;                              /* place high nibble */
	if((nibl = *s++) && nibl != ' ') {
	    nibl -= ( nibl > '9') ?  ('A' - 10): '0';
	    *byt |= nibl;                                /*  place low nibble */
	}
	else break;
	++byt;
    }
    *++byt = '\0';
    return ( bin );
}

/****************************************************************************
 *                                                                           *
 *           hasChanged() - see if area has been changed from NULLs          *
 *                                                                           *
 ****************************************************************************/
static int hasChanged ( char *loc, int len )
{
    char *wrk;
    int changed = 1;

    wrk = malloc ( len );
    memset ( wrk, 0, len );
    if ( memcmp ( loc, wrk, len ) == 0 )
	changed = 0;
    free ( wrk );

    return ( changed );
}



//============================================================================
// Copyright (c) 2013 Ubiquitous Computing 
// All rights reserved. 
//
// This program is derived from the lipupnp sample code and used to 
// demostrate a user defined device type -- remote control agent.
//
// Currently is maintained by Jian Zhang (imagezhang@hotmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//============================================================================
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

#include "upnp.h"
#include "UpnpString.h"
#include "upnptools.h"

#include "sample_util.h"

#define RCA_SERVICE_SERVCOUNT	1
#define RCA_SERVICE_CONTROL	0

#define RCA_CONTROL_VARCOUNT	3
#define RCA_CONTROL_POWER	0
#define RCA_CONTROL_MODEL	1
#define RCA_CONTROL_KEY         2

#define RCA_MAX_VAL_LEN		6

#define CTRLPT_SUCCESS		0
#define CTRLPT_ERROR		(-1)
#define CTRLPT_WARNING		1

/* This should be the maximum VARCOUNT from above */
#define RCA_MAXVARS		RCA_CONTROL_VARCOUNT

#define UPNP_MAXVARS            RCA_MAXVARS
#define UPNP_SERVICE_SERVCOUNT  RCA_SERVICE_SERVCOUNT
#define UPNP_CONTROL_VARCOUNT	RCA_CONTROL_VARCOUNT
#define UPNP_MAX_VAL_LEN        6

extern const char *rcaServiceName[];
extern const char *rcaVarName[RCA_SERVICE_SERVCOUNT][RCA_MAXVARS];
extern char rcaVarCount[];

struct UpnpService {
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char *VariableStrVal[UPNP_MAXVARS];
    char EventURL[NAME_SIZE];
    char ControlURL[NAME_SIZE];
    char SID[NAME_SIZE];
};

extern struct UpnpDeviceNode *GlobalDeviceList;

struct UpnpDevice {
    char UDN[250];
    char DescDocURL[250];
    char FriendlyName[250];
    char PresURL[250];
    int  AdvrTimeOut;
    struct UpnpService UpnpService[UPNP_SERVICE_SERVCOUNT];
};

struct UpnpDeviceNode {
    struct UpnpDevice device;
    struct UpnpDeviceNode *next;
};

extern ithread_mutex_t DeviceListMutex;

extern UpnpClient_Handle ctrlptHandle;

void RcaCtrlPointPrintHelp(void);
int RcaCtrlPointDeleteNode(struct UpnpDeviceNode *);
int RcaCtrlPointRemoveDevice(const char *);
int RcaCtrlPointRemoveAll(void);
int RcaCtrlPointRefresh(void);

int RcaCtrlPointSendAction(int, int, const char *, const char **,
			   char **, int);
int RcaCtrlPointSendActionNumericArg(int devnum, int service,
				     const char *actionName,
				     const char *paramName, int paramValue);
int RcaCtrlPointSendPowerOn(int devnum);
int RcaCtrlPointSendPowerOff(int devnum);
int RcaCtrlPointSendSetModel(int, int);
int RcaCtrlPointSendSendKey(int, int);
int RcaCtrlPointGetVar(int, int, const char *);
int RcaCtrlPointGetDevice(int, struct UpnpDeviceNode **);
int RcaCtrlPointPrintList(void);
int RcaCtrlPointPrintDevice(int);

void RcaCtrlPointAddDevice(IXML_Document *, const char *, int); 
void RcaCtrlPointHandleGetVar(const char *, const char *, const DOMString);

void RcaStateUpdate(char *UDN, int Service, IXML_Document *ChangedVariables,
		    char **State);
void RcaCtrlPointHandleEvent(const char *, int, IXML_Document *); 
void RcaCtrlPointHandleSubscribeUpdate(const char *, const Upnp_SID, int); 
int RcaCtrlPointCallbackEventHandler(Upnp_EventType, void *, void *);

void RcaCtrlPointVerifyTimeouts(int incr);

void RcaCtrlPointPrintCommands(void);
void* RcaCtrlPointCommandLoop(void *);
int RcaCtrlPointStart(print_string printFunctionPtr,
		      state_update updateFunctionPtr);
int RcaCtrlPointStop(void);
int RcaCtrlPointProcessCommand(char *cmdline);

void RcaCtrlPointPrintShortHelp(void);
void RcaCtrlPointPrintLongHelp(void);
void RcaCtrlPointPrintCommands(void);

void* RcaCtrlPointCommandLoop(void *args);
int RcaCtrlPointProcessCommand(char *cmdline);

#ifdef __cplusplus
};
#endif

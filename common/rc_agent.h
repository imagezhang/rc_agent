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

#include <stdio.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "sample_util.h"

#include "ithread.h"
#include "upnp.h"

#include <stdlib.h>
#include <string.h>


#define POWER_ON 1
#define POWER_OFF 0

#define RCA_SERVICE_SERVCOUNT  1
#define RCA_SERVICE_CONTROL    0

#define RCA_CONTROL_VARCOUNT   3
#define RCA_CONTROL_POWER      0
#define RCA_CONTROL_MODEL      1
#define RCA_CONTROL_KEY        2

#define RCA_MODEL_BEGIN 0
#define RCA_MODEL_END 32

#define RCA_MIN_KEY 0
#define RCA_MAX_KEY 100

#define RCA_MAX_VAL_LEN 5

#define RCA_MAXACTIONS 5
#define RCA_MAXVARS RCA_CONTROL_VARCOUNT

/*!
 * \brief Prototype for all actions. For each action that a service 
 * implements, there is a corresponding function with this prototype.
 *
 * Pointers to these functions, along with action names, are stored
 * in the service table. When an action request comes in the action
 * name is matched, and the appropriate function is called.
 * Each function returns UPNP_E_SUCCESS, on success, and a nonzero 
 * error code on failure.
 */
typedef int (*upnp_action)(
	/*! [in] Document of action request. */
	IXML_Document *request,
	/*! [out] Action result. */
	IXML_Document **out,
	/*! [out] Error string in case action was unsuccessful. */
	const char **errorString);

struct RcaService {
	char UDN[NAME_SIZE];
	char ServiceId[NAME_SIZE];
	char ServiceType[NAME_SIZE];
	const char *VariableName[RCA_MAXVARS]; 
	char *VariableStrVal[RCA_MAXVARS];
	const char *ActionNames[RCA_MAXACTIONS];
	upnp_action actions[RCA_MAXACTIONS];
	int VariableCount;
};

extern struct RcaService rcaServTbl[];

/*! Device handle returned from sdk */
extern UpnpDevice_Handle deviceHandle;

/*! Mutex for protecting the global state table data
 * in a multi-threaded, asynchronous environment.
 * All functions should lock this mutex before reading
 * or writing the state table data. */
extern ithread_mutex_t rcaDevMutex;

int SetActionTable(int serviceType, struct RcaService *out);
int RcaDeviceStateTableInit(char *DescDocURL);

int RcaDeviceHandleSubscriptionRequest(
	struct Upnp_Subscription_Request *sr_event);
int RcaDeviceHandleGetVarRequest(struct Upnp_State_Var_Request *cgv_event);
int RcaDeviceHandleActionRequest(struct Upnp_Action_Request *ca_event);

int RcaDeviceCallbackEventHandler(Upnp_EventType, void *Event, void *Cookie);
int RcaDeviceSetServiceTableVar(unsigned int service, int variable,
				char *value);

int RcaDevicePowerOn(IXML_Document *in,	IXML_Document **out,
		     const char **errorString);
int RcaDevicePowerOff(IXML_Document *in, IXML_Document **out,
		     const char **errorString);

int RcaDeviceSetModel(IXML_Document *in, IXML_Document **out,
		      const char **errorString);

int RcaDeviceSendKey(IXML_Document *in, IXML_Document **out,
		     const char **errorString);

int RcaDeviceStart(
	/*! [in] ip address to initialize the sdk (may be NULL)
	 * if null, then the first non null loopback address is used. */
	char *ip_address,
	/*! [in] port number to initialize the sdk (may be 0)
	 * if zero, then a random number is used. */
	unsigned short port,
	/*! [in] name of description document.
	 * may be NULL. Default is tvdevicedesc.xml. */
	const char *desc_doc_name,
	/*! [in] path of web directory.
	 * may be NULL. Default is ./web (for Linux) or ../tvdevice/web. */
	const char *web_dir_path,
	/*! [in] print function to use. */
	print_string pfun);

int RcaDeviceStop(void);

void *RcaDeviceCommandLoop(void *args);

/*!
 * \brief Main entry point for tv device application.
 *
 * Initializes and registers with the sdk.
 * Initializes the state stables of the service.
 * Starts the command loop.
 *
 * Accepts the following optional arguments:
 *	\li \c -ip ipaddress
 *	\li \c -port port
 *	\li \c -desc desc_doc_name
 *	\li \c -webdir web_dir_path
 *	\li \c -help
 */
int device_main(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

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

#include <assert.h>
#include "rc_agent.h"

#define DEFAULT_WEB_DIR "./web"
#define DESC_URL_SIZE 200

//============================================================================
const char *RcaServiceType[] = {
	"urn:ubicomp.co:service:rcacontrol:1",
};

const char *rcaCtrlVarName[] = { "Power", "Model", "Key" };

char rcaCtrlVarVal[RCA_CONTROL_VARCOUNT][RCA_MAX_VAL_LEN];
const char *rcaCtrlVarValDef[] = { "1", "1", "1" };

int dfltAdvrExpire = 100;

struct RcaService rcaServTbl[1];

UpnpDevice_Handle deviceHandle = -1;

// Mutex for the global state table data
ithread_mutex_t rcaDevMutex;

static int SetServiceTable(
	int serviceType,
	const char *UDN,
	const char *serviceId,
	const char *serviceTypeS,
	struct RcaService *out)
{
	int i = 0;

	strcpy(out->UDN, UDN);
	strcpy(out->ServiceId, serviceId);
	strcpy(out->ServiceType, serviceTypeS);
	
	switch (serviceType) {
	case RCA_SERVICE_CONTROL:
		out->VariableCount = RCA_CONTROL_VARCOUNT;
		for (i = 0;
		     i < rcaServTbl[RCA_SERVICE_CONTROL].VariableCount;
		     i++) {
			rcaServTbl[RCA_SERVICE_CONTROL].VariableName[i]
				= rcaCtrlVarName[i];
			rcaServTbl[RCA_SERVICE_CONTROL].VariableStrVal[i]
				= rcaCtrlVarVal[i];
			strcpy(rcaServTbl[RCA_SERVICE_CONTROL].
			       VariableStrVal[i], rcaCtrlVarValDef[i]);
		}
		break;

	default:
		assert(0);
	}
	
	return SetActionTable(serviceType, out);
}

int SetActionTable(int serviceType, struct RcaService *out)
{
	if (serviceType == RCA_SERVICE_CONTROL) {
		out->ActionNames[0] = "PowerOn";
		out->actions[0] = RcaDevicePowerOn;
		out->ActionNames[1] = "PowerOff";
		out->actions[1] = RcaDevicePowerOff;
		out->ActionNames[2] = "SetModel";
		out->actions[2] = RcaDeviceSetModel;
		out->ActionNames[3] = "SendKey";
		out->actions[3] = RcaDeviceSendKey;
		out->ActionNames[4] = NULL;
		return 1;
	}

	return 0;
}

int RcaDeviceStateTableInit(char *DescDocURL)
{
	IXML_Document *DescDoc = NULL;
	int ret = UPNP_E_SUCCESS;
	char *servid_ctrl = NULL;
	char *evnturl_ctrl = NULL;
	char *ctrlurl_ctrl = NULL;
	char *servid_pict = NULL;
	char *evnturl_pict = NULL;
	char *ctrlurl_pict = NULL;
	char *udn = NULL;

	/*Download description document */
	if (UpnpDownloadXmlDoc(DescDocURL, &DescDoc) != UPNP_E_SUCCESS) {
		SampleUtil_Print("RcaDeviceStateTableInit -- Error Parsing %s\n",
				 DescDocURL);
		ret = UPNP_E_INVALID_DESC;
		goto error_handler;
	}
	udn = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");
	/* Find the Rca Control Service identifiers */
	if (!SampleUtil_FindAndParseService(DescDoc, DescDocURL,
					    RcaServiceType[RCA_SERVICE_CONTROL],
					    &servid_ctrl, &evnturl_ctrl,
					    &ctrlurl_ctrl)) {
		SampleUtil_Print("RcaDeviceStateTableInit -- Error: Could not find Service: %s\n",
				 RcaServiceType[RCA_SERVICE_CONTROL]);
		ret = UPNP_E_INVALID_DESC;
		goto error_handler;
	}
	/* set control service table */
	SetServiceTable(RCA_SERVICE_CONTROL, udn, servid_ctrl,
			RcaServiceType[RCA_SERVICE_CONTROL],
			&rcaServTbl[RCA_SERVICE_CONTROL]);

error_handler:
	/* clean up */
	if (udn)
		free(udn);
	if (servid_ctrl)
		free(servid_ctrl);
	if (evnturl_ctrl)
		free(evnturl_ctrl);
	if (ctrlurl_ctrl)
		free(ctrlurl_ctrl);
	if (servid_pict)
		free(servid_pict);
	if (evnturl_pict)
		free(evnturl_pict);
	if (ctrlurl_pict)
		free(ctrlurl_pict);
	if (DescDoc)
		ixmlDocument_free(DescDoc);

	return (ret);
}

int RcaDeviceHandleSubscriptionRequest(
	struct Upnp_Subscription_Request *sr_event)
{
	unsigned int i = 0;
	int cmp1 = 0;
	int cmp2 = 0;
	const char *l_serviceId = NULL;
	const char *l_udn = NULL;
	const char *l_sid = NULL;

	/* lock state mutex */
	ithread_mutex_lock(&rcaDevMutex);

	l_serviceId = sr_event->ServiceId;
	l_udn = sr_event->UDN;
	l_sid = sr_event->Sid;
	for (i = 0; i < RCA_SERVICE_SERVCOUNT; ++i) {
		cmp1 = strcmp(l_udn, rcaServTbl[i].UDN);
		cmp2 = strcmp(l_serviceId, rcaServTbl[i].ServiceId);
		if (cmp1 == 0 && cmp2 == 0) {
			UpnpAcceptSubscription(deviceHandle,
					       l_udn,
					       l_serviceId,
					       (const char **)
					       rcaServTbl[i].VariableName,
					       (const char **)
					       rcaServTbl[i].VariableStrVal,
					       rcaServTbl[i].VariableCount,
					       l_sid);
		}
	}

	ithread_mutex_unlock(&rcaDevMutex);

	return 1;
}

// Note:
// UPnP device architecture v1.1 -- 3.3
// The QueryStateVariable action has been deprecated by the UPnP Forum.
// Working committees and vendors MUST explicitly define actions for 
// querying of state variables for which this capability is desired.
int RcaDeviceHandleGetVarRequest(struct Upnp_State_Var_Request *cgv_event)
{
	unsigned int i = 0;
	int j = 0;
	int getvar_succeeded = 0;
	int cmp0;

	cgv_event->CurrentVal = NULL;

	ithread_mutex_lock(&rcaDevMutex);

	for (i = 0; i < RCA_SERVICE_SERVCOUNT; i++) {
		/* check udn and service id */
		const char *devUDN = cgv_event->DevUDN;
		const char *serviceID = cgv_event->ServiceID;
		if (strcmp(devUDN, rcaServTbl[i].UDN) == 0 &&
		    strcmp(serviceID, rcaServTbl[i].ServiceId) == 0) {
			/* check variable name */
			for (j = 0; j < rcaServTbl[i].VariableCount; j++) {
				const char *stateVarName =
					cgv_event->StateVarName;
				cmp0 = strcmp(stateVarName,
					      rcaServTbl[i].VariableName[j]);
				if (cmp0 == 0) {
					getvar_succeeded = 1;
					cgv_event->CurrentVal = 
						ixmlCloneDOMString(
							rcaServTbl[i].
							VariableStrVal[j]);
					break;
				}
			}
		}
	}
	if (getvar_succeeded) {
		cgv_event->ErrCode = UPNP_E_SUCCESS;
	} else {
		SampleUtil_Print("Error in UPNP_CONTROL_GET_VAR_REQUEST "
				 "callback:\n   Unknown variable name = %s\n",
				 cgv_event->StateVarName);
		cgv_event->ErrCode = 404;
		strcpy(cgv_event->ErrStr, "Invalid Variable");
	}

	ithread_mutex_unlock(&rcaDevMutex);

	return cgv_event->ErrCode == UPNP_E_SUCCESS;
}

int RcaDeviceHandleActionRequest(struct Upnp_Action_Request *ca_event)
{
	/* Defaults if action not found. */
	int action_found = 0;
	int i = 0;
	int service = -1;
	int retCode = 0;
	const char *errorString = NULL;
	const char *devUDN = NULL;
	const char *serviceID = NULL;
	const char *actionName = NULL;

	ca_event->ErrCode = 0;
	ca_event->ActionResult = NULL;

	devUDN     = ca_event->DevUDN;
	serviceID  = ca_event->ServiceID;
	actionName = ca_event->ActionName;

	if (strcmp(devUDN, rcaServTbl[RCA_SERVICE_CONTROL].UDN) == 0 &&
	    strcmp(serviceID,
		   rcaServTbl[RCA_SERVICE_CONTROL].ServiceId) == 0) {
		service = RCA_SERVICE_CONTROL;
	} else {
		SampleUtil_Print("Unexpected devUDN %s or ServiceId %s\n",
				 devUDN, serviceID);
	}

	for (i = 0;
	     i < RCA_MAXACTIONS && rcaServTbl[service].ActionNames[i] != NULL;
	     i++) {
		if (!strcmp(actionName, rcaServTbl[service].ActionNames[i])) {
			if (!strcmp(rcaServTbl[RCA_SERVICE_CONTROL].
				    VariableStrVal[RCA_CONTROL_POWER], "1") ||
			    !strcmp(actionName, "PowerOn")) {
				retCode = rcaServTbl[service].actions[i](
					ca_event->ActionRequest,
					&ca_event->ActionResult,
					&errorString);
			} else {
				errorString = "Power is Off";
				retCode = UPNP_E_INTERNAL_ERROR;
			}
			action_found = 1;
			break;
		}
	}

	if (!action_found) {
		ca_event->ActionResult = NULL;
		strcpy(ca_event->ErrStr, "Invalid Action");
		ca_event->ErrCode = 401;
	} else {
		if (retCode == UPNP_E_SUCCESS) {
			ca_event->ErrCode = UPNP_E_SUCCESS;
		} else {
			strcpy(ca_event->ErrStr, errorString);
			switch (retCode) {
			case UPNP_E_INVALID_PARAM:
				ca_event->ErrCode = 402;
				break;
			case UPNP_E_INTERNAL_ERROR:
			default:
				ca_event->ErrCode = 501;
				break;
			}
		}
	}
	
	return ca_event->ErrCode;
}

int RcaDeviceSetServiceTableVar(unsigned int service, int variable,
				char *value)
{
	/* IXML_Document  *PropSet= NULL; */
	if (service >= RCA_SERVICE_SERVCOUNT ||
	    variable >= rcaServTbl[service].VariableCount ||
	    strlen(value) >= RCA_MAX_VAL_LEN)
		return (0);

	ithread_mutex_lock(&rcaDevMutex);

	strcpy(rcaServTbl[service].VariableStrVal[variable], value);

	UpnpNotify(
		deviceHandle, rcaServTbl[service].UDN,
		rcaServTbl[service].ServiceId,
		(const char **)&rcaServTbl[service].VariableName[variable],
		(const char **)&rcaServTbl[service].
		VariableStrVal[variable],
		1);
	
	ithread_mutex_unlock(&rcaDevMutex);

	return 1;
}

static int RcaDeviceSetPower(
	/*! [in] If 1, turn power on. If 0, turn power off. */
	int on)
{
	char value[RCA_MAX_VAL_LEN];
	int ret = 0;

	if (on != POWER_ON && on != POWER_OFF) {
		SampleUtil_Print("error: can't set power to value %d\n", on);
		return 0;
	}

	/* Vendor-specific code to turn the power on/off goes here. */
	SampleUtil_Print("USER DEFINED PROCESS : Rca Power set to %d\n", on);

	sprintf(value, "%d", on);
	ret = RcaDeviceSetServiceTableVar(RCA_SERVICE_CONTROL,
					  RCA_CONTROL_POWER,
					  value);

	return ret;
}

int RcaDevicePowerOn(IXML_Document * in,IXML_Document **out,
		     const char **errorString)
{
	(*out) = NULL;
	(*errorString) = NULL;

	if (RcaDeviceSetPower(POWER_ON)) {
		if (UpnpAddToActionResponse(
			    out, "PowerOn",
			    RcaServiceType[RCA_SERVICE_CONTROL],
			    "Power", "1") != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int RcaDevicePowerOff(IXML_Document *in, IXML_Document **out,
		      const char **errorString)
{
	(*out) = NULL;
	(*errorString) = NULL;
	if (RcaDeviceSetPower(POWER_OFF)) {
		if (UpnpAddToActionResponse(
			    out, "PowerOff",
			    RcaServiceType[RCA_SERVICE_CONTROL],
			    "Power", "0") != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	}
	(*errorString) = "Internal Error";
	return UPNP_E_INTERNAL_ERROR;
	in = in;
}

int RcaDeviceSetModel(IXML_Document * in, IXML_Document ** out,
		      const char **errorString)
{
	char *value = NULL;
	int model = 0;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Model"))) {
		(*errorString) = "Invalid model";
		return UPNP_E_INVALID_PARAM;
	}
	model = atoi(value);
	if (model < RCA_MODEL_BEGIN || model > RCA_MODEL_END) {
		free(value);
		SampleUtil_Print("error: can't change to model %d\n",
				 model);
		(*errorString) = "Invalid Model";
		return UPNP_E_INVALID_PARAM;
	}

	SampleUtil_Print("USER DEFINED PROCESS : Change Model to %d\n",
			 model);

	if (RcaDeviceSetServiceTableVar(RCA_SERVICE_CONTROL,
					RCA_CONTROL_MODEL, value)) {
		if (UpnpAddToActionResponse(
			    out, "SetModel",
			    RcaServiceType[RCA_SERVICE_CONTROL],
			    "NewModel",
			    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

int RcaDeviceSendKey(IXML_Document * in, IXML_Document ** out,
		      const char **errorString)
{
	char *value = NULL;
	int key = 0;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Key"))) {
		(*errorString) = "Invalid key";
		return UPNP_E_INVALID_PARAM;
	}
	key = atoi(value);
	if (key < RCA_MIN_KEY || key > RCA_MAX_KEY) {
		free(value);
		SampleUtil_Print("error: can't send key %d\n",
				 key);
		(*errorString) = "Invalid key";
		return UPNP_E_INVALID_PARAM;
	}

	SampleUtil_Print("USER DEFINED PROCESS : Send key %d Model %s\n",
			 key, rcaServTbl[RCA_SERVICE_CONTROL].
			 VariableStrVal[RCA_CONTROL_MODEL]);

	if (RcaDeviceSetServiceTableVar(RCA_SERVICE_CONTROL,
					RCA_CONTROL_KEY, value)) {
		if (UpnpAddToActionResponse(
			    out, "SendKey",
			    RcaServiceType[RCA_SERVICE_CONTROL],
			    "NewKey",
			    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

int RcaDeviceCallbackEventHandler(Upnp_EventType EventType, void *Event,
				  void *Cookie)
{
	switch (EventType) {
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		RcaDeviceHandleSubscriptionRequest(
			(struct Upnp_Subscription_Request *) Event);
		break;
		
	case UPNP_CONTROL_ACTION_REQUEST:
		RcaDeviceHandleActionRequest(
			(struct Upnp_Action_Request *) Event);
		break;

	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	case UPNP_DISCOVERY_SEARCH_RESULT:
	case UPNP_DISCOVERY_SEARCH_TIMEOUT:
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
	case UPNP_CONTROL_ACTION_COMPLETE:
	case UPNP_CONTROL_GET_VAR_COMPLETE:
	case UPNP_EVENT_RECEIVED:
	case UPNP_EVENT_RENEWAL_COMPLETE:
	case UPNP_EVENT_SUBSCRIBE_COMPLETE:
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		break;
	default:
		SampleUtil_Print(
			"Error in RcaDeviceCallbackEventHandler: "
			"unknown event type %d\n",
			EventType);
	}
	/* Print a summary of the event received */
	SampleUtil_PrintEvent(EventType, Event);

	return 0;
	Cookie = Cookie;
}

int RcaDeviceStart(char *ip_address, unsigned short port,
		   const char *desc_doc_name, const char *web_dir_path,
		   print_string pfun)
{
	int ret = UPNP_E_SUCCESS;
	char desc_doc_url[DESC_URL_SIZE];
	
	ithread_mutex_init(&rcaDevMutex, NULL);

	SampleUtil_Initialize(pfun);
	SampleUtil_Print("Initializing UPnP Sdk with\n"
			 "\tipaddress = %s port = %u\n",
			 ip_address ? ip_address : "{NULL}", port);
	ret = UpnpInit(ip_address, port);
	if (ret != UPNP_E_SUCCESS) {
		SampleUtil_Print("Error with UpnpInit -- %d\n", ret);
		UpnpFinish();

		return ret;
	}
	ip_address = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	SampleUtil_Print("UPnP Initialized\n"
			 "\tipaddress = %s port = %u\n",
			 ip_address ? ip_address : "{NULL}", port);
	if (!desc_doc_name) {
		desc_doc_name = "rcadevicedesc.xml";
	}
	if (!web_dir_path) {
		web_dir_path = DEFAULT_WEB_DIR;
	}
	snprintf(desc_doc_url, DESC_URL_SIZE, "http://%s:%d/%s", ip_address,
		 port, desc_doc_name);
	SampleUtil_Print("Specifying the webserver root directory -- %s\n",
			 web_dir_path);
	ret = UpnpSetWebServerRootDir(web_dir_path);
	if (ret != UPNP_E_SUCCESS) {
		SampleUtil_Print
		    ("Error specifying webserver root directory -- %s: %d\n",
		     web_dir_path, ret);
		UpnpFinish();
		
		return ret;
	}
	SampleUtil_Print("Registering the RootDevice\n"
			 "\t with desc_doc_url: %s\n", desc_doc_url);
	ret = UpnpRegisterRootDevice(desc_doc_url,
				     RcaDeviceCallbackEventHandler,
				     &deviceHandle, &deviceHandle);
	if (ret != UPNP_E_SUCCESS) {
		SampleUtil_Print("Error registering the rootdevice : %d\n",
				 ret);
		UpnpFinish();
		
		return ret;
	} else {
		SampleUtil_Print("RootDevice Registered\n"
				 "Initializing State Table\n");
		RcaDeviceStateTableInit(desc_doc_url);
		SampleUtil_Print("State Table Initialized\n");
		ret = UpnpSendAdvertisement(deviceHandle, dfltAdvrExpire);
		if (ret != UPNP_E_SUCCESS) {
			SampleUtil_Print("Error sending advertisements : %d\n",
					 ret);
			UpnpFinish();

			return ret;
		}
		SampleUtil_Print("Advertisements Sent\n");
	}

	return UPNP_E_SUCCESS;
}

int RcaDeviceStop(void)
{
	UpnpUnRegisterRootDevice(deviceHandle);
	UpnpFinish();
	SampleUtil_Finish();
	ithread_mutex_destroy(&rcaDevMutex);

	return UPNP_E_SUCCESS;
}

void *RcaDeviceCommandLoop(void *args)
{
	int stoploop = 0;
	char cmdline[100];
	char cmd[100];
	
	while (!stoploop) {
		sprintf(cmdline, " ");
		sprintf(cmd, " ");
		SampleUtil_Print("\n>> ");
		/* Get a command line */
		fgets(cmdline, 100, stdin);
		sscanf(cmdline, "%s", cmd);
		if (strcasecmp(cmd, "exit") == 0) {
			SampleUtil_Print("Shutting down...\n");
			RcaDeviceStop();
			exit(0);
		} else {
			SampleUtil_Print("\n   Unknown command: %s\n\n", cmd);
			SampleUtil_Print("   Valid Commands:\n"
					 "     Exit\n\n");
		}
	}

	return NULL;
	args = args;
}

int device_main(int argc, char *argv[])
{
	unsigned int portTemp = 0;
	char *ip_address = NULL;
	char *desc_doc_name = NULL;
	char *web_dir_path = NULL;
	unsigned short port = 0;
	int i = 0;

	SampleUtil_Initialize(linux_print);
	/* Parse options */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-ip") == 0) {
			ip_address = argv[++i];
		} else if (strcmp(argv[i], "-port") == 0) {
			sscanf(argv[++i], "%u", &portTemp);
		} else if (strcmp(argv[i], "-desc") == 0) {
			desc_doc_name = argv[++i];
		} else if (strcmp(argv[i], "-webdir") == 0) {
			web_dir_path = argv[++i];
		} else if (strcmp(argv[i], "-help") == 0) {
			SampleUtil_Print("Usage: %s -ip ipaddress -port port"
					 " -desc desc_doc_name -webdir web_dir_path"
					 " -help (this message)\n", argv[0]);
			SampleUtil_Print
			    ("\tipaddress:     IP address of the device"
			     " (must match desc. doc)\n"
			     "\t\te.g.: 192.168.0.4\n"
			     "\tport:          Port number to use for"
			     " receiving UPnP messages (must match desc. doc)\n"
			     "\t\te.g.: 5431\n"
			     "\tdesc_doc_name: name of device description document\n"
			     "\t\te.g.: tvdevicedesc.xml\n"
			     "\tweb_dir_path: Filesystem path where web files"
			     " related to the device are stored\n"
			     "\t\te.g.: /upnp/sample/tvdevice/web\n");
			return 1;
		}
	}
	port = (unsigned short)portTemp;
	return RcaDeviceStart(ip_address, port, desc_doc_name, web_dir_path,
			      linux_print);
}

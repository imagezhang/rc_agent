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

#include "rca_ctrlpt.h"
#include "upnp.h"

//============================================================================
ithread_mutex_t DeviceListMutex;

UpnpClient_Handle ctrlptHandle = -1;

const char *rcaDeviceType = "urn:ubicomp.com.au:device:rcadevice:1";
const char *rcaServiceType[] = {"urn:ubicomp.com.au:service:rcacontrol:1"};
const char *rcaServiceName[] = {"Control"};
const char *rcaVarName[RCA_SERVICE_SERVCOUNT][RCA_MAXVARS] = {
	{"Power", "Model", "Key"}
};

char rcaVarCount[RCA_SERVICE_SERVCOUNT] = {RCA_CONTROL_VARCOUNT};

const char *upnpServiceType[] = {"urn:ubicomp.com.au:service:rcacontrol:1"};
const char *upnpServiceName[] = {"Control"};
char upnpVarCount[UPNP_SERVICE_SERVCOUNT] = {UPNP_CONTROL_VARCOUNT};
const char *upnpVarName[UPNP_SERVICE_SERVCOUNT][UPNP_MAXVARS] = {
	{"Power", "Model", "Key"}
};

int defaultTimeout = 1801;

struct UpnpDeviceNode *GlobalDeviceList = NULL;

//============================================================================

int UpnpCtrlPointDeleteNode(struct UpnpDeviceNode *node)
{
	int rc, service, var;

	if (NULL == node) {
		SampleUtil_Print("ERROR: %s: Node is empty\n", __FUNCTION__);
		return CTRLPT_ERROR;
	}

	for (service = 0; service < UPNP_SERVICE_SERVCOUNT; service++) {
		if (strcmp(node->device.UpnpService[service].SID, "") != 0) {
			rc = UpnpUnSubscribe(ctrlptHandle,
					     node->device.UpnpService[service].
					     SID);
			if (UPNP_E_SUCCESS == rc) {
				SampleUtil_Print
				    ("Unsubscribed from %s EventURL with "
				     "SID=%s\n", upnpServiceName[service],
				     node->device.UpnpService[service].SID);
			} else {
				SampleUtil_Print
				    ("Error unsubscribing to %s EventURL "
				     "-- %d\n", upnpServiceName[service], rc);
			}
		}
		
		for (var = 0; var < upnpVarCount[service]; var++) {
			if (node->device.UpnpService[service].
			    VariableStrVal[var]) {
				free(node->device.UpnpService[service].
				     VariableStrVal[var]);
			}
		}
	}

	SampleUtil_StateUpdate(NULL, NULL, node->device.UDN, DEVICE_REMOVED);
	free(node);
	node = NULL;

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointRemoveDevice(const char *UDN)
{
	struct UpnpDeviceNode *curdevnode;
	struct UpnpDeviceNode *prevdevnode;

	ithread_mutex_lock(&DeviceListMutex);

	curdevnode = GlobalDeviceList;
	if (!curdevnode) {
		SampleUtil_Print(
			"WARNING: %s: Device list empty\n", __FUNCTION__);
	} else {
		if (0 == strcmp(curdevnode->device.UDN, UDN)) {
			GlobalDeviceList = curdevnode->next;
			UpnpCtrlPointDeleteNode(curdevnode);
		} else {
			prevdevnode = curdevnode;
			curdevnode = curdevnode->next;
			while (curdevnode) {
				if (strcmp(curdevnode->device.UDN, UDN) == 0) {
					prevdevnode->next = curdevnode->next;
					UpnpCtrlPointDeleteNode(curdevnode);
					break;
				}
				prevdevnode = curdevnode;
				curdevnode = curdevnode->next;
			}
		}
	}

	ithread_mutex_unlock(&DeviceListMutex);

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointRemoveAll(void)
{
	struct UpnpDeviceNode *curdevnode, *next;

	ithread_mutex_lock(&DeviceListMutex);

	curdevnode = GlobalDeviceList;
	GlobalDeviceList = NULL;

	while (curdevnode) {
		next = curdevnode->next;
		UpnpCtrlPointDeleteNode(curdevnode);
		curdevnode = next;
	}

	ithread_mutex_unlock(&DeviceListMutex);

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointRefresh(void)
{
	int rc;

	UpnpCtrlPointRemoveAll();

	rc = UpnpSearchAsync(ctrlptHandle, 5, rcaDeviceType, NULL);
	if (UPNP_E_SUCCESS != rc) {
		SampleUtil_Print("Error sending search request%d\n", rc);

		return CTRLPT_ERROR;
	}

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointGetDevice(int devnum, struct UpnpDeviceNode **devnode)
{
	int count = devnum;
	struct UpnpDeviceNode *tmpdevnode = NULL;

	if (count)
		tmpdevnode = GlobalDeviceList;
	while (--count && tmpdevnode) {
		tmpdevnode = tmpdevnode->next;
	}
	if (!tmpdevnode) {
		SampleUtil_Print("Error finding RcaDevice number -- %d\n",
				 devnum);
		return CTRLPT_ERROR;
	}
	*devnode = tmpdevnode;

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointGetVar(int service, int devnum, const char *varname)
{
	struct UpnpDeviceNode *devnode;
	int rc;

	ithread_mutex_lock(&DeviceListMutex);

	rc = UpnpCtrlPointGetDevice(devnum, &devnode);

	if (CTRLPT_SUCCESS == rc) {
		rc = UpnpGetServiceVarStatusAsync(
			ctrlptHandle,
			devnode->device.UpnpService[service].ControlURL,
			varname,
			RcaCtrlPointCallbackEventHandler,
			NULL);
		if (rc != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error in UpnpGetServiceVarStatusAsync "
				"-- %d\n", rc);
			rc = CTRLPT_ERROR;
		}
	}

	ithread_mutex_unlock(&DeviceListMutex);

	return rc;
}

int UpnpCtrlPointPrintList(void)
{
	struct UpnpDeviceNode *tmpdevnode;
	int i = 0;

	ithread_mutex_lock(&DeviceListMutex);

	SampleUtil_Print("%s:\n", __FUNCTION__);
	tmpdevnode = GlobalDeviceList;
	while (tmpdevnode) {
		SampleUtil_Print(" %3d -- %s\n", ++i, tmpdevnode->device.UDN);
		tmpdevnode = tmpdevnode->next;
	}
	SampleUtil_Print("\n");
	ithread_mutex_unlock(&DeviceListMutex);

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointPrintDevice(int devnum)
{
	struct UpnpDeviceNode *tmpdevnode;
	int i = 0, service, var;
	char spacer[15];

	if (devnum <= 0) {
		SampleUtil_Print(
			"Error in %s: invalid devnum = %d\n", __FUNCTION__,
			devnum);
		return CTRLPT_ERROR;
	}

	ithread_mutex_lock(&DeviceListMutex);

	SampleUtil_Print("%s:\n", __FUNCTION__);
	tmpdevnode = GlobalDeviceList;
	while (tmpdevnode) {
		i++;
		if (i == devnum)
			break;
		tmpdevnode = tmpdevnode->next;
	}
	if (!tmpdevnode) {
		SampleUtil_Print(
			"Error in %s: "
			"invalid devnum = %d  --  actual device count = %d\n",
			__FUNCTION__, devnum, i);
	} else {
		SampleUtil_Print(
			"  RcaDevice -- %d\n"
			"    |                  \n"
			"    +- UDN        = %s\n"
			"    +- DescDocURL     = %s\n"
			"    +- FriendlyName   = %s\n"
			"    +- PresURL        = %s\n"
			"    +- Adver. TimeOut = %d\n",
			devnum,
			tmpdevnode->device.UDN,
			tmpdevnode->device.DescDocURL,
			tmpdevnode->device.FriendlyName,
			tmpdevnode->device.PresURL,
			tmpdevnode->device.AdvrTimeOut);
		service = 0;
		for (; service < UPNP_SERVICE_SERVCOUNT; service++) {
			if (service < RCA_SERVICE_SERVCOUNT - 1)
				sprintf(spacer, "    |    ");
			else
				sprintf(spacer, "         ");
			SampleUtil_Print(
				"    |                  \n"
				"    +- Rca %s Service\n"
				"%s+- ServiceId       = %s\n"
				"%s+- ServiceType     = %s\n"
				"%s+- EventURL        = %s\n"
				"%s+- ControlURL      = %s\n"
				"%s+- SID             = %s\n"
				"%s+- ServiceStateTable\n",
				rcaServiceName[service], spacer,
				tmpdevnode->device.UpnpService[service].
				ServiceId, spacer,
				tmpdevnode->device.UpnpService[service].
				ServiceType, spacer,
				tmpdevnode->device.UpnpService[service].
				EventURL, spacer,
				tmpdevnode->device.UpnpService[service].
				ControlURL, spacer,
				tmpdevnode->device.UpnpService[service].
				SID, spacer);
			for (var = 0; var < upnpVarCount[service]; var++) {
				SampleUtil_Print(
					"%s     +- %-10s = %s\n",
					spacer,
					upnpVarName[service][var],
					tmpdevnode->device.
					UpnpService[service].
					VariableStrVal[var]);
			}
		}
	}
	SampleUtil_Print("\n");
	ithread_mutex_unlock(&DeviceListMutex);

	return CTRLPT_SUCCESS;
}

int UpnpCtrlPointSendAction(
	int service,
	int devnum,
	const char *actionname,
	const char **param_name,
	char **param_val,
	int param_count)
{
	struct UpnpDeviceNode *devnode;
	IXML_Document *actionNode = NULL;
	int rc = CTRLPT_SUCCESS;
	int param;

	ithread_mutex_lock(&DeviceListMutex);

	rc = UpnpCtrlPointGetDevice(devnum, &devnode);
	if (CTRLPT_SUCCESS == rc) {
		if (0 == param_count) {
			actionNode =
				UpnpMakeAction(actionname,
					       rcaServiceType[service],
					       0, NULL);
		} else {
			for (param = 0; param < param_count; param++) {
				if (UpnpAddToAction
				    (&actionNode, actionname,
				     upnpServiceType[service],
				     param_name[param],
				     param_val[param]) != UPNP_E_SUCCESS) {
					SampleUtil_Print(
						"ERROR: %s: Trying to add "
						"action param\n",
						__FUNCTION__);
				}
			}
		}
		
		rc = UpnpSendActionAsync(ctrlptHandle,
					 devnode->device.
					 UpnpService[service].ControlURL,
					 upnpServiceType[service], NULL,
					 actionNode,
					 RcaCtrlPointCallbackEventHandler, NULL);

		if (rc != UPNP_E_SUCCESS) {
			SampleUtil_Print("Error in UpnpSendActionAsync -- "
					 "%d\n", rc);
			rc = CTRLPT_ERROR;
		}
	}

	ithread_mutex_unlock(&DeviceListMutex);

	if (actionNode) {
		ixmlDocument_free(actionNode);
	}

	return rc;
}

int UpnpCtrlPointSendActionNumericArg(int devnum, int service,
				     const char *actionName,
				     const char *paramName, int paramValue)
{
	char param_val_a[50];
	char *param_val = param_val_a;

	sprintf(param_val_a, "%d", paramValue);
	return UpnpCtrlPointSendAction(
		service, devnum, actionName, &paramName,
		&param_val, 1);
}

void UpnpCtrlPointAddDevice(
	IXML_Document *DescDoc,
	const char *location,
	int expires)
{
	char *deviceType = NULL;
	char *friendlyName = NULL;
	char presURL[200];
	char *baseURL = NULL;
	char *relURL = NULL;
	char *UDN = NULL;
	char *serviceId[UPNP_SERVICE_SERVCOUNT] = {NULL};
	char *eventURL[UPNP_SERVICE_SERVCOUNT] = {NULL};
	char *controlURL[UPNP_SERVICE_SERVCOUNT] = {NULL};
	Upnp_SID eventSID[UPNP_SERVICE_SERVCOUNT];
	int TimeOut[UPNP_SERVICE_SERVCOUNT] = {defaultTimeout};
	struct UpnpDeviceNode *devNode;
	struct UpnpDeviceNode *tmpdevnode;
	int ret = 1;
	int found = 0;
	int service;
	int var;

	ithread_mutex_lock(&DeviceListMutex);

	/* Read key elements from description document */
	UDN = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");
	deviceType = SampleUtil_GetFirstDocumentItem(DescDoc, "deviceType");
	friendlyName = SampleUtil_GetFirstDocumentItem(DescDoc,
						       "friendlyName");
	relURL = SampleUtil_GetFirstDocumentItem(DescDoc, "presentationURL");

	ret = UpnpResolveURL((baseURL ? baseURL : location), relURL, presURL);

	if (UPNP_E_SUCCESS != ret)
		SampleUtil_Print("Error generating presURL from %s + %s\n",
				 baseURL, relURL);

	if (strcmp(deviceType, rcaDeviceType) == 0) {
		SampleUtil_Print("Found target device\n");

		/* Check if this device is already in the list */
		tmpdevnode = GlobalDeviceList;
		while (tmpdevnode) {
			if (strcmp(tmpdevnode->device.UDN, UDN) == 0) {
				found = 1;
				break;
			}
			tmpdevnode = tmpdevnode->next;
		}

		if (found) {
			/* The device is already there, so just update  */
			/* the advertisement timeout field */
			tmpdevnode->device.AdvrTimeOut = expires;
		} else {
			for (service = 0; service < UPNP_SERVICE_SERVCOUNT;
			     service++) {
				if (SampleUtil_FindAndParseService
				    (DescDoc, location,
				     rcaServiceType[service],
				     &serviceId[service], &eventURL[service],
				     &controlURL[service])) {
					SampleUtil_Print
						("Subscribing to EventURL "
						 "%s...\n", 
						 eventURL[service]);
					ret = UpnpSubscribe(
						ctrlptHandle,
						eventURL[service],
						&TimeOut[service],
						eventSID[service]);
					if (ret == UPNP_E_SUCCESS) {
						SampleUtil_Print
							("Subscribed to "
							 "EventURL with "
							 "SID=%s\n",
							 eventSID[service]);
					} else {
						SampleUtil_Print
							("Error Subscribing "
							 "to EventURL -- "
							 "%d\n", ret);
						strcpy(eventSID[service], "");
					}
				} else {
					SampleUtil_Print
						("Error: Could not find "
						 "Service: %s\n",
						 rcaServiceType[service]);
				}
			}
			
			/* Create a new device node */
			devNode = (struct UpnpDeviceNode *)
				malloc(sizeof(struct UpnpDeviceNode));
			strcpy(devNode->device.UDN, UDN);
			strcpy(devNode->device.DescDocURL, location);
			strcpy(devNode->device.FriendlyName, friendlyName);
			strcpy(devNode->device.PresURL, presURL);
			devNode->device.AdvrTimeOut = expires;
			for (service = 0; service < UPNP_SERVICE_SERVCOUNT;
			     service++) {
				strcpy(devNode->device.UpnpService[service].
				       ServiceId, serviceId[service]);
				strcpy(devNode->device.UpnpService[service].
				       ServiceType, rcaServiceType[service]);
				strcpy(devNode->device.UpnpService[service].
				       ControlURL, controlURL[service]);
				strcpy(devNode->device.UpnpService[service].
				       EventURL, eventURL[service]);
				strcpy(devNode->device.UpnpService[service].
				       SID, eventSID[service]);
				for (var = 0; var < rcaVarCount[service];
				     var++) {
					devNode->device.
						UpnpService[service].
						VariableStrVal[var] =
						(char *)malloc(
							UPNP_MAX_VAL_LEN);
					strcpy(devNode->device.
					       UpnpService[service].
					       VariableStrVal[var], "");
				}
			}
			devNode->next = NULL;
			/* Insert the new device node in the list */
			if ((tmpdevnode = GlobalDeviceList)) {
				while (tmpdevnode) {
					if (tmpdevnode->next) {
						tmpdevnode = tmpdevnode->next;
					} else {
						tmpdevnode->next = devNode;
						break;
					}
				}
			} else {
				GlobalDeviceList = devNode;
			}
			/*Notify New Device Added */
			SampleUtil_StateUpdate(NULL, NULL,
					       devNode->device.UDN,
					       DEVICE_ADDED);
		}
	}

	ithread_mutex_unlock(&DeviceListMutex);

	if (deviceType)
		free(deviceType);
	if (friendlyName)
		free(friendlyName);
	if (UDN)
		free(UDN);
	if (baseURL)
		free(baseURL);
	if (relURL)
		free(relURL);
	for (service = 0; service < UPNP_SERVICE_SERVCOUNT; service++) {
		if (serviceId[service])
			free(serviceId[service]);
		if (controlURL[service])
			free(controlURL[service]);
		if (eventURL[service])
			free(eventURL[service]);
	}
}

int RcaCtrlPointGetPower(int devnum)
{
	return UpnpCtrlPointGetVar(RCA_SERVICE_CONTROL, devnum, "Power");
}

int RcaCtrlPointGetModel(int devnum)
{
	return UpnpCtrlPointGetVar(RCA_SERVICE_CONTROL, devnum, "Model");
}

int RcaCtrlPointGetKey(int devnum)
{
	return UpnpCtrlPointGetVar(RCA_SERVICE_CONTROL, devnum, "Key");
}

int RcaCtrlPointSendPowerOn(int devnum)
{
	return UpnpCtrlPointSendAction(
		RCA_SERVICE_CONTROL, devnum, "PowerOn", NULL, NULL, 0);
}

int RcaCtrlPointSendPowerOff(int devnum)
{
	return UpnpCtrlPointSendAction(
		RCA_SERVICE_CONTROL, devnum, "PowerOff", NULL, NULL, 0);
}

int RcaCtrlPointSendSetModel(int devnum, int model)
{
	return UpnpCtrlPointSendActionNumericArg(
		devnum,	RCA_SERVICE_CONTROL, "SetModel", "Model", model);
}

int RcaCtrlPointSendSendKey(int devnum, int key)
{
	return UpnpCtrlPointSendActionNumericArg(
		devnum, RCA_SERVICE_CONTROL, "SendKey", "Key", key);
}

void RcaStateUpdate(char *UDN, int Service, IXML_Document *ChangedVariables,
		    char **State)
{
	IXML_NodeList *properties;
	IXML_NodeList *variables;
	IXML_Element *property;
	IXML_Element *variable;
	long unsigned int length;
	long unsigned int length1;
	long unsigned int i;
	int j;
	char *tmpstate = NULL;
	
	SampleUtil_Print("Rca State Update (service %d):\n", Service);
	/* Find all of the e:property tags in the document */
	properties = ixmlDocument_getElementsByTagName(
		ChangedVariables, "e:property");
	
	if (!properties) {
		return;
	}

	length = ixmlNodeList_length(properties);
	for (i = 0; i < length; i++) {
		/* Loop through each property change found */
		property = (IXML_Element *)ixmlNodeList_item(
			properties, i);
		/* For each variable name in the state table,
		 * check if this is a corresponding property change */
		for (j = 0; j < rcaVarCount[Service]; j++) {
			variables = ixmlElement_getElementsByTagName(
				property, rcaVarName[Service][j]);
			/* If a match is found, extract 
			 * the value, and update the state table */
			if (!variables) {
				continue;
			}
			
			length1 = ixmlNodeList_length(variables);
			if (length1) {
				variable = (IXML_Element *)
					ixmlNodeList_item(
						variables, 0);
				tmpstate =
					SampleUtil_GetElementValue(
						variable);
				if (tmpstate) {
					strcpy(State[j], tmpstate);
					SampleUtil_Print(
						" Variable Name: %s "
						"New Value:'%s'\n",
						rcaVarName[Service][j],
						State[j]);
				}
				if (tmpstate) {
					free(tmpstate);
				}
				tmpstate = NULL;
			}
			ixmlNodeList_free(variables);
			variables = NULL;
		}
	}

	ixmlNodeList_free(properties);

	return;
	UDN = UDN;
}

void RcaCtrlPointHandleEvent(
	const char *sid,
	int evntkey,
	IXML_Document *changes)
{
	struct UpnpDeviceNode *tmpdevnode;
	int service;
	int r;

	ithread_mutex_lock(&DeviceListMutex);

	tmpdevnode = GlobalDeviceList;
	while (tmpdevnode) {
		for (service = 0; service < RCA_SERVICE_SERVCOUNT;
		     ++service) {
			
			r = strcmp(tmpdevnode->device.UpnpService[service].SID,
				   sid);
			if (r != 0) {
				continue;
			}

			SampleUtil_Print(
				"Received Tv %s Event: %d for SID %s\n",
				rcaServiceName[service],
				evntkey,
				sid);
			RcaStateUpdate(
				tmpdevnode->device.UDN,
				service,
				changes,
				(char **)&tmpdevnode->device.
				UpnpService[service].VariableStrVal);
			break;
		}
		tmpdevnode = tmpdevnode->next;
	}

	ithread_mutex_unlock(&DeviceListMutex);
}

void RcaCtrlPointHandleSubscribeUpdate(
	const char *eventURL,
	const Upnp_SID sid,
	int timeout)
{
	struct UpnpDeviceNode *tmpdevnode;
	int service;
	int r;

	ithread_mutex_lock(&DeviceListMutex);

	tmpdevnode = GlobalDeviceList;
	while (tmpdevnode) {
		for (service = 0; service < RCA_SERVICE_SERVCOUNT;
		     service++) {
			r = strcmp(
				tmpdevnode->device.UpnpService[service].
				EventURL, eventURL);
			
			if (r == 0) {
				SampleUtil_Print(
					"Received Rca %s Event Renewal "
					"for eventURL %s\n",
					rcaServiceName[service], eventURL);
				strcpy(tmpdevnode->device.
				       UpnpService[service].SID, sid);
				break;
			}
		}
		
		tmpdevnode = tmpdevnode->next;
	}
	
	ithread_mutex_unlock(&DeviceListMutex);
	
	return;
	timeout = timeout;
}

void RcaCtrlPointHandleGetVar(
	const char *controlURL,
	const char *varName,
	const DOMString varValue)
{

	struct UpnpDeviceNode *tmpdevnode;
	int service;
	int r;

	ithread_mutex_lock(&DeviceListMutex);

	tmpdevnode = GlobalDeviceList;
	while (tmpdevnode) {
		for (service = 0; service < RCA_SERVICE_SERVCOUNT;
		     service++) {
			r = strcmp(
				tmpdevnode->device.UpnpService[service].
				ControlURL, controlURL);
			if (r == 0) {
				SampleUtil_StateUpdate(varName, varValue,
						       tmpdevnode->device.UDN,
						       GET_VAR_COMPLETE);
				break;
			}
		}
		tmpdevnode = tmpdevnode->next;
	}

	ithread_mutex_unlock(&DeviceListMutex);
}

int RcaCtrlPointCallbackEventHandler(Upnp_EventType EventType,
				     void *Event, void *Cookie)
{
	SampleUtil_PrintEvent(EventType, Event);
	switch (EventType) {
	/* SSDP Stuff */
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	case UPNP_DISCOVERY_SEARCH_RESULT: {
		struct Upnp_Discovery *d_event = 
			(struct Upnp_Discovery *)Event;
		IXML_Document *DescDoc = NULL;
		int ret;

		if (d_event->ErrCode != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error in Discovery Callback -- %d\n",
				d_event->ErrCode);
		}
		ret = UpnpDownloadXmlDoc(d_event->Location, &DescDoc);
		if (ret != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error obtaining device description "
				"from %s -- error = %d\n",
				d_event->Location, ret);
		} else {
			UpnpCtrlPointAddDevice(
				DescDoc, d_event->Location, d_event->Expires);
		}
		if (DescDoc) {
			ixmlDocument_free(DescDoc);
		}
		UpnpCtrlPointPrintList();
		break;
	}
	case UPNP_DISCOVERY_SEARCH_TIMEOUT:
		/* Nothing to do here... */
		break;
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE: {
		struct Upnp_Discovery *d_event =
			(struct Upnp_Discovery *)Event;

		if (d_event->ErrCode != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error in Discovery ByeBye Callback -- %d\n",
				d_event->ErrCode);
		}
		SampleUtil_Print("Received ByeBye for Device: %s\n",
				 d_event->DeviceId);
		UpnpCtrlPointRemoveDevice(d_event->DeviceId);
		SampleUtil_Print("After byebye:\n");
		UpnpCtrlPointPrintList();
		break;
	}
	/* SOAP Stuff */
	case UPNP_CONTROL_ACTION_COMPLETE: {
		struct Upnp_Action_Complete *a_event =
			(struct Upnp_Action_Complete *)Event;

		if (a_event->ErrCode != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error in  Action Complete Callback -- %d\n",
				a_event->ErrCode);
		}
		/* No need for any processing here, just print out results.
		 * Service state table updates are handled by events. */
		break;
	}
	case UPNP_CONTROL_GET_VAR_COMPLETE: {
		struct Upnp_State_Var_Complete *sv_event =
			(struct Upnp_State_Var_Complete *)Event;

		if (sv_event->ErrCode != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error in Get Var Complete Callback -- %d\n",
				sv_event->ErrCode);
		} else {
			RcaCtrlPointHandleGetVar(
				sv_event->CtrlUrl,
				sv_event->StateVarName,
				sv_event->CurrentVal);
		}
		break;
	}
	/* GENA Stuff */
	case UPNP_EVENT_RECEIVED: {
		struct Upnp_Event *e_event = (struct Upnp_Event *)Event;

		RcaCtrlPointHandleEvent(
			e_event->Sid,
			e_event->EventKey,
			e_event->ChangedVariables);
		break;
	}
	case UPNP_EVENT_SUBSCRIBE_COMPLETE:
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
	case UPNP_EVENT_RENEWAL_COMPLETE: {
		struct Upnp_Event_Subscribe *es_event =
			(struct Upnp_Event_Subscribe *)Event;
		
		if (es_event->ErrCode != UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Error in Event Subscribe Callback -- %d\n",
				es_event->ErrCode);
		} else {
			RcaCtrlPointHandleSubscribeUpdate(
				es_event->PublisherUrl,
				es_event->Sid,
				es_event->TimeOut);
		}
		break;
	}
	case UPNP_EVENT_AUTORENEWAL_FAILED:
	case UPNP_EVENT_SUBSCRIPTION_EXPIRED: {
		struct Upnp_Event_Subscribe *es_event = 
			(struct Upnp_Event_Subscribe *)Event;
		int TimeOut = defaultTimeout;
		Upnp_SID newSID;
		int ret;
		
		ret = UpnpSubscribe(
			ctrlptHandle,
			es_event->PublisherUrl,
			&TimeOut,
			newSID);
		if (ret == UPNP_E_SUCCESS) {
			SampleUtil_Print(
				"Subscribed to EventURL with SID=%s\n",
				newSID);
			RcaCtrlPointHandleSubscribeUpdate(
				es_event->PublisherUrl,
				newSID,
				TimeOut);
		} else {
			SampleUtil_Print(
				"Error Subscribing to EventURL -- %d\n",
				ret);
		}
		break;
	}
	/* ignore these cases, since this is not a device */
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
	case UPNP_CONTROL_GET_VAR_REQUEST:
	case UPNP_CONTROL_ACTION_REQUEST:
		break;
	}

	return 0;
	Cookie = Cookie;
}

void RcaCtrlPointVerifyTimeouts(int incr)
{
	struct UpnpDeviceNode *prevdevnode;
	struct UpnpDeviceNode *curdevnode;
	int ret;
	
	ithread_mutex_lock(&DeviceListMutex);

	prevdevnode = NULL;
	curdevnode = GlobalDeviceList;
	while (curdevnode) {
		curdevnode->device.AdvrTimeOut -= incr;
		/*SampleUtil_Print("Advertisement Timeout: %d\n", curdevnode->device.AdvrTimeOut); */
		if (curdevnode->device.AdvrTimeOut <= 0) {
			/* This advertisement has expired, so we should remove the device
			 * from the list */
			if (GlobalDeviceList == curdevnode)
				GlobalDeviceList = curdevnode->next;
			else
				prevdevnode->next = curdevnode->next;
			UpnpCtrlPointDeleteNode(curdevnode);
			if (prevdevnode)
				curdevnode = prevdevnode->next;
			else
				curdevnode = GlobalDeviceList;
		} else {
			if (curdevnode->device.AdvrTimeOut < 2 * incr) {
				/* This advertisement is about to expire, so
				 * send out a search request for this device
				 * UDN to try to renew */
				ret = UpnpSearchAsync(ctrlptHandle, incr,
						      curdevnode->device.UDN,
						      NULL);
				if (ret != UPNP_E_SUCCESS)
					SampleUtil_Print(
						"Error sending search "
						"request for Device UDN: %s "
						"-- err = %d\n",
						curdevnode->device.UDN, ret);
			}
			prevdevnode = curdevnode;
			curdevnode = curdevnode->next;
		}
	}

	ithread_mutex_unlock(&DeviceListMutex);
}

static int RcaCtrlPointTimerLoopRun = 1;
void *RcaCtrlPointTimerLoop(void *args)
{
	/* how often to verify the timeouts, in seconds */
	int incr = 30;

	while (RcaCtrlPointTimerLoopRun) {
		isleep((unsigned int)incr);
		RcaCtrlPointVerifyTimeouts(incr);
	}

	return NULL;
	args = args;
}

int RcaCtrlPointStart(print_string printFunctionPtr,
		      state_update updateFunctionPtr)
{
	ithread_t timer_thread;
	int rc;
	unsigned short port = 0;
	char *ip_address = NULL;

	SampleUtil_Initialize(printFunctionPtr);
	SampleUtil_RegisterUpdateFunction(updateFunctionPtr);

	ithread_mutex_init(&DeviceListMutex, 0);

	SampleUtil_Print("Initializing UPnP Sdk with\n"
			 "\tipaddress = %s port = %u\n",
			 ip_address ? ip_address : "{NULL}", port);

	rc = UpnpInit(ip_address, port);
	if (rc != UPNP_E_SUCCESS) {
		SampleUtil_Print("%s: UpnpInit() Error: %d\n",
				 __FUNCTION__, rc);
	}
	if (!ip_address) {
		ip_address = UpnpGetServerIpAddress();
	}
	if (!port) {
		port = UpnpGetServerPort();
	}

	SampleUtil_Print("UPnP Initialized\n"
			 "\tipaddress = %s port = %u\n",
			 ip_address ? ip_address : "{NULL}", port);
	SampleUtil_Print("Registering Control Point\n");
	rc = UpnpRegisterClient(RcaCtrlPointCallbackEventHandler,
				&ctrlptHandle, &ctrlptHandle);
	if (rc != UPNP_E_SUCCESS) {
		SampleUtil_Print("Error registering CP: %d\n", rc);
		UpnpFinish();

		return CTRLPT_ERROR;
	}
	
	SampleUtil_Print("Control Point Registered\n");

	UpnpCtrlPointRefresh();

	/* start a timer thread */
	ithread_create(&timer_thread, NULL, RcaCtrlPointTimerLoop, NULL);
	ithread_detach(timer_thread);

	return CTRLPT_SUCCESS;
}

int RcaCtrlPointStop(void)
{
	RcaCtrlPointTimerLoopRun = 0;
	UpnpCtrlPointRemoveAll();
	UpnpUnRegisterClient(ctrlptHandle);
	UpnpFinish();
	SampleUtil_Finish();

	return CTRLPT_SUCCESS;
}

void RcaCtrlPointPrintShortHelp(void)
{
	SampleUtil_Print(
		"Commands:\n"
		"  Help\n"
		"  HelpFull\n"
		"  ListDev\n"
		"  Refresh\n"
		"  PrintDev      <devnum>\n"
		"  PowerOn       <devnum>\n"
		"  PowerOff      <devnum>\n"
		"  SetModel      <devnum> <model>\n"
		"  SendKey       <devnum> <key>\n"
		"  CtrlAction    <devnum> <action>\n"
		"  CtrlGetVar    <devnum> <varname>\n"
		"  Exit\n");
}

void RcaCtrlPointPrintLongHelp(void)
{
	SampleUtil_Print(
		"\n"
		"******************************\n"
		"* RCA Control Point Help Info *\n"
		"******************************\n"
		"\n"
		"This sample control point application automatically searches\n"
		"for and subscribes to the services of remote control agent emulator\n"
		"devices, described in the rcadevicedesc.xml description document.\n"
		"\n"
		"Commands:\n"
		"  Help\n"
		"       Print this help info.\n"
		"  ListDev\n"
		"       Print the current list of TV Device Emulators that this\n"
		"         control point is aware of.  Each device is preceded by a\n"
		"         device number which corresponds to the devnum argument of\n"
		"         commands listed below.\n"
		"  Refresh\n"
		"       Delete all of the devices from the device list and issue new\n"
		"         search request to rebuild the list from scratch.\n"
		"  PrintDev       <devnum>\n"
		"       Print the state table for the device <devnum>.\n"
		"         e.g., 'PrintDev 1' prints the state table for the first\n"
		"         device in the device list.\n"
		"  PowerOn        <devnum>\n"
		"       Sends the PowerOn action to the Control Service of\n"
		"         device <devnum>.\n"
		"  PowerOff       <devnum>\n"
		"       Sends the PowerOff action to the Control Service of\n"
		"         device <devnum>.\n"
		"  SetModel     <devnum> <model>\n"
		"       Sends the SetModel action to the Control Service of\n"
		"         device <devnum>, requesting the model to be changed\n"
		"         to <model>.\n"
		"  SendKey      <devnum> <key>\n"
		"       Sends the SendKey action to the Control Service of\n"
		"         device <devnum>, requesting the key <key> to be sent \n"
		"  CtrlAction     <devnum> <action>\n"
		"       Sends an action request specified by the string <action>\n"
		"         to the Control Service of device <devnum>.  This command\n"
		"         only works for actions that have no arguments.\n"
		"         (e.g., \"CtrlAction 1 IncreaseKey\")\n"
		"  CtrlGetVar     <devnum> <varname>\n"
		"       Requests the value of a variable specified by the string <varname>\n"
		"         from the Control Service of device <devnum>.\n"
		"         (e.g., \"CtrlGetVar 1 Model\")\n"
		"  Exit\n"
		"       Exits the control point application.\n");
}

/*! Tags for valid commands issued at the command prompt. */
enum cmdloop_tvcmds {
	PRTHELP = 0,
	PRTFULLHELP,
	POWON,
	POWOFF,
	SETMODEL,
	SENDKEY,
	CTRLACTION,
	CTRLGETVAR,
	PRTDEV,
	LSTDEV,
	REFRESH,
	EXITCMD
};

/*! Data structure for parsing commands from the command line. */
struct cmdloop_commands {
	/* the string  */
	const char *str;
	/* the command */
	int cmdnum;
	/* the number of arguments */
	int numargs;
	/* the args */
	const char *args;
} cmdloop_commands;

/*! Mappings between command text names, command tag,
 * and required command arguments for command line
 * commands */
static struct cmdloop_commands cmdloop_cmdlist[] = {
	{"Help",          PRTHELP,     1, ""},
	{"HelpFull",      PRTFULLHELP, 1, ""},
	{"ListDev",       LSTDEV,      1, ""},
	{"Refresh",       REFRESH,     1, ""},
	{"PrintDev",      PRTDEV,      2, "<devnum>"},
	{"PowerOn",       POWON,       2, "<devnum>"},
	{"PowerOff",      POWOFF,      2, "<devnum>"},
	{"SetModel",      SETMODEL,    3, "<devnum> <model (int)>"},
	{"SendKey",       SENDKEY,     3, "<devnum> <key (int)>"},
	{"CtrlAction",    CTRLACTION,  2, "<devnum> <action (string)>"},
	{"CtrlGetVar",    CTRLGETVAR,  2, "<devnum> <varname (string)>"},
	{"Exit", EXITCMD, 1, ""}
};

void RcaCtrlPointPrintCommands(void)
{
	int i;
	int numofcmds = (sizeof cmdloop_cmdlist) / sizeof (cmdloop_commands);

	SampleUtil_Print("Valid Commands:\n");
	for (i = 0; i < numofcmds; ++i) {
		SampleUtil_Print("  %-14s %s\n",
				 cmdloop_cmdlist[i].str,
				 cmdloop_cmdlist[i].args);
	}
	SampleUtil_Print("\n");
}

void *RcaCtrlPointCommandLoop(void *args)
{
	char cmdline[100];

	while (1) {
		SampleUtil_Print("\n>> ");
		fgets(cmdline, 100, stdin);
		RcaCtrlPointProcessCommand(cmdline);
	}

	return NULL;
	args = args;
}

int RcaCtrlPointProcessCommand(char *cmdline)
{
	char cmd[100];
	char strarg[100];
	int arg_val_err = -99999;
	int arg1 = arg_val_err;
	int arg2 = arg_val_err;
	int cmdnum = -1;
	int numofcmds = (sizeof cmdloop_cmdlist) / sizeof (cmdloop_commands);
	int cmdfound = 0;
	int i;
	int rc;
	int invalidargs = 0;
	int validargs;

	validargs = sscanf(cmdline, "%s %d %d", cmd, &arg1, &arg2);
	for (i = 0; i < numofcmds; ++i) {
		if (strcasecmp(cmd, cmdloop_cmdlist[i].str ) == 0) {
			cmdnum = cmdloop_cmdlist[i].cmdnum;
			cmdfound++;
			if (validargs != cmdloop_cmdlist[i].numargs)
				invalidargs++;
			break;
		}
	}
	if (!cmdfound) {
		SampleUtil_Print("Command not found; try 'Help'\n");
		return CTRLPT_SUCCESS;
	}
	if (invalidargs) {
		SampleUtil_Print("Invalid arguments; try 'Help'\n");
		return CTRLPT_SUCCESS;
	}
	switch (cmdnum) {
	case PRTHELP:
		RcaCtrlPointPrintShortHelp();
		break;
	case PRTFULLHELP:
		RcaCtrlPointPrintLongHelp();
		break;
	case POWON:
		RcaCtrlPointSendPowerOn(arg1);
		break;
	case POWOFF:
		RcaCtrlPointSendPowerOff(arg1);
		break;
	case SETMODEL:
		RcaCtrlPointSendSetModel(arg1, arg2);
		break;
	case SENDKEY:
		RcaCtrlPointSendSendKey(arg1, arg2);
		break;
	case CTRLACTION:
		/* re-parse commandline since second arg is string. */
		validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (validargs == 3)
			UpnpCtrlPointSendAction(
				RCA_SERVICE_CONTROL, arg1, strarg,
				NULL, NULL, 0);
		else
			invalidargs++;
		break;
	case CTRLGETVAR:
		/* re-parse commandline since second arg is string. */
		validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (validargs == 3)
			UpnpCtrlPointGetVar(RCA_SERVICE_CONTROL, arg1, strarg);
		else
			invalidargs++;
		break;
	case PRTDEV:
		UpnpCtrlPointPrintDevice(arg1);
		break;
	case LSTDEV:
		UpnpCtrlPointPrintList();
		break;
	case REFRESH:
		UpnpCtrlPointRefresh();
		break;
	case EXITCMD:
		rc = RcaCtrlPointStop();
		exit(rc);
		break;
	default:
		SampleUtil_Print("Command not implemented; see 'Help'\n");
		break;
	}
	if(invalidargs)
		SampleUtil_Print("Invalid args in command; see 'Help'\n");

	return CTRLPT_SUCCESS;
}

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
#include <stdarg.h>
#include <stdio.h>

#include "sample_util.h"
#include "rc_agent.h"

//============================================================================

int main(int argc, char *argv[])
{
	int rc;
	ithread_t cmdloop_thread;
#ifdef WIN32
#else
	int sig;
	sigset_t sigs_to_catch;
#endif
	int code;

	rc = device_main(argc, argv);
	if (rc != UPNP_E_SUCCESS) {
		return rc;
	}

	code = ithread_create(&cmdloop_thread, NULL,
			      RcaDeviceCommandLoop, NULL);
	if (code !=  0) {
		return UPNP_E_INTERNAL_ERROR;
	}

#ifdef WIN32
	ithread_join(cmdloop_thread, NULL);
#else

	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGINT);
	sigwait(&sigs_to_catch, &sig);
	SampleUtil_Print("Shutting down on signal %d...\n", sig);
#endif
	rc = RcaDeviceStop();

	return rc;
}

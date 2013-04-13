#=============================================================================
# Copyright 2013 Ubiquitous Computing
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# Makefile - to build remote station demo with libupnp source build.
#=============================================================================

#
LIBUPNP_ROOT=../libupnp-1.6.18

#
CC=gcc
CFLAGS=-g
#=============================================================================

LIBUPNP_INCS=$(LIBUPNP_ROOT)/ixml/inc $(LIBUPNP_ROOT)/upnp/inc \
	$(LIBUPNP_ROOT)/threadutil/inc

LIBUPNP_LIBS=$(LIBUPNP_ROOT)/ixml/.libs/ixml \
	$(LIBUPNP_ROOT)/threadutil/.libs/threadutil \
	$(LIBUPNP_ROOT)/upnp/.libs/upnp

LIBUPNP_LIB_DIRS=$(dir $(LIBUPNP_LIBS))
LIBUPNP_LIB_LINK=$(patsubst %,-l%,$(notdir $(LIBUPNP_LIBS)))

LIBUPNP_LAS=$(patsubst %.a,%.la,$(LIBUPNP_LIBS))

CFLAGS+= -I./common $(patsubst %,-I%,$(LIBUPNP_INCS))

LFLAGS+= $(patsubst %,-L%, $(LIBUPNP_LIB_DIRS))

rc_agent_resource = \
	common/sample_util.c \
	common/sample_util.h \
	common/rc_agent.c \
	common/rc_agent.h \
	linux/rc_agent_main.c
rca_ctrlpt_resource = \
	common/sample_util.c \
	common/sample_util.h \
	common/rca_ctrlpt.c \
	common/rca_ctrlpt.h \
	linux/rca_ctrlpt_main.c

rc_agent_src = $(filter %.c, $(rc_agent_resource))
rca_ctrlpt_src = $(filter %.c, $(rca_ctrlpt_resource))

rc_agent_obj = $(rc_agent_src:.c=.o)
rca_ctrlpt_obj = $(rca_ctrlpt_src:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^ 

rc_agent: $(rc_agent_obj)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBUPNP_LIB_LINK)

rca_ctrlpt: $(rca_ctrlpt_obj)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBUPNP_LIB_LINK)

all: rc_agent rca_ctrlpt

clean:
	rm -f $(rc_agent_obj) $(rca_ctrlpt_obj) rc_agent rca_ctrlpt
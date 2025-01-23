# Copyright (c) ZeroC, Inc.

$(project)_client_sources  = Test.ice \
                          Client.cpp \
                          AllTests.cpp \
                          Twoways.cpp \
                          Oneways.cpp \
                          TwowaysAMI.cpp \
                          OnewaysAMI.cpp \
                          BatchOneways.cpp \
                          BatchOnewaysAMI.cpp

ifeq ($(xlc_compiler),yes)
    $(project)_cppflags += -qsuppress="1540-0895"
endif

#
# Disable var tracking assignments for Linux with this test
#
ifneq ($(linux_id),)
    $(project)_cppflags += $(if $(filter yes,$(OPTIMIZE)),-fno-var-tracking-assignments)
endif

tests += $(project)

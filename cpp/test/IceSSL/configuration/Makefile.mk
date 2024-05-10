#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

$(test)_dependencies = TestCommon Ice

$(test)_client_sources := Client.cpp AllTests.cpp Test.ice TestI.cpp

ifeq ($(os),Darwin)
$(test)_client_sources += SecureTransportTests.cpp
$(test)_client_ldflags = -framework Security -framework CoreFoundation
endif

#
# Disable var tracking assignments for Linux with this test
#
ifneq ($(linux_id),)
    $(test)_cppflags += $(if $(filter yes,$(OPTIMIZE)),-fno-var-tracking-assignments)
endif

# Need to load certificates with functions from src/IceSSL/SSLUtil.h
$(test)[iphoneos]_cppflags              := -Isrc
$(test)[iphonesimulator]_cppflags       := -Isrc

tests += $(test)

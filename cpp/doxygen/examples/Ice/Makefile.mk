#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

$(project)_libraries    = IceExamples

IceExamples_dependencies     := Ice
IceExamples_targetdir        := doxygen/examples/lib

ifeq ($(os),Darwin)
IceExamples_extra_sources += $(filter-out doxygen/examples/Ice/SSL/OpenSSL%.cpp doxygen/examples/Ice/SSL/Schannel%.cpp, $(wildcard doxygen/examples/Ice/SSL/*.cpp))
IceExamples_ldflags       += -framework Security -framework CoreFoundation
else
IceExamples_extra_sources += $(filter-out doxygen/examples/Ice/SSL/SecureTransport%.cpp doxygen/examples/Ice/SSL/Schannel%.cpp, $(wildcard doxygen/examples/Ice/SSL/*.cpp))
IceExamples_ldflags       += -lssl -lcrypto
IceExamples_cppflags      += -Wno-missing-field-initializers
endif

projects += $(project)

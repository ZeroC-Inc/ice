#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

$(project)_type                 = xcode
$(project)_platforms            = iphonesimulator iphoneos
$(project)_configs              = xcodesdk
$(project)_dependencies         = $(tests)
$(project)_project              = C++ Test Controller.xcodeproj
$(project)_scheme               = C++ Test Controller
$(project)_configuration        = $(if $(filter $(OPTIMIZE),yes),Release,Debug)
$(project)_slicecompiler        = slice2cpp

projects += $(project)
tests:: $(project)

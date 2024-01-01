#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

$(project)_libraries    := Slice

Slice_targetdir         := $(libdir)
Slice_libs              := mcpp
Slice_cppflags          := -std=c++17
Slice_excludes          = $(srcdir)/Slice/Python.cpp \
                          $(srcdir)/Slice/PythonUtil.cpp \
                          $(srcdir)/Slice/Ruby.cpp \
                          $(srcdir)/Slice/RubyUtil.cpp \

# Always enable the static configuration for the Slice library and never
# install it
Slice_always_enable_configs     := static
Slice_always_enable_platforms   := $(build-platform)
Slice_install_configs           := none
Slice_bisonflags                := --name-prefix "slice_"

projects += $(project)

#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

# ruff: noqa: F401, F821

"""
Glacier2 module
"""

#
# Import the Python extension.
#
import Ice

Ice.updateModule("Glacier2")

import Glacier2.Router_ice
import Glacier2.Session_ice
import Glacier2.PermissionsVerifier_ice
import Glacier2.SSLInfo_ice
import Glacier2.Metrics_ice

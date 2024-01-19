# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

def clientProps(process, current): return {"TestDir": current.getBuildDir("server")}


if isinstance(platform, Windows) or os.getuid() != 0:
    TestSuite(__file__, [
        IceGridTestCase("without targets",
                        icegridnode=IceGridNode(envs={"MY_FOO": 12}),
                        client=IceGridClient(props=clientProps)),
        IceGridTestCase("with targets",
                        icegridnode=IceGridNode(envs={"MY_FOO": 12}),
                        client=IceGridClient(props=clientProps),
                        targets=["moreservers", "moreservices", "moreproperties"])
    ], libDirs=["testservice"], multihost=False)

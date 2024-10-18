#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

from DataStormUtil import Reader, Writer
from Util import ClientServerTestCase, TestSuite

traceProps = {
    "DataStorm.Trace.Topic" : 1,
    "DataStorm.Trace.Session" : 3,
    "DataStorm.Trace.Data" : 2,
    "Ice.Trace.Protocol" : 2,
    "Ice.Trace.Network" : 2
}

multicastProps = { "DataStorm.Node.Multicast.Enabled" : 1 }

TestSuite(
    __file__,
    [
        ClientServerTestCase(
            name = "Writer/Reader",
            client = Writer(),
            server = Reader(),
            traceProps=traceProps),
        ClientServerTestCase(
            name = "Writer/Reader multicast enabled",
            client = Writer(props = multicastProps),
            server = Reader(props = multicastProps),
            traceProps=traceProps),
    ],
)

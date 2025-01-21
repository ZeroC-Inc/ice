# Copyright (c) ZeroC, Inc.

$(test)_programs        = publisher subscriber
$(test)_dependencies    = IceStorm Ice TestCommon

$(test)_publisher_sources       = Publisher.cpp Single.ice
$(test)_subscriber_sources      = Subscriber.cpp Single.ice

tests += $(test)

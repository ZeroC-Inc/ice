# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
#
# Ice version 3.7.10
#
# <auto-generated>
#
# Generated from file `Properties.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice
import IcePy
import Ice.BuiltinSequences_ice
import Ice.PropertyDict_ice

# Included module Ice
_M_Ice = Ice.openModule('Ice')

# Start of module Ice
__name__ = 'Ice'

if 'Properties' not in _M_Ice.__dict__:
    _M_Ice.Properties = Ice.createTempClass()

    class Properties(object):
        """
         A property set used to configure Ice and Ice applications. Properties are key/value pairs, with both keys and
         values being strings. By convention, property keys should have the form
         application-name[.category[.sub-category]].name.
        """

        def __init__(self):
            if Ice.getType(self) == _M_Ice.Properties:
                raise RuntimeError('Ice.Properties is an abstract class')

        def getProperty(self, key):
            """
             Get a property by key. If the property is not set, an empty string is returned.
            Arguments:
            key -- The property key.
            Returns: The property value.
            """
            raise NotImplementedError("method 'getProperty' not implemented")

        def getPropertyWithDefault(self, key, value):
            """
             Get a property by key. If the property is not set, the given default value is returned.
            Arguments:
            key -- The property key.
            value -- The default value to use if the property does not exist.
            Returns: The property value or the default value.
            """
            raise NotImplementedError("method 'getPropertyWithDefault' not implemented")

        def getPropertyAsInt(self, key):
            """
             Get a property as an integer. If the property is not set, 0 is returned.
            Arguments:
            key -- The property key.
            Returns: The property value interpreted as an integer.
            """
            raise NotImplementedError("method 'getPropertyAsInt' not implemented")

        def getPropertyAsIntWithDefault(self, key, value):
            """
             Get a property as an integer. If the property is not set, the given default value is returned.
            Arguments:
            key -- The property key.
            value -- The default value to use if the property does not exist.
            Returns: The property value interpreted as an integer, or the default value.
            """
            raise NotImplementedError("method 'getPropertyAsIntWithDefault' not implemented")

        def getPropertyAsList(self, key):
            """
             Get a property as a list of strings. The strings must be separated by whitespace or comma. If the property is
             not set, an empty list is returned. The strings in the list can contain whitespace and commas if they are
             enclosed in single or double quotes. If quotes are mismatched, an empty list is returned. Within single quotes
             or double quotes, you can escape the quote in question with a backslash, e.g. O'Reilly can be written as
             O'Reilly, "O'Reilly" or 'O\'Reilly'.
            Arguments:
            key -- The property key.
            Returns: The property value interpreted as a list of strings.
            """
            raise NotImplementedError("method 'getPropertyAsList' not implemented")

        def getPropertyAsListWithDefault(self, key, value):
            """
             Get a property as a list of strings.  The strings must be separated by whitespace or comma. If the property is
             not set, the default list is returned. The strings in the list can contain whitespace and commas if they are
             enclosed in single or double quotes. If quotes are mismatched, the default list is returned. Within single
             quotes or double quotes, you can escape the quote in question with a backslash, e.g. O'Reilly can be written as
             O'Reilly, "O'Reilly" or 'O\'Reilly'.
            Arguments:
            key -- The property key.
            value -- The default value to use if the property is not set.
            Returns: The property value interpreted as list of strings, or the default value.
            """
            raise NotImplementedError("method 'getPropertyAsListWithDefault' not implemented")

        def getPropertiesForPrefix(self, prefix):
            """
             Get all properties whose keys begins with prefix. If prefix is an empty string, then all
             properties are returned.
            Arguments:
            prefix -- The prefix to search for (empty string if none).
            Returns: The matching property set.
            """
            raise NotImplementedError("method 'getPropertiesForPrefix' not implemented")

        def setProperty(self, key, value):
            """
             Set a property. To unset a property, set it to the empty string.
            Arguments:
            key -- The property key.
            value -- The property value.
            """
            raise NotImplementedError("method 'setProperty' not implemented")

        def getCommandLineOptions(self):
            """
             Get a sequence of command-line options that is equivalent to this property set. Each element of the returned
             sequence is a command-line option of the form --key=value.
            Returns: The command line options for this property set.
            """
            raise NotImplementedError("method 'getCommandLineOptions' not implemented")

        def parseCommandLineOptions(self, prefix, options):
            """
             Convert a sequence of command-line options into properties. All options that begin with
             --prefix. are converted into properties. If the prefix is empty, all options that begin with
             -- are converted to properties.
            Arguments:
            prefix -- The property prefix, or an empty string to convert all options starting with --.
            options -- The command-line options.
            Returns: The command-line options that do not start with the specified prefix, in their original order.
            """
            raise NotImplementedError("method 'parseCommandLineOptions' not implemented")

        def parseIceCommandLineOptions(self, options):
            """
             Convert a sequence of command-line options into properties. All options that begin with one of the following
             prefixes are converted into properties: --Ice, --IceBox, --IceGrid,
             --IcePatch2, --IceSSL, --IceStorm, --Freeze, and --Glacier2.
            Arguments:
            options -- The command-line options.
            Returns: The command-line options that do not start with one of the listed prefixes, in their original order.
            """
            raise NotImplementedError("method 'parseIceCommandLineOptions' not implemented")

        def load(self, file):
            """
             Load properties from a file.
            Arguments:
            file -- The property file.
            """
            raise NotImplementedError("method 'load' not implemented")

        def clone(self):
            """
             Create a copy of this property set.
            Returns: A copy of this property set.
            """
            raise NotImplementedError("method 'clone' not implemented")

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_Properties)

        __repr__ = __str__

    _M_Ice._t_Properties = IcePy.defineValue('::Ice::Properties', Properties, -1, (), False, True, None, ())
    Properties._ice_type = _M_Ice._t_Properties

    _M_Ice.Properties = Properties
    del Properties

# End of module Ice

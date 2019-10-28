//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
namespace Ice
{
    public partial interface Properties
    {
        /// <summary>
        /// Get a property by key.
        /// If the property is not set, an empty
        /// string is returned.
        ///
        /// </summary>
        /// <param name="key">The property key.
        ///
        /// </param>
        /// <returns>The property value.
        ///
        /// </returns>
        string getProperty(string key);

        /// <summary>
        /// Get a property by key.
        /// If the property is not set, the
        /// given default value is returned.
        ///
        /// </summary>
        /// <param name="key">The property key.
        ///
        /// </param>
        /// <param name="value">The default value to use if the property does not
        /// exist.
        ///
        /// </param>
        /// <returns>The property value or the default value.
        ///
        /// </returns>
        string getPropertyWithDefault(string key, string value);

        /// <summary>
        /// Get a property as an integer.
        /// If the property is not set, 0
        /// is returned.
        ///
        /// </summary>
        /// <param name="key">The property key.
        ///
        /// </param>
        /// <returns>The property value interpreted as an integer.
        ///
        /// </returns>
        int getPropertyAsInt(string key);

        /// <summary>
        /// Get a property as an integer.
        /// If the property is not set, the
        /// given default value is returned.
        ///
        /// </summary>
        /// <param name="key">The property key.
        ///
        /// </param>
        /// <param name="value">The default value to use if the property does not
        /// exist.
        ///
        /// </param>
        /// <returns>The property value interpreted as an integer, or the
        /// default value.
        ///
        /// </returns>
        int getPropertyAsIntWithDefault(string key, int value);

        /// <summary>
        /// Get a property as a list of strings.
        /// The strings must be
        /// separated by whitespace or comma. If the property is not set,
        /// an empty list is returned. The strings in the list can contain
        /// whitespace and commas if they are enclosed in single or double
        /// quotes. If quotes are mismatched, an empty list is returned.
        /// Within single quotes or double quotes, you can escape the
        /// quote in question with \, e.g. O'Reilly can be written as
        /// O'Reilly, "O'Reilly" or 'O\'Reilly'.
        ///
        /// </summary>
        /// <param name="key">The property key.
        ///
        /// </param>
        /// <returns>The property value interpreted as a list of strings.
        ///
        /// </returns>
        string[] getPropertyAsList(string key);

        /// <summary>
        /// Get a property as a list of strings.
        /// The strings must be
        /// separated by whitespace or comma. If the property is not set,
        /// the default list is returned. The strings in the list can contain
        /// whitespace and commas if they are enclosed in single or double
        /// quotes. If quotes are mismatched, the default list is returned.
        /// Within single quotes or double quotes, you can escape the
        /// quote in question with \, e.g. O'Reilly can be written as
        /// O'Reilly, "O'Reilly" or 'O\'Reilly'.
        ///
        /// </summary>
        /// <param name="key">The property key.
        ///
        /// </param>
        /// <param name="value">The default value to use if the property is not set.
        ///
        /// </param>
        /// <returns>The property value interpreted as list of strings, or the
        /// default value.
        ///
        /// </returns>
        string[] getPropertyAsListWithDefault(string key, string[] value);

        /// <summary>
        /// Get all properties whose keys begins with
        /// prefix.
        /// If
        /// prefix is an empty string,
        /// then all properties are returned.
        ///
        /// </summary>
        /// <param name="prefix">The prefix to search for (empty string if none).
        /// </param>
        /// <returns>The matching property set.</returns>
        global::System.Collections.Generic.Dictionary<string, string> getPropertiesForPrefix(string prefix);

        /// <summary>
        /// Set a property.
        /// To unset a property, set it to
        /// the empty string.
        ///
        /// </summary>
        /// <param name="key">The property key.
        /// </param>
        /// <param name="value">The property value.
        ///
        /// </param>
        void setProperty(string key, string value);

        /// <summary>
        /// Get a sequence of command-line options that is equivalent to
        /// this property set.
        /// Each element of the returned sequence is
        /// a command-line option of the form
        /// --key=value.
        ///
        /// </summary>
        /// <returns>The command line options for this property set.</returns>

        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.3")]
        string[] getCommandLineOptions();

        /// <summary>
        /// Convert a sequence of command-line options into properties.
        /// All options that begin with
        /// --prefix. are
        /// converted into properties. If the prefix is empty, all options
        /// that begin with -- are converted to properties.
        ///
        /// </summary>
        /// <param name="prefix">The property prefix, or an empty string to
        /// convert all options starting with --.
        ///
        /// </param>
        /// <param name="options">The command-line options.
        ///
        /// </param>
        /// <returns>The command-line options that do not start with the specified
        /// prefix, in their original order.</returns>
        string[] parseCommandLineOptions(string prefix, string[] options);

        /// <summary>
        /// Convert a sequence of command-line options into properties.
        /// All options that begin with one of the following prefixes
        /// are converted into properties: --Ice, --IceBox, --IceGrid,
        /// --IceSSL, --IceStorm, and --Glacier2.
        ///
        /// </summary>
        /// <param name="options">The command-line options.
        ///
        /// </param>
        /// <returns>The command-line options that do not start with one of
        /// the listed prefixes, in their original order.</returns>
        string[] parseIceCommandLineOptions(string[] options);

        /// <summary>
        /// Load properties from a file.
        /// </summary>
        /// <param name="file">The property file.</param>
        void load(string file);

        /// <summary>
        /// Create a copy of this property set.
        /// </summary>
        /// <returns>A copy of this property set.</returns>
        Properties ice_clone_();
    }
}

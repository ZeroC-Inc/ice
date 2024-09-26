//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package test.Ice.properties;

import com.zeroc.Ice.Properties;

public class Client extends test.TestHelper {
    public static void test(boolean b) {
        if (!b) {
            throw new RuntimeException();
        }
    }

    public void run(String[] args) {
        {
            System.out.print("testing load properties from UTF-8 path... ");
            Properties properties = new Properties();
            properties.load(configPath);
            test(properties.getProperty("Ice.Trace.Network").equals("1"));
            test(properties.getProperty("Ice.Trace.Protocol").equals("1"));
            test(properties.getProperty("Config.Path").equals(configPath));
            test(properties.getProperty("Ice.ProgramName").equals("PropertiesClient"));
            System.out.println("ok");
        }

        {
            //
            // Try to load multiple config files.
            //
            System.out.print("testing using Ice.Config with multiple config files... ");
            String[] args1 =
                    new String[] {"--Ice.Config=config/config.1, config/config.2, config/config.3"};
            Properties properties = new Properties(args1);
            test(properties.getProperty("Config1").equals("Config1"));
            test(properties.getProperty("Config2").equals("Config2"));
            test(properties.getProperty("Config3").equals("Config3"));
            System.out.println("ok");
        }

        {
            System.out.print("testing configuration file escapes... ");
            String[] args1 = new String[] {"--Ice.Config=config/escapes.cfg"};
            Properties properties = new Properties(args1);

            String[] props =
                    new String[] {
                        "Foo\tBar", "3",
                        "Foo\\tBar", "4",
                        "Escape\\ Space", "2",
                        "Prop1", "1",
                        "Prop2", "2",
                        "Prop3", "3",
                        "My Prop1", "1",
                        "My Prop2", "2",
                        "My.Prop1", "a property",
                        "My.Prop2", "a     property",
                        "My.Prop3", "  a     property  ",
                        "My.Prop4", "  a     property  ",
                        "My.Prop5", "a \\ property",
                        "foo=bar", "1",
                        "foo#bar", "2",
                        "foo bar", "3",
                        "A", "1",
                        "B", "2 3 4",
                        "C", "5=#6",
                        "AServer", "\\\\server\\dir",
                        "BServer", "\\server\\dir",
                        ""
                    };

            for (int i = 0; !props[i].isEmpty(); i += 2) {
                test(properties.getProperty(props[i]).equals(props[i + 1]));
            }
            System.out.println("ok");
        }

        {
            System.out.print("testing ice properties with set default values...");
            Properties properties = new Properties();

            String toStringMode = properties.getIceProperty("Ice.ToStringMode");
            test(toStringMode.equals("Unicode"));

            int closeTimeout = properties.getIcePropertyAsInt("Ice.Connection.Client.CloseTimeout");
            test(closeTimeout == 10);

            String[] retryIntervals = properties.getIcePropertyAsList("Ice.RetryIntervals");
            test(retryIntervals.length == 1);
            test(retryIntervals[0].equals("0"));

            System.out.println("ok");
        }

        {
            System.out.print("testing ice properties with unset default values...");
            Properties properties = new Properties();

            String stringValue = properties.getIceProperty("Ice.Default.Host");
            test(stringValue.isEmpty());

            int intValue = properties.getIcePropertyAsInt("Ice.Default.Host");
            test(intValue == 0);

            String[] listValue = properties.getIcePropertyAsList("Ice.Default.Host");
            test(listValue.length == 0);

            System.out.println("ok");
        }

        {
            System.out.print("testing that getting an unknown ice property throws an exception...");
            Properties properties = new Properties();
            try {
                properties.getIceProperty("Ice.UnknownProperty");
                test(false);
            } catch (IllegalArgumentException ex) {
            }
            System.out.println("ok");
        }
    }

    private static String configPath = "./config/\u4E2D\u56FD_client.config";
}

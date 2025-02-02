// Copyright (c) ZeroC, Inc.

#pragma once

module await
{

enum var
{
    base
}

struct break
{
    int while;
    string clone;
    string equals;
    string hashCode;
    string constructor;
}

interface case
{
    ["amd"] void catch(int checked, out int continue);
}

interface typeof
{
    void default();
}

class delete
{
    int if;
    case* else;
    int export;
    string clone;
    string equals;
    string hashCode;
    string constructor;
}

interface explicit extends typeof, case
{
}

dictionary<string, break> while;

class package
{
    optional(1) break for;
    optional(2) var goto;
    optional(3) explicit* if;
    optional(5) while internal;
    optional(7) string debugger;
    optional(8) explicit* null;
}

interface optionalParams
{
    optional(1) break for(optional(2) var goto,
                          optional(3) explicit* if,
                          optional(5) while internal,
                          optional(7) string namespace,
                          optional(8) explicit* null);

    ["amd"]
    optional(1) break continue(optional(2) var goto,
                               optional(3) explicit* if,
                               optional(5) while internal,
                               optional(7) string namespace,
                               optional(8) explicit* null);

    optional(1) break in(out optional(2) var goto,
                         out optional(3) explicit* if,
                         out optional(5) while internal,
                         out optional(7) string namespace,
                         out optional(8) explicit* null);

    ["amd"]
    optional(1) break foreach(out optional(2) var goto,
                              out optional(3) explicit* if,
                              out optional(5) while internal,
                              out optional(7) string namespace,
                              out optional(8) explicit* null);
}

exception fixed
{
    int for;
}

exception foreach extends fixed
{
    int goto;
    int if;
}

interface implicit
{
    var in(break internal, delete is, explicit* lock, case* namespace, typeof* new, delete null,
          explicit* operator, int override, int params, int private)
        throws fixed, foreach;
}

const int protected = 0;
const int public = 0;

//
// System as inner module.
//
module System
{

interface Test
{
    void op();
}

}

}

//
// System as outer module.
//
module System
{

interface Test
{
    void op();
}

}

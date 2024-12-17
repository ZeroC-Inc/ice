%code top{

// Copyright (c) ZeroC, Inc.

// NOLINTBEGIN

}

%code requires{

// NOLINTBEGIN

#include <list>
#include <string>

// I must set the initial stack depth to the maximum stack depth to
// disable bison stack resizing. The bison stack resizing routines use
// simple malloc/alloc/memcpy calls, which do not work for the
// YYSTYPE, since YYSTYPE is a C++ type, with constructor, destructor,
// assignment operator, etc.
#define YYMAXDEPTH  10000      // 10000 should suffice. Bison default is 10000 as maximum.
#define YYINITDEPTH YYMAXDEPTH // Initial depth is set to max depth, for the reasons described above.

// Newer bison versions allow to disable stack resizing by defining yyoverflow.
#define yyoverflow(a, b, c, d, e, f) yyerror(a)

}

%code{

// Forward declaration of the lexing function generated by flex, so bison knows about it.
// This must match the definition of 'yylex' in the generated scanner.
int yylex(YYSTYPE* yylvalp);

}

%{

#include "Ice/Ice.h"
#include "Parser.h"

#ifdef _MSC_VER
// warning C4102: 'yyoverflowlab' : unreferenced label
#   pragma warning(disable:4102)
// warning C4702: unreachable code
#    pragma warning(disable:4702)
#endif

// Avoid old style cast warnings in generated grammar
#ifdef __GNUC__
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wunused-label"
#endif

// Avoid clang warnings in generated grammar
#if defined(__clang__)
#    pragma clang diagnostic ignored "-Wunused-but-set-variable"
#    pragma clang diagnostic ignored "-Wunused-label"
#endif

using namespace std;
using namespace Ice;
using namespace IceStorm;

void
yyerror(const char* s)
{
    parser->error(s);
}

%}

// Directs Bison to generate a re-entrant parser.
%define api.pure
// Specifies what type to back the tokens with (their semantic values).
%define api.value.type {std::list<std::string>}

// All keyword tokens. Make sure to modify the "keyword" rule in this
// file if the list of keywords is changed. Also make sure to add the
// keyword to the keyword table in Scanner.l.
%token ICESTORM_HELP
%token ICESTORM_EXIT
%token ICESTORM_CURRENT
%token ICESTORM_CREATE
%token ICESTORM_DESTROY
%token ICESTORM_LINK
%token ICESTORM_UNLINK
%token ICESTORM_LINKS
%token ICESTORM_TOPICS
%token ICESTORM_REPLICA
%token ICESTORM_SUBSCRIBERS
%token ICESTORM_STRING

%%

// ----------------------------------------------------------------------
start
// ----------------------------------------------------------------------
: commands
{
}
| %empty
{
}
;

// ----------------------------------------------------------------------
commands
// ----------------------------------------------------------------------
: commands command
{
}
| command
{
}
;

// ----------------------------------------------------------------------
command
// ----------------------------------------------------------------------
: ICESTORM_HELP ';'
{
    parser->usage();
}
| ICESTORM_EXIT ';'
{
    return 0;
}
| ICESTORM_CREATE strings ';'
{
    parser->create($2);
}
| ICESTORM_CURRENT strings ';'
{
    parser->current($2);
}
| ICESTORM_DESTROY strings ';'
{
    parser->destroy($2);
}
| ICESTORM_LINK strings ';'
{
    parser->link($2);
}
| ICESTORM_UNLINK strings ';'
{
    parser->unlink($2);
}
| ICESTORM_LINKS strings ';'
{
    parser->links($2);
}
| ICESTORM_TOPICS strings ';'
{
    parser->topics($2);
}
| ICESTORM_REPLICA strings ';'
{
    parser->replica($2);
}
| ICESTORM_SUBSCRIBERS strings ';'
{
    parser->subscribers($2);
}
| ICESTORM_STRING error ';'
{
    parser->invalidCommand("unknown command `" + $1.front() + "' (type `help' for more info)");
}
| error ';'
{
    yyerrok;
}
| ';'
{
}
;

// ----------------------------------------------------------------------
strings
// ----------------------------------------------------------------------
: ICESTORM_STRING strings
{
    $$ = $2;
    $$.push_front($1.front());
}
| keyword strings
{
    $$ = $2;
    $$.push_front($1.front());
}
| %empty
{
    $$ = YYSTYPE();
}
;

// ----------------------------------------------------------------------
keyword
// ----------------------------------------------------------------------
: ICESTORM_HELP
{
}
| ICESTORM_EXIT
{
}
| ICESTORM_CURRENT
{
}
| ICESTORM_CREATE
{
}
| ICESTORM_DESTROY
{
}
| ICESTORM_LINK
{
}
| ICESTORM_UNLINK
{
}
| ICESTORM_LINKS
{
}
| ICESTORM_TOPICS
{
}

%%

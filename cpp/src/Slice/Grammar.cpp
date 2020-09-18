/* A Bison parser, made by GNU Bison 3.7.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.7.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* "%code top" blocks.  */
#line 1 "src/Slice/Grammar.y"


//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

// Included first to get 'TokenContext' which we need to define YYLTYPE before flex does.
#include <Slice/GrammarUtil.h>

#line 30 "src/Slice/Grammar.y"


// Defines the rule bison uses to reduce token locations. Bison asks that the macro should
// be one-line, and treatable as a single statement when followed by a semi-colon.
// `N` is the number of tokens that are being combined, and (Cur) is their combined location.
#define YYLLOC_DEFAULT(Cur, Rhs, N)                               \
do                                                                \
    if(N == 1)                                                    \
    {                                                             \
        (Cur) = (YYRHSLOC((Rhs), 1));                             \
    }                                                             \
    else                                                          \
    {                                                             \
        if(N)                                                     \
        {                                                         \
            (Cur).firstLine = (YYRHSLOC((Rhs), 1)).firstLine;     \
            (Cur).firstColumn = (YYRHSLOC((Rhs), 1)).firstColumn; \
        }                                                         \
        else                                                      \
        {                                                         \
            (Cur).firstLine = (YYRHSLOC((Rhs), 0)).lastLine;      \
            (Cur).firstColumn = (YYRHSLOC((Rhs), 0)).lastColumn;  \
        }                                                         \
        (Cur).filename = (YYRHSLOC((Rhs), N)).filename;           \
        (Cur).lastLine = (YYRHSLOC((Rhs), N)).lastLine;           \
        (Cur).lastColumn = (YYRHSLOC((Rhs), N)).lastColumn;       \
    }                                                             \
while(0)


#line 109 "src/Slice/Grammar.cpp"

/* Substitute the variable and function names.  */
#define yyparse         slice_parse
#define yylex           slice_lex
#define yyerror         slice_error
#define yydebug         slice_debug
#define yynerrs         slice_nerrs

/* First part of user prologue.  */
#line 69 "src/Slice/Grammar.y"


#include <IceUtil/InputUtil.h>
#include <IceUtil/UUID.h>
#include <cstring>

#ifdef _MSC_VER
// warning C4102: 'yyoverflowlab' : unreferenced label
#   pragma warning(disable:4102)
// warning C4065: switch statement contains 'default' but no 'case' labels
#   pragma warning(disable:4065)
// warning C4244: '=': conversion from 'int' to 'yytype_int16', possible loss of data
#   pragma warning(disable:4244)
// warning C4127: conditional expression is constant
#   pragma warning(disable:4127)
#endif

// Avoid old style cast warnings in generated grammar
#ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

// Avoid clang conversion warnings
#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wconversion"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#endif

using namespace std;
using namespace Slice;

void
slice_error(const char* s)
{
    // yacc and recent versions of Bison use "syntax error" instead
    // of "parse error".

    if (strcmp(s, "parse error") == 0)
    {
        unit->error("syntax error");
    }
    else
    {
        unit->error(s);
    }
}


#line 168 "src/Slice/Grammar.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "Grammar.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ICE_MODULE = 3,                 /* ICE_MODULE  */
  YYSYMBOL_ICE_CLASS = 4,                  /* ICE_CLASS  */
  YYSYMBOL_ICE_INTERFACE = 5,              /* ICE_INTERFACE  */
  YYSYMBOL_ICE_EXCEPTION = 6,              /* ICE_EXCEPTION  */
  YYSYMBOL_ICE_STRUCT = 7,                 /* ICE_STRUCT  */
  YYSYMBOL_ICE_SEQUENCE = 8,               /* ICE_SEQUENCE  */
  YYSYMBOL_ICE_DICTIONARY = 9,             /* ICE_DICTIONARY  */
  YYSYMBOL_ICE_ENUM = 10,                  /* ICE_ENUM  */
  YYSYMBOL_ICE_OUT = 11,                   /* ICE_OUT  */
  YYSYMBOL_ICE_EXTENDS = 12,               /* ICE_EXTENDS  */
  YYSYMBOL_ICE_IMPLEMENTS = 13,            /* ICE_IMPLEMENTS  */
  YYSYMBOL_ICE_THROWS = 14,                /* ICE_THROWS  */
  YYSYMBOL_ICE_VOID = 15,                  /* ICE_VOID  */
  YYSYMBOL_ICE_BOOL = 16,                  /* ICE_BOOL  */
  YYSYMBOL_ICE_BYTE = 17,                  /* ICE_BYTE  */
  YYSYMBOL_ICE_SHORT = 18,                 /* ICE_SHORT  */
  YYSYMBOL_ICE_USHORT = 19,                /* ICE_USHORT  */
  YYSYMBOL_ICE_INT = 20,                   /* ICE_INT  */
  YYSYMBOL_ICE_UINT = 21,                  /* ICE_UINT  */
  YYSYMBOL_ICE_VARINT = 22,                /* ICE_VARINT  */
  YYSYMBOL_ICE_VARUINT = 23,               /* ICE_VARUINT  */
  YYSYMBOL_ICE_LONG = 24,                  /* ICE_LONG  */
  YYSYMBOL_ICE_ULONG = 25,                 /* ICE_ULONG  */
  YYSYMBOL_ICE_VARLONG = 26,               /* ICE_VARLONG  */
  YYSYMBOL_ICE_VARULONG = 27,              /* ICE_VARULONG  */
  YYSYMBOL_ICE_FLOAT = 28,                 /* ICE_FLOAT  */
  YYSYMBOL_ICE_DOUBLE = 29,                /* ICE_DOUBLE  */
  YYSYMBOL_ICE_STRING = 30,                /* ICE_STRING  */
  YYSYMBOL_ICE_OBJECT = 31,                /* ICE_OBJECT  */
  YYSYMBOL_ICE_CONST = 32,                 /* ICE_CONST  */
  YYSYMBOL_ICE_FALSE = 33,                 /* ICE_FALSE  */
  YYSYMBOL_ICE_TRUE = 34,                  /* ICE_TRUE  */
  YYSYMBOL_ICE_IDEMPOTENT = 35,            /* ICE_IDEMPOTENT  */
  YYSYMBOL_ICE_TAG = 36,                   /* ICE_TAG  */
  YYSYMBOL_ICE_OPTIONAL = 37,              /* ICE_OPTIONAL  */
  YYSYMBOL_ICE_ANYCLASS = 38,              /* ICE_ANYCLASS  */
  YYSYMBOL_ICE_VALUE = 39,                 /* ICE_VALUE  */
  YYSYMBOL_ICE_UNCHECKED = 40,             /* ICE_UNCHECKED  */
  YYSYMBOL_ICE_STRING_LITERAL = 41,        /* ICE_STRING_LITERAL  */
  YYSYMBOL_ICE_INTEGER_LITERAL = 42,       /* ICE_INTEGER_LITERAL  */
  YYSYMBOL_ICE_FLOATING_POINT_LITERAL = 43, /* ICE_FLOATING_POINT_LITERAL  */
  YYSYMBOL_ICE_IDENTIFIER = 44,            /* ICE_IDENTIFIER  */
  YYSYMBOL_ICE_SCOPED_IDENTIFIER = 45,     /* ICE_SCOPED_IDENTIFIER  */
  YYSYMBOL_ICE_LOCAL_METADATA_OPEN = 46,   /* ICE_LOCAL_METADATA_OPEN  */
  YYSYMBOL_ICE_LOCAL_METADATA_CLOSE = 47,  /* ICE_LOCAL_METADATA_CLOSE  */
  YYSYMBOL_ICE_FILE_METADATA_OPEN = 48,    /* ICE_FILE_METADATA_OPEN  */
  YYSYMBOL_ICE_FILE_METADATA_IGNORE = 49,  /* ICE_FILE_METADATA_IGNORE  */
  YYSYMBOL_ICE_FILE_METADATA_CLOSE = 50,   /* ICE_FILE_METADATA_CLOSE  */
  YYSYMBOL_ICE_IDENT_OPEN = 51,            /* ICE_IDENT_OPEN  */
  YYSYMBOL_ICE_KEYWORD_OPEN = 52,          /* ICE_KEYWORD_OPEN  */
  YYSYMBOL_ICE_TAG_OPEN = 53,              /* ICE_TAG_OPEN  */
  YYSYMBOL_ICE_OPTIONAL_OPEN = 54,         /* ICE_OPTIONAL_OPEN  */
  YYSYMBOL_BAD_CHAR = 55,                  /* BAD_CHAR  */
  YYSYMBOL_56_ = 56,                       /* ';'  */
  YYSYMBOL_57_ = 57,                       /* '{'  */
  YYSYMBOL_58_ = 58,                       /* '}'  */
  YYSYMBOL_59_ = 59,                       /* ')'  */
  YYSYMBOL_60_ = 60,                       /* ':'  */
  YYSYMBOL_61_ = 61,                       /* '='  */
  YYSYMBOL_62_ = 62,                       /* ','  */
  YYSYMBOL_63_ = 63,                       /* '('  */
  YYSYMBOL_64_ = 64,                       /* '<'  */
  YYSYMBOL_65_ = 65,                       /* '>'  */
  YYSYMBOL_66_ = 66,                       /* '*'  */
  YYSYMBOL_67_ = 67,                       /* '?'  */
  YYSYMBOL_YYACCEPT = 68,                  /* $accept  */
  YYSYMBOL_start = 69,                     /* start  */
  YYSYMBOL_opt_semicolon = 70,             /* opt_semicolon  */
  YYSYMBOL_file_metadata = 71,             /* file_metadata  */
  YYSYMBOL_local_metadata = 72,            /* local_metadata  */
  YYSYMBOL_definitions = 73,               /* definitions  */
  YYSYMBOL_definition = 74,                /* definition  */
  YYSYMBOL_75_1 = 75,                      /* $@1  */
  YYSYMBOL_76_2 = 76,                      /* $@2  */
  YYSYMBOL_77_3 = 77,                      /* $@3  */
  YYSYMBOL_78_4 = 78,                      /* $@4  */
  YYSYMBOL_79_5 = 79,                      /* $@5  */
  YYSYMBOL_80_6 = 80,                      /* $@6  */
  YYSYMBOL_81_7 = 81,                      /* $@7  */
  YYSYMBOL_82_8 = 82,                      /* $@8  */
  YYSYMBOL_83_9 = 83,                      /* $@9  */
  YYSYMBOL_84_10 = 84,                     /* $@10  */
  YYSYMBOL_85_11 = 85,                     /* $@11  */
  YYSYMBOL_86_12 = 86,                     /* $@12  */
  YYSYMBOL_87_13 = 87,                     /* $@13  */
  YYSYMBOL_module_def = 88,                /* module_def  */
  YYSYMBOL_89_14 = 89,                     /* @14  */
  YYSYMBOL_90_15 = 90,                     /* @15  */
  YYSYMBOL_exception_id = 91,              /* exception_id  */
  YYSYMBOL_exception_decl = 92,            /* exception_decl  */
  YYSYMBOL_exception_def = 93,             /* exception_def  */
  YYSYMBOL_94_16 = 94,                     /* @16  */
  YYSYMBOL_exception_extends = 95,         /* exception_extends  */
  YYSYMBOL_tag = 96,                       /* tag  */
  YYSYMBOL_optional = 97,                  /* optional  */
  YYSYMBOL_struct_id = 98,                 /* struct_id  */
  YYSYMBOL_struct_decl = 99,               /* struct_decl  */
  YYSYMBOL_struct_def = 100,               /* struct_def  */
  YYSYMBOL_101_17 = 101,                   /* @17  */
  YYSYMBOL_class_name = 102,               /* class_name  */
  YYSYMBOL_class_id = 103,                 /* class_id  */
  YYSYMBOL_class_decl = 104,               /* class_decl  */
  YYSYMBOL_class_def = 105,                /* class_def  */
  YYSYMBOL_106_18 = 106,                   /* @18  */
  YYSYMBOL_class_extends = 107,            /* class_extends  */
  YYSYMBOL_extends = 108,                  /* extends  */
  YYSYMBOL_data_member = 109,              /* data_member  */
  YYSYMBOL_data_member_list = 110,         /* data_member_list  */
  YYSYMBOL_data_members = 111,             /* data_members  */
  YYSYMBOL_return_tuple = 112,             /* return_tuple  */
  YYSYMBOL_return_type = 113,              /* return_type  */
  YYSYMBOL_operation_preamble = 114,       /* operation_preamble  */
  YYSYMBOL_operation = 115,                /* operation  */
  YYSYMBOL_116_19 = 116,                   /* @19  */
  YYSYMBOL_117_20 = 117,                   /* @20  */
  YYSYMBOL_operation_list = 118,           /* operation_list  */
  YYSYMBOL_interface_id = 119,             /* interface_id  */
  YYSYMBOL_interface_decl = 120,           /* interface_decl  */
  YYSYMBOL_interface_def = 121,            /* interface_def  */
  YYSYMBOL_122_21 = 122,                   /* @21  */
  YYSYMBOL_interface_list = 123,           /* interface_list  */
  YYSYMBOL_interface_extends = 124,        /* interface_extends  */
  YYSYMBOL_exception_list = 125,           /* exception_list  */
  YYSYMBOL_exception = 126,                /* exception  */
  YYSYMBOL_sequence_def = 127,             /* sequence_def  */
  YYSYMBOL_dictionary_def = 128,           /* dictionary_def  */
  YYSYMBOL_enum_start = 129,               /* enum_start  */
  YYSYMBOL_enum_id = 130,                  /* enum_id  */
  YYSYMBOL_enum_def = 131,                 /* enum_def  */
  YYSYMBOL_132_22 = 132,                   /* @22  */
  YYSYMBOL_133_23 = 133,                   /* @23  */
  YYSYMBOL_enum_underlying = 134,          /* enum_underlying  */
  YYSYMBOL_enumerator_list = 135,          /* enumerator_list  */
  YYSYMBOL_enumerator = 136,               /* enumerator  */
  YYSYMBOL_enumerator_initializer = 137,   /* enumerator_initializer  */
  YYSYMBOL_out_qualifier = 138,            /* out_qualifier  */
  YYSYMBOL_parameter = 139,                /* parameter  */
  YYSYMBOL_parameter_list = 140,           /* parameter_list  */
  YYSYMBOL_parameters = 141,               /* parameters  */
  YYSYMBOL_throws = 142,                   /* throws  */
  YYSYMBOL_scoped_name = 143,              /* scoped_name  */
  YYSYMBOL_builtin = 144,                  /* builtin  */
  YYSYMBOL_type = 145,                     /* type  */
  YYSYMBOL_tagged_type = 146,              /* tagged_type  */
  YYSYMBOL_member = 147,                   /* member  */
  YYSYMBOL_string_literal = 148,           /* string_literal  */
  YYSYMBOL_string_list = 149,              /* string_list  */
  YYSYMBOL_const_initializer = 150,        /* const_initializer  */
  YYSYMBOL_const_def = 151,                /* const_def  */
  YYSYMBOL_keyword = 152                   /* keyword  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;



/* Unqualified %code blocks.  */
#line 61 "src/Slice/Grammar.y"


// Forward declaration of the lexing function generated by flex, so bison knows about it.
// This must match the definition of 'yylex' (or 'slice_lex') in the generated scanner.
int slice_lex(YYSTYPE* lvalp, YYLTYPE* llocp);


#line 363 "src/Slice/Grammar.cpp"

#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1051

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  85
/* YYNRULES -- Number of rules.  */
#define YYNRULES  240
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  340

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   310


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      63,    59,    66,     2,    62,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    60,    56,
      64,    61,    65,    67,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,     2,    58,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   195,   195,   201,   202,   208,   212,   222,   226,   233,
     242,   250,   259,   266,   265,   271,   270,   275,   280,   279,
     285,   284,   289,   294,   293,   299,   298,   303,   308,   307,
     313,   312,   317,   322,   321,   327,   326,   331,   336,   335,
     340,   345,   344,   350,   349,   354,   358,   368,   367,   406,
     405,   483,   487,   498,   509,   508,   534,   542,   551,   569,
     643,   649,   660,   682,   760,   770,   785,   789,   800,   811,
     810,   851,   855,   866,   891,   981,   993,  1006,  1005,  1039,
    1073,  1082,  1083,  1089,  1104,  1126,  1127,  1128,  1132,  1138,
    1139,  1145,  1157,  1173,  1188,  1197,  1202,  1211,  1246,  1281,
    1315,  1355,  1354,  1377,  1376,  1399,  1417,  1421,  1422,  1428,
    1432,  1443,  1457,  1456,  1490,  1525,  1560,  1565,  1570,  1584,
    1588,  1597,  1604,  1616,  1628,  1639,  1647,  1661,  1671,  1687,
    1691,  1700,  1716,  1730,  1729,  1752,  1751,  1770,  1774,  1783,
    1789,  1795,  1807,  1820,  1827,  1837,  1841,  1879,  1883,  1892,
    1911,  1912,  1918,  1919,  1925,  1929,  1938,  1939,  1945,  1946,
    1947,  1948,  1949,  1950,  1951,  1952,  1953,  1954,  1955,  1956,
    1957,  1958,  1959,  1964,  1968,  1972,  1976,  1984,  1989,  2000,
    2004,  2016,  2021,  2047,  2079,  2107,  2122,  2136,  2147,  2159,
    2170,  2181,  2192,  2198,  2204,  2211,  2223,  2232,  2241,  2281,
    2288,  2295,  2307,  2316,  2330,  2331,  2332,  2333,  2334,  2335,
    2336,  2337,  2338,  2339,  2340,  2341,  2342,  2343,  2344,  2345,
    2346,  2347,  2348,  2349,  2350,  2351,  2352,  2353,  2354,  2355,
    2356,  2357,  2358,  2359,  2360,  2361,  2362,  2363,  2364,  2365,
    2366
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ICE_MODULE",
  "ICE_CLASS", "ICE_INTERFACE", "ICE_EXCEPTION", "ICE_STRUCT",
  "ICE_SEQUENCE", "ICE_DICTIONARY", "ICE_ENUM", "ICE_OUT", "ICE_EXTENDS",
  "ICE_IMPLEMENTS", "ICE_THROWS", "ICE_VOID", "ICE_BOOL", "ICE_BYTE",
  "ICE_SHORT", "ICE_USHORT", "ICE_INT", "ICE_UINT", "ICE_VARINT",
  "ICE_VARUINT", "ICE_LONG", "ICE_ULONG", "ICE_VARLONG", "ICE_VARULONG",
  "ICE_FLOAT", "ICE_DOUBLE", "ICE_STRING", "ICE_OBJECT", "ICE_CONST",
  "ICE_FALSE", "ICE_TRUE", "ICE_IDEMPOTENT", "ICE_TAG", "ICE_OPTIONAL",
  "ICE_ANYCLASS", "ICE_VALUE", "ICE_UNCHECKED", "ICE_STRING_LITERAL",
  "ICE_INTEGER_LITERAL", "ICE_FLOATING_POINT_LITERAL", "ICE_IDENTIFIER",
  "ICE_SCOPED_IDENTIFIER", "ICE_LOCAL_METADATA_OPEN",
  "ICE_LOCAL_METADATA_CLOSE", "ICE_FILE_METADATA_OPEN",
  "ICE_FILE_METADATA_IGNORE", "ICE_FILE_METADATA_CLOSE", "ICE_IDENT_OPEN",
  "ICE_KEYWORD_OPEN", "ICE_TAG_OPEN", "ICE_OPTIONAL_OPEN", "BAD_CHAR",
  "';'", "'{'", "'}'", "')'", "':'", "'='", "','", "'('", "'<'", "'>'",
  "'*'", "'?'", "$accept", "start", "opt_semicolon", "file_metadata",
  "local_metadata", "definitions", "definition", "$@1", "$@2", "$@3",
  "$@4", "$@5", "$@6", "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13",
  "module_def", "@14", "@15", "exception_id", "exception_decl",
  "exception_def", "@16", "exception_extends", "tag", "optional",
  "struct_id", "struct_decl", "struct_def", "@17", "class_name",
  "class_id", "class_decl", "class_def", "@18", "class_extends", "extends",
  "data_member", "data_member_list", "data_members", "return_tuple",
  "return_type", "operation_preamble", "operation", "@19", "@20",
  "operation_list", "interface_id", "interface_decl", "interface_def",
  "@21", "interface_list", "interface_extends", "exception_list",
  "exception", "sequence_def", "dictionary_def", "enum_start", "enum_id",
  "enum_def", "@22", "@23", "enum_underlying", "enumerator_list",
  "enumerator", "enumerator_initializer", "out_qualifier", "parameter",
  "parameter_list", "parameters", "throws", "scoped_name", "builtin",
  "type", "tagged_type", "member", "string_literal", "string_list",
  "const_initializer", "const_def", "keyword", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,    59,   123,   125,    41,
      58,    61,    44,    40,    60,    62,    42,    63
};
#endif

#define YYPACT_NINF (-278)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-154)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -278,    10,    35,  -278,   -24,   -24,   -24,  -278,   202,   -24,
    -278,   -32,   -31,    80,   -12,    49,   384,   574,   616,   658,
      16,    18,  -278,    27,    77,   -24,  -278,  -278,    31,    42,
    -278,   -34,    61,  -278,    53,     4,    93,  -278,    54,   114,
    -278,   120,   123,   700,    55,  -278,   127,  -278,  -278,   -24,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,   106,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,    27,    27,   975,  -278,    50,   129,  -278,  -278,  -278,
     100,   132,   129,    79,   163,   129,  -278,   100,   164,   129,
      94,  -278,   172,   129,   173,   174,  -278,   135,  -278,  1006,
    -278,   129,   175,  -278,   179,   181,   182,  -278,  -278,   185,
     975,   975,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,    74,   178,   201,
     102,   203,   -33,  -278,  -278,  -278,   183,  -278,  -278,  -278,
     236,  -278,  -278,   189,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,   177,   190,  -278,  -278,  -278,  -278,   742,  -278,   192,
    -278,  -278,  -278,  -278,  -278,  -278,   204,   209,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,   215,   180,   236,   221,  -278,
    -278,    57,    84,   897,  1006,  1006,   222,   282,   226,  -278,
     784,   218,   236,    94,    32,   225,   229,   230,  -278,   742,
     138,   169,   826,    27,   180,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,   233,  -278,   234,  -278,   237,   257,  -278,   258,
     -24,  -278,  -278,  -278,  -278,   328,  -278,  -278,  -278,   180,
     265,  -278,   232,   421,   266,   149,  -278,   742,   267,  -278,
    -278,  -278,  -278,   975,  -278,  -278,  -278,  -278,  -278,  -278,
      59,  -278,  -278,    32,  -278,   470,    13,   146,   136,   239,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,   268,  -278,  -278,
     148,  -278,  -278,   -41,   936,  -278,  -278,   271,   936,  -278,
     269,   273,    32,   868,  -278,  -278,  -278,   323,  -278,  -278,
    -278,   323,  -278,  -278,  -278,  -278,   936,   324,  -278,   324,
    -278,   531,  -278,  -278,  -278,   275,  -278,  -278,   531,  -278
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      12,     0,     9,     1,     0,     0,     0,    10,     0,   193,
     195,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   130,     9,     0,     0,    11,    13,    53,    27,
      28,    68,    32,    33,    76,    80,    17,    18,   111,    22,
      23,    37,    40,   135,   138,    41,    45,   192,     7,     0,
       5,     6,    46,    47,    49,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,    71,     0,    72,   109,   110,    51,    52,    66,
      67,     9,     9,     0,   129,     0,     4,    81,    82,    54,
       0,     0,     4,     0,     0,     4,    77,     0,     0,     4,
       0,   112,     0,     4,     0,     0,   131,     0,   132,     0,
     133,     4,     0,   194,     0,     0,     0,   156,   157,     0,
       0,     0,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   178,   179,   180,
     182,   181,     0,     8,     3,    14,     0,    56,    26,    29,
       0,    31,    34,     0,    79,    16,    19,   116,   117,   118,
     119,   115,     0,    21,    24,    36,    39,   144,   137,     0,
      42,    44,    12,    12,    73,    74,     0,     0,   173,   174,
     175,   176,   183,   184,   177,     0,     0,     0,     0,    61,
      65,     0,     0,     0,     0,     0,    87,     0,     0,   187,
     190,    83,     0,     0,     0,   141,     0,   140,   143,   144,
       9,     9,     0,     9,     0,   200,   201,   199,   196,   197,
     198,   203,     0,    86,     0,    60,     0,     0,    64,     0,
       0,   191,   185,   186,    85,     0,    70,   188,   189,     0,
       0,   114,     0,     0,     0,     0,   136,   144,     0,    48,
      50,   125,   126,     0,   202,    55,    58,    59,    62,    63,
       0,    84,    78,     0,    96,     0,   148,     0,     0,   106,
      93,   113,   145,   142,   146,   139,   134,     0,     7,   107,
       0,   147,    95,     0,     0,    97,    99,     0,     0,   150,
     152,     0,     0,     0,    98,   100,    94,   148,    91,   103,
     149,   148,   101,   105,   127,   128,     0,   155,   151,   155,
      92,     0,   104,   102,   154,   122,   123,   124,     0,   121
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -278,  -278,   -23,  -278,     2,     9,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
      96,  -278,  -181,  -100,  -278,    56,  -278,  -278,  -278,  -278,
    -271,  -278,  -278,  -278,  -278,   116,  -278,     5,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -192,  -278,  -278,
    -277,    21,  -278,  -278,    33,   -88,  -278,   -95,  -243,  -199,
      30,     1,  -221,  -278,   -16
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,   165,     7,   213,     2,    26,   106,   118,   119,
     122,   123,   111,   112,   114,   115,   124,   125,   131,   132,
      27,   134,   135,    28,    29,    30,   166,   109,   214,   215,
      31,    32,    33,   113,    34,    35,    36,    37,   173,   116,
     110,   216,   217,   218,   303,   287,   288,   289,   329,   327,
     264,    38,    39,    40,   182,   180,   121,   334,   335,    41,
      42,    43,    44,    45,   189,   127,   130,   226,   227,   293,
     308,   309,   310,   311,   332,   160,   161,   219,   220,   221,
      10,    11,   241,    46,   228
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      94,    96,    98,   100,     8,   139,    12,    13,   162,   304,
       3,   205,   299,   274,   251,    48,   107,     9,   316,    50,
     290,   317,   167,   -69,   301,   103,   105,   128,   206,   174,
      49,    49,   181,   262,   188,    -2,   255,   268,   281,    47,
     326,   323,   290,   107,    52,   196,   197,    -9,    -9,    -9,
      -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,
      -9,    -9,    -9,    -9,   108,   -75,   107,    -9,    -9,    -9,
      -9,    -9,   302,     4,   255,   295,    -9,    -9,     4,   133,
     101,     4,   102,     5,     6,    -9,    -9,   104,   -57,   169,
    -108,   108,   172,    53,    54,    -9,   176,   163,   -25,   244,
     184,   137,   138,   140,   141,   318,   298,   242,   190,   320,
     -75,  -120,    49,   -75,   108,   129,   245,   -30,   240,   252,
     253,    49,   260,   246,   249,   177,   247,   330,   137,   138,
      51,   117,   178,   179,   120,   181,   170,   307,   137,   138,
     198,   199,    49,   248,   137,   138,   240,   301,   136,   -15,
     137,   138,  -148,  -148,  -148,  -148,  -148,  -148,  -148,  -148,
    -148,  -148,  -148,  -148,  -148,  -148,  -148,  -148,   202,   203,
     -20,   240,  -148,  -148,  -148,  -148,   -35,   294,   297,   -38,
    -148,  -148,  -148,   -43,     4,   164,     5,     6,   168,  -148,
    -148,   292,   187,   137,   138,  -153,   269,   305,   306,   314,
     315,   230,   231,    14,   258,    15,    16,    17,    18,    19,
      20,    21,    22,   235,   236,     4,   272,     5,     6,   171,
     175,   237,   238,   239,   137,   138,   263,   270,   183,   185,
     186,   191,     8,     8,    23,   273,   192,   208,   193,   223,
     207,   194,    24,   336,   195,   200,   222,   224,    25,   229,
     336,   280,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   201,   232,
     204,   233,   209,   210,   158,   159,   234,   243,   254,   259,
     137,   138,     4,   208,   256,   263,   265,   266,   283,   211,
     212,   275,   267,   276,   -90,   312,   277,   325,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   263,   337,   278,   279,   209,   210,
     158,   159,   337,   282,   291,   296,   137,   138,     4,   208,
     319,   321,   322,   313,   301,   211,   212,   338,   331,   261,
     -89,   300,   328,   339,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
       0,     0,   333,     0,   209,   210,   158,   159,     0,     0,
       0,     0,   137,   138,     4,     0,     0,     0,     0,     0,
       0,   211,   212,     0,     0,     0,   -88,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,     0,     0,     0,     0,    92,     0,
       0,     0,     0,     0,     0,    93,   284,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,     0,     0,     0,   285,   209,   210,   158,
     159,     0,     0,     0,     0,   137,   138,    25,     0,     0,
       0,     0,     0,     0,   211,   212,     0,     0,     0,     0,
       0,     0,     0,     0,   286,   284,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,     0,     0,     0,     0,   209,   210,   158,   159,
       0,     0,     0,     0,   137,   138,     0,     0,     0,     0,
       0,     0,     0,   211,   212,     0,     0,     0,     0,     0,
       0,     0,     0,   286,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,     0,     0,     0,     0,   137,   138,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,     0,     0,     0,     0,    95,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,     0,     0,     0,     0,
      97,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,     0,     0,
       0,     0,    99,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       0,     0,     0,     0,   126,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,     0,     0,     0,     0,   225,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,     0,     0,     0,     0,   257,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,     0,     0,     0,     0,
     271,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,     0,     0,
       0,     0,   324,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,     0,
       0,     0,     0,   209,   210,   158,   159,     0,     0,     0,
       0,   137,   138,   250,     0,     0,     0,     0,     0,     0,
     211,   212,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,     0,     0,
       0,     0,   209,   210,   158,   159,     0,     0,     0,     0,
     137,   138,     4,     0,     0,     0,     0,     0,     0,   211,
     212,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     0,     0,     0,
       0,     0,     0,   158,   159,     0,     0,     0,     0,   137,
     138,    25,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,     0,     0,
       0,     0,     0,     0,   158,   159,     0,     0,     0,     0,
     137,   138
};

static const yytype_int16 yycheck[] =
{
      16,    17,    18,    19,     2,    93,     5,     6,   103,   286,
       0,    44,   283,   234,   213,    47,    12,    41,    59,    50,
     263,    62,   110,    57,    11,    23,    25,    43,    61,   117,
      62,    62,   120,     1,   129,     0,   217,   229,   259,     9,
     317,   312,   285,    12,    56,   140,   141,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    60,    12,    12,    35,    36,    37,
      38,    39,    59,    46,   255,   267,    44,    45,    46,    49,
      64,    46,    64,    48,    49,    53,    54,    10,    57,   112,
      58,    60,   115,    44,    45,    63,   119,    47,    56,    42,
     123,    44,    45,   101,   102,   304,    47,   207,   131,   308,
      57,    57,    62,    60,    60,    60,    59,    56,   206,   214,
     215,    62,   222,   211,   212,    31,    42,   326,    44,    45,
      50,    35,    38,    39,    38,   223,    57,     1,    44,    45,
      66,    67,    62,    59,    44,    45,   234,    11,    42,    56,
      44,    45,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    66,    67,
      56,   259,    36,    37,    38,    39,    56,   265,   273,    56,
      44,    45,    46,    56,    46,    56,    48,    49,    56,    53,
      54,    42,    57,    44,    45,    59,    58,    51,    52,    51,
      52,   192,   193,     1,   220,     3,     4,     5,     6,     7,
       8,     9,    10,    33,    34,    46,   232,    48,    49,    56,
      56,    41,    42,    43,    44,    45,   224,    58,    56,    56,
      56,    56,   230,   231,    32,   233,    57,     1,    57,    62,
      57,    59,    40,   331,    59,    67,    57,    57,    46,    57,
     338,   250,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    67,    65,
      67,    62,    36,    37,    38,    39,    61,    56,    56,    61,
      44,    45,    46,     1,    58,   283,    61,    58,    56,    53,
      54,    58,    62,    59,    58,    56,    59,   313,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,   312,   331,    59,    59,    36,    37,
      38,    39,   338,    58,    58,    58,    44,    45,    46,     1,
      59,    62,    59,    65,    11,    53,    54,    62,    14,   223,
      58,   285,   321,   338,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,   329,    -1,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    53,    54,    -1,    -1,    -1,    58,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    51,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    35,    36,    37,    38,
      39,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    53,    54,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    63,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    44,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    53,    54,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    63,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    -1,    -1,    -1,    -1,    44,    45,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    -1,    44,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      44,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    44,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    44,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    -1,    44,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    -1,    44,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      44,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    44,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      -1,    -1,    -1,    36,    37,    38,    39,    -1,    -1,    -1,
      -1,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      53,    54,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    53,
      54,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    39,    -1,    -1,    -1,    -1,    44,
      45,    46,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    39,    -1,    -1,    -1,    -1,
      44,    45
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    69,    73,     0,    46,    48,    49,    71,    72,    41,
     148,   149,   149,   149,     1,     3,     4,     5,     6,     7,
       8,     9,    10,    32,    40,    46,    74,    88,    91,    92,
      93,    98,    99,   100,   102,   103,   104,   105,   119,   120,
     121,   127,   128,   129,   130,   131,   151,   148,    47,    62,
      50,    50,    56,    44,    45,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    44,    51,   152,    44,   152,    44,   152,    44,
     152,    64,    64,    72,    10,   149,    75,    12,    60,    95,
     108,    80,    81,   101,    82,    83,   107,   108,    76,    77,
     108,   124,    78,    79,    84,    85,    44,   133,   152,    60,
     134,    86,    87,   148,    89,    90,    42,    44,    45,   143,
      72,    72,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    38,    39,
     143,   144,   145,    47,    56,    70,    94,   143,    56,    70,
      57,    56,    70,   106,   143,    56,    70,    31,    38,    39,
     123,   143,   122,    56,    70,    56,    56,    57,   145,   132,
      70,    56,    57,    57,    59,    59,   145,   145,    66,    67,
      67,    67,    66,    67,    67,    44,    61,    57,     1,    36,
      37,    53,    54,    72,    96,    97,   109,   110,   111,   145,
     146,   147,    57,    62,    57,    44,   135,   136,   152,    57,
      73,    73,    65,    62,    61,    33,    34,    41,    42,    43,
     143,   150,   111,    56,    42,    59,   143,    42,    59,   143,
      46,   147,   145,   145,    56,   110,    58,    44,   152,    61,
     111,   123,     1,    72,   118,    61,    58,    62,   135,    58,
      58,    44,   152,    72,   150,    58,    59,    59,    59,    59,
     149,   150,    58,    56,    15,    35,    63,   113,   114,   115,
     146,    58,    42,   137,   143,   135,    58,   145,    47,   118,
     113,    11,    59,   112,   138,    51,    52,     1,   138,   139,
     140,   141,    56,    65,    51,    52,    59,    62,   147,    59,
     147,    62,    59,   118,    44,   152,   138,   117,   139,   116,
     147,    14,   142,   142,   125,   126,   143,   152,    62,   125
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    68,    69,    70,    70,    71,    71,    72,    72,    72,
      73,    73,    73,    75,    74,    76,    74,    74,    77,    74,
      78,    74,    74,    79,    74,    80,    74,    74,    81,    74,
      82,    74,    74,    83,    74,    84,    74,    74,    85,    74,
      74,    86,    74,    87,    74,    74,    74,    89,    88,    90,
      88,    91,    91,    92,    94,    93,    95,    95,    96,    96,
      96,    96,    97,    97,    97,    97,    98,    98,    99,   101,
     100,   102,   102,   103,   103,   103,   104,   106,   105,   107,
     107,   108,   108,   109,   109,   110,   110,   110,   110,   111,
     111,   112,   112,   113,   113,   113,   113,   114,   114,   114,
     114,   116,   115,   117,   115,   118,   118,   118,   118,   119,
     119,   120,   122,   121,   123,   123,   123,   123,   123,   124,
     124,   125,   125,   126,   126,   127,   127,   128,   128,   129,
     129,   130,   130,   132,   131,   133,   131,   134,   134,   135,
     135,   136,   136,   136,   136,   137,   137,   138,   138,   139,
     140,   140,   141,   141,   142,   142,   143,   143,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   145,   146,   146,   146,   147,   147,
     147,   147,   148,   148,   149,   149,   150,   150,   150,   150,
     150,   150,   151,   151,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     152
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     0,     3,     3,     3,     4,     0,
       2,     3,     0,     0,     3,     0,     3,     1,     0,     3,
       0,     3,     1,     0,     3,     0,     3,     1,     0,     3,
       0,     3,     1,     0,     3,     0,     3,     1,     0,     3,
       1,     0,     3,     0,     3,     1,     2,     0,     6,     0,
       6,     2,     2,     1,     0,     6,     2,     0,     3,     3,
       2,     1,     3,     3,     2,     1,     2,     2,     1,     0,
       5,     2,     2,     4,     4,     1,     1,     0,     6,     2,
       0,     1,     1,     1,     3,     2,     2,     1,     2,     1,
       0,     2,     4,     1,     3,     2,     1,     2,     3,     2,
       3,     0,     5,     0,     5,     4,     2,     3,     0,     2,
       2,     1,     0,     6,     3,     1,     1,     1,     1,     2,
       0,     3,     1,     1,     1,     6,     6,     9,     9,     2,
       1,     2,     2,     0,     6,     0,     5,     2,     0,     3,
       1,     1,     3,     1,     0,     1,     1,     1,     0,     2,
       1,     3,     1,     0,     2,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     2,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     1,     2,     2,
       1,     2,     2,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     6,     5,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YY_LOCATION_PRINT
#  if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#   define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

#  else
#   define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#  endif
# endif /* !defined YY_LOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YY_LOCATION_PRINT (yyo, *yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 5: /* file_metadata: ICE_FILE_METADATA_OPEN string_list ICE_FILE_METADATA_CLOSE  */
#line 209 "src/Slice/Grammar.y"
{
    yyval = yyvsp[-1];
}
#line 1845 "src/Slice/Grammar.cpp"
    break;

  case 6: /* file_metadata: ICE_FILE_METADATA_IGNORE string_list ICE_FILE_METADATA_CLOSE  */
#line 213 "src/Slice/Grammar.y"
{
    unit->error("file metadata must appear before any definitions");
    yyval = yyvsp[-1]; // Dummy
}
#line 1854 "src/Slice/Grammar.cpp"
    break;

  case 7: /* local_metadata: ICE_LOCAL_METADATA_OPEN string_list ICE_LOCAL_METADATA_CLOSE  */
#line 223 "src/Slice/Grammar.y"
{
    yyval = yyvsp[-1];
}
#line 1862 "src/Slice/Grammar.cpp"
    break;

  case 8: /* local_metadata: local_metadata ICE_LOCAL_METADATA_OPEN string_list ICE_LOCAL_METADATA_CLOSE  */
#line 227 "src/Slice/Grammar.y"
{
    StringListTokPtr metadata1 = StringListTokPtr::dynamicCast(yyvsp[-3]);
    StringListTokPtr metadata2 = StringListTokPtr::dynamicCast(yyvsp[-1]);
    metadata1->v.splice(metadata1->v.end(), metadata2->v);
    yyval = metadata1;
}
#line 1873 "src/Slice/Grammar.cpp"
    break;

  case 9: /* local_metadata: %empty  */
#line 234 "src/Slice/Grammar.y"
{
    yyval = new StringListTok;
}
#line 1881 "src/Slice/Grammar.cpp"
    break;

  case 10: /* definitions: definitions file_metadata  */
#line 243 "src/Slice/Grammar.y"
{
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[0]);
    if(!metaData->v.empty())
    {
        unit->addFileMetaData(metaData->v);
    }
}
#line 1893 "src/Slice/Grammar.cpp"
    break;

  case 11: /* definitions: definitions local_metadata definition  */
#line 251 "src/Slice/Grammar.y"
{
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[-1]);
    ContainedPtr contained = ContainedPtr::dynamicCast(yyvsp[0]);
    if(contained && !metaData->v.empty())
    {
        contained->setMetaData(metaData->v);
    }
}
#line 1906 "src/Slice/Grammar.cpp"
    break;

  case 13: /* $@1: %empty  */
#line 266 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || ModulePtr::dynamicCast(yyvsp[0]));
}
#line 1914 "src/Slice/Grammar.cpp"
    break;

  case 15: /* $@2: %empty  */
#line 271 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || ClassDeclPtr::dynamicCast(yyvsp[0]));
}
#line 1922 "src/Slice/Grammar.cpp"
    break;

  case 17: /* definition: class_decl  */
#line 276 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after class forward declaration");
}
#line 1930 "src/Slice/Grammar.cpp"
    break;

  case 18: /* $@3: %empty  */
#line 280 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || ClassDefPtr::dynamicCast(yyvsp[0]));
}
#line 1938 "src/Slice/Grammar.cpp"
    break;

  case 20: /* $@4: %empty  */
#line 285 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || InterfaceDeclPtr::dynamicCast(yyvsp[0]));
}
#line 1946 "src/Slice/Grammar.cpp"
    break;

  case 22: /* definition: interface_decl  */
#line 290 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after interface forward declaration");
}
#line 1954 "src/Slice/Grammar.cpp"
    break;

  case 23: /* $@5: %empty  */
#line 294 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || InterfaceDefPtr::dynamicCast(yyvsp[0]));
}
#line 1962 "src/Slice/Grammar.cpp"
    break;

  case 25: /* $@6: %empty  */
#line 299 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0);
}
#line 1970 "src/Slice/Grammar.cpp"
    break;

  case 27: /* definition: exception_decl  */
#line 304 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after exception forward declaration");
}
#line 1978 "src/Slice/Grammar.cpp"
    break;

  case 28: /* $@7: %empty  */
#line 308 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || ExceptionPtr::dynamicCast(yyvsp[0]));
}
#line 1986 "src/Slice/Grammar.cpp"
    break;

  case 30: /* $@8: %empty  */
#line 313 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0);
}
#line 1994 "src/Slice/Grammar.cpp"
    break;

  case 32: /* definition: struct_decl  */
#line 318 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after struct forward declaration");
}
#line 2002 "src/Slice/Grammar.cpp"
    break;

  case 33: /* $@9: %empty  */
#line 322 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || StructPtr::dynamicCast(yyvsp[0]));
}
#line 2010 "src/Slice/Grammar.cpp"
    break;

  case 35: /* $@10: %empty  */
#line 327 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || SequencePtr::dynamicCast(yyvsp[0]));
}
#line 2018 "src/Slice/Grammar.cpp"
    break;

  case 37: /* definition: sequence_def  */
#line 332 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after sequence definition");
}
#line 2026 "src/Slice/Grammar.cpp"
    break;

  case 38: /* $@11: %empty  */
#line 336 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || DictionaryPtr::dynamicCast(yyvsp[0]));
}
#line 2034 "src/Slice/Grammar.cpp"
    break;

  case 40: /* definition: dictionary_def  */
#line 341 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after dictionary definition");
}
#line 2042 "src/Slice/Grammar.cpp"
    break;

  case 41: /* $@12: %empty  */
#line 345 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || EnumPtr::dynamicCast(yyvsp[0]));
}
#line 2050 "src/Slice/Grammar.cpp"
    break;

  case 43: /* $@13: %empty  */
#line 350 "src/Slice/Grammar.y"
{
    assert(yyvsp[0] == 0 || ConstPtr::dynamicCast(yyvsp[0]));
}
#line 2058 "src/Slice/Grammar.cpp"
    break;

  case 45: /* definition: const_def  */
#line 355 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after const definition");
}
#line 2066 "src/Slice/Grammar.cpp"
    break;

  case 46: /* definition: error ';'  */
#line 359 "src/Slice/Grammar.y"
{
    yyerrok;
}
#line 2074 "src/Slice/Grammar.cpp"
    break;

  case 47: /* @14: %empty  */
#line 368 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainerPtr cont = unit->currentContainer();

    ModulePtr module;
    if (UnitPtr ut = UnitPtr::dynamicCast(cont))
    {
        module = ut->createModule(ident->v);
    }
    else if (ModulePtr mod = ModulePtr::dynamicCast(cont))
    {
        module = mod->createModule(ident->v);
    }

    if(module)
    {
        cont->checkIntroduced(ident->v, module);
        unit->pushContainer(module);
        yyval = module;
    }
    else
    {
        yyval = 0;
    }
}
#line 2104 "src/Slice/Grammar.cpp"
    break;

  case 48: /* module_def: ICE_MODULE ICE_IDENTIFIER @14 '{' definitions '}'  */
#line 394 "src/Slice/Grammar.y"
{
    if(yyvsp[-3])
    {
        unit->popContainer();
        yyval = yyvsp[-3];
    }
    else
    {
        yyval = 0;
    }
}
#line 2120 "src/Slice/Grammar.cpp"
    break;

  case 49: /* @15: %empty  */
#line 406 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);

    // Split the 'scoped identifier' into separate module names.
    vector<string> modules;
    size_t startPos = 0;
    size_t endPos;
    while((endPos = ident->v.find("::", startPos)) != string::npos)
    {
        modules.push_back(ident->v.substr(startPos, (endPos - startPos)));
        startPos = endPos + 2;
    }
    modules.push_back(ident->v.substr(startPos));

    // Create the nested modules.
    ContainerPtr cont = unit->currentContainer();
    ModulePtr parent;
    size_t i = 0;
    if (UnitPtr ut = UnitPtr::dynamicCast(cont))
    {
        parent = ut->createModule(modules[i++]);
        if (parent)
        {
            ut->checkIntroduced(ident->v, parent);
            unit->pushContainer(parent);
        }
    }

    if (i == 0 || parent)
    {
        parent = unit->currentModule();
        for (; i < modules.size(); i++)
        {
            if (ModulePtr module = parent->createModule(modules[i]))
            {
                parent->checkIntroduced(ident->v, module);
                unit->pushContainer(module);
                parent = module;
            }
            else
            {
                // If an error occurs creating one of the modules, back up the entire chain.
                for(; i > 0; i--)
                {
                    unit->popContainer();
                }
                parent = nullptr;
                break;
            }
        }
    }

    yyval = parent;
}
#line 2179 "src/Slice/Grammar.cpp"
    break;

  case 50: /* module_def: ICE_MODULE ICE_SCOPED_IDENTIFIER @15 '{' definitions '}'  */
#line 461 "src/Slice/Grammar.y"
{
    if(yyvsp[-3])
    {
        StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[-4]);
        size_t startPos = 0;
        while((startPos = ident->v.find("::", startPos + 2)) != string::npos)
        {
            unit->popContainer();
        }
        yyval = unit->currentContainer();
        unit->popContainer();
    }
    else
    {
        yyval = 0;
    }
}
#line 2201 "src/Slice/Grammar.cpp"
    break;

  case 51: /* exception_id: ICE_EXCEPTION ICE_IDENTIFIER  */
#line 484 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 2209 "src/Slice/Grammar.cpp"
    break;

  case 52: /* exception_id: ICE_EXCEPTION keyword  */
#line 488 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    unit->error("keyword `" + ident->v + "' cannot be used as exception name");
    yyval = yyvsp[0]; // Dummy
}
#line 2219 "src/Slice/Grammar.cpp"
    break;

  case 53: /* exception_decl: exception_id  */
#line 499 "src/Slice/Grammar.y"
{
    unit->error("exceptions cannot be forward declared");
    yyval = 0;
}
#line 2228 "src/Slice/Grammar.cpp"
    break;

  case 54: /* @16: %empty  */
#line 509 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[-1]);
    ExceptionPtr base = ExceptionPtr::dynamicCast(yyvsp[0]);
    ModulePtr cont = unit->currentModule();
    ExceptionPtr ex = cont->createException(ident->v, base);
    if(ex)
    {
        cont->checkIntroduced(ident->v, ex);
        unit->pushContainer(ex);
    }
    yyval = ex;
}
#line 2245 "src/Slice/Grammar.cpp"
    break;

  case 55: /* exception_def: exception_id exception_extends @16 '{' data_members '}'  */
#line 522 "src/Slice/Grammar.y"
{
    if(yyvsp[-3])
    {
        unit->popContainer();
    }
    yyval = yyvsp[-3];
}
#line 2257 "src/Slice/Grammar.cpp"
    break;

  case 56: /* exception_extends: extends scoped_name  */
#line 535 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainerPtr cont = unit->currentContainer();
    ContainedPtr contained = cont->lookupException(scoped->v);
    cont->checkIntroduced(scoped->v);
    yyval = contained;
}
#line 2269 "src/Slice/Grammar.cpp"
    break;

  case 57: /* exception_extends: %empty  */
#line 543 "src/Slice/Grammar.y"
{
    yyval = 0;
}
#line 2277 "src/Slice/Grammar.cpp"
    break;

  case 58: /* tag: ICE_TAG_OPEN ICE_INTEGER_LITERAL ')'  */
#line 552 "src/Slice/Grammar.y"
{
    IntegerTokPtr i = IntegerTokPtr::dynamicCast(yyvsp[-1]);

    int tag;
    if(i->v < 0 || i->v > INT32_MAX)
    {
        unit->error("tag is out of range");
        tag = -1;
    }
    else
    {
        tag = static_cast<int>(i->v);
    }

    TaggedDefTokPtr m = new TaggedDefTok(tag);
    yyval = m;
}
#line 2299 "src/Slice/Grammar.cpp"
    break;

  case 59: /* tag: ICE_TAG_OPEN scoped_name ')'  */
#line 570 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[-1]);

    ContainerPtr cont = unit->currentContainer();
    assert(cont);
    ContainedList cl = cont->lookupContained(scoped->v, false);
    if(cl.empty())
    {
        EnumeratorList enumerators = cont->enumerators(scoped->v);
        if(enumerators.size() == 1)
        {
            // Found
            cl.push_back(enumerators.front());
            scoped->v = enumerators.front()->scoped();
            unit->warning(Deprecated, string("referencing enumerator `") + scoped->v
                          + "' without its enumeration's scope is deprecated");
        }
        else if(enumerators.size() > 1)
        {
            ostringstream os;
            os << "enumerator `" << scoped->v << "' could designate";
            bool first = true;
            for(const auto& p : enumerators)
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    os << " or";
                }

                os << " `" << p->scoped() << "'";
            }
            unit->error(os.str());
        }
        else
        {
            unit->error(string("`") + scoped->v + "' is not defined");
        }
    }

    if(cl.empty())
    {
        YYERROR; // Can't continue, jump to next yyerrok
    }
    cont->checkIntroduced(scoped->v);

    std::int64_t tag = -1;
    EnumeratorPtr enumerator = EnumeratorPtr::dynamicCast(cl.front());
    ConstPtr constant = ConstPtr::dynamicCast(cl.front());
    if (constant)
    {
        BuiltinPtr b = BuiltinPtr::dynamicCast(constant->type());
        if (b && b->isIntegralType())
        {
            tag = IceUtilInternal::strToInt64(constant->value().c_str(), 0, 0);
        }
    }
    else if(enumerator)
    {
        tag = enumerator->value();
    }

    if (tag < 0 || tag > INT32_MAX)
    {
        unit->error("cannot use value of `" + scoped->v + "' as a tag");
    }

    TaggedDefTokPtr m = new TaggedDefTok(static_cast<int>(tag));
    yyval = m;
}
#line 2377 "src/Slice/Grammar.cpp"
    break;

  case 60: /* tag: ICE_TAG_OPEN ')'  */
#line 644 "src/Slice/Grammar.y"
{
    unit->error("missing tag");
    TaggedDefTokPtr m = new TaggedDefTok; // Dummy
    yyval = m;
}
#line 2387 "src/Slice/Grammar.cpp"
    break;

  case 61: /* tag: ICE_TAG  */
#line 650 "src/Slice/Grammar.y"
{
    unit->error("missing tag");
    TaggedDefTokPtr m = new TaggedDefTok; // Dummy
    yyval = m;
}
#line 2397 "src/Slice/Grammar.cpp"
    break;

  case 62: /* optional: ICE_OPTIONAL_OPEN ICE_INTEGER_LITERAL ')'  */
#line 661 "src/Slice/Grammar.y"
{
    IntegerTokPtr i = IntegerTokPtr::dynamicCast(yyvsp[-1]);
    if (!unit->compatMode())
    {
        unit->warning(Deprecated, string("The `optional' keyword is deprecated, use `tag' instead"));
    }

    int tag;
    if(i->v < 0 || i->v > INT32_MAX)
    {
        unit->error("tag is out of range");
        tag = -1;
    }
    else
    {
        tag = static_cast<int>(i->v);
    }

    TaggedDefTokPtr m = new TaggedDefTok(tag);
    yyval = m;
}
#line 2423 "src/Slice/Grammar.cpp"
    break;

  case 63: /* optional: ICE_OPTIONAL_OPEN scoped_name ')'  */
#line 683 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[-1]);
    if (!unit->compatMode())
    {
        unit->warning(Deprecated, string("The `optional' keyword is deprecated, use `tag' instead"));
    }

    ContainerPtr cont = unit->currentContainer();
    assert(cont);
    ContainedList cl = cont->lookupContained(scoped->v, false);
    if(cl.empty())
    {
        EnumeratorList enumerators = cont->enumerators(scoped->v);
        if(enumerators.size() == 1)
        {
            // Found
            cl.push_back(enumerators.front());
            scoped->v = enumerators.front()->scoped();
            unit->warning(Deprecated, string("referencing enumerator `") + scoped->v
                          + "' without its enumeration's scope is deprecated");
        }
        else if(enumerators.size() > 1)
        {
            ostringstream os;
            os << "enumerator `" << scoped->v << "' could designate";
            bool first = true;
            for(const auto& p : enumerators)
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    os << " or";
                }

                os << " `" << p->scoped() << "'";
            }
            unit->error(os.str());
        }
        else
        {
            unit->error(string("`") + scoped->v + "' is not defined");
        }
    }

    if(cl.empty())
    {
        YYERROR; // Can't continue, jump to next yyerrok
    }
    cont->checkIntroduced(scoped->v);

    std::int64_t tag = -1;
    EnumeratorPtr enumerator = EnumeratorPtr::dynamicCast(cl.front());
    ConstPtr constant = ConstPtr::dynamicCast(cl.front());
    if (constant)
    {
        BuiltinPtr b = BuiltinPtr::dynamicCast(constant->type());
        if(b && b->isIntegralType())
        {
            tag = IceUtilInternal::strToInt64(constant->value().c_str(), 0, 0);
        }
    }
    else if(enumerator)
    {
        tag = enumerator->value();
    }

    if (tag < 0 || tag > INT32_MAX)
    {
        unit->error("cannot use value of `" + scoped->v + "' as a tag");
    }

    TaggedDefTokPtr m = new TaggedDefTok(static_cast<int>(tag));
    yyval = m;
}
#line 2505 "src/Slice/Grammar.cpp"
    break;

  case 64: /* optional: ICE_OPTIONAL_OPEN ')'  */
#line 761 "src/Slice/Grammar.y"
{
    if (!unit->compatMode())
    {
        unit->warning(Deprecated, string("The `optional' keyword is deprecated, use `tag' instead"));
    }
    unit->error("missing tag");
    TaggedDefTokPtr m = new TaggedDefTok; // Dummy
    yyval = m;
}
#line 2519 "src/Slice/Grammar.cpp"
    break;

  case 65: /* optional: ICE_OPTIONAL  */
#line 771 "src/Slice/Grammar.y"
{
    if (!unit->compatMode())
    {
        unit->warning(Deprecated, string("The `optional' keyword is deprecated, use `tag' instead"));
    }
    unit->error("missing tag");
    TaggedDefTokPtr m = new TaggedDefTok; // Dummy
    yyval = m;
}
#line 2533 "src/Slice/Grammar.cpp"
    break;

  case 66: /* struct_id: ICE_STRUCT ICE_IDENTIFIER  */
#line 786 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 2541 "src/Slice/Grammar.cpp"
    break;

  case 67: /* struct_id: ICE_STRUCT keyword  */
#line 790 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    unit->error("keyword `" + ident->v + "' cannot be used as struct name");
    yyval = yyvsp[0]; // Dummy
}
#line 2551 "src/Slice/Grammar.cpp"
    break;

  case 68: /* struct_decl: struct_id  */
#line 801 "src/Slice/Grammar.y"
{
    unit->error("structs cannot be forward declared");
    yyval = 0; // Dummy
}
#line 2560 "src/Slice/Grammar.cpp"
    break;

  case 69: /* @17: %empty  */
#line 811 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    ModulePtr cont = unit->currentModule();
    StructPtr st = cont->createStruct(ident->v);
    if(st)
    {
        cont->checkIntroduced(ident->v, st);
        unit->pushContainer(st);
    }
    else
    {
        st = cont->createStruct(IceUtil::generateUUID()); // Dummy
        assert(st);
        unit->pushContainer(st);
    }
    yyval = st;
}
#line 2582 "src/Slice/Grammar.cpp"
    break;

  case 70: /* struct_def: struct_id @17 '{' data_members '}'  */
#line 829 "src/Slice/Grammar.y"
{
    if(yyvsp[-3])
    {
        unit->popContainer();
    }
    yyval = yyvsp[-3];

    //
    // Empty structures are not allowed
    //
    StructPtr st = StructPtr::dynamicCast(yyval);
    assert(st);
    if (!st->hasDataMembers())
    {
        unit->error("struct `" + st->name() + "' must have at least one member"); // $$ is a dummy
    }
}
#line 2604 "src/Slice/Grammar.cpp"
    break;

  case 71: /* class_name: ICE_CLASS ICE_IDENTIFIER  */
#line 852 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 2612 "src/Slice/Grammar.cpp"
    break;

  case 72: /* class_name: ICE_CLASS keyword  */
#line 856 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    unit->error("keyword `" + ident->v + "' cannot be used as class name");
    yyval = yyvsp[0]; // Dummy
}
#line 2622 "src/Slice/Grammar.cpp"
    break;

  case 73: /* class_id: ICE_CLASS ICE_IDENT_OPEN ICE_INTEGER_LITERAL ')'  */
#line 867 "src/Slice/Grammar.y"
{
    IceUtil::Int64 id = IntegerTokPtr::dynamicCast(yyvsp[-1])->v;
    if(id < 0)
    {
        unit->error("invalid compact id for class: id must be a positive integer");
    }
    else if(id > INT32_MAX)
    {
        unit->error("invalid compact id for class: value is out of range");
    }
    else
    {
        string typeId = unit->getTypeId(static_cast<int>(id));
        if(!typeId.empty())
        {
            unit->error("invalid compact id for class: already assigned to class `" + typeId + "'");
        }
    }

    ClassIdTokPtr classId = new ClassIdTok();
    classId->v = StringTokPtr::dynamicCast(yyvsp[-2])->v;
    classId->t = static_cast<int>(id);
    yyval = classId;
}
#line 2651 "src/Slice/Grammar.cpp"
    break;

  case 74: /* class_id: ICE_CLASS ICE_IDENT_OPEN scoped_name ')'  */
#line 892 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[-1]);

    ContainerPtr cont = unit->currentContainer();
    assert(cont);
    ContainedList cl = cont->lookupContained(scoped->v, false);
    if(cl.empty())
    {
        EnumeratorList enumerators = cont->enumerators(scoped->v);
        if(enumerators.size() == 1)
        {
            // Found
            cl.push_back(enumerators.front());
            scoped->v = enumerators.front()->scoped();
            unit->warning(Deprecated, string("referencing enumerator `") + scoped->v
                          + "' without its enumeration's scope is deprecated");
        }
        else if(enumerators.size() > 1)
        {
            ostringstream os;
            os << "enumerator `" << scoped->v << "' could designate";
            bool first = true;
            for(EnumeratorList::iterator p = enumerators.begin(); p != enumerators.end(); ++p)
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    os << " or";
                }

                os << " `" << (*p)->scoped() << "'";
            }
            unit->error(os.str());
        }
        else
        {
            unit->error(string("`") + scoped->v + "' is not defined");
        }
    }

    if(cl.empty())
    {
        YYERROR; // Can't continue, jump to next yyerrok
    }
    cont->checkIntroduced(scoped->v);

    int id = -1;
    EnumeratorPtr enumerator = EnumeratorPtr::dynamicCast(cl.front());
    ConstPtr constant = ConstPtr::dynamicCast(cl.front());
    if(constant)
    {
        BuiltinPtr b = BuiltinPtr::dynamicCast(constant->type());
        if(b && b->isIntegralType())
        {
            IceUtil::Int64 l = IceUtilInternal::strToInt64(constant->value().c_str(), 0, 0);
            if(l < 0 || l > INT32_MAX)
            {
                unit->error("compact id for class is out of range");
            }
            id = static_cast<int>(l);
        }
    }
    else if(enumerator)
    {
        id = enumerator->value();
    }

    if(id < 0)
    {
        unit->error("invalid compact id for class: id must be a positive integer");
    }
    else
    {
        string typeId = unit->getTypeId(id);
        if(!typeId.empty())
        {
            unit->error("invalid compact id for class: already assigned to class `" + typeId + "'");
        }
    }

    ClassIdTokPtr classId = new ClassIdTok();
    classId->v = StringTokPtr::dynamicCast(yyvsp[-2])->v;
    classId->t = id;
    yyval = classId;

}
#line 2745 "src/Slice/Grammar.cpp"
    break;

  case 75: /* class_id: class_name  */
#line 982 "src/Slice/Grammar.y"
{
    ClassIdTokPtr classId = new ClassIdTok();
    classId->v = StringTokPtr::dynamicCast(yyvsp[0])->v;
    classId->t = -1;
    yyval = classId;
}
#line 2756 "src/Slice/Grammar.cpp"
    break;

  case 76: /* class_decl: class_name  */
#line 994 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    ModulePtr cont = unit->currentModule();
    ClassDeclPtr cl = cont->createClassDecl(ident->v);
    yyval = cl;
}
#line 2767 "src/Slice/Grammar.cpp"
    break;

  case 77: /* @18: %empty  */
#line 1006 "src/Slice/Grammar.y"
{
    ClassIdTokPtr ident = ClassIdTokPtr::dynamicCast(yyvsp[-1]);
    ModulePtr cont = unit->currentModule();
    ClassDefPtr base = ClassDefPtr::dynamicCast(yyvsp[0]);
    ClassDefPtr cl = cont->createClassDef(ident->v, ident->t, base);
    if(cl)
    {
        cont->checkIntroduced(ident->v, cl);
        unit->pushContainer(cl);
        yyval = cl;
    }
    else
    {
        yyval = 0;
    }
}
#line 2788 "src/Slice/Grammar.cpp"
    break;

  case 78: /* class_def: class_id class_extends @18 '{' data_members '}'  */
#line 1023 "src/Slice/Grammar.y"
{
    if(yyvsp[-3])
    {
        unit->popContainer();
        yyval = yyvsp[-3];
    }
    else
    {
        yyval = 0;
    }
}
#line 2804 "src/Slice/Grammar.cpp"
    break;

  case 79: /* class_extends: extends scoped_name  */
#line 1040 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainerPtr cont = unit->currentContainer();
    TypeList types = cont->lookupType(scoped->v);
    yyval = 0;
    if(!types.empty())
    {
        ClassDeclPtr cl = ClassDeclPtr::dynamicCast(types.front());
        if(!cl)
        {
            string msg = "`";
            msg += scoped->v;
            msg += "' is not a class";
            unit->error(msg);
        }
        else
        {
            ClassDefPtr def = cl->definition();
            if(!def)
            {
                string msg = "`";
                msg += scoped->v;
                msg += "' has been declared but not defined";
                unit->error(msg);
            }
            else
            {
                cont->checkIntroduced(scoped->v);
                yyval = def;
            }
        }
    }
}
#line 2842 "src/Slice/Grammar.cpp"
    break;

  case 80: /* class_extends: %empty  */
#line 1074 "src/Slice/Grammar.y"
{
    yyval = 0;
}
#line 2850 "src/Slice/Grammar.cpp"
    break;

  case 83: /* data_member: member  */
#line 1090 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[0]);

    // Check if the container was created successfully. If it wasn't skip creating the data member and continue parsing.
    if (DataMemberContainerPtr cont = DataMemberContainerPtr::dynamicCast(unit->currentContainer()))
    {
        MemberPtr dm = cont->createDataMember(def->name, def->type, def->isTagged, def->tag);
        unit->currentContainer()->checkIntroduced(def->name, dm);
        if (dm && !def->metadata.empty())
        {
            dm->setMetaData(def->metadata);
        }
    }
}
#line 2869 "src/Slice/Grammar.cpp"
    break;

  case 84: /* data_member: member '=' const_initializer  */
#line 1105 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[-2]);
    ConstDefTokPtr value = ConstDefTokPtr::dynamicCast(yyvsp[0]);

    // Check if the container was created successfully. If it wasn't skip creating the data member and continue parsing.
    if (DataMemberContainerPtr cont = DataMemberContainerPtr::dynamicCast(unit->currentContainer()))
    {
        MemberPtr dm = cont->createDataMember(def->name, def->type, def->isTagged, def->tag, value->v,
                                              value->valueAsString, value->valueAsLiteral);
        unit->currentContainer()->checkIntroduced(def->name, dm);
        if (dm && !def->metadata.empty())
        {
            dm->setMetaData(def->metadata);
        }
    }
}
#line 2890 "src/Slice/Grammar.cpp"
    break;

  case 87: /* data_member_list: data_member  */
#line 1129 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after definition");
}
#line 2898 "src/Slice/Grammar.cpp"
    break;

  case 91: /* return_tuple: out_qualifier member  */
#line 1146 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr returnMember = TaggedDefTokPtr::dynamicCast(yyvsp[0]);
    if (BoolTokPtr::dynamicCast(yyvsp[-1])->v)
    {
        unit->error("`" + returnMember->name + "': return members cannot be marked as out");
    }

    TaggedDefListTokPtr returnMembers = new TaggedDefListTok();
    returnMembers->v.push_back(returnMember);
    yyval = returnMembers;
}
#line 2914 "src/Slice/Grammar.cpp"
    break;

  case 92: /* return_tuple: return_tuple ',' out_qualifier member  */
#line 1158 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr returnMember = TaggedDefTokPtr::dynamicCast(yyvsp[0]);
    if (BoolTokPtr::dynamicCast(yyvsp[-1])->v)
    {
        unit->error("`" + returnMember->name + "': return members cannot be marked as out");
    }

    TaggedDefListTokPtr returnMembers = TaggedDefListTokPtr::dynamicCast(yyvsp[-3]);
    returnMembers->v.push_back(returnMember);
    yyval = returnMembers;
}
#line 2930 "src/Slice/Grammar.cpp"
    break;

  case 93: /* return_type: tagged_type  */
#line 1174 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr returnMember = TaggedDefTokPtr::dynamicCast(yyvsp[0]);
    // For unnamed return types we infer their name to be 'returnValue'.
    returnMember->name = "returnValue";

    if (returnMember->isTagged)
    {
        checkForTaggableType(returnMember->type);
    }

    TaggedDefListTokPtr returnMembers = new TaggedDefListTok();
    returnMembers->v.push_back(returnMember);
    yyval = returnMembers;
}
#line 2949 "src/Slice/Grammar.cpp"
    break;

  case 94: /* return_type: '(' return_tuple ')'  */
#line 1189 "src/Slice/Grammar.y"
{
    TaggedDefListTokPtr returnMembers = TaggedDefListTokPtr::dynamicCast(yyvsp[-1]);
    if (returnMembers->v.size() == 1)
    {
        unit->error("return tuples must contain at least 2 elements");
    }
    yyval = yyvsp[-1];
}
#line 2962 "src/Slice/Grammar.cpp"
    break;

  case 95: /* return_type: '(' ')'  */
#line 1198 "src/Slice/Grammar.y"
{
    unit->error("return tuples must contain at least 2 elements");
    yyval = new TaggedDefListTok();
}
#line 2971 "src/Slice/Grammar.cpp"
    break;

  case 96: /* return_type: ICE_VOID  */
#line 1203 "src/Slice/Grammar.y"
{
    yyval = new TaggedDefListTok();
}
#line 2979 "src/Slice/Grammar.cpp"
    break;

  case 97: /* operation_preamble: return_type ICE_IDENT_OPEN  */
#line 1212 "src/Slice/Grammar.y"
{
    TaggedDefListTokPtr returnMembers = TaggedDefListTokPtr::dynamicCast(yyvsp[-1]);
    string name = StringTokPtr::dynamicCast(yyvsp[0])->v;

    if (InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(unit->currentContainer()))
    {
        if (OperationPtr op = interface->createOperation(name))
        {
            interface->checkIntroduced(name, op);
            unit->pushContainer(op);

            // Set the return members for the operation.
            for (const auto& returnMember : returnMembers->v)
            {
                MemberPtr p = op->createReturnMember(returnMember->name, returnMember->type, returnMember->isTagged,
                                                     returnMember->tag);
                if (p && !returnMember->metadata.empty())
                {
                    p->setMetaData(returnMember->metadata);
                }
            }

            yyval = op;
        }
        else
        {
            yyval = 0;
        }
    }
    else
    {
        yyval = 0;
    }
}
#line 3018 "src/Slice/Grammar.cpp"
    break;

  case 98: /* operation_preamble: ICE_IDEMPOTENT return_type ICE_IDENT_OPEN  */
#line 1247 "src/Slice/Grammar.y"
{
    TaggedDefListTokPtr returnMembers = TaggedDefListTokPtr::dynamicCast(yyvsp[-1]);
    string name = StringTokPtr::dynamicCast(yyvsp[0])->v;

    if (InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(unit->currentContainer()))
    {
        if (OperationPtr op = interface->createOperation(name, Operation::Idempotent))
        {
            interface->checkIntroduced(name, op);
            unit->pushContainer(op);

            // Set the return members for the operation.
            for (const auto& returnMember : returnMembers->v)
            {
                MemberPtr p = op->createReturnMember(returnMember->name, returnMember->type, returnMember->isTagged,
                                                     returnMember->tag);
                if (p && !returnMember->metadata.empty())
                {
                    p->setMetaData(returnMember->metadata);
                }
            }

            yyval = op;
        }
        else
        {
            yyval = 0;
        }
    }
    else
    {
        yyval = 0;
    }
}
#line 3057 "src/Slice/Grammar.cpp"
    break;

  case 99: /* operation_preamble: return_type ICE_KEYWORD_OPEN  */
#line 1282 "src/Slice/Grammar.y"
{
    TaggedDefListTokPtr returnMembers = TaggedDefListTokPtr::dynamicCast(yyvsp[-1]);
    string name = StringTokPtr::dynamicCast(yyvsp[0])->v;
    if (InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(unit->currentContainer()))
    {
        if (OperationPtr op = interface->createOperation(name))
        {
            unit->pushContainer(op);

            // Set the return members for the operation.
            for (const auto& returnMember : returnMembers->v)
            {
                MemberPtr p = op->createReturnMember(returnMember->name, returnMember->type, returnMember->isTagged,
                                                     returnMember->tag);
                if (p && !returnMember->metadata.empty())
                {
                    p->setMetaData(returnMember->metadata);
                }
            }

            unit->error("keyword `" + name + "' cannot be used as operation name");
            yyval = op; // Dummy
        }
        else
        {
            yyval = 0;
        }
    }
    else
    {
        yyval = 0;
    }
}
#line 3095 "src/Slice/Grammar.cpp"
    break;

  case 100: /* operation_preamble: ICE_IDEMPOTENT return_type ICE_KEYWORD_OPEN  */
#line 1316 "src/Slice/Grammar.y"
{
    TaggedDefListTokPtr returnMembers = TaggedDefListTokPtr::dynamicCast(yyvsp[-1]);
    string name = StringTokPtr::dynamicCast(yyvsp[0])->v;
    if (InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(unit->currentContainer()))
    {
        if (OperationPtr op = interface->createOperation(name, Operation::Idempotent))
        {
            unit->pushContainer(op);

            // Set the return members for the operation.
            for (const auto& returnMember : returnMembers->v)
            {
                MemberPtr p = op->createReturnMember(returnMember->name, returnMember->type, returnMember->isTagged,
                                                     returnMember->tag);
                if (p && !returnMember->metadata.empty())
                {
                    p->setMetaData(returnMember->metadata);
                }
            }

            unit->error("keyword `" + name + "' cannot be used as operation name");
            yyval = op; // Dummy
        }
        else
        {
            yyval = 0;
        }
    }
    else
    {
        yyval = 0;
    }
}
#line 3133 "src/Slice/Grammar.cpp"
    break;

  case 101: /* @19: %empty  */
#line 1355 "src/Slice/Grammar.y"
{
    if(yyvsp[-2])
    {
        unit->popContainer();
        yyval = yyvsp[-2];
    }
    else
    {
        yyval = 0;
    }
}
#line 3149 "src/Slice/Grammar.cpp"
    break;

  case 102: /* operation: operation_preamble parameters ')' @19 throws  */
#line 1367 "src/Slice/Grammar.y"
{
    OperationPtr op = OperationPtr::dynamicCast(yyvsp[-1]);
    ExceptionListTokPtr el = ExceptionListTokPtr::dynamicCast(yyvsp[0]);
    assert(el);
    if(op)
    {
        op->setExceptionList(el->v);
    }
}
#line 3163 "src/Slice/Grammar.cpp"
    break;

  case 103: /* @20: %empty  */
#line 1377 "src/Slice/Grammar.y"
{
    if(yyvsp[-2])
    {
        unit->popContainer();
    }
    yyerrok;
}
#line 3175 "src/Slice/Grammar.cpp"
    break;

  case 104: /* operation: operation_preamble error ')' @20 throws  */
#line 1385 "src/Slice/Grammar.y"
{
    OperationPtr op = OperationPtr::dynamicCast(yyvsp[-1]);
    ExceptionListTokPtr el = ExceptionListTokPtr::dynamicCast(yyvsp[0]);
    assert(el);
    if(op)
    {
        op->setExceptionList(el->v); // Dummy
    }
}
#line 3189 "src/Slice/Grammar.cpp"
    break;

  case 105: /* operation_list: local_metadata operation ';' operation_list  */
#line 1400 "src/Slice/Grammar.y"
{
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[-3]);
    OperationPtr operation = OperationPtr::dynamicCast(yyvsp[-2]);
    if (operation && !metaData->v.empty())
    {
        operation->setMetaData(metaData->v);

        // If the operation had a single return type (not a return tuple), also apply the metadata to the return type.
        // TODO: once we introduce more concrete metadata validation, we could sort the metadata out between the return
        // type and the operation itself. So metadata relevant to operations would only be set for the operation, and
        // metadata only relevant to the return type would only be set on the return type.
        if (operation->hasSingleReturnType())
        {
            operation->returnType().front()->setMetaData(metaData->v);
        }
    }
}
#line 3211 "src/Slice/Grammar.cpp"
    break;

  case 106: /* operation_list: local_metadata operation  */
#line 1418 "src/Slice/Grammar.y"
{
    unit->error("`;' missing after definition");
}
#line 3219 "src/Slice/Grammar.cpp"
    break;

  case 109: /* interface_id: ICE_INTERFACE ICE_IDENTIFIER  */
#line 1429 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 3227 "src/Slice/Grammar.cpp"
    break;

  case 110: /* interface_id: ICE_INTERFACE keyword  */
#line 1433 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    unit->error("keyword `" + ident->v + "' cannot be used as interface name");
    yyval = yyvsp[0]; // Dummy
}
#line 3237 "src/Slice/Grammar.cpp"
    break;

  case 111: /* interface_decl: interface_id  */
#line 1444 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    ModulePtr cont = unit->currentModule();
    InterfaceDeclPtr cl = cont->createInterfaceDecl(ident->v);
    cont->checkIntroduced(ident->v, cl);
    yyval = cl;
}
#line 3249 "src/Slice/Grammar.cpp"
    break;

  case 112: /* @21: %empty  */
#line 1457 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[-1]);
    ModulePtr cont = unit->currentModule();
    InterfaceListTokPtr bases = InterfaceListTokPtr::dynamicCast(yyvsp[0]);
    InterfaceDefPtr interface = cont->createInterfaceDef(ident->v, bases->v);
    if(interface)
    {
        cont->checkIntroduced(ident->v, interface);
        unit->pushContainer(interface);
        yyval = interface;
    }
    else
    {
        yyval = 0;
    }
}
#line 3270 "src/Slice/Grammar.cpp"
    break;

  case 113: /* interface_def: interface_id interface_extends @21 '{' operation_list '}'  */
#line 1474 "src/Slice/Grammar.y"
{
    if(yyvsp[-3])
    {
        unit->popContainer();
        yyval = yyvsp[-3];
    }
    else
    {
        yyval = 0;
    }
}
#line 3286 "src/Slice/Grammar.cpp"
    break;

  case 114: /* interface_list: scoped_name ',' interface_list  */
#line 1491 "src/Slice/Grammar.y"
{
    InterfaceListTokPtr intfs = InterfaceListTokPtr::dynamicCast(yyvsp[0]);
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[-2]);
    ContainerPtr cont = unit->currentContainer();
    TypeList types = cont->lookupType(scoped->v);
    if(!types.empty())
    {
        InterfaceDeclPtr interface = InterfaceDeclPtr::dynamicCast(types.front());
        if(!interface)
        {
            string msg = "`";
            msg += scoped->v;
            msg += "' is not an interface";
            unit->error(msg);
        }
        else
        {
            InterfaceDefPtr def = interface->definition();
            if(!def)
            {
                string msg = "`";
                msg += scoped->v;
                msg += "' has been declared but not defined";
                unit->error(msg);
            }
            else
            {
                cont->checkIntroduced(scoped->v);
                intfs->v.push_front(def);
            }
        }
    }
    yyval = intfs;
}
#line 3325 "src/Slice/Grammar.cpp"
    break;

  case 115: /* interface_list: scoped_name  */
#line 1526 "src/Slice/Grammar.y"
{
    InterfaceListTokPtr intfs = new InterfaceListTok;
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainerPtr cont = unit->currentContainer();
    TypeList types = cont->lookupType(scoped->v);
    if(!types.empty())
    {
        InterfaceDeclPtr interface = InterfaceDeclPtr::dynamicCast(types.front());
        if(!interface)
        {
            string msg = "`";
            msg += scoped->v;
            msg += "' is not an interface";
            unit->error(msg); // $$ is a dummy
        }
        else
        {
            InterfaceDefPtr def = interface->definition();
            if(!def)
            {
                string msg = "`";
                msg += scoped->v;
                msg += "' has been declared but not defined";
                unit->error(msg); // $$ is a dummy
            }
            else
            {
                cont->checkIntroduced(scoped->v);
                intfs->v.push_front(def);
            }
        }
    }
    yyval = intfs;
}
#line 3364 "src/Slice/Grammar.cpp"
    break;

  case 116: /* interface_list: ICE_OBJECT  */
#line 1561 "src/Slice/Grammar.y"
{
    unit->error("illegal inheritance from type Object");
    yyval = new InterfaceListTok; // Dummy
}
#line 3373 "src/Slice/Grammar.cpp"
    break;

  case 117: /* interface_list: ICE_ANYCLASS  */
#line 1566 "src/Slice/Grammar.y"
{
    unit->error("illegal inheritance from type AnyClass");
    yyval = new ClassListTok; // Dummy
}
#line 3382 "src/Slice/Grammar.cpp"
    break;

  case 118: /* interface_list: ICE_VALUE  */
#line 1571 "src/Slice/Grammar.y"
{
    if (!unit->compatMode())
    {
        unit->warning(Deprecated, "the `Value' keyword is deprecated, use `AnyClass' instead");
    }
    unit->error("illegal inheritance from type Value");
    yyval = new ClassListTok; // Dummy
}
#line 3395 "src/Slice/Grammar.cpp"
    break;

  case 119: /* interface_extends: extends interface_list  */
#line 1585 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 3403 "src/Slice/Grammar.cpp"
    break;

  case 120: /* interface_extends: %empty  */
#line 1589 "src/Slice/Grammar.y"
{
    yyval = new InterfaceListTok;
}
#line 3411 "src/Slice/Grammar.cpp"
    break;

  case 121: /* exception_list: exception ',' exception_list  */
#line 1598 "src/Slice/Grammar.y"
{
    ExceptionPtr exception = ExceptionPtr::dynamicCast(yyvsp[-2]);
    ExceptionListTokPtr exceptionList = ExceptionListTokPtr::dynamicCast(yyvsp[0]);
    exceptionList->v.push_front(exception);
    yyval = exceptionList;
}
#line 3422 "src/Slice/Grammar.cpp"
    break;

  case 122: /* exception_list: exception  */
#line 1605 "src/Slice/Grammar.y"
{
    ExceptionPtr exception = ExceptionPtr::dynamicCast(yyvsp[0]);
    ExceptionListTokPtr exceptionList = new ExceptionListTok;
    exceptionList->v.push_front(exception);
    yyval = exceptionList;
}
#line 3433 "src/Slice/Grammar.cpp"
    break;

  case 123: /* exception: scoped_name  */
#line 1617 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainerPtr cont = unit->currentContainer();
    ExceptionPtr exception = cont->lookupException(scoped->v);
    if(!exception)
    {
        exception = unit->currentModule()->createException(IceUtil::generateUUID(), 0, Dummy); // Dummy
    }
    cont->checkIntroduced(scoped->v, exception);
    yyval = exception;
}
#line 3449 "src/Slice/Grammar.cpp"
    break;

  case 124: /* exception: keyword  */
#line 1629 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    unit->error("keyword `" + ident->v + "' cannot be used as exception name");
    yyval = unit->currentModule()->createException(IceUtil::generateUUID(), 0, Dummy); // Dummy
}
#line 3459 "src/Slice/Grammar.cpp"
    break;

  case 125: /* sequence_def: ICE_SEQUENCE '<' local_metadata type '>' ICE_IDENTIFIER  */
#line 1640 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[-3]);
    TypePtr type = TypePtr::dynamicCast(yyvsp[-2]);
    ModulePtr cont = unit->currentModule();
    yyval = cont->createSequence(ident->v, type, metaData->v);
}
#line 3471 "src/Slice/Grammar.cpp"
    break;

  case 126: /* sequence_def: ICE_SEQUENCE '<' local_metadata type '>' keyword  */
#line 1648 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[-3]);
    TypePtr type = TypePtr::dynamicCast(yyvsp[-2]);
    ModulePtr cont = unit->currentModule();
    yyval = cont->createSequence(ident->v, type, metaData->v); // Dummy
    unit->error("keyword `" + ident->v + "' cannot be used as sequence name");
}
#line 3484 "src/Slice/Grammar.cpp"
    break;

  case 127: /* dictionary_def: ICE_DICTIONARY '<' local_metadata type ',' local_metadata type '>' ICE_IDENTIFIER  */
#line 1662 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    StringListTokPtr keyMetaData = StringListTokPtr::dynamicCast(yyvsp[-6]);
    TypePtr keyType = TypePtr::dynamicCast(yyvsp[-5]);
    StringListTokPtr valueMetaData = StringListTokPtr::dynamicCast(yyvsp[-3]);
    TypePtr valueType = TypePtr::dynamicCast(yyvsp[-2]);
    ModulePtr cont = unit->currentModule();
    yyval = cont->createDictionary(ident->v, keyType, keyMetaData->v, valueType, valueMetaData->v);
}
#line 3498 "src/Slice/Grammar.cpp"
    break;

  case 128: /* dictionary_def: ICE_DICTIONARY '<' local_metadata type ',' local_metadata type '>' keyword  */
#line 1672 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    StringListTokPtr keyMetaData = StringListTokPtr::dynamicCast(yyvsp[-6]);
    TypePtr keyType = TypePtr::dynamicCast(yyvsp[-5]);
    StringListTokPtr valueMetaData = StringListTokPtr::dynamicCast(yyvsp[-3]);
    TypePtr valueType = TypePtr::dynamicCast(yyvsp[-2]);
    ModulePtr cont = unit->currentModule();
    yyval = cont->createDictionary(ident->v, keyType, keyMetaData->v, valueType, valueMetaData->v); // Dummy
    unit->error("keyword `" + ident->v + "' cannot be used as dictionary name");
}
#line 3513 "src/Slice/Grammar.cpp"
    break;

  case 129: /* enum_start: ICE_UNCHECKED ICE_ENUM  */
#line 1688 "src/Slice/Grammar.y"
{
    yyval = new BoolTok(true);
}
#line 3521 "src/Slice/Grammar.cpp"
    break;

  case 130: /* enum_start: ICE_ENUM  */
#line 1692 "src/Slice/Grammar.y"
{
    yyval = new BoolTok(false);
}
#line 3529 "src/Slice/Grammar.cpp"
    break;

  case 131: /* enum_id: enum_start ICE_IDENTIFIER  */
#line 1701 "src/Slice/Grammar.y"
{
    bool unchecked = BoolTokPtr::dynamicCast(yyvsp[-1])->v;
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    ModulePtr cont = unit->currentModule();
    EnumPtr en = cont->createEnum(ident->v, unchecked);
    if (en)
    {
        cont->checkIntroduced(ident->v, en);
    }
    else
    {
        en = cont->createEnum(IceUtil::generateUUID(), unchecked, Dummy);
    }
    yyval = en;
}
#line 3549 "src/Slice/Grammar.cpp"
    break;

  case 132: /* enum_id: enum_start keyword  */
#line 1717 "src/Slice/Grammar.y"
{
    bool unchecked = BoolTokPtr::dynamicCast(yyvsp[-1])->v;
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    ModulePtr cont = unit->currentModule();
    unit->error("keyword `" + ident->v + "' cannot be used as enumeration name");
    yyval = cont->createEnum(IceUtil::generateUUID(), unchecked, Dummy);
}
#line 3561 "src/Slice/Grammar.cpp"
    break;

  case 133: /* @22: %empty  */
#line 1730 "src/Slice/Grammar.y"
{
    EnumPtr en = EnumPtr::dynamicCast(yyvsp[-1]);
    en->initUnderlying(TypePtr::dynamicCast(yyvsp[0]));
    unit->pushContainer(en);
    yyval = en;
}
#line 3572 "src/Slice/Grammar.cpp"
    break;

  case 134: /* enum_def: enum_id enum_underlying @22 '{' enumerator_list '}'  */
#line 1737 "src/Slice/Grammar.y"
{
    EnumPtr en = EnumPtr::dynamicCast(yyvsp[-3]);
    if(en)
    {
        EnumeratorListTokPtr enumerators = EnumeratorListTokPtr::dynamicCast(yyvsp[-1]);
        if(enumerators->v.empty())
        {
            unit->error("enum `" + en->name() + "' must have at least one enumerator");
        }
        unit->popContainer();
    }
    yyval = yyvsp[-3];
}
#line 3590 "src/Slice/Grammar.cpp"
    break;

  case 135: /* @23: %empty  */
#line 1752 "src/Slice/Grammar.y"
{
    bool unchecked = BoolTokPtr::dynamicCast(yyvsp[0])->v;
    unit->error("missing enumeration name");
    ModulePtr cont = unit->currentModule();
    EnumPtr en = cont->createEnum(IceUtil::generateUUID(), unchecked, Dummy);
    unit->pushContainer(en);
    yyval = en;
}
#line 3603 "src/Slice/Grammar.cpp"
    break;

  case 136: /* enum_def: enum_start @23 '{' enumerator_list '}'  */
#line 1761 "src/Slice/Grammar.y"
{
    unit->popContainer();
    yyval = yyvsp[-4];
}
#line 3612 "src/Slice/Grammar.cpp"
    break;

  case 137: /* enum_underlying: ':' type  */
#line 1771 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 3620 "src/Slice/Grammar.cpp"
    break;

  case 138: /* enum_underlying: %empty  */
#line 1775 "src/Slice/Grammar.y"
{
    yyval = 0;
}
#line 3628 "src/Slice/Grammar.cpp"
    break;

  case 139: /* enumerator_list: enumerator ',' enumerator_list  */
#line 1784 "src/Slice/Grammar.y"
{
    EnumeratorListTokPtr ens = EnumeratorListTokPtr::dynamicCast(yyvsp[-2]);
    ens->v.splice(ens->v.end(), EnumeratorListTokPtr::dynamicCast(yyvsp[0])->v);
    yyval = ens;
}
#line 3638 "src/Slice/Grammar.cpp"
    break;

  case 141: /* enumerator: ICE_IDENTIFIER  */
#line 1796 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    EnumeratorListTokPtr ens = new EnumeratorListTok;
    EnumPtr cont = EnumPtr::dynamicCast(unit->currentContainer());
    EnumeratorPtr en = cont->createEnumerator(ident->v);
    if(en)
    {
        ens->v.push_front(en);
    }
    yyval = ens;
}
#line 3654 "src/Slice/Grammar.cpp"
    break;

  case 142: /* enumerator: ICE_IDENTIFIER '=' enumerator_initializer  */
#line 1808 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[-2]);
    EnumeratorListTokPtr ens = new EnumeratorListTok;
    EnumPtr cont = EnumPtr::dynamicCast(unit->currentContainer());
    IntegerTokPtr intVal = IntegerTokPtr::dynamicCast(yyvsp[0]);
    if (intVal)
    {
       EnumeratorPtr en = cont->createEnumerator(ident->v, intVal->v);
       ens->v.push_front(en);
    }
    yyval = ens;
}
#line 3671 "src/Slice/Grammar.cpp"
    break;

  case 143: /* enumerator: keyword  */
#line 1821 "src/Slice/Grammar.y"
{
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[0]);
    unit->error("keyword `" + ident->v + "' cannot be used as enumerator");
    EnumeratorListTokPtr ens = new EnumeratorListTok; // Dummy
    yyval = ens;
}
#line 3682 "src/Slice/Grammar.cpp"
    break;

  case 144: /* enumerator: %empty  */
#line 1828 "src/Slice/Grammar.y"
{
    EnumeratorListTokPtr ens = new EnumeratorListTok;
    yyval = ens; // Dummy
}
#line 3691 "src/Slice/Grammar.cpp"
    break;

  case 145: /* enumerator_initializer: ICE_INTEGER_LITERAL  */
#line 1838 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 3699 "src/Slice/Grammar.cpp"
    break;

  case 146: /* enumerator_initializer: scoped_name  */
#line 1842 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainedList cl = unit->currentContainer()->lookupContained(scoped->v);
    IntegerTokPtr tok;
    if(!cl.empty())
    {
        ConstPtr constant = ConstPtr::dynamicCast(cl.front());
        if(constant)
        {
            unit->currentContainer()->checkIntroduced(scoped->v, constant);
            BuiltinPtr b = BuiltinPtr::dynamicCast(constant->type());
            if(b && b->isIntegralType())
            {
                IceUtil::Int64 v;
                if(IceUtilInternal::stringToInt64(constant->value(), v))
                {
                    tok = new IntegerTok;
                    tok->v = v;
                    tok->literal = constant->value();
                }
            }
        }
    }

    if(!tok)
    {
        string msg = "illegal initializer: `" + scoped->v + "' is not an integer constant";
        unit->error(msg); // $$ is dummy
    }

    yyval = tok;
}
#line 3736 "src/Slice/Grammar.cpp"
    break;

  case 147: /* out_qualifier: ICE_OUT  */
#line 1880 "src/Slice/Grammar.y"
{
    yyval = new BoolTok(true);
}
#line 3744 "src/Slice/Grammar.cpp"
    break;

  case 148: /* out_qualifier: %empty  */
#line 1884 "src/Slice/Grammar.y"
{
    yyval = new BoolTok(false);
}
#line 3752 "src/Slice/Grammar.cpp"
    break;

  case 149: /* parameter: out_qualifier member  */
#line 1893 "src/Slice/Grammar.y"
{
    BoolTokPtr isOutParam = BoolTokPtr::dynamicCast(yyvsp[-1]);
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[0]);

    if (OperationPtr op = OperationPtr::dynamicCast(unit->currentContainer()))
    {
        MemberPtr param = op->createParameter(def->name, def->type, isOutParam->v, def->isTagged, def->tag);
        unit->currentContainer()->checkIntroduced(def->name, param);
        if(param && !def->metadata.empty())
        {
            param->setMetaData(def->metadata);
        }
    }
}
#line 3771 "src/Slice/Grammar.cpp"
    break;

  case 154: /* throws: ICE_THROWS exception_list  */
#line 1926 "src/Slice/Grammar.y"
{
    yyval = yyvsp[0];
}
#line 3779 "src/Slice/Grammar.cpp"
    break;

  case 155: /* throws: %empty  */
#line 1930 "src/Slice/Grammar.y"
{
    yyval = new ExceptionListTok;
}
#line 3787 "src/Slice/Grammar.cpp"
    break;

  case 158: /* builtin: ICE_BOOL  */
#line 1945 "src/Slice/Grammar.y"
           {}
#line 3793 "src/Slice/Grammar.cpp"
    break;

  case 159: /* builtin: ICE_BYTE  */
#line 1946 "src/Slice/Grammar.y"
           {}
#line 3799 "src/Slice/Grammar.cpp"
    break;

  case 160: /* builtin: ICE_SHORT  */
#line 1947 "src/Slice/Grammar.y"
            {}
#line 3805 "src/Slice/Grammar.cpp"
    break;

  case 161: /* builtin: ICE_USHORT  */
#line 1948 "src/Slice/Grammar.y"
             {}
#line 3811 "src/Slice/Grammar.cpp"
    break;

  case 162: /* builtin: ICE_INT  */
#line 1949 "src/Slice/Grammar.y"
          {}
#line 3817 "src/Slice/Grammar.cpp"
    break;

  case 163: /* builtin: ICE_UINT  */
#line 1950 "src/Slice/Grammar.y"
           {}
#line 3823 "src/Slice/Grammar.cpp"
    break;

  case 164: /* builtin: ICE_VARINT  */
#line 1951 "src/Slice/Grammar.y"
             {}
#line 3829 "src/Slice/Grammar.cpp"
    break;

  case 165: /* builtin: ICE_VARUINT  */
#line 1952 "src/Slice/Grammar.y"
              {}
#line 3835 "src/Slice/Grammar.cpp"
    break;

  case 166: /* builtin: ICE_LONG  */
#line 1953 "src/Slice/Grammar.y"
           {}
#line 3841 "src/Slice/Grammar.cpp"
    break;

  case 167: /* builtin: ICE_ULONG  */
#line 1954 "src/Slice/Grammar.y"
            {}
#line 3847 "src/Slice/Grammar.cpp"
    break;

  case 168: /* builtin: ICE_VARLONG  */
#line 1955 "src/Slice/Grammar.y"
              {}
#line 3853 "src/Slice/Grammar.cpp"
    break;

  case 169: /* builtin: ICE_VARULONG  */
#line 1956 "src/Slice/Grammar.y"
               {}
#line 3859 "src/Slice/Grammar.cpp"
    break;

  case 170: /* builtin: ICE_FLOAT  */
#line 1957 "src/Slice/Grammar.y"
            {}
#line 3865 "src/Slice/Grammar.cpp"
    break;

  case 171: /* builtin: ICE_DOUBLE  */
#line 1958 "src/Slice/Grammar.y"
             {}
#line 3871 "src/Slice/Grammar.cpp"
    break;

  case 172: /* builtin: ICE_STRING  */
#line 1959 "src/Slice/Grammar.y"
             {}
#line 3877 "src/Slice/Grammar.cpp"
    break;

  case 173: /* type: ICE_OBJECT '*'  */
#line 1965 "src/Slice/Grammar.y"
{
    yyval = unit->optionalBuiltin(Builtin::KindObject);
}
#line 3885 "src/Slice/Grammar.cpp"
    break;

  case 174: /* type: ICE_OBJECT '?'  */
#line 1969 "src/Slice/Grammar.y"
{
    yyval = unit->optionalBuiltin(Builtin::KindObject);
}
#line 3893 "src/Slice/Grammar.cpp"
    break;

  case 175: /* type: ICE_ANYCLASS '?'  */
#line 1973 "src/Slice/Grammar.y"
{
    yyval = unit->optionalBuiltin(Builtin::KindAnyClass);
}
#line 3901 "src/Slice/Grammar.cpp"
    break;

  case 176: /* type: ICE_VALUE '?'  */
#line 1977 "src/Slice/Grammar.y"
{
    if (!unit->compatMode())
    {
        unit->warning(Deprecated, "the `Value' keyword is deprecated, use `AnyClass' instead");
    }
    yyval = unit->optionalBuiltin(Builtin::KindAnyClass);
}
#line 3913 "src/Slice/Grammar.cpp"
    break;

  case 177: /* type: builtin '?'  */
#line 1985 "src/Slice/Grammar.y"
{
    StringTokPtr typeName = StringTokPtr::dynamicCast(yyvsp[-1]);
    yyval = unit->optionalBuiltin(Builtin::kindFromString(typeName->v).value());
}
#line 3922 "src/Slice/Grammar.cpp"
    break;

  case 178: /* type: ICE_OBJECT  */
#line 1990 "src/Slice/Grammar.y"
{
    if (unit->compatMode())
    {
        yyval = unit->optionalBuiltin(Builtin::KindAnyClass);
    }
    else
    {
        yyval = unit->builtin(Builtin::KindObject);
    }
}
#line 3937 "src/Slice/Grammar.cpp"
    break;

  case 179: /* type: ICE_ANYCLASS  */
#line 2001 "src/Slice/Grammar.y"
{
    yyval = unit->builtin(Builtin::KindAnyClass);
}
#line 3945 "src/Slice/Grammar.cpp"
    break;

  case 180: /* type: ICE_VALUE  */
#line 2005 "src/Slice/Grammar.y"
{
    if (unit->compatMode())
    {
        yyval = unit->optionalBuiltin(Builtin::KindAnyClass);
    }
    else
    {
        unit->warning(Deprecated, "the `Value' keyword is deprecated, use `AnyClass' instead");
        yyval = unit->builtin(Builtin::KindAnyClass);
    }
}
#line 3961 "src/Slice/Grammar.cpp"
    break;

  case 181: /* type: builtin  */
#line 2017 "src/Slice/Grammar.y"
{
    StringTokPtr typeName = StringTokPtr::dynamicCast(yyvsp[0]);
    yyval = unit->builtin(Builtin::kindFromString(typeName->v).value());
}
#line 3970 "src/Slice/Grammar.cpp"
    break;

  case 182: /* type: scoped_name  */
#line 2022 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ContainerPtr cont = unit->currentContainer();
    if(cont)
    {
        TypeList types = cont->lookupType(scoped->v);
        if(types.empty())
        {
            YYERROR; // Can't continue, jump to next yyerrok
        }
        cont->checkIntroduced(scoped->v);
        if (ClassDeclPtr::dynamicCast(types.front()) && unit->compatMode())
        {
            yyval = new Optional(types.front());
        }
        else
        {
            yyval = types.front();
        }
    }
    else
    {
        yyval = 0;
    }
}
#line 4000 "src/Slice/Grammar.cpp"
    break;

  case 183: /* type: scoped_name '*'  */
#line 2048 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[-1]);
    ContainerPtr cont = unit->currentContainer();
    if(cont)
    {
        TypeList types = cont->lookupType(scoped->v);
        if(types.empty())
        {
            YYERROR; // Can't continue, jump to next yyerrok
        }
        for(TypeList::iterator p = types.begin(); p != types.end(); ++p)
        {
            InterfaceDeclPtr interface = InterfaceDeclPtr::dynamicCast(*p);
            if(!interface)
            {
                string msg = "`";
                msg += scoped->v;
                msg += "' must be an interface";
                unit->error(msg);
                YYERROR; // Can't continue, jump to next yyerrok
            }
            cont->checkIntroduced(scoped->v);
            *p = new Optional(interface);
        }
        yyval = types.front();
    }
    else
    {
        yyval = 0;
    }
}
#line 4036 "src/Slice/Grammar.cpp"
    break;

  case 184: /* type: scoped_name '?'  */
#line 2080 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[-1]);
    ContainerPtr cont = unit->currentContainer();
    if(cont)
    {
        TypeList types = cont->lookupType(scoped->v);
        if(types.empty())
        {
            YYERROR; // Can't continue, jump to next yyerrok
        }
        for(TypeList::iterator p = types.begin(); p != types.end(); ++p)
        {
            cont->checkIntroduced(scoped->v);
            *p = new Optional(*p);
        }
        yyval = types.front();
    }
    else
    {
        yyval = 0;
    }
}
#line 4063 "src/Slice/Grammar.cpp"
    break;

  case 185: /* tagged_type: tag type  */
#line 2108 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr taggedDef = TaggedDefTokPtr::dynamicCast(yyvsp[-1]);
    OptionalPtr type = OptionalPtr::dynamicCast(yyvsp[0]);

    if (!type)
    {
        // Infer the type to be optional so parsing can continue without nullptrs.
        type = new Optional(TypePtr::dynamicCast(yyvsp[0]));
        unit->error("only optional types can be tagged");
    }

    taggedDef->type = type;
    yyval = taggedDef;
}
#line 4082 "src/Slice/Grammar.cpp"
    break;

  case 186: /* tagged_type: optional type  */
#line 2123 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr taggedDef = TaggedDefTokPtr::dynamicCast(yyvsp[-1]);
    OptionalPtr type = OptionalPtr::dynamicCast(yyvsp[0]);

    if (!type)
    {
        // Infer the type to be optional for backwards compatibility.
        type = new Optional(TypePtr::dynamicCast(yyvsp[0]));
    }

    taggedDef->type = type;
    yyval = taggedDef;
}
#line 4100 "src/Slice/Grammar.cpp"
    break;

  case 187: /* tagged_type: type  */
#line 2137 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr taggedDef = new TaggedDefTok;
    taggedDef->type = TypePtr::dynamicCast(yyvsp[0]);
    yyval = taggedDef;
}
#line 4110 "src/Slice/Grammar.cpp"
    break;

  case 188: /* member: tagged_type ICE_IDENTIFIER  */
#line 2148 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[-1]);
    def->name = StringTokPtr::dynamicCast(yyvsp[0])->v;
    checkIdentifier(def->name);
    if (def->isTagged)
    {
        checkForTaggableType(def->type, def->name);
    }

    yyval = def;
}
#line 4126 "src/Slice/Grammar.cpp"
    break;

  case 189: /* member: tagged_type keyword  */
#line 2160 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[-1]);
    def->name = StringTokPtr::dynamicCast(yyvsp[0])->v;
    if (def->isTagged)
    {
        checkForTaggableType(def->type, def->name);
    }
    unit->error("keyword `" + def->name + "' cannot be used as an identifier");
    yyval = def;
}
#line 4141 "src/Slice/Grammar.cpp"
    break;

  case 190: /* member: tagged_type  */
#line 2171 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[0]);
    def->name = IceUtil::generateUUID(); // Dummy
    if (def->isTagged)
    {
        checkForTaggableType(def->type);
    }
    unit->error("missing identifier");
    yyval = def;
}
#line 4156 "src/Slice/Grammar.cpp"
    break;

  case 191: /* member: local_metadata member  */
#line 2182 "src/Slice/Grammar.y"
{
    TaggedDefTokPtr def = TaggedDefTokPtr::dynamicCast(yyvsp[0]);
    def->metadata = StringListTokPtr::dynamicCast(yyvsp[-1])->v;
    yyval = def;
}
#line 4166 "src/Slice/Grammar.cpp"
    break;

  case 192: /* string_literal: ICE_STRING_LITERAL string_literal  */
#line 2193 "src/Slice/Grammar.y"
{
    StringTokPtr str1 = StringTokPtr::dynamicCast(yyvsp[-1]);
    StringTokPtr str2 = StringTokPtr::dynamicCast(yyvsp[0]);
    str1->v += str2->v;
}
#line 4176 "src/Slice/Grammar.cpp"
    break;

  case 194: /* string_list: string_list ',' string_literal  */
#line 2205 "src/Slice/Grammar.y"
{
    StringTokPtr str = StringTokPtr::dynamicCast(yyvsp[0]);
    StringListTokPtr stringList = StringListTokPtr::dynamicCast(yyvsp[-2]);
    stringList->v.push_back(str->v);
    yyval = stringList;
}
#line 4187 "src/Slice/Grammar.cpp"
    break;

  case 195: /* string_list: string_literal  */
#line 2212 "src/Slice/Grammar.y"
{
    StringTokPtr str = StringTokPtr::dynamicCast(yyvsp[0]);
    StringListTokPtr stringList = new StringListTok;
    stringList->v.push_back(str->v);
    yyval = stringList;
}
#line 4198 "src/Slice/Grammar.cpp"
    break;

  case 196: /* const_initializer: ICE_INTEGER_LITERAL  */
#line 2224 "src/Slice/Grammar.y"
{
    BuiltinPtr type = unit->builtin(Builtin::KindLong);
    IntegerTokPtr intVal = IntegerTokPtr::dynamicCast(yyvsp[0]);
    ostringstream sstr;
    sstr << intVal->v;
    ConstDefTokPtr def = new ConstDefTok(type, sstr.str(), intVal->literal);
    yyval = def;
}
#line 4211 "src/Slice/Grammar.cpp"
    break;

  case 197: /* const_initializer: ICE_FLOATING_POINT_LITERAL  */
#line 2233 "src/Slice/Grammar.y"
{
    BuiltinPtr type = unit->builtin(Builtin::KindDouble);
    FloatingTokPtr floatVal = FloatingTokPtr::dynamicCast(yyvsp[0]);
    ostringstream sstr;
    sstr << floatVal->v;
    ConstDefTokPtr def = new ConstDefTok(type, sstr.str(), floatVal->literal);
    yyval = def;
}
#line 4224 "src/Slice/Grammar.cpp"
    break;

  case 198: /* const_initializer: scoped_name  */
#line 2242 "src/Slice/Grammar.y"
{
    StringTokPtr scoped = StringTokPtr::dynamicCast(yyvsp[0]);
    ConstDefTokPtr def;
    ContainedList cl = unit->currentContainer()->lookupContained(scoped->v, false);
    if(cl.empty())
    {
        // Could be an enumerator
        def = new ConstDefTok(SyntaxTreeBasePtr(0), scoped->v, scoped->v);
    }
    else
    {
        EnumeratorPtr enumerator = EnumeratorPtr::dynamicCast(cl.front());
        ConstPtr constant = ConstPtr::dynamicCast(cl.front());
        if(enumerator)
        {
            unit->currentContainer()->checkIntroduced(scoped->v, enumerator);
            def = new ConstDefTok(enumerator, scoped->v, scoped->v);
        }
        else if(constant)
        {
            unit->currentContainer()->checkIntroduced(scoped->v, constant);
            def = new ConstDefTok(constant, constant->value(), constant->value());
        }
        else
        {
            def = new ConstDefTok;
            string msg = "illegal initializer: `" + scoped->v + "' is a";
            static const string vowels = "aeiou";
            string kindOf = cl.front()->kindOf();
            if(vowels.find_first_of(kindOf[0]) != string::npos)
            {
                msg += "n";
            }
            msg += " " + kindOf;
            unit->error(msg); // $$ is dummy
        }
    }
    yyval = def;
}
#line 4268 "src/Slice/Grammar.cpp"
    break;

  case 199: /* const_initializer: ICE_STRING_LITERAL  */
#line 2282 "src/Slice/Grammar.y"
{
    BuiltinPtr type = unit->builtin(Builtin::KindString);
    StringTokPtr literal = StringTokPtr::dynamicCast(yyvsp[0]);
    ConstDefTokPtr def = new ConstDefTok(type, literal->v, literal->literal);
    yyval = def;
}
#line 4279 "src/Slice/Grammar.cpp"
    break;

  case 200: /* const_initializer: ICE_FALSE  */
#line 2289 "src/Slice/Grammar.y"
{
    BuiltinPtr type = unit->builtin(Builtin::KindBool);
    StringTokPtr literal = StringTokPtr::dynamicCast(yyvsp[0]);
    ConstDefTokPtr def = new ConstDefTok(type, "false", "false");
    yyval = def;
}
#line 4290 "src/Slice/Grammar.cpp"
    break;

  case 201: /* const_initializer: ICE_TRUE  */
#line 2296 "src/Slice/Grammar.y"
{
    BuiltinPtr type = unit->builtin(Builtin::KindBool);
    StringTokPtr literal = StringTokPtr::dynamicCast(yyvsp[0]);
    ConstDefTokPtr def = new ConstDefTok(type, "true", "true");
    yyval = def;
}
#line 4301 "src/Slice/Grammar.cpp"
    break;

  case 202: /* const_def: ICE_CONST local_metadata type ICE_IDENTIFIER '=' const_initializer  */
#line 2308 "src/Slice/Grammar.y"
{
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[-4]);
    TypePtr const_type = TypePtr::dynamicCast(yyvsp[-3]);
    StringTokPtr ident = StringTokPtr::dynamicCast(yyvsp[-2]);
    ConstDefTokPtr value = ConstDefTokPtr::dynamicCast(yyvsp[0]);
    yyval = unit->currentModule()->createConst(ident->v, const_type, metaData->v, value->v,
                                               value->valueAsString, value->valueAsLiteral);
}
#line 4314 "src/Slice/Grammar.cpp"
    break;

  case 203: /* const_def: ICE_CONST local_metadata type '=' const_initializer  */
#line 2317 "src/Slice/Grammar.y"
{
    StringListTokPtr metaData = StringListTokPtr::dynamicCast(yyvsp[-3]);
    TypePtr const_type = TypePtr::dynamicCast(yyvsp[-2]);
    ConstDefTokPtr value = ConstDefTokPtr::dynamicCast(yyvsp[0]);
    unit->error("missing constant name");
    yyval = unit->currentModule()->createConst(IceUtil::generateUUID(), const_type, metaData->v, value->v,
                                               value->valueAsString, value->valueAsLiteral, Dummy); // Dummy
}
#line 4327 "src/Slice/Grammar.cpp"
    break;

  case 204: /* keyword: ICE_MODULE  */
#line 2330 "src/Slice/Grammar.y"
             {}
#line 4333 "src/Slice/Grammar.cpp"
    break;

  case 205: /* keyword: ICE_CLASS  */
#line 2331 "src/Slice/Grammar.y"
            {}
#line 4339 "src/Slice/Grammar.cpp"
    break;

  case 206: /* keyword: ICE_INTERFACE  */
#line 2332 "src/Slice/Grammar.y"
                {}
#line 4345 "src/Slice/Grammar.cpp"
    break;

  case 207: /* keyword: ICE_EXCEPTION  */
#line 2333 "src/Slice/Grammar.y"
                {}
#line 4351 "src/Slice/Grammar.cpp"
    break;

  case 208: /* keyword: ICE_STRUCT  */
#line 2334 "src/Slice/Grammar.y"
             {}
#line 4357 "src/Slice/Grammar.cpp"
    break;

  case 209: /* keyword: ICE_SEQUENCE  */
#line 2335 "src/Slice/Grammar.y"
               {}
#line 4363 "src/Slice/Grammar.cpp"
    break;

  case 210: /* keyword: ICE_DICTIONARY  */
#line 2336 "src/Slice/Grammar.y"
                 {}
#line 4369 "src/Slice/Grammar.cpp"
    break;

  case 211: /* keyword: ICE_ENUM  */
#line 2337 "src/Slice/Grammar.y"
           {}
#line 4375 "src/Slice/Grammar.cpp"
    break;

  case 212: /* keyword: ICE_OUT  */
#line 2338 "src/Slice/Grammar.y"
          {}
#line 4381 "src/Slice/Grammar.cpp"
    break;

  case 213: /* keyword: ICE_EXTENDS  */
#line 2339 "src/Slice/Grammar.y"
              {}
#line 4387 "src/Slice/Grammar.cpp"
    break;

  case 214: /* keyword: ICE_IMPLEMENTS  */
#line 2340 "src/Slice/Grammar.y"
                 {}
#line 4393 "src/Slice/Grammar.cpp"
    break;

  case 215: /* keyword: ICE_THROWS  */
#line 2341 "src/Slice/Grammar.y"
             {}
#line 4399 "src/Slice/Grammar.cpp"
    break;

  case 216: /* keyword: ICE_VOID  */
#line 2342 "src/Slice/Grammar.y"
           {}
#line 4405 "src/Slice/Grammar.cpp"
    break;

  case 217: /* keyword: ICE_BOOL  */
#line 2343 "src/Slice/Grammar.y"
           {}
#line 4411 "src/Slice/Grammar.cpp"
    break;

  case 218: /* keyword: ICE_BYTE  */
#line 2344 "src/Slice/Grammar.y"
           {}
#line 4417 "src/Slice/Grammar.cpp"
    break;

  case 219: /* keyword: ICE_SHORT  */
#line 2345 "src/Slice/Grammar.y"
            {}
#line 4423 "src/Slice/Grammar.cpp"
    break;

  case 220: /* keyword: ICE_USHORT  */
#line 2346 "src/Slice/Grammar.y"
             {}
#line 4429 "src/Slice/Grammar.cpp"
    break;

  case 221: /* keyword: ICE_INT  */
#line 2347 "src/Slice/Grammar.y"
          {}
#line 4435 "src/Slice/Grammar.cpp"
    break;

  case 222: /* keyword: ICE_UINT  */
#line 2348 "src/Slice/Grammar.y"
           {}
#line 4441 "src/Slice/Grammar.cpp"
    break;

  case 223: /* keyword: ICE_VARINT  */
#line 2349 "src/Slice/Grammar.y"
             {}
#line 4447 "src/Slice/Grammar.cpp"
    break;

  case 224: /* keyword: ICE_VARUINT  */
#line 2350 "src/Slice/Grammar.y"
              {}
#line 4453 "src/Slice/Grammar.cpp"
    break;

  case 225: /* keyword: ICE_LONG  */
#line 2351 "src/Slice/Grammar.y"
           {}
#line 4459 "src/Slice/Grammar.cpp"
    break;

  case 226: /* keyword: ICE_ULONG  */
#line 2352 "src/Slice/Grammar.y"
            {}
#line 4465 "src/Slice/Grammar.cpp"
    break;

  case 227: /* keyword: ICE_VARLONG  */
#line 2353 "src/Slice/Grammar.y"
              {}
#line 4471 "src/Slice/Grammar.cpp"
    break;

  case 228: /* keyword: ICE_VARULONG  */
#line 2354 "src/Slice/Grammar.y"
               {}
#line 4477 "src/Slice/Grammar.cpp"
    break;

  case 229: /* keyword: ICE_FLOAT  */
#line 2355 "src/Slice/Grammar.y"
            {}
#line 4483 "src/Slice/Grammar.cpp"
    break;

  case 230: /* keyword: ICE_DOUBLE  */
#line 2356 "src/Slice/Grammar.y"
             {}
#line 4489 "src/Slice/Grammar.cpp"
    break;

  case 231: /* keyword: ICE_STRING  */
#line 2357 "src/Slice/Grammar.y"
             {}
#line 4495 "src/Slice/Grammar.cpp"
    break;

  case 232: /* keyword: ICE_OBJECT  */
#line 2358 "src/Slice/Grammar.y"
             {}
#line 4501 "src/Slice/Grammar.cpp"
    break;

  case 233: /* keyword: ICE_CONST  */
#line 2359 "src/Slice/Grammar.y"
            {}
#line 4507 "src/Slice/Grammar.cpp"
    break;

  case 234: /* keyword: ICE_FALSE  */
#line 2360 "src/Slice/Grammar.y"
            {}
#line 4513 "src/Slice/Grammar.cpp"
    break;

  case 235: /* keyword: ICE_TRUE  */
#line 2361 "src/Slice/Grammar.y"
           {}
#line 4519 "src/Slice/Grammar.cpp"
    break;

  case 236: /* keyword: ICE_IDEMPOTENT  */
#line 2362 "src/Slice/Grammar.y"
                 {}
#line 4525 "src/Slice/Grammar.cpp"
    break;

  case 237: /* keyword: ICE_TAG  */
#line 2363 "src/Slice/Grammar.y"
          {}
#line 4531 "src/Slice/Grammar.cpp"
    break;

  case 238: /* keyword: ICE_OPTIONAL  */
#line 2364 "src/Slice/Grammar.y"
               {}
#line 4537 "src/Slice/Grammar.cpp"
    break;

  case 239: /* keyword: ICE_ANYCLASS  */
#line 2365 "src/Slice/Grammar.y"
               {}
#line 4543 "src/Slice/Grammar.cpp"
    break;

  case 240: /* keyword: ICE_VALUE  */
#line 2366 "src/Slice/Grammar.y"
            {}
#line 4549 "src/Slice/Grammar.cpp"
    break;


#line 4553 "src/Slice/Grammar.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 2369 "src/Slice/Grammar.y"


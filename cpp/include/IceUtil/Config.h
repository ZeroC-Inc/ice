//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_UTIL_CONFIG_H
#define ICE_UTIL_CONFIG_H

// Compiler extensions to export and import symbols: see the documentation for Visual Studio, Clang and GCC.
#if defined(_MSC_VER)
#    define ICE_DECLSPEC_EXPORT __declspec(dllexport)
#    define ICE_DECLSPEC_IMPORT __declspec(dllimport)
//  With Visual Studio, we can import/export member functions without importing/exporting the whole class.
#    define ICE_MEMBER_IMPORT_EXPORT
#elif defined(__GNUC__) || defined(__clang__)
#    define ICE_DECLSPEC_EXPORT __attribute__((visibility("default")))
#    define ICE_DECLSPEC_IMPORT __attribute__((visibility("default")))
#else
#    define ICE_DECLSPEC_EXPORT /**/
#    define ICE_DECLSPEC_IMPORT /**/
#endif

#ifdef ICE_MEMBER_IMPORT_EXPORT
#    define ICE_CLASS(API) /**/
#    define ICE_MEMBER(API) API
#else
#    define ICE_CLASS(API) API
#    define ICE_MEMBER(API) /**/
#endif

#ifndef ICE_API
#    if defined(ICE_STATIC_LIBS)
#        define ICE_API /**/
#    elif defined(ICE_API_EXPORTS)
#        define ICE_API ICE_DECLSPEC_EXPORT
#    else
#        define ICE_API ICE_DECLSPEC_IMPORT
#    endif
#endif

#ifdef __APPLE__
#    include <TargetConditionals.h>
#endif

// The Ice version.
#define ICE_STRING_VERSION "3.8.0-alpha.0" // "A.B.C", with A=major, B=minor, C=patch
#define ICE_INT_VERSION 30850              // AABBCC, with AA=major, BB=minor, CC=patch
#define ICE_SO_VERSION "38a0"              // "ABC", with A=major, B=minor, C=patch

#if !defined(ICE_BUILDING_ICE) && defined(ICE_API_EXPORTS)
#    define ICE_BUILDING_ICE
#endif

#if defined(_MSC_VER)
#    if !defined(ICE_STATIC_LIBS) && (!defined(_DLL) || !defined(_MT))
#        error "Only multi-threaded DLL libraries can be used with Ice!"
#    endif
#    if defined(_DEBUG)
#        define ICE_LIBNAME(NAME) NAME ICE_SO_VERSION "D.lib"
#    else
#        define ICE_LIBNAME(NAME) NAME ICE_SO_VERSION ".lib"
#    endif
//  Automatically link with Ice[D].lib when using MSVC
#    if !defined(ICE_BUILDING_ICE) && !defined(ICE_BUILDING_SLICE_COMPILERS)
#        pragma comment(lib, ICE_LIBNAME("Ice"))
#    endif
#endif

#endif

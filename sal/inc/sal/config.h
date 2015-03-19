/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 * 
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Portions of this file are part of the LibreOffice project.
 *
 *   This Source Code Form is subject to the terms of the Mozilla Public
 *   License, v. 2.0. If a copy of the MPL was not distributed with this
 *   file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 ************************************************************************/

#ifndef _SAL_CONFIG_H_
#define _SAL_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#ifndef INCLUDED_STDLIB_H
#include <stdlib.h>
#define INCLUDED_STDLIB_H
#endif

#ifdef WIN32
#define SAL_W32
#define SAL_DLLEXTENSION ".dll"
#define SAL_PRGEXTENSION ".exe"
#define SAL_PATHSEPARATOR ';'
#define SAL_PATHDELIMITER '\\'
#if SUPD == 310
#define SAL_NEWLINE_STRING "\r\n"
#endif	// SUPD == 310
#define SAL_CONFIGFILE( name ) name ".ini"
#define SAL_SYSCONFIGFILE( name ) name ".ini"

#ifdef _MSC_VER

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES // needed by Visual C++ for math constants
#endif

/* warnings ought to be disabled using /wd:n option of msvc.net compiler */
#if _MSC_VER < 1300
/* no performance warning: int to bool */
#pragma warning( disable : 4800 )

/* No warning for: C++ Exception Specification ignored */
#pragma warning( disable : 4290 )

#if OSL_DEBUG_LEVEL > 0
/* No warning for: identifier was truncated to '255' characters in the browser information */
#pragma warning( disable : 4786 )
#endif
#endif /* defined _MSC_VER < 1300 */

#endif /* defined _MSC_VER */

/* Provide ISO C99 compatible versions of snprint and vsnprintf */
#ifdef __MINGW32__
#define _SNPRINTF_DLLIMPORT
#endif
#ifndef _SNPRINTF_H
#include <systools/win32/snprintf.h>
#endif

#endif /* defined WIN32 */

/* BR: 16bit fuer Borland-Compiler */
#ifdef __BORLANDC__
#define SAL_W16
#define SAL_DLLEXTENSION ".dll"
#endif
/* BR: 16bit fuer Borland-Compiler */

#ifdef OS2
#define SAL_OS2
#define SAL_DLLEXTENSION ".dll"
#define SAL_PRGEXTENSION ".exe"
#define SAL_PATHSEPARATOR ';'
#define SAL_PATHDELIMITER '\\'
#define SAL_CONFIGFILE( name ) name ".ini"
#define SAL_SYSCONFIGFILE( name ) name ".ini"
#endif

#if defined(SOLARIS) || defined(LINUX) || defined(NETBSD) || defined(FREEBSD) || defined(SCO) || defined(IRIX)
#define SAL_UNX
#define SAL_DLLEXTENSION ".so"
#define SAL_DLLPREFIX "lib"
#define SAL_PRGEXTENSION ".bin"
#define SAL_PATHSEPARATOR ':'
#define SAL_PATHDELIMITER '/'
#if SUPD == 310
#define SAL_NEWLINE_STRING "\n"
#endif	// SUPD == 310
#define SAL_CONFIGFILE( name ) name "rc"
#define SAL_SYSCONFIGFILE( name ) "." name "rc"
#endif

#ifdef MACOSX
#define SAL_UNX
#define SAL_DLLEXTENSION ".dylib"
#define SAL_DLLPREFIX "lib"
#define SAL_PRGEXTENSION ".bin"
#define SAL_PATHSEPARATOR ':'
#define SAL_PATHDELIMITER '/'
#if SUPD == 310
#define SAL_NEWLINE_STRING "\n"
#endif	// SUPD == 310
#define SAL_CONFIGFILE( name ) name "rc"
#define SAL_SYSCONFIGFILE( name ) "." name "rc"
#endif

#ifdef HPUX
#define SAL_UNX
#define SAL_DLLEXTENSION ".sl"
#define SAL_DLLPREFIX "lib"
#define SAL_PRGEXTENSION ".bin"
#define SAL_CONFIGFILE( name ) name "rc"
#define SAL_SYSCONFIGFILE( name ) "." name "rc"
#endif

#ifdef sun
#undef sun
#define sun sun
#endif

/* This is to work around a gcc 3.3 error that fixing actually breaks other
 * compilers.  This will create a dummy variable specifically for gcc 3.3 that
 * allows it to compile and not break the others.  Other compilers may follow
 * with this eror later. */
#if defined __GNUC__
#if ((__GNUC__ == 3) && (__GNUC_MINOR__ > 2))
#  define SAL_ISO_CONST const
#else
#  define SAL_ISO_CONST
#endif
#endif

#endif /*_SAL_CONFIG_H_ */


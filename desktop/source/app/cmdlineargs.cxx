/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <config_features.h>

#if HAVE_FEATURE_MACOSX_SANDBOX
#include <premac.h>
#include <Foundation/Foundation.h>
#include <postmac.h>
#endif

#include "cmdlineargs.hxx"
#include <vcl/svapp.hxx>
#include <rtl/uri.hxx>
#ifndef NO_LIBO_NEW_COMMAND_LINE_ARGS
#include <tools/urlobj.hxx>
#endif	// !NO_LIBO_NEW_COMMAND_LINE_ARGS
#include <rtl/ustring.hxx>
#include <rtl/process.h>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/uri/ExternalUriReferenceTranslator.hpp>
#include <tools/getprocessworkingdir.hxx>

#include <svl/documentlockfile.hxx>

#include <cstdio>

using namespace com::sun::star::lang;
using namespace com::sun::star::uri;
using namespace com::sun::star::uno;


namespace desktop
{

namespace {

OUString translateExternalUris(OUString const & input) {
    OUString t(
        com::sun::star::uri::ExternalUriReferenceTranslator::create(
            comphelper::getProcessComponentContext())->
        translateToInternal(input));
    return t.isEmpty() ? input : t;
}

std::vector< OUString > translateExternalUris(
    std::vector< OUString > const & input)
{
    std::vector< OUString > t;
    for (std::vector< OUString >::const_iterator i(input.begin());
         i != input.end(); ++i)
    {
        t.push_back(translateExternalUris(*i));
    }
    return t;
}

class ExtCommandLineSupplier: public CommandLineArgs::Supplier {
public:
    explicit ExtCommandLineSupplier():
        m_count(rtl_getAppCommandArgCount()),
        m_index(0)
    {
        OUString url;
        if (tools::getProcessWorkingDir(url)) {
            m_cwdUrl.reset(url);
        }
    }

    virtual ~ExtCommandLineSupplier() {}

    virtual boost::optional< OUString > getCwdUrl() SAL_OVERRIDE { return m_cwdUrl; }

    virtual bool next(OUString * argument) SAL_OVERRIDE {
        OSL_ASSERT(argument != NULL);
        if (m_index < m_count) {
            rtl_getAppCommandArg(m_index++, &argument->pData);
            return true;
        } else {
            return false;
        }
    }

private:
    boost::optional< OUString > m_cwdUrl;
    sal_uInt32 m_count;
    sal_uInt32 m_index;
};

#ifndef NO_LIBO_NEW_COMMAND_LINE_ARGS

enum class CommandLineEvent {
    Open, Print, View, Start, PrintTo,
    ForceOpen, ForceNew, Conversion, BatchPrint
};

// Office URI Schemes: see https://msdn.microsoft.com/en-us/library/dn906146
// This functions checks if the arg is an Office URI.
// If applicable, it updates arg to inner URI.
// If no event argument is explicitly set in command line,
// then it returns updated command line event,
// according to Office URI command.
CommandLineEvent CheckOfficeURI(/* in,out */ OUString& arg, CommandLineEvent curEvt)
{
     // 1. Strip the scheme name
     OUString rest1;
     bool isOfficeURI = ( arg.startsWithIgnoreAsciiCase("vnd.libreoffice.command:", &rest1) // Proposed extended schema
                       || arg.startsWithIgnoreAsciiCase("ms-word:",                 &rest1)
                       || arg.startsWithIgnoreAsciiCase("ms-powerpoint:",           &rest1)
                       || arg.startsWithIgnoreAsciiCase("ms-excel:",                &rest1)
                       || arg.startsWithIgnoreAsciiCase("ms-visio:",                &rest1)
                       || arg.startsWithIgnoreAsciiCase("ms-access:",               &rest1));
    if (!isOfficeURI)
        return curEvt;

    OUString rest2;
    long nURIlen = -1;
    // 2. Discriminate by command name (incl. 1st command argument descriptor)
    //    Extract URI: everything up to possible next argument
    if (rest1.startsWith("ofv|u|", &rest2))
    {
        // Open for view - override only in default mode
        if (curEvt == CommandLineEvent::Open)
            curEvt = CommandLineEvent::View;
        nURIlen = rest2.indexOf("|");
    }
    else if (rest1.startsWith("ofe|u|", &rest2))
    {
        // Open for editing - override only in default mode
        if (curEvt == CommandLineEvent::Open)
            curEvt = CommandLineEvent::ForceOpen;
        nURIlen = rest2.indexOf("|");
    }
    else
    {
        // Abbreviated scheme: <scheme-name>:URI
        // "ofv|u|" implied
        // override only in default mode
        if (curEvt == CommandLineEvent::Open)
            curEvt = CommandLineEvent::View;
        rest2 = rest1;
    }
    if (nURIlen < 0)
        nURIlen = rest2.getLength();
    auto const uri = rest2.copy(0, nURIlen);
    if (INetURLObject(uri).GetProtocol() == INetProtocol::INET_PROT_MACRO) {
        // Let the "Open" machinery process the full command URI (leading to failure, by intention,
        // as the "Open" machinery does not know about those command URI schemes):
        curEvt = CommandLineEvent::Open;
    } else {
        arg = uri;
    }
    return curEvt;
#endif	// !NO_LIBO_NEW_COMMAND_LINE_ARGS
}

#ifndef NO_LIBO_NEW_COMMAND_LINE_ARGS
} // namespace
#endif	// !NO_LIBO_NEW_COMMAND_LINE_ARGS

CommandLineArgs::Supplier::Exception::Exception() {}

CommandLineArgs::Supplier::Exception::Exception(Exception const &) {}

CommandLineArgs::Supplier::Exception::~Exception() {}

CommandLineArgs::Supplier::Exception &
CommandLineArgs::Supplier::Exception::operator =(Exception const &)
{ return *this; }

CommandLineArgs::Supplier::~Supplier() {}

// intialize class with command line parameters from process environment
CommandLineArgs::CommandLineArgs()
{
    InitParamValues();
    ExtCommandLineSupplier s;
    ParseCommandLine_Impl( s );
}

CommandLineArgs::CommandLineArgs( Supplier& supplier )
{
    InitParamValues();
    ParseCommandLine_Impl( supplier );
}



void CommandLineArgs::ParseCommandLine_Impl( Supplier& supplier )
{
    m_cwdUrl = supplier.getCwdUrl();

#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
    // parse command line arguments
    bool bOpenEvent(true);
    bool bPrintEvent(false);
    bool bViewEvent(false);
    bool bStartEvent(false);
    bool bPrintToEvent(false);
    bool bPrinterName(false);
    bool bForceOpenEvent(false);
    bool bForceNewEvent(false);
    bool bDisplaySpec(false);
    bool bOpenDoc(false);
    bool bConversionEvent(false);
    bool bConversionParamsEvent(false);
    bool bBatchPrintEvent(false);
    bool bBatchPrinterNameEvent(false);
    bool bConversionOutEvent(false);
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
    CommandLineEvent eCurrentEvent = CommandLineEvent::Open;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS

    for (;;)
    {
        OUString aArg;
        if ( !supplier.next( &aArg ) )
        {
            break;
        }

        if ( !aArg.isEmpty() )
        {
            m_bEmpty = false;
            OUString oArg;
            bool bDeprecated = !aArg.startsWith("--", &oArg)
                && aArg.startsWith("-", &oArg) && aArg.getLength() > 2;
                // -h, -?, -n, -o, -p are still valid

            OUString rest;
            if ( oArg == "minimized" )
            {
                m_minimized = true;
            }
            else if ( oArg == "invisible" )
            {
                m_invisible = true;
            }
            else if ( oArg == "norestore" )
            {
                m_norestore = true;
            }
            else if ( oArg == "nodefault" )
            {
                m_nodefault = true;
            }
            else if ( oArg == "headless" )
            {
                // Headless means also invisibile, so set this parameter to true!
                m_headless = true;
                m_invisible = true;
            }
            else if ( oArg == "cat" )
            {
                m_textcat = true;
                m_conversionparams = "txt:Text";
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bOpenEvent = false;
                bConversionEvent = true;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::Conversion;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "quickstart" )
            {
#if defined(ENABLE_QUICKSTART_APPLET)
                m_quickstart = true;
#endif
                m_noquickstart = false;
            }
            else if ( oArg == "quickstart=no" )
            {
                m_noquickstart = true;
                m_quickstart = false;
            }
            else if ( oArg == "terminate_after_init" )
            {
                m_terminateafterinit = true;
            }
            else if ( oArg == "nofirststartwizard" )
            {
                m_nofirststartwizard = true;
            }
            else if ( oArg == "nologo" )
            {
                m_nologo = true;
            }
#if HAVE_FEATURE_MULTIUSER_ENVIRONMENT
            else if ( oArg == "nolockcheck" )
            {
                m_nolockcheck = true;
            }
#endif
            else if ( oArg == "help" || aArg == "-h" || aArg == "-?" )
            {
                m_help = true;
            }
            else if ( oArg == "helpwriter" )
            {
                m_helpwriter = true;
            }
            else if ( oArg == "helpcalc" )
            {
                m_helpcalc = true;
            }
            else if ( oArg == "helpdraw" )
            {
                m_helpdraw = true;
            }
            else if ( oArg == "helpimpress" )
            {
                m_helpimpress = true;
            }
            else if ( oArg == "helpbase" )
            {
                m_helpbase = true;
            }
            else if ( oArg == "helpbasic" )
            {
                m_helpbasic = true;
            }
            else if ( oArg == "helpmath" )
            {
                m_helpmath = true;
            }
            else if ( oArg == "version" )
            {
                m_version = true;
            }
            else if ( oArg.startsWith("splash-pipe=") )
            {
                m_splashpipe = true;
            }
#ifdef MACOSX
            /* #i84053# ignore -psn on Mac
               Platform dependent #ifdef here is ugly, however this is currently
               the only platform dependent parameter. Should more appear
               we should find a better solution
            */
            else if ( aArg.startsWith("-psn") )
            {
                bDeprecated = false;
            }
#endif
#if HAVE_FEATURE_MACOSX_SANDBOX
            else if ( oArg == "nstemporarydirectory" )
            {
                printf("%s\n", [NSTemporaryDirectory() UTF8String]);
                exit(0);
            }
#endif
#ifdef WIN32
            /* fdo#57203 ignore -Embedding on Windows
               when LibreOffice is launched by COM+
            */
            else if ( oArg == "Embedding" )
            {
                bDeprecated = false;
            }
#endif
            else if ( oArg.startsWith("infilter=", &rest))
            {
                m_infilter.push_back(rest);
            }
            else if ( oArg.startsWith("accept=", &rest))
            {
                m_accept.push_back(rest);
            }
            else if ( oArg.startsWith("unaccept=", &rest))
            {
                m_unaccept.push_back(rest);
            }
            else if ( oArg.startsWith("language=", &rest))
            {
                m_language = rest;
            }
            else if ( oArg.startsWith("pidfile=", &rest))
            {
                m_pidfile = rest;
            }
            else if ( oArg == "writer" )
            {
                m_writer = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "calc" )
            {
                m_calc = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "draw" )
            {
                m_draw = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "impress" )
            {
                m_impress = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "base" )
            {
                m_base = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "global" )
            {
                m_global = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "math" )
            {
                m_math = true;
                m_bDocumentArgs = true;
            }
            else if ( oArg == "web" )
            {
                m_web = true;
                m_bDocumentArgs = true;
            }
            else if ( aArg == "-n" )
            {
                // force new documents based on the following documents
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bForceNewEvent  = true;
                bOpenEvent      = false;
                bForceOpenEvent = false;
                bPrintToEvent   = false;
                bPrintEvent     = false;
                bViewEvent      = false;
                bStartEvent     = false;
                bDisplaySpec    = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::ForceNew;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( aArg == "-o" )
            {
                // force open documents regardless if they are templates or not
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bForceOpenEvent = true;
                bOpenEvent      = false;
                bForceNewEvent  = false;
                bPrintToEvent   = false;
                bPrintEvent     = false;
                bViewEvent      = false;
                bStartEvent     = false;
                bDisplaySpec    = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::ForceOpen;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "pt" )
            {
                // Print to special printer
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bPrintToEvent   = true;
                bPrinterName    = true;
                bPrintEvent     = false;
                bOpenEvent      = false;
                bForceNewEvent  = false;
                bViewEvent      = false;
                bStartEvent     = false;
                bDisplaySpec    = false;
                bForceOpenEvent = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::PrintTo;
                // first argument after "-pt" must be the printer name
                if (supplier.next(&aArg))
                    m_printername = aArg;
                else if (m_unknown.isEmpty())
                    m_unknown = "--pt must be followed by printername";
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( aArg == "-p" )
            {
                // Print to default printer
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bPrintEvent     = true;
                bPrintToEvent   = false;
                bOpenEvent      = false;
                bForceNewEvent  = false;
                bForceOpenEvent = false;
                bViewEvent      = false;
                bStartEvent     = false;
                bDisplaySpec    = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::Print;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "view")
            {
                // open in viewmode
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bOpenEvent      = false;
                bPrintEvent     = false;
                bPrintToEvent   = false;
                bForceNewEvent  = false;
                bForceOpenEvent = false;
                bViewEvent      = true;
                bStartEvent     = false;
                bDisplaySpec    = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::View;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "show" )
            {
                // open in viewmode
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bOpenEvent      = false;
                bViewEvent      = false;
                bStartEvent     = true;
                bPrintEvent     = false;
                bPrintToEvent   = false;
                bForceNewEvent  = false;
                bForceOpenEvent = false;
                bDisplaySpec    = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::Start;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "display" )
            {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                // set display
                bOpenEvent      = false;
                bPrintEvent     = false;
                bForceOpenEvent = false;
                bPrintToEvent   = false;
                bForceNewEvent  = false;
                bViewEvent      = false;
                bStartEvent     = false;
                bDisplaySpec    = true;
            }
            else if ( oArg == "language" )
            {
                bOpenEvent      = false;
                bPrintEvent     = false;
                bForceOpenEvent = false;
                bPrintToEvent   = false;
                bForceNewEvent  = false;
                bViewEvent      = false;
                bStartEvent     = false;
                bDisplaySpec    = false;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                // The command line argument following --display should
                // always be treated as the argument of --display.
                // --display and its argument are handled "out of line"
                // in Unix-only desktop/unx/source/splashx.c and vcl/unx/*,
                // and just ignored here
                supplier.next(&aArg);
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "convert-to" )
            {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bOpenEvent = false;
                bConversionEvent = true;
                bConversionParamsEvent = true;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::Conversion;
                // first argument must be the params
                if (supplier.next(&aArg))
                {
                    m_conversionparams = aArg;
                    // It doesn't make sense to use convert-to without headless.
                    setHeadless();
                }
                else if (m_unknown.isEmpty())
                    m_unknown = "--convert-to must be followed by output_file_extension[:output_filter_name]";
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( oArg == "print-to-file" )
            {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bOpenEvent = false;
                bBatchPrintEvent = true;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                eCurrentEvent = CommandLineEvent::BatchPrint;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
            else if ( oArg == "printer-name" && bBatchPrintEvent )
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            else if ( eCurrentEvent == CommandLineEvent::BatchPrint && oArg == "printer-name" )
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bBatchPrinterNameEvent = true;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                // first argument is the printer name
                if (supplier.next(&aArg))
                    m_printername = aArg;
                else if (m_unknown.isEmpty())
                    m_unknown = "--printer-name must be followed by printername";
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
            else if ( oArg == "outdir" &&
                      (bConversionEvent || bBatchPrintEvent) )
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            else if ( (eCurrentEvent == CommandLineEvent::Conversion ||
                       eCurrentEvent == CommandLineEvent::BatchPrint)
                      && oArg == "outdir" )
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                bConversionOutEvent = true;
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                if (supplier.next(&aArg))
                    m_conversionout = aArg;
                else if (m_unknown.isEmpty())
                    m_unknown = "--outdir must be followed by output directory path";
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
            }
            else if ( aArg.startsWith("-") )
            {
                // because it's impossible to filter these options that
                // are handled in the soffice shell script with the
                // primitive tools that /bin/sh offers, ignore them here
                if (
#if defined UNX
                    oArg != "backtrace" &&
                    oArg != "strace" &&
                    oArg != "valgrind" &&
                    // for X Session Management, handled in
                    // vcl/unx/generic/app/sm.cxx:
                    oArg != "session=" &&
#endif
                    //ignore additional legacy options that don't do anything anymore
                    oArg != "nocrashreport" &&
                    m_unknown.isEmpty())
                {
                    m_unknown = aArg;
                }
                bDeprecated = false;
            }
            else
            {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                if ( bPrinterName && bPrintToEvent )
                {
                    // first argument after "-pt" this must be the printer name
                    m_printername = aArg;
                    bPrinterName = false;
                }
                else if ( bConversionParamsEvent && bConversionEvent )
                {
                    // first argument must be the params
                    m_conversionparams = aArg;
                    bConversionParamsEvent = false;
                }
                else if ( bBatchPrinterNameEvent && bBatchPrintEvent )
                {
                    // first argument is the printer name
                    m_printername = aArg;
                    bBatchPrinterNameEvent = false;
                }
                else if ( (bConversionEvent || bBatchPrintEvent) && bConversionOutEvent )
                {
                    m_conversionout = aArg;
                    bConversionOutEvent = false;
                }
                else
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                // handle this argument as a filename

                // First check if this is an Office URI
                // This will possibly adjust event for this argument
                // and put real URI to aArg
                CommandLineEvent eThisEvent = CheckOfficeURI(aArg, eCurrentEvent);

                switch (eThisEvent)
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                {
#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
                    // handle this argument as a filename
                    if ( bOpenEvent )
                    {
                        m_openlist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bViewEvent )
                    {
                        m_viewlist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bStartEvent )
                    {
                        m_startlist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bPrintEvent )
                    {
                        m_printlist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bPrintToEvent )
                    {
                        m_printtolist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bForceNewEvent )
                    {
                        m_forcenewlist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bForceOpenEvent )
                    {
                        m_forceopenlist.push_back(aArg);
                        bOpenDoc = true;
                    }
                    else if ( bDisplaySpec )
                    {
                        bDisplaySpec = false; // only one display, not a lsit
                        bOpenEvent = true;    // set back to standard
                    }
                    else if ( bConversionEvent || bBatchPrintEvent )
                        m_conversionlist.push_back(aArg);
#else	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                case CommandLineEvent::Open:
                    m_openlist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::View:
                    m_viewlist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::Start:
                    m_startlist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::Print:
                    m_printlist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::PrintTo:
                    m_printtolist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::ForceNew:
                    m_forcenewlist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::ForceOpen:
                    m_forceopenlist.push_back(aArg);
                    m_bDocumentArgs = true;
                    break;
                case CommandLineEvent::Conversion:
                case CommandLineEvent::BatchPrint:
                    m_conversionlist.push_back(aArg);
                    break;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
                }
            }

            if (bDeprecated)
            {
                OString sArg(OUStringToOString(aArg, osl_getThreadTextEncoding()));
                fprintf(stderr, "Warning: %s is deprecated.  Use -%s instead.\n", sArg.getStr(), sArg.getStr());
            }
        }
    }

#ifdef NO_LIBO_NEW_COMMAND_LINE_ARGS
    if ( bOpenDoc )
        m_bDocumentArgs = true;
#endif	// NO_LIBO_NEW_COMMAND_LINE_ARGS
}

void CommandLineArgs::InitParamValues()
{
    m_minimized = false;
    m_norestore = false;
#ifdef LIBO_HEADLESS
    m_invisible = true;
    m_headless = true;
#else
    m_invisible = false;
    m_headless = false;
#endif
    m_quickstart = false;
    m_noquickstart = false;
    m_terminateafterinit = false;
    m_nofirststartwizard = false;
    m_nologo = false;
    m_nolockcheck = false;
    m_nodefault = false;
    m_help = false;
    m_writer = false;
    m_calc = false;
    m_draw = false;
    m_impress = false;
    m_global = false;
    m_math = false;
    m_web = false;
    m_base = false;
    m_helpwriter = false;
    m_helpcalc = false;
    m_helpdraw = false;
    m_helpbasic = false;
    m_helpmath = false;
    m_helpimpress = false;
    m_helpbase = false;
    m_version = false;
    m_splashpipe = false;
    m_bEmpty = true;
    m_bDocumentArgs  = false;
    m_textcat = false;
}


























bool CommandLineArgs::HasModuleParam() const
{
    return m_writer || m_calc || m_draw || m_impress || m_global || m_math
        || m_web || m_base;
}




std::vector< OUString > CommandLineArgs::GetOpenList() const
{
    return translateExternalUris(m_openlist);
}

std::vector< OUString > CommandLineArgs::GetViewList() const
{
    return translateExternalUris(m_viewlist);
}

std::vector< OUString > CommandLineArgs::GetStartList() const
{
    return translateExternalUris(m_startlist);
}

std::vector< OUString > CommandLineArgs::GetForceOpenList() const
{
    return translateExternalUris(m_forceopenlist);
}

std::vector< OUString > CommandLineArgs::GetForceNewList() const
{
    return translateExternalUris(m_forcenewlist);
}

std::vector< OUString > CommandLineArgs::GetPrintList() const
{
    return translateExternalUris(m_printlist);
}

std::vector< OUString > CommandLineArgs::GetPrintToList() const
{
    return translateExternalUris(m_printtolist);
}




std::vector< OUString > CommandLineArgs::GetConversionList() const
{
    return translateExternalUris(m_conversionlist);
}

OUString CommandLineArgs::GetConversionOut() const
{
    return translateExternalUris(m_conversionout);
}




} // namespace desktop

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
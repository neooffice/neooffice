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
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_ucb.hxx"

#include <string.h>
#include <ne_xml.h>
#include "LockEntrySequence.hxx"

using namespace webdav_ucp;
using namespace com::sun::star;

//////////////////////////////////////////////////////////////////////////

struct LockEntrySequenceParseContext
{
    ucb::LockEntry * pEntry;
    bool hasScope;
    bool hasType;

    LockEntrySequenceParseContext()
    : pEntry( 0 ), hasScope( false ), hasType( false ) {}
    ~LockEntrySequenceParseContext() { delete pEntry; }
};

#define STATE_TOP (1)

#define STATE_LOCKENTRY (STATE_TOP)
#define STATE_LOCKSCOPE (STATE_TOP + 1)
#define STATE_EXCLUSIVE (STATE_TOP + 2)
#define STATE_SHARED    (STATE_TOP + 3)
#define STATE_LOCKTYPE  (STATE_TOP + 4)
#define STATE_WRITE     (STATE_TOP + 5)

//////////////////////////////////////////////////////////////////////////
extern "C" int LockEntrySequence_startelement_callback(
    void *,
    int parent,
    const char *nspace,
    const char *name,
    const char ** )
{
    if ( ( name != 0 ) &&
         ( ( nspace == 0 ) || ( strcmp( nspace, "" ) == 0 ) ) )
    {
        switch ( parent )
        {
            case NE_XML_STATEROOT:
                if ( strcmp( name, "lockentry" ) == 0 )
                    return STATE_LOCKENTRY;
                break;

            case STATE_LOCKENTRY:
                if ( strcmp( name, "lockscope" ) == 0 )
                    return STATE_LOCKSCOPE;
                else if ( strcmp( name, "locktype" ) == 0 )
                    return STATE_LOCKTYPE;
                break;

            case STATE_LOCKSCOPE:
                if ( strcmp( name, "exclusive" ) == 0 )
                    return STATE_EXCLUSIVE;
                else if ( strcmp( name, "shared" ) == 0 )
                    return STATE_SHARED;
                break;

            case STATE_LOCKTYPE:
                if ( strcmp( name, "write" ) == 0 )
                    return STATE_WRITE;
                break;
        }
    }
    return NE_XML_DECLINE;
}

//////////////////////////////////////////////////////////////////////////
extern "C" int LockEntrySequence_chardata_callback(
    void *,
    int,
    const char *,
    size_t )
{
    return 0; // zero to continue, non-zero to abort parsing
}

//////////////////////////////////////////////////////////////////////////
extern "C" int LockEntrySequence_endelement_callback(
    void *userdata,
    int state,
    const char *,
    const char * )
{
    LockEntrySequenceParseContext * pCtx
                = static_cast< LockEntrySequenceParseContext * >( userdata );
    if ( !pCtx->pEntry )
        pCtx->pEntry = new ucb::LockEntry;

    switch ( state )
    {
        case STATE_EXCLUSIVE:
            pCtx->pEntry->Scope = ucb::LockScope_EXCLUSIVE;
            pCtx->hasScope = true;
            break;

        case STATE_SHARED:
            pCtx->pEntry->Scope = ucb::LockScope_SHARED;
            pCtx->hasScope = true;
            break;

        case STATE_WRITE:
            pCtx->pEntry->Type = ucb::LockType_WRITE;
            pCtx->hasType = true;
            break;

        case STATE_LOCKSCOPE:
            if ( !pCtx->hasScope )
                return 1; // abort
            break;

        case STATE_LOCKTYPE:
            if ( !pCtx->hasType )
                return 1; // abort
            break;

        case STATE_LOCKENTRY:
            if ( !pCtx->hasType || !pCtx->hasType )
                return 1; // abort
            break;

        default:
            break;
    }
    return 0; // zero to continue, non-zero to abort parsing
}

//////////////////////////////////////////////////////////////////////////
// static
bool LockEntrySequence::createFromXML( const rtl::OString & rInData,
                                       uno::Sequence<
									   		ucb::LockEntry > & rOutData )
{
    const sal_Int32 TOKEN_LENGTH = 12; // </lockentry>
    bool success = true;

    // rInData may contain multiple <lockentry>...</lockentry> tags.
    sal_Int32 nCount = 0;
    sal_Int32 nStart = 0;
    sal_Int32 nEnd   = rInData.indexOf( "</lockentry>" );
    while ( nEnd > -1 )
    {
        ne_xml_parser * parser = ne_xml_create();
        if ( !parser )
        {
            success = false;
            break;
        }

        LockEntrySequenceParseContext aCtx;
        ne_xml_push_handler( parser,
                             LockEntrySequence_startelement_callback,
                             LockEntrySequence_chardata_callback,
                             LockEntrySequence_endelement_callback,
                             &aCtx );

        ne_xml_parse( parser,
                       rInData.getStr() + nStart,
                       nEnd - nStart + TOKEN_LENGTH );

#if NEON_VERSION >= 0x0250
        success = !ne_xml_failed( parser );
#else
        success = !!ne_xml_valid( parser );
#endif

        ne_xml_destroy( parser );

        if ( !success )
            break;

        if ( aCtx.pEntry )
        {
            nCount++;
            if ( nCount > rOutData.getLength() )
                rOutData.realloc( rOutData.getLength() + 2 );

            rOutData[ nCount - 1 ] = *aCtx.pEntry;
        }

        nStart = nEnd + TOKEN_LENGTH + 1;
        nEnd   = rInData.indexOf( "</lockentry>", nStart );
    }

    rOutData.realloc( nCount );
    return success;
}
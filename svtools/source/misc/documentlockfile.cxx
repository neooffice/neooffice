/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified January 2015 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svtools.hxx"

#include <stdio.h>

#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/InsertCommandArgument.hpp>
#include <com/sun/star/ucb/NameClashException.hpp>
#include <com/sun/star/io/WrongFormatException.hpp>

#include <osl/time.h>
#include <osl/security.hxx>
#include <osl/socket.hxx>

#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/ustrbuf.hxx>

#include <comphelper/processfactory.hxx>

#include <tools/urlobj.hxx>
#include <unotools/bootstrap.hxx>

#include <ucbhelper/content.hxx>

#include <svtools/useroptions.hxx>

#include <svtools/documentlockfile.hxx>

#if defined USE_JAVA && defined MACOSX

#include <osl/file.hxx>
#include <sys/stat.h>

#endif	// USE_JAVA && MACOSX

using namespace ::com::sun::star;

namespace svt {

sal_Bool DocumentLockFile::m_bAllowInteraction = sal_True;

// ----------------------------------------------------------------------
DocumentLockFile::DocumentLockFile( const ::rtl::OUString& aOrigURL, const uno::Reference< lang::XMultiServiceFactory >& xFactory )
: LockFileCommon( aOrigURL, xFactory, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".~lock." ) ) )
{
}

// ----------------------------------------------------------------------
DocumentLockFile::~DocumentLockFile()
{
}

// ----------------------------------------------------------------------
void DocumentLockFile::WriteEntryToStream( uno::Sequence< ::rtl::OUString > aEntry, uno::Reference< io::XOutputStream > xOutput )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    ::rtl::OUStringBuffer aBuffer;

    for ( sal_Int32 nEntryInd = 0; nEntryInd < aEntry.getLength(); nEntryInd++ )
    {
        aBuffer.append( EscapeCharacters( aEntry[nEntryInd] ) );
        if ( nEntryInd < aEntry.getLength() - 1 )
            aBuffer.append( (sal_Unicode)',' );
        else
            aBuffer.append( (sal_Unicode)';' );
    }

    ::rtl::OString aStringData( ::rtl::OUStringToOString( aBuffer.makeStringAndClear(), RTL_TEXTENCODING_UTF8 ) );
    uno::Sequence< sal_Int8 > aData( (sal_Int8*)aStringData.getStr(), aStringData.getLength() );
    xOutput->writeBytes( aData );
}

// ----------------------------------------------------------------------
sal_Bool DocumentLockFile::CreateOwnLockFile()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    try
    {
        uno::Reference< io::XStream > xTempFile(
            m_xFactory->createInstance( ::rtl::OUString::createFromAscii( "com.sun.star.io.TempFile" ) ),
            uno::UNO_QUERY_THROW );
        uno::Reference< io::XSeekable > xSeekable( xTempFile, uno::UNO_QUERY_THROW );

        uno::Reference< io::XInputStream > xInput = xTempFile->getInputStream();
        uno::Reference< io::XOutputStream > xOutput = xTempFile->getOutputStream();

        if ( !xInput.is() || !xOutput.is() )
            throw uno::RuntimeException();

        uno::Sequence< ::rtl::OUString > aNewEntry = GenerateOwnEntry();
        WriteEntryToStream( aNewEntry, xOutput );
        xOutput->closeOutput();

        xSeekable->seek( 0 );

        uno::Reference < ::com::sun::star::ucb::XCommandEnvironment > xEnv;
        ::ucbhelper::Content aTargetContent( m_aURL, xEnv );

        ucb::InsertCommandArgument aInsertArg;
        aInsertArg.Data = xInput;
        aInsertArg.ReplaceExisting = sal_False;
        uno::Any aCmdArg;
        aCmdArg <<= aInsertArg;
        aTargetContent.executeCommand( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "insert" ) ), aCmdArg );

        // try to let the file be hidden if possible
        try {
            aTargetContent.setPropertyValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsHidden" ) ), uno::makeAny( sal_True ) );
        } catch( uno::Exception& ) {}
    }
    catch( ucb::NameClashException& )
    {
        return sal_False;
    }

    return sal_True;
}

// ----------------------------------------------------------------------
uno::Sequence< ::rtl::OUString > DocumentLockFile::GetLockData()
{
    ::osl::MutexGuard aGuard( m_aMutex );

#if defined USE_JAVA && defined MACOSX
    // Fix unexpected display of native open dialog when no lock file exists
    // by throwing an exception if the stat() function fails
    ::rtl::OUString aSystemPath;
    struct stat aSystemPathStat;
    if ( ::osl::FileBase::getSystemPathFromFileURL( m_aURL, aSystemPath ) != ::osl::FileBase::E_None || stat( ::rtl::OUStringToOString( aSystemPath, osl_getThreadTextEncoding() ), &aSystemPathStat ) )
        throw uno::RuntimeException();
#endif	// USE_JAVA && MACOSX

    uno::Reference< io::XInputStream > xInput = OpenStream();
    if ( !xInput.is() )
        throw uno::RuntimeException();

    const sal_Int32 nBufLen = 32000;
    uno::Sequence< sal_Int8 > aBuffer( nBufLen );

    sal_Int32 nRead = 0;
     
    nRead = xInput->readBytes( aBuffer, nBufLen );
    xInput->closeInput();

    if ( nRead == nBufLen )
        throw io::WrongFormatException();

    sal_Int32 nCurPos = 0;
    return ParseEntry( aBuffer, nCurPos );
}

// ----------------------------------------------------------------------
uno::Reference< io::XInputStream > DocumentLockFile::OpenStream()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    uno::Reference< lang::XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
        uno::Reference< ::com::sun::star::ucb::XSimpleFileAccess > xSimpleFileAccess(
            xFactory->createInstance( ::rtl::OUString::createFromAscii("com.sun.star.ucb.SimpleFileAccess") ),
            uno::UNO_QUERY_THROW );
    
    // the file can be opened readonly, no locking will be done  
    return xSimpleFileAccess->openFileRead( m_aURL );
}

// ----------------------------------------------------------------------
sal_Bool DocumentLockFile::OverwriteOwnLockFile()
{
    // allows to overwrite the lock file with the current data
    try
    {
        uno::Reference < ::com::sun::star::ucb::XCommandEnvironment > xEnv;
        ::ucbhelper::Content aTargetContent( m_aURL, xEnv );

        uno::Sequence< ::rtl::OUString > aNewEntry = GenerateOwnEntry();

        uno::Reference< io::XStream > xStream = aTargetContent.openWriteableStreamNoLock();
        uno::Reference< io::XOutputStream > xOutput = xStream->getOutputStream();
        uno::Reference< io::XTruncate > xTruncate( xOutput, uno::UNO_QUERY_THROW );

        xTruncate->truncate();
        WriteEntryToStream( aNewEntry, xOutput );
        xOutput->closeOutput();
    }
    catch( ucb::NameClashException& )
    {
        return sal_False;
    }

    return sal_True;
}

// ----------------------------------------------------------------------
void DocumentLockFile::RemoveFile()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    // TODO/LATER: the removing is not atomar, is it possible in general to make it atomar?
    uno::Sequence< ::rtl::OUString > aNewEntry = GenerateOwnEntry();
    uno::Sequence< ::rtl::OUString > aFileData = GetLockData();

    if ( aFileData.getLength() < LOCKFILE_ENTRYSIZE )
        throw io::WrongFormatException();

    if ( !aFileData[LOCKFILE_SYSUSERNAME_ID].equals( aNewEntry[LOCKFILE_SYSUSERNAME_ID] )
      || !aFileData[LOCKFILE_LOCALHOST_ID].equals( aNewEntry[LOCKFILE_LOCALHOST_ID] )
      || !aFileData[LOCKFILE_USERURL_ID].equals( aNewEntry[LOCKFILE_USERURL_ID] ) )
        throw io::IOException(); // not the owner, access denied

    uno::Reference< lang::XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
    uno::Reference< ::com::sun::star::ucb::XSimpleFileAccess > xSimpleFileAccess(
        xFactory->createInstance( ::rtl::OUString::createFromAscii("com.sun.star.ucb.SimpleFileAccess") ),
        uno::UNO_QUERY_THROW );
    xSimpleFileAccess->kill( m_aURL );
}

} // namespace svt

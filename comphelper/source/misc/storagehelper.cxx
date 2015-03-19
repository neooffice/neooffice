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
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_comphelper.hxx"
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XEncryptionProtectedSource.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/IllegalTypeException.hpp>

#include <ucbhelper/content.hxx>

#include <comphelper/fileformat.h>
#include <comphelper/processfactory.hxx>
#include <comphelper/documentconstants.hxx>

#include <comphelper/storagehelper.hxx>

#if SUPD == 310
#include <com/sun/star/embed/XEncryptionProtectedSource2.hpp>
#include <com/sun/star/xml/crypto/XNSSInitializer.hpp>
#include <com/sun/star/xml/crypto/DigestID.hpp>
#include <rtl/digest.h>
#endif	// SUPD == 310


using namespace ::com::sun::star;

namespace comphelper {

// ----------------------------------------------------------------------
uno::Reference< lang::XSingleServiceFactory > OStorageHelper::GetStorageFactory(
							const uno::Reference< lang::XMultiServiceFactory >& xSF )
		throw ( uno::Exception )
{
	uno::Reference< lang::XMultiServiceFactory > xFactory = xSF.is() ? xSF : ::comphelper::getProcessServiceFactory();
	if ( !xFactory.is() )
		throw uno::RuntimeException();

	uno::Reference < lang::XSingleServiceFactory > xStorageFactory(
					xFactory->createInstance ( ::rtl::OUString::createFromAscii( "com.sun.star.embed.StorageFactory" ) ),
					uno::UNO_QUERY );

	if ( !xStorageFactory.is() )
		throw uno::RuntimeException();

	return xStorageFactory;
}

// ----------------------------------------------------------------------
uno::Reference< lang::XSingleServiceFactory > OStorageHelper::GetFileSystemStorageFactory(
							const uno::Reference< lang::XMultiServiceFactory >& xSF )
		throw ( uno::Exception )
{
	uno::Reference< lang::XMultiServiceFactory > xFactory = xSF.is() ? xSF : ::comphelper::getProcessServiceFactory();
	if ( !xFactory.is() )
		throw uno::RuntimeException();

	uno::Reference < lang::XSingleServiceFactory > xStorageFactory(
					xFactory->createInstance ( ::rtl::OUString::createFromAscii( "com.sun.star.embed.FileSystemStorageFactory" ) ),
					uno::UNO_QUERY );

	if ( !xStorageFactory.is() )
		throw uno::RuntimeException();

	return xStorageFactory;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetTemporaryStorage(
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
	throw ( uno::Exception )
{
	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstance(),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageFromURL(
			const ::rtl::OUString& aURL,
			sal_Int32 nStorageMode,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
	throw ( uno::Exception )
{
	uno::Sequence< uno::Any > aArgs( 2 );
	aArgs[0] <<= aURL;
	aArgs[1] <<= nStorageMode;

	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstanceWithArguments( aArgs ),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageFromURL2(
			const ::rtl::OUString& aURL,
			sal_Int32 nStorageMode,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
	throw ( uno::Exception )
{
	uno::Sequence< uno::Any > aArgs( 2 );
	aArgs[0] <<= aURL;
	aArgs[1] <<= nStorageMode;

    uno::Reference< lang::XSingleServiceFactory > xFact;
    try {
        ::ucbhelper::Content aCntnt( aURL,
            uno::Reference< ::com::sun::star::ucb::XCommandEnvironment > () );
        if (aCntnt.isDocument()) {
            xFact = GetStorageFactory( xFactory );
        } else {
            xFact = GetFileSystemStorageFactory( xFactory );
        }
    } catch (uno::Exception &) { }

    if (!xFact.is()) throw uno::RuntimeException();

	uno::Reference< embed::XStorage > xTempStorage(
        xFact->createInstanceWithArguments( aArgs ), uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageFromInputStream(
            const uno::Reference < io::XInputStream >& xStream,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
		throw ( uno::Exception )
{
	uno::Sequence< uno::Any > aArgs( 2 );
	aArgs[0] <<= xStream;
	aArgs[1] <<= embed::ElementModes::READ;

	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstanceWithArguments( aArgs ),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageFromStream(
            const uno::Reference < io::XStream >& xStream,
			sal_Int32 nStorageMode,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
		throw ( uno::Exception )
{
	uno::Sequence< uno::Any > aArgs( 2 );
	aArgs[0] <<= xStream;
	aArgs[1] <<= nStorageMode;

	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstanceWithArguments( aArgs ),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
void OStorageHelper::CopyInputToOutput(
			const uno::Reference< io::XInputStream >& xInput,
			const uno::Reference< io::XOutputStream >& xOutput )
	throw ( uno::Exception )
{
	static const sal_Int32 nConstBufferSize = 32000;

	sal_Int32 nRead;
	uno::Sequence < sal_Int8 > aSequence ( nConstBufferSize );

	do
	{
		nRead = xInput->readBytes ( aSequence, nConstBufferSize );
		if ( nRead < nConstBufferSize )
		{
			uno::Sequence < sal_Int8 > aTempBuf ( aSequence.getConstArray(), nRead );
			xOutput->writeBytes ( aTempBuf );
		}
		else
			xOutput->writeBytes ( aSequence );
	}
	while ( nRead == nConstBufferSize );
}

// ----------------------------------------------------------------------
uno::Reference< io::XInputStream > OStorageHelper::GetInputStreamFromURL(
			const ::rtl::OUString& aURL,
			const uno::Reference< lang::XMultiServiceFactory >& xSF )
	throw ( uno::Exception )
{
	uno::Reference< lang::XMultiServiceFactory > xFactory = xSF.is() ? xSF : ::comphelper::getProcessServiceFactory();
	if ( !xFactory.is() )
		throw uno::RuntimeException();

	uno::Reference < ::com::sun::star::ucb::XSimpleFileAccess > xTempAccess(
			xFactory->createInstance ( ::rtl::OUString::createFromAscii( "com.sun.star.ucb.SimpleFileAccess" ) ),
			uno::UNO_QUERY );

	if ( !xTempAccess.is() )
		throw uno::RuntimeException();

	uno::Reference< io::XInputStream > xInputStream = xTempAccess->openFileRead( aURL );
	if ( !xInputStream.is() )
		throw uno::RuntimeException();

	return xInputStream;
}

// ----------------------------------------------------------------------
void OStorageHelper::SetCommonStoragePassword(
			const uno::Reference< embed::XStorage >& xStorage,
			const ::rtl::OUString& aPass )
	throw ( uno::Exception )
{
	uno::Reference< embed::XEncryptionProtectedSource > xEncrSet( xStorage, uno::UNO_QUERY );
	if ( !xEncrSet.is() )
		throw io::IOException(); // TODO

	xEncrSet->setEncryptionPassword( aPass );
}

#if SUPD == 310

// ----------------------------------------------------------------------
void OStorageHelper::SetCommonStorageEncryptionData(
			const uno::Reference< embed::XStorage >& xStorage,
			const uno::Sequence< beans::NamedValue >& aEncryptionData )
	throw ( uno::Exception )
{
	uno::Reference< embed::XEncryptionProtectedSource2 > xEncrSet( xStorage, uno::UNO_QUERY );
	if ( !xEncrSet.is() )
		throw io::IOException(); // TODO

	xEncrSet->setEncryptionData( aEncryptionData );
}

#endif	// SUPD == 310

// ----------------------------------------------------------------------
sal_Int32 OStorageHelper::GetXStorageFormat(
			const uno::Reference< embed::XStorage >& xStorage )
		throw ( uno::Exception )
{
	uno::Reference< beans::XPropertySet > xStorProps( xStorage, uno::UNO_QUERY_THROW );

	::rtl::OUString aMediaType;
	xStorProps->getPropertyValue( ::rtl::OUString::createFromAscii( "MediaType" ) ) >>= aMediaType;

	sal_Int32 nResult = 0;

	// TODO/LATER: the filter configuration could be used to detect it later, or batter a special service
    if (
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_WRITER_ASCII       ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_WRITER_WEB_ASCII   ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_WRITER_GLOBAL_ASCII) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_DRAW_ASCII         ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_IMPRESS_ASCII      ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_CALC_ASCII         ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_CHART_ASCII        ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_VND_SUN_XML_MATH_ASCII         )
       )
	{
		nResult = SOFFICE_FILEFORMAT_60;
	}
    else
    if (
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_TEXT_ASCII        ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_TEXT_WEB_ASCII    ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_TEXT_GLOBAL_ASCII ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_DRAWING_ASCII     ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_PRESENTATION_ASCII) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_SPREADSHEET_ASCII ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_CHART_ASCII       ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_FORMULA_ASCII     ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_DATABASE_ASCII    ) ||
		aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_REPORT_ASCII    ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_REPORT_CHART_ASCII    ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_TEXT_TEMPLATE_ASCII        ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_DRAWING_TEMPLATE_ASCII     ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_PRESENTATION_TEMPLATE_ASCII) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_SPREADSHEET_TEMPLATE_ASCII ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_CHART_TEMPLATE_ASCII       ) ||
        aMediaType.equalsIgnoreAsciiCaseAscii(MIMETYPE_OASIS_OPENDOCUMENT_FORMULA_TEMPLATE_ASCII     )
       )
	{
		nResult = SOFFICE_FILEFORMAT_8;
	}
	else
	{
		// the mediatype is not known
		throw beans::IllegalTypeException();
	}

	return nResult;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetTemporaryStorageOfFormat(
			const ::rtl::OUString& aFormat,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
	throw ( uno::Exception )
{
	uno::Reference< lang::XMultiServiceFactory > xFactoryToUse = xFactory.is() ? xFactory : ::comphelper::getProcessServiceFactory();
	if ( !xFactoryToUse.is() )
		throw uno::RuntimeException();

	uno::Reference< io::XStream > xTmpStream(
		xFactoryToUse->createInstance( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.io.TempFile" ) ) ),
		uno::UNO_QUERY_THROW );

	return GetStorageOfFormatFromStream( aFormat, xTmpStream, embed::ElementModes::READWRITE, xFactoryToUse );
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageOfFormatFromURL(
			const ::rtl::OUString& aFormat,
			const ::rtl::OUString& aURL,
			sal_Int32 nStorageMode,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
	throw ( uno::Exception )
{
	uno::Sequence< beans::PropertyValue > aProps( 1 );
	aProps[0].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "StorageFormat" ) );
	aProps[0].Value <<= aFormat;

	uno::Sequence< uno::Any > aArgs( 3 );
	aArgs[0] <<= aURL;
	aArgs[1] <<= nStorageMode;
	aArgs[2] <<= aProps;

	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstanceWithArguments( aArgs ),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageOfFormatFromInputStream(
			const ::rtl::OUString& aFormat,
            const uno::Reference < io::XInputStream >& xStream,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
		throw ( uno::Exception )
{
	uno::Sequence< beans::PropertyValue > aProps( 1 );
	aProps[0].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "StorageFormat" ) );
	aProps[0].Value <<= aFormat;

	uno::Sequence< uno::Any > aArgs( 3 );
	aArgs[0] <<= xStream;
	aArgs[1] <<= embed::ElementModes::READ;
	aArgs[2] <<= aProps;

	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstanceWithArguments( aArgs ),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

// ----------------------------------------------------------------------
uno::Reference< embed::XStorage > OStorageHelper::GetStorageOfFormatFromStream(
			const ::rtl::OUString& aFormat,
            const uno::Reference < io::XStream >& xStream,
			sal_Int32 nStorageMode,
			const uno::Reference< lang::XMultiServiceFactory >& xFactory )
		throw ( uno::Exception )
{
	uno::Sequence< beans::PropertyValue > aProps( 1 );
	aProps[0].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "StorageFormat" ) );
	aProps[0].Value <<= aFormat;

	uno::Sequence< uno::Any > aArgs( 3 );
	aArgs[0] <<= xStream;
	aArgs[1] <<= nStorageMode;
	aArgs[2] <<= aProps;

	uno::Reference< embed::XStorage > xTempStorage( GetStorageFactory( xFactory )->createInstanceWithArguments( aArgs ),
													uno::UNO_QUERY );
	if ( !xTempStorage.is() )
		throw uno::RuntimeException();

	return xTempStorage;
}

#if SUPD == 310

// ----------------------------------------------------------------------
uno::Sequence< beans::NamedValue > OStorageHelper::CreatePackageEncryptionData( const ::rtl::OUString& aPassword, const uno::Reference< lang::XMultiServiceFactory >& xSF )
{
    // TODO/LATER: Should not the method be part of DocPasswordHelper?
    uno::Sequence< beans::NamedValue > aEncryptionData;
    sal_Int32 nSha1Ind = 0;
    if ( !aPassword.isEmpty() )
    {
        // generate SHA256 start key
        try
        {
            uno::Reference< lang::XMultiServiceFactory > xFactory = xSF.is() ? xSF : ::comphelper::getProcessServiceFactory();
            if ( !xFactory.is() )
                throw uno::RuntimeException();

            uno::Reference< xml::crypto::XDigestContextSupplier > xDigestContextSupplier( xFactory->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.xml.crypto.NSSInitializer" ) ) ), uno::UNO_QUERY_THROW );
            uno::Reference< xml::crypto::XDigestContext > xDigestContext( xDigestContextSupplier->getDigestContext( xml::crypto::DigestID::SHA256, uno::Sequence< beans::NamedValue >() ), uno::UNO_SET_THROW );
            
            ::rtl::OString aUTF8Password( ::rtl::OUStringToOString( aPassword, RTL_TEXTENCODING_UTF8 ) );
            xDigestContext->updateDigest( uno::Sequence< sal_Int8 >( reinterpret_cast< const sal_Int8* >( aUTF8Password.getStr() ), aUTF8Password.getLength() ) );
            uno::Sequence< sal_Int8 > aDigest = xDigestContext->finalizeDigestAndDispose();

            aEncryptionData.realloc( ++nSha1Ind );
            aEncryptionData[0].Name = PACKAGE_ENCRYPTIONDATA_SHA256UTF8;
            aEncryptionData[0].Value <<= aDigest;
        }
        catch ( uno::Exception& )
        {
            OSL_ENSURE( false, "Can not create SHA256 digest!" );
        }

        // MS_1252 encoding was used for SO60 document format password encoding,
        // this encoding supports only a minor subset of nonascii characters,
        // but for compatibility reasons it has to be used for old document formats
        aEncryptionData.realloc( nSha1Ind + 2 );
        aEncryptionData[nSha1Ind].Name = PACKAGE_ENCRYPTIONDATA_SHA1UTF8;
        aEncryptionData[nSha1Ind + 1].Name = PACKAGE_ENCRYPTIONDATA_SHA1MS1252;

        rtl_TextEncoding pEncoding[2] = { RTL_TEXTENCODING_UTF8, RTL_TEXTENCODING_MS_1252 };

        for ( sal_Int32 nInd = 0; nInd < 2; nInd++ )
        {
            ::rtl::OString aByteStrPass = ::rtl::OUStringToOString( aPassword, pEncoding[nInd] );

            sal_uInt8 pBuffer[RTL_DIGEST_LENGTH_SHA1];
            rtlDigestError nError = rtl_digest_SHA1( aByteStrPass.getStr(),
                                                    aByteStrPass.getLength(),
                                                    pBuffer,
                                                    RTL_DIGEST_LENGTH_SHA1 );

            if ( nError != rtl_Digest_E_None )
            {
                aEncryptionData.realloc( nSha1Ind );
                break;
            }

            aEncryptionData[nSha1Ind+nInd].Value <<= uno::Sequence< sal_Int8 >( (sal_Int8*)pBuffer, RTL_DIGEST_LENGTH_SHA1 );
        }
    }

    return aEncryptionData;
}

#endif	// SUPD == 310

// ----------------------------------------------------------------------
sal_Bool OStorageHelper::IsValidZipEntryFileName( const ::rtl::OUString& aName, sal_Bool bSlashAllowed )
{
    return IsValidZipEntryFileName( aName.getStr(), aName.getLength(), bSlashAllowed );
}

// ----------------------------------------------------------------------
sal_Bool OStorageHelper::IsValidZipEntryFileName(
    const sal_Unicode *pChar, sal_Int32 nLength, sal_Bool bSlashAllowed )
{
    for ( sal_Int32 i = 0; i < nLength; i++ )
    {
        switch ( pChar[i] )
        {
            case '\\':
            case '?':
            case '<':
            case '>':
            case '\"':
            case '|':
            case ':':
                return sal_False;
            case '/':
                if ( !bSlashAllowed )
                    return sal_False;
                break;
            default:
                if ( pChar[i] < 32  || pChar[i] >= 0xD800 && pChar[i] <= 0xDFFF )
                    return sal_False;
        }
    }
    return sal_True;
}

}

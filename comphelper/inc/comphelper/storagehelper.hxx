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
#ifndef _COMPHELPER_STORAGEHELPER_HXX
#define _COMPHELPER_STORAGEHELPER_HXX

#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XStream.hpp>
#include "comphelper/comphelperdllapi.h"

#if SUPD == 310
#include <com/sun/star/beans/NamedValue.hpp>
#endif	// SUPD == 310

#define PACKAGE_STORAGE_FORMAT_STRING	::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "PackageFormat" ) )
#define ZIP_STORAGE_FORMAT_STRING		::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ZipFormat" ) )
#define OFOPXML_STORAGE_FORMAT_STRING	::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "OFOPXMLFormat" ) )

#if SUPD == 310
#define PACKAGE_ENCRYPTIONDATA_SHA256UTF8 ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "PackageSHA256UTF8EncryptionKey" ) )
#define PACKAGE_ENCRYPTIONDATA_SHA1UTF8   ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "PackageSHA1UTF8EncryptionKey" ) )
#define PACKAGE_ENCRYPTIONDATA_SHA1MS1252 ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "PackageSHA1MS1252EncryptionKey" ) )
#endif	// SUPD == 310

namespace comphelper {

sal_Bool COMPHELPER_DLLPUBLIC IsValidZipEntryFileName(
    const sal_Unicode *pChar, sal_Int32 nLength, sal_Bool bSlashAllowed );

class COMPHELPER_DLLPUBLIC OStorageHelper
{
public:
	static ::com::sun::star::uno::Reference< ::com::sun::star::lang::XSingleServiceFactory >
		GetStorageFactory(
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xSF
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::lang::XSingleServiceFactory >
		GetFileSystemStorageFactory(
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xSF
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
		GetTemporaryStorage(
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

    /// this one will only return Storage
	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
		GetStorageFromURL(
			const ::rtl::OUString& aURL,
			sal_Int32 nStorageMode,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

    /// this one will return either Storage or FileSystemStorage
	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
		GetStorageFromURL2(
			const ::rtl::OUString& aURL,
			sal_Int32 nStorageMode,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )

		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
        GetStorageFromInputStream(
            const ::com::sun::star::uno::Reference < ::com::sun::star::io::XInputStream >& xStream,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
        GetStorageFromStream(
            const ::com::sun::star::uno::Reference < ::com::sun::star::io::XStream >& xStream,
			sal_Int32 nStorageMode = ::com::sun::star::embed::ElementModes::READWRITE,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static void CopyInputToOutput(
			const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& xInput,
			const ::com::sun::star::uno::Reference< ::com::sun::star::io::XOutputStream >& xOutput )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >
		GetInputStreamFromURL(
			const ::rtl::OUString& aURL,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static void SetCommonStoragePassword(
			const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& xStorage,
			const ::rtl::OUString& aPass )
		throw ( ::com::sun::star::uno::Exception );

#if SUPD == 310
	static void SetCommonStorageEncryptionData(
			const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& xStorage,
			const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::NamedValue >& aEncryptionData )
		throw ( ::com::sun::star::uno::Exception );
#endif	// SUPD == 310

	// the following method supports only storages of OOo formats
	static sal_Int32 GetXStorageFormat(
			const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& xStorage )
		throw ( ::com::sun::star::uno::Exception );

	// The followin methods are related to creation of a storage of specified format
	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
		GetTemporaryStorageOfFormat(
			const ::rtl::OUString& aFormat,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
		GetStorageOfFormatFromURL(
			const ::rtl::OUString& aFormat,
			const ::rtl::OUString& aURL,
			sal_Int32 nStorageMode,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
        GetStorageOfFormatFromInputStream(
			const ::rtl::OUString& aFormat,
            const ::com::sun::star::uno::Reference < ::com::sun::star::io::XInputStream >& xStream,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

	static ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >
        GetStorageOfFormatFromStream(
			const ::rtl::OUString& aFormat,
            const ::com::sun::star::uno::Reference < ::com::sun::star::io::XStream >& xStream,
			sal_Int32 nStorageMode = ::com::sun::star::embed::ElementModes::READWRITE,
			const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory
							= ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() )
		throw ( ::com::sun::star::uno::Exception );

#if SUPD == 310
    static ::com::sun::star::uno::Sequence< ::com::sun::star::beans::NamedValue >
        CreatePackageEncryptionData(
            const ::rtl::OUString& aPassword,
            const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xSF
                = ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() );
#endif	// SUPD == 310

    static sal_Bool IsValidZipEntryFileName( const ::rtl::OUString& aName, sal_Bool bSlashAllowed );
    static sal_Bool IsValidZipEntryFileName( const sal_Unicode *pChar, sal_Int32 nLength, sal_Bool bSlashAllowed );
};

}

#endif

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
 * Modified March 2013 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/
#ifndef _ZIP_PACKAGE_STREAM_HXX
#define _ZIP_PACKAGE_STREAM_HXX

#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/packages/XDataSinkEncrSupport.hpp>
#include <ZipPackageEntry.hxx>
#ifndef _VOS_REF_H_
#include <vos/ref.hxx>
#endif
#include <EncryptionData.hxx>
#ifndef _CPPUHELPER_IMPLBASE2_HXX
#include <cppuhelper/implbase2.hxx>
#endif
#include <mutexholder.hxx>

#define PACKAGE_STREAM_NOTSET			0
#define PACKAGE_STREAM_PACKAGEMEMBER	1
#define PACKAGE_STREAM_DETECT			2
#define PACKAGE_STREAM_DATA				3
#define PACKAGE_STREAM_RAW				4

class ZipPackage;
struct ZipEntry;
class ZipPackageStream : public cppu::ImplInheritanceHelper2
<
	ZipPackageEntry,
	::com::sun::star::io::XActiveDataSink,
	::com::sun::star::packages::XDataSinkEncrSupport
>
{
	static com::sun::star::uno::Sequence < sal_Int8 > aImplementationId;
protected:
	com::sun::star::uno::Reference < com::sun::star::io::XInputStream > xStream;
	const ::com::sun::star::uno::Reference < com::sun::star::lang::XMultiServiceFactory > m_xFactory;
	ZipPackage 			&rZipPackage;
	sal_Bool			bToBeCompressed, bToBeEncrypted, bHaveOwnKey, bIsEncrypted;
	vos::ORef < EncryptionData > xEncryptionData;

	sal_uInt8	m_nStreamMode;
	sal_uInt32	m_nMagicalHackPos;
	sal_uInt32	m_nMagicalHackSize;

	SotMutexHolderRef m_aSharedMutexRef;

	sal_Bool m_bHasSeekable;

	sal_Bool m_bCompressedIsSetFromOutside;

	::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& GetOwnSeekStream();

public:
	sal_Bool HasOwnKey () const	 { return bHaveOwnKey;}
	sal_Bool IsToBeCompressed () const { return bToBeCompressed;}
	sal_Bool IsToBeEncrypted () const { return bToBeEncrypted;}
	sal_Bool IsEncrypted () const  	 { return bIsEncrypted;}
	sal_Bool IsPackageMember () const { return m_nStreamMode == PACKAGE_STREAM_PACKAGEMEMBER;}
	vos::ORef < EncryptionData > & getEncryptionData ()
	{ return xEncryptionData;}
	const com::sun::star::uno::Sequence < sal_Int8 >& getKey () const
	{ return xEncryptionData->aKey;}
	const com::sun::star::uno::Sequence < sal_uInt8 >& getInitialisationVector () const
	{ return xEncryptionData->aInitVector;}
	const com::sun::star::uno::Sequence < sal_uInt8 >& getDigest () const
	{ return xEncryptionData->aDigest;}
	const com::sun::star::uno::Sequence < sal_uInt8 >& getSalt () const
	{ return xEncryptionData->aSalt;}
	sal_Int32 getIterationCount () const
	{ return xEncryptionData->nIterationCount;}
	sal_Int32 getSize () const
	{ return aEntry.nSize;}

	sal_uInt8 GetStreamMode() const { return m_nStreamMode; }
	sal_uInt32 GetMagicalHackPos() const { return m_nMagicalHackPos; }
	sal_uInt32 GetMagicalHackSize() const { return m_nMagicalHackSize; }

	void SetToBeCompressed (sal_Bool bNewValue) { bToBeCompressed = bNewValue;}
	void SetIsEncrypted (sal_Bool bNewValue) { bIsEncrypted = bNewValue;}
	void SetToBeEncrypted (sal_Bool bNewValue)  
    { 
        bToBeEncrypted  = bNewValue;
        if ( bToBeEncrypted && xEncryptionData.isEmpty())
            xEncryptionData = new EncryptionData;
		else if ( !bToBeEncrypted && !xEncryptionData.isEmpty() )
			xEncryptionData.unbind();
    }
	void SetPackageMember (sal_Bool bNewValue);
	void setKey (const com::sun::star::uno::Sequence < sal_Int8 >& rNewKey )
	{ xEncryptionData->aKey = rNewKey;}
	void setInitialisationVector (const com::sun::star::uno::Sequence < sal_uInt8 >& rNewVector )
	{ xEncryptionData->aInitVector = rNewVector;}
	void setSalt (const com::sun::star::uno::Sequence < sal_uInt8 >& rNewSalt )
	{ xEncryptionData->aSalt = rNewSalt;}
	void setDigest (const com::sun::star::uno::Sequence < sal_uInt8 >& rNewDigest )
	{ xEncryptionData->aDigest = rNewDigest;}
	void setIterationCount (const sal_Int32 nNewCount)
	{ xEncryptionData->nIterationCount = nNewCount;}
	void setSize (const sal_Int32 nNewSize);
#ifndef NO_OOO_3_4_1_AES_ENCRYPTION
	void setDigestType (const sal_Int32 nNewDigestType)
	{ xEncryptionData->nDigestType = nNewDigestType;}
	void setAlgorithm (const sal_Int32 nNewAlgorithm)
	{ xEncryptionData->nAlgorithm = nNewAlgorithm;}
	void setDerivedKeySize (const sal_Int32 nNewDerivedKeySize)
	{ xEncryptionData->nDerivedKeySize = nNewDerivedKeySize;}
	void setStartKeyAlgorithm (const sal_Int32 nNewStartKeyAlgorithm)
	{ xEncryptionData->nStartKeyAlgorithm = nNewStartKeyAlgorithm;}
#endif	// !NO_OOO_3_4_1_AES_ENCRYPTION

	::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > GetOwnStreamNoWrap() { return xStream; }

	void CloseOwnStreamIfAny();

	ZipPackageStream ( ZipPackage & rNewPackage,
						const ::com::sun::star::uno::Reference < com::sun::star::lang::XMultiServiceFactory >& xFactory,
						sal_Bool bAllowRemoveOnInsert );
	virtual ~ZipPackageStream( void );

	::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > GetRawEncrStreamNoHeaderCopy();
	::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > TryToGetRawFromDataStream(
																					sal_Bool bAddHeaderForEncr );

	sal_Bool ParsePackageRawStream();

	void setZipEntryOnLoading( const ZipEntry &rInEntry);
    ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getRawData()
		throw(::com::sun::star::uno::RuntimeException);
	
	static ::com::sun::star::uno::Sequence < sal_Int8 >& static_getImplementationId()
	{
		return aImplementationId;
	}

	// XActiveDataSink
    virtual void SAL_CALL setInputStream( const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& aStream ) 
		throw(::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getInputStream(  ) 
		throw(::com::sun::star::uno::RuntimeException);

	// XDataSinkEncrSupport
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getDataStream()
		throw ( ::com::sun::star::packages::WrongPasswordException,
				::com::sun::star::io::IOException,
				::com::sun::star::uno::RuntimeException );
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getRawStream()
		throw ( ::com::sun::star::packages::NoEncryptionException,
				::com::sun::star::io::IOException,
				::com::sun::star::uno::RuntimeException );
    virtual void SAL_CALL setDataStream(
					const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& aStream )
		throw ( ::com::sun::star::io::IOException,
				::com::sun::star::uno::RuntimeException );
    virtual void SAL_CALL setRawStream(
					const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& aStream )
		throw ( ::com::sun::star::packages::EncryptionNotAllowedException,
				::com::sun::star::packages::NoRawFormatException,
				::com::sun::star::io::IOException,
				::com::sun::star::uno::RuntimeException );
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getPlainRawStream()
		throw ( ::com::sun::star::io::IOException,
				::com::sun::star::uno::RuntimeException );

	// XUnoTunnel
    virtual sal_Int64 SAL_CALL getSomething( const ::com::sun::star::uno::Sequence< sal_Int8 >& aIdentifier ) 
		throw(::com::sun::star::uno::RuntimeException);

	// XPropertySet
    virtual void SAL_CALL setPropertyValue( const ::rtl::OUString& aPropertyName, const ::com::sun::star::uno::Any& aValue ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Any SAL_CALL getPropertyValue( const ::rtl::OUString& PropertyName ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

	// XServiceInfo
    virtual ::rtl::OUString SAL_CALL getImplementationName(  ) 
		throw (::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) 
		throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) 
		throw (::com::sun::star::uno::RuntimeException);
};
#endif
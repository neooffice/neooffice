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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_package.hxx"
#include <ZipFile.hxx>
#include <ZipEnumeration.hxx>
#include <com/sun/star/packages/zip/ZipConstants.hpp>
#include <rtl/cipher.h>
#include <rtl/digest.h>
/*
#include <XMemoryStream.hxx>
#include <XFileStream.hxx>
*/
#include <XUnbufferedStream.hxx>
#include <PackageConstants.hxx>
#include <EncryptedDataHeader.hxx>
#include <EncryptionData.hxx>
#include <MemoryByteGrabber.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/ucb/XProgressHandler.hpp>

#ifndef _CRC32_HXX_
#include <CRC32.hxx>
#endif

#include <string.h> // for memcpy
#include <vector>

#include <comphelper/storagehelper.hxx>

#ifndef NO_OOO_3_4_1_AES_ENCRYPTION

#include <com/sun/star/xml/crypto/XSEInitializer.hpp>
#include <com/sun/star/xml/crypto/XXMLSecurityContext.hpp>
#include <comphelper/processfactory.hxx>
#include <pk11func.h>

#endif	// !NO_OOO_3_4_1_AES_ENCRYPTION

using namespace vos;
using namespace rtl;
using namespace com::sun::star;
using namespace com::sun::star::io;
using namespace com::sun::star::uno;
using namespace com::sun::star::ucb;
using namespace com::sun::star::lang;
using namespace com::sun::star::packages;
using namespace com::sun::star::packages::zip;
using namespace com::sun::star::packages::zip::ZipConstants;


/** This class is used to read entries from a zip file
 */
ZipFile::ZipFile( Reference < XInputStream > &xInput, const Reference < XMultiServiceFactory > &xNewFactory, sal_Bool bInitialise )
	throw(IOException, ZipException, RuntimeException)
: aGrabber(xInput)
, aInflater (sal_True)
, xStream(xInput)
, xSeek(xInput, UNO_QUERY)
, xFactory ( xNewFactory )
, bRecoveryMode( sal_False )
{
	if (bInitialise)
	{
		if ( readCEN() == -1 )
		{
			aEntries.clear();
			throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "stream data looks to be broken" ) ), Reference < XInterface > () );
		}
	}
}



ZipFile::ZipFile( Reference < XInputStream > &xInput, const Reference < XMultiServiceFactory > &xNewFactory, sal_Bool bInitialise, sal_Bool bForceRecovery, Reference < XProgressHandler > xProgress )
	throw(IOException, ZipException, RuntimeException)
: aGrabber(xInput)
, aInflater (sal_True)
, xStream(xInput)
, xSeek(xInput, UNO_QUERY)
, xFactory ( xNewFactory )
, xProgressHandler( xProgress )
, bRecoveryMode( bForceRecovery )
{
	if (bInitialise)
	{
		if ( bForceRecovery )
		{
			recover();
		}
		else if ( readCEN() == -1 )
		{
			aEntries.clear();
			throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "stream data looks to be broken" ) ), Reference < XInterface > () );
		}
	}
}

ZipFile::~ZipFile()
{
    aEntries.clear();
}

void ZipFile::setInputStream ( Reference < XInputStream > xNewStream )
{
	xStream = xNewStream;
	xSeek = Reference < XSeekable > ( xStream, UNO_QUERY );
	aGrabber.setInputStream ( xStream );
}

void ZipFile::StaticGetCipher ( const ORef < EncryptionData > & xEncryptionData, rtlCipher &rCipher )
{
    if ( ! xEncryptionData.isEmpty() )
	{
		Sequence < sal_uInt8 > aDerivedKey (16);
		rtlCipherError aResult;
		Sequence < sal_Int8 > aDecryptBuffer;

		// Get the key
		rtl_digest_PBKDF2 ( aDerivedKey.getArray(), 16,
							reinterpret_cast < const sal_uInt8 * > (xEncryptionData->aKey.getConstArray() ),
							xEncryptionData->aKey.getLength(),
							reinterpret_cast < const sal_uInt8 * > ( xEncryptionData->aSalt.getConstArray() ),
							xEncryptionData->aSalt.getLength(),
							xEncryptionData->nIterationCount );

		rCipher = rtl_cipher_create (rtl_Cipher_AlgorithmBF, rtl_Cipher_ModeStream);
		aResult = rtl_cipher_init( rCipher, rtl_Cipher_DirectionDecode,
								   aDerivedKey.getConstArray(),
								   aDerivedKey.getLength(),
								   reinterpret_cast < const sal_uInt8 * > ( xEncryptionData->aInitVector.getConstArray() ),
								   xEncryptionData->aInitVector.getLength());
		OSL_ASSERT (aResult == rtl_Cipher_E_None);
	}
}

#ifndef NO_OOO_3_4_1_AES_ENCRYPTION

sal_Bool ZipFile::StaticGetDecryptedData( const ::com::sun::star::uno::Sequence< sal_Int8 > &aReadBuffer, const vos::ORef < EncryptionData > &rData, ::com::sun::star::uno::Sequence< sal_Int8 > &aDecryptedBuffer )
{
	sal_Bool bRet = sal_False;

	if ( !rData.isValid() )
		return bRet;

	if ( rData->nAlgorithm == ENCRYPTION_DATA_AES_CBC_W3C_PADDING )
	{
		Sequence < sal_Int8 > aKey = rData->nStartKeyAlgorithm == ENCRYPTION_DATA_SHA256 ? rData->aKeySHA256 : rData->aKey;
		if ( aKey.getLength() && rData->nDerivedKeySize > 0 && rData->aInitVector.getLength() )
		{
			// Get the key
			Sequence< sal_uInt8 > aDerivedKey( rData->nDerivedKeySize );
			rtl_digest_PBKDF2( aDerivedKey.getArray(), aDerivedKey.getLength(), reinterpret_cast< const sal_uInt8* >( aKey.getConstArray() ), aKey.getLength(), reinterpret_cast< const sal_uInt8* >( rData->aSalt.getConstArray() ), rData->aSalt.getLength(), rData->nIterationCount );

			// Decrypt data
			uno::Reference< xml::crypto::XSEInitializer > xSEInitializer( ::comphelper::getProcessServiceFactory()->createInstance( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.xml.crypto.SEInitializer" ) ) ), uno::UNO_QUERY );
			if ( xSEInitializer.is() )
			{
				uno::Reference< xml::crypto::XXMLSecurityContext > xSecurityContext = xSEInitializer->createSecurityContext( ::rtl::OUString() );
				if ( xSecurityContext.is() )
				{
					CK_MECHANISM_TYPE nCipherType = CKM_AES_CBC;
					PK11SlotInfo *pSlot = PK11_GetBestSlot( nCipherType, NULL );
					if ( pSlot )
					{
						SECItem aKeyItem = { siBuffer, const_cast< unsigned char* >( reinterpret_cast< const unsigned char* >( aDerivedKey.getConstArray() ) ), aDerivedKey.getLength() };
						PK11SymKey *pSymKey = PK11_ImportSymKey( pSlot, nCipherType, PK11_OriginDerive, CKA_DECRYPT, &aKeyItem, NULL );
						if ( pSymKey )
						{
							SECItem aIVItem = { siBuffer, const_cast< unsigned char* >( reinterpret_cast< const unsigned char* >( rData->aInitVector.getConstArray() ) ), rData->aInitVector.getLength() };
							SECItem *pSecParam = PK11_ParamFromIV( nCipherType, &aIVItem );
							if ( pSecParam )
							{
								PK11Context *pEncContext = PK11_CreateContextBySymKey( nCipherType, CKA_DECRYPT, pSymKey, pSecParam );
								if ( pEncContext )
								{
									int nBlockSize = PK11_GetBlockSize( nCipherType, pSecParam );
									aDecryptedBuffer.realloc( aReadBuffer.getLength() + nBlockSize );
									int nDecryptedLen = 0;
									if ( PK11_CipherOp( pEncContext, reinterpret_cast< unsigned char* >( aDecryptedBuffer.getArray() ), &nDecryptedLen, aDecryptedBuffer.getLength(), const_cast< unsigned char* >( reinterpret_cast< const unsigned char* >( aReadBuffer.getConstArray() ) ), aReadBuffer.getLength() ) == SECSuccess )
									{
										unsigned int nFinalizedLen = 0;
										aDecryptedBuffer.realloc( nDecryptedLen + ( nBlockSize * 2 ) );
										if ( PK11_DigestFinal( pEncContext, reinterpret_cast< unsigned char* >( aDecryptedBuffer.getArray() + nDecryptedLen ), &nFinalizedLen, aDecryptedBuffer.getLength() - nDecryptedLen ) == SECSuccess )
										{
											aDecryptedBuffer.realloc( nDecryptedLen + nFinalizedLen );
											bRet = sal_True;
										}
									}

									PK11_DestroyContext( pEncContext, PR_TRUE );
								}

								SECITEM_FreeItem( pSecParam, PR_TRUE );
							}

							PK11_FreeSymKey( pSymKey );
						}

						PK11_FreeSlot( pSlot );
					}

      		 	 	xSEInitializer->freeSecurityContext( xSecurityContext );
				}
			}
		}
	}
	return bRet;
}

#endif	// !NO_OOO_3_4_1_AES_ENCRYPTION

void ZipFile::StaticFillHeader ( const ORef < EncryptionData > & rData, 
								sal_Int32 nSize,
								const ::rtl::OUString& aMediaType,
								sal_Int8 * & pHeader )
{
	// I think it's safe to restrict vector and salt length to 2 bytes !
	sal_Int16 nIVLength = static_cast < sal_Int16 > ( rData->aInitVector.getLength() );
	sal_Int16 nSaltLength = static_cast < sal_Int16 > ( rData->aSalt.getLength() );
	sal_Int16 nDigestLength = static_cast < sal_Int16 > ( rData->aDigest.getLength() );
	sal_Int16 nMediaTypeLength = static_cast < sal_Int16 > ( aMediaType.getLength() * sizeof( sal_Unicode ) );

	// First the header
	*(pHeader++) = ( n_ConstHeader >> 0 ) & 0xFF;
	*(pHeader++) = ( n_ConstHeader >> 8 ) & 0xFF;
	*(pHeader++) = ( n_ConstHeader >> 16 ) & 0xFF;
	*(pHeader++) = ( n_ConstHeader >> 24 ) & 0xFF;

	// Then the version
	*(pHeader++) = ( n_ConstCurrentVersion >> 0 ) & 0xFF;
	*(pHeader++) = ( n_ConstCurrentVersion >> 8 ) & 0xFF;

	// Then the iteration Count
	sal_Int32 nIterationCount = rData->nIterationCount;
	*(pHeader++) = static_cast< sal_Int8 >(( nIterationCount >> 0 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nIterationCount >> 8 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nIterationCount >> 16 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nIterationCount >> 24 ) & 0xFF);

	// Then the size
	*(pHeader++) = static_cast< sal_Int8 >(( nSize >> 0 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nSize >> 8 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nSize >> 16 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nSize >> 24 ) & 0xFF);

	// Then the salt length
	*(pHeader++) = static_cast< sal_Int8 >(( nSaltLength >> 0 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nSaltLength >> 8 ) & 0xFF);

	// Then the IV length
	*(pHeader++) = static_cast< sal_Int8 >(( nIVLength >> 0 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nIVLength >> 8 ) & 0xFF);

	// Then the digest length
	*(pHeader++) = static_cast< sal_Int8 >(( nDigestLength >> 0 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nDigestLength >> 8 ) & 0xFF);

	// Then the mediatype length
	*(pHeader++) = static_cast< sal_Int8 >(( nMediaTypeLength >> 0 ) & 0xFF);
	*(pHeader++) = static_cast< sal_Int8 >(( nMediaTypeLength >> 8 ) & 0xFF);

	// Then the salt content
	memcpy ( pHeader, rData->aSalt.getConstArray(), nSaltLength ); 
	pHeader += nSaltLength;

	// Then the IV content
	memcpy ( pHeader, rData->aInitVector.getConstArray(), nIVLength ); 
	pHeader += nIVLength;

	// Then the digest content
	memcpy ( pHeader, rData->aDigest.getConstArray(), nDigestLength ); 
	pHeader += nDigestLength;

	// Then the mediatype itself
	memcpy ( pHeader, aMediaType.getStr(), nMediaTypeLength ); 
	pHeader += nMediaTypeLength;
}

sal_Bool ZipFile::StaticFillData ( ORef < EncryptionData > & rData,
									sal_Int32 &rSize,
									::rtl::OUString& aMediaType,
									Reference < XInputStream > &rStream )
{
	sal_Bool bOk = sal_False;
	const sal_Int32 nHeaderSize = n_ConstHeaderSize - 4;
	Sequence < sal_Int8 > aBuffer ( nHeaderSize );
	if ( nHeaderSize == rStream->readBytes ( aBuffer, nHeaderSize ) )
	{
		sal_Int16 nPos = 0;
		sal_Int8 *pBuffer = aBuffer.getArray();
		sal_Int16 nVersion = pBuffer[nPos++] & 0xFF;
		nVersion |= ( pBuffer[nPos++] & 0xFF ) << 8;
		if ( nVersion == n_ConstCurrentVersion )
		{
			sal_Int32 nCount = pBuffer[nPos++] & 0xFF;
			nCount |= ( pBuffer[nPos++] & 0xFF ) << 8;
			nCount |= ( pBuffer[nPos++] & 0xFF ) << 16;
			nCount |= ( pBuffer[nPos++] & 0xFF ) << 24;
			rData->nIterationCount = nCount;

			rSize  =   pBuffer[nPos++] & 0xFF;
			rSize |= ( pBuffer[nPos++] & 0xFF ) << 8;
			rSize |= ( pBuffer[nPos++] & 0xFF ) << 16;
			rSize |= ( pBuffer[nPos++] & 0xFF ) << 24;

			sal_Int16 nSaltLength =   pBuffer[nPos++] & 0xFF;
			nSaltLength          |= ( pBuffer[nPos++] & 0xFF ) << 8;
			sal_Int16 nIVLength   = ( pBuffer[nPos++] & 0xFF );
			nIVLength 			 |= ( pBuffer[nPos++] & 0xFF ) << 8;
			sal_Int16 nDigestLength = pBuffer[nPos++] & 0xFF;
			nDigestLength 	     |= ( pBuffer[nPos++] & 0xFF ) << 8;

			sal_Int16 nMediaTypeLength = pBuffer[nPos++] & 0xFF;
			nMediaTypeLength |= ( pBuffer[nPos++] & 0xFF ) << 8;

			if ( nSaltLength == rStream->readBytes ( aBuffer, nSaltLength ) )
			{
				rData->aSalt.realloc ( nSaltLength );
				memcpy ( rData->aSalt.getArray(), aBuffer.getConstArray(), nSaltLength );
				if ( nIVLength == rStream->readBytes ( aBuffer, nIVLength ) )
				{
					rData->aInitVector.realloc ( nIVLength );
					memcpy ( rData->aInitVector.getArray(), aBuffer.getConstArray(), nIVLength );
					if ( nDigestLength == rStream->readBytes ( aBuffer, nDigestLength ) )
					{
						rData->aDigest.realloc ( nDigestLength );
						memcpy ( rData->aDigest.getArray(), aBuffer.getConstArray(), nDigestLength );

						if ( nMediaTypeLength == rStream->readBytes ( aBuffer, nMediaTypeLength ) )
						{
							aMediaType = ::rtl::OUString( (sal_Unicode*)aBuffer.getConstArray(),
															nMediaTypeLength / sizeof( sal_Unicode ) );
							bOk = sal_True;
						}
					}
				}
			}
		}
	}
	return bOk;
}

Reference< XInputStream > ZipFile::StaticGetDataFromRawStream(	const Reference< XInputStream >& xStream,
																const ORef < EncryptionData > &rData )
		throw ( packages::WrongPasswordException, ZipIOException, RuntimeException )
{
	if ( rData.isEmpty() )
		throw ZipIOException( OUString::createFromAscii( "Encrypted stream without encryption data!\n" ),
							Reference< XInterface >() );

	if ( !rData->aKey.getLength() )
		throw packages::WrongPasswordException();

	Reference< XSeekable > xSeek( xStream, UNO_QUERY );
	if ( !xSeek.is() )
		throw ZipIOException( OUString::createFromAscii( "The stream must be seekable!\n" ),
							Reference< XInterface >() );


	// if we have a digest, then this file is an encrypted one and we should
	// check if we can decrypt it or not
	OSL_ENSURE( rData->aDigest.getLength(), "Can't detect password correctness without digest!\n" );
	if ( rData->aDigest.getLength() )
	{
            sal_Int32 nSize = sal::static_int_cast< sal_Int32 >( xSeek->getLength() );
		nSize = nSize > n_ConstDigestLength ? n_ConstDigestLength : nSize;

		// skip header
		xSeek->seek( n_ConstHeaderSize + rData->aInitVector.getLength() + 
								rData->aSalt.getLength() + rData->aDigest.getLength() );

		// Only want to read enough to verify the digest
		Sequence < sal_Int8 > aReadBuffer ( nSize );

		xStream->readBytes( aReadBuffer, nSize ); 
	
		if ( !StaticHasValidPassword( aReadBuffer, rData ) )
			throw packages::WrongPasswordException();
	}

	return new XUnbufferedStream ( xStream, rData );
}

sal_Bool ZipFile::StaticHasValidPassword( const Sequence< sal_Int8 > &aReadBuffer, const ORef < EncryptionData > &rData )
{
	if ( !rData.isValid() || !rData->aKey.getLength() )
		return sal_False;

	sal_Bool bRet = sal_False;
	sal_Int32 nSize = aReadBuffer.getLength();

#ifndef NO_OOO_3_4_1_AES_ENCRYPTION
	Sequence< sal_Int8 > aDecryptedBuffer;
	if ( StaticGetDecryptedData( aReadBuffer, rData, aDecryptedBuffer ) )
	{
		// Check digest of decrypted data
		PK11Context *pDigestContext = PK11_CreateDigestContext( rData->nDigestType == ENCRYPTION_DATA_SHA256_1K ? SEC_OID_SHA256 : SEC_OID_SHA1 );
		if ( pDigestContext && PK11_DigestBegin( pDigestContext ) == SECSuccess )
		{
			if ( PK11_DigestOp( pDigestContext, reinterpret_cast< const unsigned char* >( aDecryptedBuffer.getConstArray() ), aDecryptedBuffer.getLength() > 1024 ? 1024 : aDecryptedBuffer.getLength() ) == SECSuccess )
			{
				uno::Sequence< sal_Int8 > aDigestSeq( 32 );
				unsigned int nDigestLen = 0;
				if ( PK11_DigestFinal( pDigestContext, reinterpret_cast< unsigned char* >( aDigestSeq.getArray() ), &nDigestLen, aDigestSeq.getLength() ) == SECSuccess )
				{
					aDigestSeq.realloc( nDigestLen );
					if ( !rData->aDigest.getLength() || ( aDigestSeq.getLength() == rData->aDigest.getLength() && !rtl_compareMemory( aDigestSeq.getConstArray(), rData->aDigest.getConstArray(), aDigestSeq.getLength() ) ) )
						bRet = sal_True;
				}
			}

			PK11_DestroyContext( pDigestContext, PR_TRUE );
		}
	}
	else
	{
#endif	// !NO_OOO_3_4_1_AES_ENCRYPTION
	// make a temporary cipher
	rtlCipher aCipher;
	StaticGetCipher ( rData, aCipher );

	Sequence < sal_Int8 > aDecryptBuffer ( nSize );
	rtlDigest aDigest = rtl_digest_createSHA1();
	rtlDigestError aDigestResult;
	Sequence < sal_uInt8 > aDigestSeq ( RTL_DIGEST_LENGTH_SHA1 );
	rtlCipherError aResult = rtl_cipher_decode ( aCipher,
								  aReadBuffer.getConstArray(),
								  nSize,
								  reinterpret_cast < sal_uInt8 * > (aDecryptBuffer.getArray()),
								  nSize);
	if(aResult != rtl_Cipher_E_None ) {
        OSL_ASSERT ( aResult == rtl_Cipher_E_None);
    }
    
	aDigestResult = rtl_digest_updateSHA1 ( aDigest,
											static_cast < const void * > ( aDecryptBuffer.getConstArray() ), nSize );
	OSL_ASSERT ( aDigestResult == rtl_Digest_E_None );

	aDigestResult = rtl_digest_getSHA1 ( aDigest, aDigestSeq.getArray(), RTL_DIGEST_LENGTH_SHA1 );
	OSL_ASSERT ( aDigestResult == rtl_Digest_E_None );

	// If we don't have a digest, then we have to assume that the password is correct
	if (  rData->aDigest.getLength() != 0  && 
	      ( aDigestSeq.getLength() != rData->aDigest.getLength() ||
	        0 != rtl_compareMemory ( aDigestSeq.getConstArray(), 
		 					        rData->aDigest.getConstArray(), 
							        aDigestSeq.getLength() ) ) )
	{
		// We should probably tell the user that the password they entered was wrong
	}
	else
		bRet = sal_True;

	rtl_digest_destroySHA1 ( aDigest );
#ifndef NO_OOO_3_4_1_AES_ENCRYPTION
	}
#endif	// !NO_OOO_3_4_1_AES_ENCRYPTION

	return bRet;
}

sal_Bool ZipFile::hasValidPassword ( ZipEntry & rEntry, const ORef < EncryptionData > &rData )
{
	sal_Bool bRet = sal_False;
	if ( rData->aKey.getLength() )
	{
		xSeek->seek( rEntry.nOffset );
		sal_Int32 nSize = rEntry.nMethod == DEFLATED ? rEntry.nCompressedSize : rEntry.nSize;

		// Only want to read enough to verify the digest
		nSize = nSize > n_ConstDigestLength ? n_ConstDigestLength : nSize;
		Sequence < sal_Int8 > aReadBuffer ( nSize );

		xStream->readBytes( aReadBuffer, nSize ); 

		bRet = StaticHasValidPassword( aReadBuffer, rData );
	}
	return bRet;
}

#if 0
Reference < XInputStream > ZipFile::createFileStream(
			ZipEntry & rEntry,
			const ORef < EncryptionData > &rData,
			sal_Bool bRawStream,
			sal_Bool bIsEncrypted )
{
	static OUString sServiceName ( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.io.TempFile" ) );
	Reference < XInputStream > xTempStream = Reference < XInputStream > ( xFactory->createInstance ( sServiceName ), UNO_QUERY );
	return new XFileStream ( rEntry, xStream, xTempStream, rData, bRawStream, bIsEncrypted );
}
Reference < XInputStream > ZipFile::createMemoryStream(
			ZipEntry & rEntry,
			const ORef < EncryptionData > &rData,
			sal_Bool bRawStream, 
			sal_Bool bIsEncrypted )
{
	sal_Int32 nUncompressedSize, nEnd;
	if (bRawStream)
	{
		nUncompressedSize = rEntry.nMethod == DEFLATED ? rEntry.nCompressedSize : rEntry.nSize;
		nEnd = rEntry.nOffset + nUncompressedSize;
	}
	else
	{
		nUncompressedSize = rEntry.nSize;
		nEnd = rEntry.nMethod == DEFLATED ? rEntry.nOffset + rEntry.nCompressedSize : rEntry.nOffset + rEntry.nSize;
	}
	sal_Int32 nSize = rEntry.nMethod == DEFLATED ? rEntry.nCompressedSize : rEntry.nSize;
	Sequence < sal_Int8 > aReadBuffer ( nSize ), aDecryptBuffer, aWriteBuffer;
	rtlCipher aCipher;

	// If the encryption key is zero, we need to return the raw stream. First check if
	// we have the salt. If we have the salt, then check if we have the encryption key
	// if not, return rawStream instead.
	
	sal_Bool bHaveEncryptData = ( !rData.isEmpty() && rData->aSalt.getLength() && rData->aInitVector.getLength() && rData->nIterationCount != 0 ) ? sal_True : sal_False;
	sal_Bool bMustDecrypt = ( !bRawStream && bHaveEncryptData && bIsEncrypted ) ? sal_True : sal_False;

	if ( bMustDecrypt )
	{
		StaticGetCipher ( rData, aCipher );
		aDecryptBuffer.realloc ( nSize );
	}

	if ( nSize <0 )
		throw IOException ( );

	xSeek->seek( rEntry.nOffset );
	xStream->readBytes( aReadBuffer, nSize ); // Now it holds the raw stuff from disk

	if ( bMustDecrypt )
	{
		rtlCipherError aResult = rtl_cipher_decode ( aCipher,
									  aReadBuffer.getConstArray(),
									  nSize,
									  reinterpret_cast < sal_uInt8 * > (aDecryptBuffer.getArray()),
									  nSize);
		OSL_ASSERT (aResult == rtl_Cipher_E_None);
		aReadBuffer = aDecryptBuffer; // Now it holds the decrypted data
	}
	if (bRawStream || rEntry.nMethod == STORED)
		aWriteBuffer = aReadBuffer; // bRawStream means the caller doesn't want it decompressed
	else
	{
		aInflater.setInputSegment( aReadBuffer, 0, nSize );
		aWriteBuffer.realloc( nUncompressedSize );
		aInflater.doInflate( aWriteBuffer );
		aInflater.reset();
	}

	if ( bHaveEncryptData && !bMustDecrypt && bIsEncrypted )
	{
		// if we have the data needed to decrypt it, but didn't want it decrypted (or
		// we couldn't decrypt it due to wrong password), then we prepend this
		// data to the stream

		// Make a buffer big enough to hold both the header and the data itself
		Sequence < sal_Int8 > aEncryptedDataHeader ( n_ConstHeaderSize + 
													 rData->aInitVector.getLength() + 
													 rData->aSalt.getLength() + 
													 rData->aDigest.getLength() + 
													 aWriteBuffer.getLength() );
		sal_Int8 * pHeader = aEncryptedDataHeader.getArray();
		StaticFillHeader ( rData, rEntry.nSize, pHeader );
		memcpy ( pHeader, aWriteBuffer.getConstArray(), aWriteBuffer.getLength() );

		// dump old buffer and point aWriteBuffer to the new one with the header
		aWriteBuffer = aEncryptedDataHeader;
	}
	return Reference < XInputStream > ( new XMemoryStream ( aWriteBuffer ) );
}
#endif
Reference < XInputStream > ZipFile::createUnbufferedStream(
			ZipEntry & rEntry,
			const ORef < EncryptionData > &rData,
			sal_Int8 nStreamMode,
			sal_Bool bIsEncrypted,
			::rtl::OUString aMediaType )
{
	return new XUnbufferedStream ( rEntry, xStream, rData, nStreamMode, bIsEncrypted, aMediaType, bRecoveryMode );
}


ZipEnumeration * SAL_CALL ZipFile::entries(  )
{
	return new ZipEnumeration ( aEntries );
}

Reference< XInputStream > SAL_CALL ZipFile::getInputStream( ZipEntry& rEntry,
		const vos::ORef < EncryptionData > &rData,
		sal_Bool bIsEncrypted )
	throw(IOException, ZipException, RuntimeException)
{
	if ( rEntry.nOffset <= 0 )
		readLOC( rEntry );

	// We want to return a rawStream if we either don't have a key or if the 
	// key is wrong
	
	sal_Bool bNeedRawStream = rEntry.nMethod == STORED;
	
	// if we have a digest, then this file is an encrypted one and we should
	// check if we can decrypt it or not
	if ( bIsEncrypted && !rData.isEmpty() && rData->aDigest.getLength() )
		bNeedRawStream = !hasValidPassword ( rEntry, rData );

	return createUnbufferedStream ( rEntry,
									rData,
									bNeedRawStream ? UNBUFF_STREAM_RAW : UNBUFF_STREAM_DATA,
									bIsEncrypted );
}

Reference< XInputStream > SAL_CALL ZipFile::getDataStream( ZipEntry& rEntry,
		const vos::ORef < EncryptionData > &rData,
		sal_Bool bIsEncrypted )
	throw ( packages::WrongPasswordException,
			IOException,
			ZipException,
			RuntimeException )
{
	if ( rEntry.nOffset <= 0 )
		readLOC( rEntry );

	// An exception must be thrown in case stream is encrypted and 
	// there is no key or the key is wrong
	sal_Bool bNeedRawStream = sal_False;
	if ( bIsEncrypted )
	{
		// in case no digest is provided there is no way
		// to detect password correctness
		if ( rData.isEmpty() )
			throw ZipException( OUString::createFromAscii( "Encrypted stream without encryption data!\n" ),
								Reference< XInterface >() );

		// if we have a digest, then this file is an encrypted one and we should
		// check if we can decrypt it or not
		OSL_ENSURE( rData->aDigest.getLength(), "Can't detect password correctness without digest!\n" );
		if ( rData->aDigest.getLength() && !hasValidPassword ( rEntry, rData ) )
				throw packages::WrongPasswordException();
	}
	else
		bNeedRawStream = ( rEntry.nMethod == STORED );

	return createUnbufferedStream ( rEntry,
									rData,
									bNeedRawStream ? UNBUFF_STREAM_RAW : UNBUFF_STREAM_DATA,
									bIsEncrypted );
}

Reference< XInputStream > SAL_CALL ZipFile::getRawData( ZipEntry& rEntry,
		const vos::ORef < EncryptionData > &rData,
		sal_Bool bIsEncrypted )
	throw(IOException, ZipException, RuntimeException)
{
	if ( rEntry.nOffset <= 0 )
		readLOC( rEntry );

	return createUnbufferedStream ( rEntry, rData, UNBUFF_STREAM_RAW, bIsEncrypted );
}

Reference< XInputStream > SAL_CALL ZipFile::getWrappedRawStream(
		ZipEntry& rEntry,
		const vos::ORef < EncryptionData > &rData,
		const ::rtl::OUString& aMediaType )
	throw ( packages::NoEncryptionException,
			IOException,
			ZipException,
			RuntimeException )
{
	if ( rData.isEmpty() )
		throw packages::NoEncryptionException();

	if ( rEntry.nOffset <= 0 )
		readLOC( rEntry );

	return createUnbufferedStream ( rEntry, rData, UNBUFF_STREAM_WRAPPEDRAW, sal_True, aMediaType );
}

sal_Bool ZipFile::readLOC( ZipEntry &rEntry )
	throw(IOException, ZipException, RuntimeException)
{
	sal_Int32 nTestSig, nTime, nCRC, nSize, nCompressedSize;
	sal_Int16 nVersion, nFlag, nHow, nNameLen, nExtraLen;
	sal_Int32 nPos = -rEntry.nOffset;

	aGrabber.seek(nPos);
	aGrabber >> nTestSig;

	if (nTestSig != LOCSIG)
		throw ZipIOException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Invalid LOC header (bad signature") ), Reference < XInterface > () );
	aGrabber >> nVersion;
	aGrabber >> nFlag;
	aGrabber >> nHow;
	aGrabber >> nTime;
	aGrabber >> nCRC;
	aGrabber >> nCompressedSize;
	aGrabber >> nSize;
	aGrabber >> nNameLen;
	aGrabber >> nExtraLen;
	rEntry.nOffset = static_cast < sal_Int32 > (aGrabber.getPosition()) + nNameLen + nExtraLen;

	if ( rEntry.nNameLen == -1 ) // the file was created
		rEntry.nNameLen = nNameLen;

	// the method can be reset for internal use so it is not checked
	sal_Bool bBroken = rEntry.nVersion != nVersion
					|| rEntry.nFlag != nFlag
					|| rEntry.nTime != nTime
					|| rEntry.nNameLen != nNameLen;

	if ( bBroken && !bRecoveryMode )
		throw ZipIOException( OUString( RTL_CONSTASCII_USTRINGPARAM( "The stream seems to be broken!" ) ),
							Reference< XInterface >() );

	return sal_True;
}

sal_Int32 ZipFile::findEND( )
	throw(IOException, ZipException, RuntimeException)
{
	sal_Int32 nLength, nPos, nEnd;
	Sequence < sal_Int8 > aBuffer;
	try
	{
		nLength = static_cast <sal_Int32 > (aGrabber.getLength());
		if (nLength == 0 || nLength < ENDHDR)
			return -1;
		nPos = nLength - ENDHDR - ZIP_MAXNAMELEN;
		nEnd = nPos >= 0 ? nPos : 0 ;

		aGrabber.seek( nEnd );
		aGrabber.readBytes ( aBuffer, nLength - nEnd );

		const sal_Int8 *pBuffer = aBuffer.getConstArray();

		nPos = nLength - nEnd - ENDHDR;
		while ( nPos >= 0 )
		{
			if (pBuffer[nPos] == 'P' && pBuffer[nPos+1] == 'K' && pBuffer[nPos+2] == 5 && pBuffer[nPos+3] == 6 )
				return nPos + nEnd;
			nPos--;
		}
	}
	catch ( IllegalArgumentException& )
	{
		throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
	}
	catch ( NotConnectedException& )
	{
		throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
	}
	catch ( BufferSizeExceededException& )
	{
		throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
	}
	throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
}

sal_Int32 ZipFile::readCEN()
	throw(IOException, ZipException, RuntimeException)
{
	sal_Int32 nCenLen, nCenPos = -1, nCenOff, nEndPos, nLocPos;
	sal_uInt16 nCount, nTotal;

	try
	{
		nEndPos = findEND();
		if (nEndPos == -1)
			return -1;
		aGrabber.seek(nEndPos + ENDTOT);
		aGrabber >> nTotal;
		aGrabber >> nCenLen;
		aGrabber >> nCenOff;

		if ( nTotal * CENHDR > nCenLen )
			throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "invalid END header (bad entry count)") ), Reference < XInterface > () );

		if ( nTotal > ZIP_MAXENTRIES )
			throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "too many entries in ZIP File") ), Reference < XInterface > () );

		if ( nCenLen < 0 || nCenLen > nEndPos )
			throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "Invalid END header (bad central directory size)") ), Reference < XInterface > () );

		nCenPos = nEndPos - nCenLen;

		if ( nCenOff < 0 || nCenOff > nCenPos )
			throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "Invalid END header (bad central directory size)") ), Reference < XInterface > () );

		nLocPos = nCenPos - nCenOff;
		aGrabber.seek( nCenPos );
		Sequence < sal_Int8 > aCENBuffer ( nCenLen );
		sal_Int64 nRead = aGrabber.readBytes ( aCENBuffer, nCenLen );
		if ( static_cast < sal_Int64 > ( nCenLen ) != nRead )
			throw ZipException ( OUString ( RTL_CONSTASCII_USTRINGPARAM ( "Error reading CEN into memory buffer!") ), Reference < XInterface > () );

		MemoryByteGrabber aMemGrabber ( aCENBuffer );

		ZipEntry aEntry;
		sal_Int32 nTestSig;
		sal_Int16 nCommentLen;

		for (nCount = 0 ; nCount < nTotal; nCount++)
		{
			aMemGrabber >> nTestSig;
			if ( nTestSig != CENSIG )
				throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "Invalid CEN header (bad signature)") ), Reference < XInterface > () );

			aMemGrabber.skipBytes ( 2 );
			aMemGrabber >> aEntry.nVersion;

			if ( ( aEntry.nVersion & 1 ) == 1 )
				throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "Invalid CEN header (encrypted entry)") ), Reference < XInterface > () );

			aMemGrabber >> aEntry.nFlag;
			aMemGrabber >> aEntry.nMethod;

			if ( aEntry.nMethod != STORED && aEntry.nMethod != DEFLATED)
				throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "Invalid CEN header (bad compression method)") ), Reference < XInterface > () );

			aMemGrabber >> aEntry.nTime;
			aMemGrabber >> aEntry.nCrc;
			aMemGrabber >> aEntry.nCompressedSize;
			aMemGrabber >> aEntry.nSize;
			aMemGrabber >> aEntry.nNameLen;
			aMemGrabber >> aEntry.nExtraLen;
			aMemGrabber >> nCommentLen;
			aMemGrabber.skipBytes ( 8 );
			aMemGrabber >> aEntry.nOffset;

			aEntry.nOffset += nLocPos;
			aEntry.nOffset *= -1;

			if ( aEntry.nNameLen < 0 || aEntry.nNameLen > ZIP_MAXNAMELEN )
				throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "name length exceeds ZIP_MAXNAMELEN bytes" ) ), Reference < XInterface > () );

			if ( nCommentLen < 0 || nCommentLen > ZIP_MAXNAMELEN )
				throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "comment length exceeds ZIP_MAXNAMELEN bytes" ) ), Reference < XInterface > () );

			if ( aEntry.nExtraLen < 0 || aEntry.nExtraLen > ZIP_MAXEXTRA )
				throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "extra header info exceeds ZIP_MAXEXTRA bytes") ), Reference < XInterface > () );

            // read always in UTF8, some tools seem not to set UTF8 bit
			aEntry.sName = rtl::OUString::intern ( (sal_Char *) aMemGrabber.getCurrentPos(), 
                                                   aEntry.nNameLen, 
                                                   RTL_TEXTENCODING_UTF8 );

            if ( !::comphelper::OStorageHelper::IsValidZipEntryFileName( aEntry.sName, sal_True ) )
				throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip entry has an invalid name.") ), Reference < XInterface > () );

			aMemGrabber.skipBytes( aEntry.nNameLen + aEntry.nExtraLen + nCommentLen );
			aEntries[aEntry.sName] = aEntry;	
		}

		if (nCount != nTotal)
			throw ZipException(OUString( RTL_CONSTASCII_USTRINGPARAM ( "Count != Total") ), Reference < XInterface > () );
	}
	catch ( IllegalArgumentException & )
	{
		// seek can throw this...
		nCenPos = -1; // make sure we return -1 to indicate an error
	}
	return nCenPos;
}

sal_Int32 ZipFile::recover()
	throw(IOException, ZipException, RuntimeException)
{
	sal_Int32 nLength;
	Sequence < sal_Int8 > aBuffer;
	Sequence < sal_Int32 > aHeaderOffsets;

	try
	{
		nLength = static_cast <sal_Int32 > (aGrabber.getLength());
		if (nLength == 0 || nLength < ENDHDR)
			return -1;

		aGrabber.seek( 0 );

		for( sal_Int32 nGenPos = 0; aGrabber.readBytes( aBuffer, 32000 ) && aBuffer.getLength() > 30; )
		{
			const sal_Int8 *pBuffer = aBuffer.getConstArray();
			sal_Int32 nBufSize = aBuffer.getLength();

			sal_Int32 nPos = 0;
			while( nPos < nBufSize - 16 )
			{
				if ( nPos < nBufSize - 30 && pBuffer[nPos] == 'P' && pBuffer[nPos+1] == 'K' && pBuffer[nPos+2] == 3 && pBuffer[nPos+3] == 4 )
				{
					ZipEntry aEntry;
					MemoryByteGrabber aMemGrabber ( Sequence< sal_Int8 >( ((sal_Int8*)(&(pBuffer[nPos+4]))), 26 ) );

					aMemGrabber >> aEntry.nVersion;
					if ( ( aEntry.nVersion & 1 ) != 1 )
					{
						aMemGrabber >> aEntry.nFlag;
						aMemGrabber >> aEntry.nMethod;

						if ( aEntry.nMethod == STORED || aEntry.nMethod == DEFLATED )
						{
							aMemGrabber >> aEntry.nTime;
							aMemGrabber >> aEntry.nCrc;
							aMemGrabber >> aEntry.nCompressedSize;
							aMemGrabber >> aEntry.nSize;
							aMemGrabber >> aEntry.nNameLen;
							aMemGrabber >> aEntry.nExtraLen;

							sal_Int32 nDescrLength = 
								( aEntry.nMethod == DEFLATED && ( aEntry.nFlag & 8 ) ) ?
														16 : 0;


							// This is a quick fix for OOo1.1RC
							// For OOo2.0 the whole package must be switched to unsigned values
							if ( aEntry.nCompressedSize < 0 ) aEntry.nCompressedSize = 0x7FFFFFFF;
							if ( aEntry.nSize < 0 ) aEntry.nSize = 0x7FFFFFFF;
							if ( aEntry.nNameLen < 0 ) aEntry.nNameLen = 0x7FFF;
							if ( aEntry.nExtraLen < 0 ) aEntry.nExtraLen = 0x7FFF;
							// End of quick fix


							sal_Int32 nBlockLength = aEntry.nSize + aEntry.nNameLen + aEntry.nExtraLen + 30 + nDescrLength;
							if ( aEntry.nNameLen <= ZIP_MAXNAMELEN && aEntry.nExtraLen < ZIP_MAXEXTRA 
								&& ( nGenPos + nPos + nBlockLength ) <= nLength )
							{
                                // read always in UTF8, some tools seem not to set UTF8 bit
								if( nPos + 30 + aEntry.nNameLen <= nBufSize )
									aEntry.sName = OUString ( (sal_Char *) &pBuffer[nPos + 30], 
									  							aEntry.nNameLen, 
																RTL_TEXTENCODING_UTF8 );
								else
								{
									Sequence < sal_Int8 > aFileName;
									aGrabber.seek( nGenPos + nPos + 30 );
									aGrabber.readBytes( aFileName, aEntry.nNameLen );
									aEntry.sName = OUString ( (sal_Char *) aFileName.getArray(), 
																aFileName.getLength(), 
																RTL_TEXTENCODING_UTF8 );
									aEntry.nNameLen = static_cast< sal_Int16 >(aFileName.getLength());
								}

								aEntry.nOffset = nGenPos + nPos + 30 + aEntry.nNameLen + aEntry.nExtraLen;

								if ( ( aEntry.nSize || aEntry.nCompressedSize ) && !checkSizeAndCRC( aEntry ) )
								{
									aEntry.nCrc = 0;
									aEntry.nCompressedSize = 0;
									aEntry.nSize = 0;
								}

								if ( aEntries.find( aEntry.sName ) == aEntries.end() )
									aEntries[aEntry.sName] = aEntry;
							}
						}
					}

					nPos += 4;
				}
				else if (pBuffer[nPos] == 'P' && pBuffer[nPos+1] == 'K' && pBuffer[nPos+2] == 7 && pBuffer[nPos+3] == 8 )
				{
					sal_Int32 nCompressedSize, nSize, nCRC32;
					MemoryByteGrabber aMemGrabber ( Sequence< sal_Int8 >( ((sal_Int8*)(&(pBuffer[nPos+4]))), 12 ) );
					aMemGrabber >> nCRC32;
					aMemGrabber >> nCompressedSize;
					aMemGrabber >> nSize;

					for( EntryHash::iterator aIter = aEntries.begin(); aIter != aEntries.end(); aIter++ )
					{
						ZipEntry aTmp = (*aIter).second;

                        // this is a broken package, accept this block not only for DEFLATED streams
						if( (*aIter).second.nFlag & 8 )
						{
							sal_Int32 nStreamOffset = nGenPos + nPos - nCompressedSize;
							if ( nStreamOffset == (*aIter).second.nOffset && nCompressedSize > (*aIter).second.nCompressedSize )
							{
                                // only DEFLATED blocks need to be checked
                                sal_Bool bAcceptBlock = ( (*aIter).second.nMethod == STORED && nCompressedSize == nSize );

                                if ( !bAcceptBlock )
                                {
                                    sal_Int32 nRealSize = 0, nRealCRC = 0;
                                    getSizeAndCRC( nStreamOffset, nCompressedSize, &nRealSize, &nRealCRC );
                                    bAcceptBlock = ( nRealSize == nSize && nRealCRC == nCRC32 );
                                }

                                if ( bAcceptBlock )
								{
									(*aIter).second.nCrc = nCRC32;
									(*aIter).second.nCompressedSize = nCompressedSize;
									(*aIter).second.nSize = nSize;
								}
							}
#if 0
// for now ignore clearly broken streams
							else if( !(*aIter).second.nCompressedSize )
							{
								(*aIter).second.nCrc = nCRC32;
								sal_Int32 nRealStreamSize = nGenPos + nPos - (*aIter).second.nOffset;
								(*aIter).second.nCompressedSize = nGenPos + nPos - (*aIter).second.nOffset;
								(*aIter).second.nSize = nSize;
							}
#endif
						}
					}

					nPos += 4;
				}
				else
					nPos++;
			}

			nGenPos += nPos;
			aGrabber.seek( nGenPos );
		}

		return 0;
	}
	catch ( IllegalArgumentException& )
	{
		throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
	}
	catch ( NotConnectedException& )
	{
		throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
	}
	catch ( BufferSizeExceededException& )
	{
		throw ZipException( OUString( RTL_CONSTASCII_USTRINGPARAM ( "Zip END signature not found!") ), Reference < XInterface > () );
	}
}

sal_Bool ZipFile::checkSizeAndCRC( const ZipEntry& aEntry )
{
	sal_Int32 nSize = 0, nCRC = 0;

	if( aEntry.nMethod == STORED )
		return ( getCRC( aEntry.nOffset, aEntry.nSize ) == aEntry.nCrc );

	getSizeAndCRC( aEntry.nOffset, aEntry.nCompressedSize, &nSize, &nCRC );
	return ( aEntry.nSize == nSize && aEntry.nCrc == nCRC );
}

sal_Int32 ZipFile::getCRC( sal_Int32 nOffset, sal_Int32 nSize )
{
	Sequence < sal_Int8 > aBuffer;
	CRC32 aCRC;
	sal_Int32 nBlockSize = ::std::min( nSize, static_cast< sal_Int32 >( 32000 ) );

	aGrabber.seek( nOffset );
	for ( int ind = 0;
		  aGrabber.readBytes( aBuffer, nBlockSize ) && ind * nBlockSize < nSize;
		  ind++ )
	{
		aCRC.updateSegment( aBuffer, 0, ::std::min( nBlockSize, nSize - ind * nBlockSize ) );
	}

	return aCRC.getValue();
}

void ZipFile::getSizeAndCRC( sal_Int32 nOffset, sal_Int32 nCompressedSize, sal_Int32 *nSize, sal_Int32 *nCRC )
{
	Sequence < sal_Int8 > aBuffer;
	CRC32 aCRC;
	sal_Int32 nRealSize = 0;
	Inflater aInflaterLocal( sal_True );
	sal_Int32 nBlockSize = ::std::min( nCompressedSize, static_cast< sal_Int32 >( 32000 ) );

	aGrabber.seek( nOffset );
	for ( int ind = 0;
		  !aInflaterLocal.finished() && aGrabber.readBytes( aBuffer, nBlockSize ) && ind * nBlockSize < nCompressedSize;
		  ind++ )
	{
		Sequence < sal_Int8 > aData( nBlockSize );
		sal_Int32 nLastInflated = 0;
		sal_Int32 nInBlock = 0;

		aInflaterLocal.setInput( aBuffer );
		do
		{
			nLastInflated = aInflaterLocal.doInflateSegment( aData, 0, nBlockSize );
			aCRC.updateSegment( aData, 0, nLastInflated );
			nInBlock += nLastInflated;
		} while( !aInflater.finished() && nLastInflated );
		
		nRealSize += nInBlock;
	}

	if( aInflaterLocal.finished() )
	{
		*nSize = nRealSize;
		*nCRC = aCRC.getValue();
	}
	else
		*nSize = *nCRC = 0;		
}
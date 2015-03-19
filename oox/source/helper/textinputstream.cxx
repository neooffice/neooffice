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

#include "oox/helper/textinputstream.hxx"

#include <com/sun/star/io/XActiveDataSink.hpp>
#if SUPD == 310
#include <com/sun/star/io/XTextInputStream.hpp>
#else	// SUPD == 310
#include <com/sun/star/io/TextInputStream.hpp>
#endif	// SUPD == 310
#include <cppuhelper/implbase1.hxx>
#include <rtl/tencinfo.h>
#include "oox/helper/binaryinputstream.hxx"

namespace oox {



using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;



namespace {

typedef ::cppu::WeakImplHelper1< XInputStream > UnoBinaryInputStream_BASE;

/** Implementation of a UNO input stream wrapping a binary input stream.
 */
class UnoBinaryInputStream : public UnoBinaryInputStream_BASE
{
public:
    explicit            UnoBinaryInputStream( BinaryInputStream& rInStrm );

    virtual sal_Int32 SAL_CALL readBytes( Sequence< sal_Int8 >& rData, sal_Int32 nBytesToRead )
#if SUPD == 310
                        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
                        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310
    virtual sal_Int32 SAL_CALL readSomeBytes( Sequence< sal_Int8 >& rData, sal_Int32 nMaxBytesToRead )
#if SUPD == 310
                        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
                        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310
    virtual void SAL_CALL skipBytes( sal_Int32 nBytesToSkip )
#if SUPD == 310
                        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
                        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310
    virtual sal_Int32 SAL_CALL available()
#if SUPD == 310
                        throw (NotConnectedException, IOException, RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
                        throw (NotConnectedException, IOException, RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310
    virtual void SAL_CALL closeInput()
#if SUPD == 310
                        throw (NotConnectedException, IOException, RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
                        throw (NotConnectedException, IOException, RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310

private:
    void                ensureConnected() const throw (NotConnectedException);

private:
    BinaryInputStream*  mpInStrm;
};



UnoBinaryInputStream::UnoBinaryInputStream( BinaryInputStream& rInStrm ) :
    mpInStrm( &rInStrm )
{
}

sal_Int32 SAL_CALL UnoBinaryInputStream::readBytes( Sequence< sal_Int8 >& rData, sal_Int32 nBytesToRead )
#if SUPD == 310
        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException)
#else	// SUPD == 310
        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException, std::exception)
#endif	// SUPD == 310
{
    ensureConnected();
    return mpInStrm->readData( rData, nBytesToRead, 1 );
}

sal_Int32 SAL_CALL UnoBinaryInputStream::readSomeBytes( Sequence< sal_Int8 >& rData, sal_Int32 nMaxBytesToRead )
#if SUPD == 310
        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException)
#else	// SUPD == 310
        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException, std::exception)
#endif	// SUPD == 310
{
    ensureConnected();
    return mpInStrm->readData( rData, nMaxBytesToRead, 1 );
}

void SAL_CALL UnoBinaryInputStream::skipBytes( sal_Int32 nBytesToSkip )
#if SUPD == 310
        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException)
#else	// SUPD == 310
        throw (NotConnectedException, BufferSizeExceededException, IOException, RuntimeException, std::exception)
#endif	// SUPD == 310
{
    ensureConnected();
    mpInStrm->skip( nBytesToSkip, 1 );
}

#if SUPD == 310
sal_Int32 SAL_CALL UnoBinaryInputStream::available() throw (NotConnectedException, IOException, RuntimeException)
#else	// SUPD == 310
sal_Int32 SAL_CALL UnoBinaryInputStream::available() throw (NotConnectedException, IOException, RuntimeException, std::exception)
#endif	// SUPD == 310
{
    ensureConnected();
    throw RuntimeException( "Functionality not supported", Reference< XInputStream >() );
}

#if SUPD == 310
void SAL_CALL UnoBinaryInputStream::closeInput() throw (NotConnectedException, IOException, RuntimeException)
#else	// SUPD == 310
void SAL_CALL UnoBinaryInputStream::closeInput() throw (NotConnectedException, IOException, RuntimeException, std::exception)
#endif	// SUPD == 310
{
    ensureConnected();
    mpInStrm->close();
    mpInStrm = 0;
}

void UnoBinaryInputStream::ensureConnected() const throw (NotConnectedException)
{
    if( !mpInStrm )
        throw NotConnectedException( "Stream closed", Reference< XInterface >() );
}

} // namespace



TextInputStream::TextInputStream( const Reference< XComponentContext >& rxContext, const Reference< XInputStream >& rxInStrm, rtl_TextEncoding eTextEnc )
{
    init( rxContext, rxInStrm, eTextEnc );
}

TextInputStream::TextInputStream( const Reference< XComponentContext >& rxContext, BinaryInputStream& rInStrm, rtl_TextEncoding eTextEnc )
{
    init( rxContext, new UnoBinaryInputStream( rInStrm ), eTextEnc );
}

TextInputStream::~TextInputStream()
{
}

bool TextInputStream::isEof() const
{
    if( mxTextStrm.is() ) try
    {
        return mxTextStrm->isEOF();
    }
    catch (const Exception&)
    {
    }
    return true;
}

OUString TextInputStream::readLine()
{
    if( mxTextStrm.is() ) try
    {
        /*  The function createFinalString() adds a character that may have
            been buffered in the previous call of readToChar() (see below). */
        return createFinalString( mxTextStrm->readLine() );
    }
    catch (const Exception&)
    {
        mxTextStrm.clear();
    }
    return OUString();
}

OUString TextInputStream::readToChar( sal_Unicode cChar, bool bIncludeChar )
{
    if( mxTextStrm.is() ) try
    {
        Sequence< sal_Unicode > aDelimiters( 1 );
        aDelimiters[ 0 ] = cChar;
        /*  Always get the delimiter character from the UNO text input stream.
            In difference to this implementation, it will not return it in the
            next call but silently skip it. If caller specifies to exclude the
            character in this call, it will be returned in the next call of one
            of the own member functions. The function createFinalString() adds
            a character that has been buffered in the previous call. */
        OUString aString = createFinalString( mxTextStrm->readString( aDelimiters, sal_False ) );
        // remove last character from string and remember it for next call
        if( !bIncludeChar && !aString.isEmpty() && (aString[ aString.getLength() - 1 ] == cChar) )
        {
            mcPendingChar = cChar;
            aString = aString.copy( 0, aString.getLength() - 1 );
        }
        return aString;
    }
    catch (const Exception&)
    {
        mxTextStrm.clear();
    }
    return OUString();
}

#if SUPD == 310
/*static*/ Reference< XTextInputStream > TextInputStream::createXTextInputStream(
#else	// SUPD == 310
Reference< XTextInputStream2 > TextInputStream::createXTextInputStream(
#endif	// SUPD == 310
        const Reference< XComponentContext >& rxContext, const Reference< XInputStream >& rxInStrm, rtl_TextEncoding eTextEnc )
{
#if SUPD == 310
    Reference< XTextInputStream > xTextStrm;
#else	// SUPD == 310
    Reference< XTextInputStream2 > xTextStrm;
#endif	// SUPD == 310
    const char* pcCharset = rtl_getBestMimeCharsetFromTextEncoding( eTextEnc );
    OSL_ENSURE( pcCharset, "TextInputStream::createXTextInputStream - unsupported text encoding" );
    if( rxContext.is() && rxInStrm.is() && pcCharset ) try
    {
#if SUPD == 310
        Reference< XMultiServiceFactory > xFactory( rxContext->getServiceManager(), UNO_QUERY_THROW );
        Reference< XActiveDataSink > xDataSink( xFactory->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.io.TextInputStream" ) ) ), UNO_QUERY_THROW );
        xDataSink->setInputStream( rxInStrm );
        xTextStrm.set( xDataSink, UNO_QUERY_THROW );
#else	// SUPD == 310
        xTextStrm = com::sun::star::io::TextInputStream::create( rxContext );
        xTextStrm->setInputStream( rxInStrm );
#endif	// SUPD == 310
        xTextStrm->setEncoding( OUString::createFromAscii( pcCharset ) );
    }
    catch (const Exception&)
    {
    }
    return xTextStrm;
}

// private --------------------------------------------------------------------

OUString TextInputStream::createFinalString( const OUString& rString )
{
    if( mcPendingChar == 0 )
        return rString;

    OUString aString = OUString( mcPendingChar ) + rString;
    mcPendingChar = 0;
    return aString;
}

void TextInputStream::init( const Reference< XComponentContext >& rxContext, const Reference< XInputStream >& rxInStrm, rtl_TextEncoding eTextEnc )
{
    mcPendingChar = 0;
    mxTextStrm = createXTextInputStream( rxContext, rxInStrm, eTextEnc );
}



} // namespace oox

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
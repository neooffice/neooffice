/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
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
 * Modified September 2011 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#include <osl/diagnose.h>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <comphelper/mediadescriptor.hxx>
#if SUPD != 310
#include <oox/core/filterdetect.hxx>
#endif	// SUPD != 310
#include <dmapper/DomainMapper.hxx>
#include <WriterFilter.hxx>
#include <doctok/WW8Document.hxx>
#include <ooxml/OOXMLDocument.hxx>
#ifdef DEBUG_IMPORT
#include <iostream>
#include <osl/process.h>
#endif

#include <resourcemodel/TagLogger.hxx>
#if SUPD != 310
#include <oox/ole/olestorage.hxx>
#include <oox/ole/vbaproject.hxx>
#include <oox/helper/graphichelper.hxx>
#endif	// SUPD != 310
using namespace ::rtl;
using namespace ::com::sun::star;
using ::comphelper::MediaDescriptor;

/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
sal_Bool WriterFilter::filter( const uno::Sequence< beans::PropertyValue >& aDescriptor )
   throw (uno::RuntimeException)
{
    if( m_xSrcDoc.is() )
    {
        uno::Reference< lang::XMultiServiceFactory > xMSF(m_xContext->getServiceManager(), uno::UNO_QUERY_THROW);
        uno::Reference< uno::XInterface > xIfc( xMSF->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.comp.Writer.DocxExport" ))), uno::UNO_QUERY_THROW);
        if (!xIfc.is())
            return sal_False;
        uno::Reference< document::XExporter > xExprtr(xIfc, uno::UNO_QUERY_THROW);
        uno::Reference< document::XFilter > xFltr(xIfc, uno::UNO_QUERY_THROW);
        if (!xExprtr.is() || !xFltr.is())
            return sal_False;
        xExprtr->setSourceDocument(m_xSrcDoc);
        return xFltr->filter(aDescriptor);
    }
    else if (m_xDstDoc.is())
    {
#if SUPD == 310
        sal_Int32 nLength = aDescriptor.getLength();
        const beans::PropertyValue * pValue = aDescriptor.getConstArray();
        uno::Reference < io::XInputStream > xInputStream;
        ::rtl::OUString sFilterName;
        for ( sal_Int32 i = 0 ; i < nLength; i++)
        {
            if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "InputStream" ) ) )
                pValue[i].Value >>= xInputStream;
            else if( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "FilterName" ) ) )
                pValue[i].Value >>= sFilterName;
        }
#else	// SUPD == 310
        MediaDescriptor aMediaDesc( aDescriptor );
        OUString sFilterName = aMediaDesc.getUnpackedValueOrDefault( MediaDescriptor::PROP_FILTERNAME(), OUString() );

        uno::Reference< io::XInputStream > xInputStream;
        try
        {
            // use the oox.core.FilterDetect implementation to extract the decrypted ZIP package
            uno::Reference< lang::XMultiServiceFactory > xFactory( m_xContext->getServiceManager(), uno::UNO_QUERY_THROW );
            ::oox::core::FilterDetect aDetector( xFactory );
            xInputStream = aDetector.extractUnencryptedPackage( aMediaDesc );
        }
        catch( uno::Exception& )
        {
        }
#endif	// SUPD == 310

        if ( !xInputStream.is() )
        {
            return sal_False;
        }

#ifdef DEBUG_ELEMENT
        OUString sURL = aMediaDesc.getUnpackedValueOrDefault( MediaDescriptor::PROP_URL(), OUString() );
        ::std::string sURLc = OUStringToOString(sURL, RTL_TEXTENCODING_ASCII_US).getStr();
        
        writerfilter::TagLogger::Pointer_t debugLogger
        (writerfilter::TagLogger::getInstance("DEBUG"));
        debugLogger->setFileName(sURLc);
        debugLogger->startDocument();
        
        writerfilter::TagLogger::Pointer_t dmapperLogger
        (writerfilter::TagLogger::getInstance("DOMAINMAPPER"));
        dmapperLogger->setFileName(sURLc);
        dmapperLogger->startDocument();
#endif

    writerfilter::dmapper::SourceDocumentType eType =
        (m_sFilterName.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "writer_MS_Word_2007" ) ) ||
         m_sFilterName.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "writer_MS_Word_2007_Template" ) ) ||
         m_sFilterName.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "writer_OOXML" ) ) ||
         m_sFilterName.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "writer_OOXML_Text_Template" ) )) ?
                writerfilter::dmapper::DOCUMENT_OOXML : writerfilter::dmapper::DOCUMENT_DOC;
    writerfilter::Stream::Pointer_t pStream(new writerfilter::dmapper::DomainMapper(m_xContext, xInputStream, m_xDstDoc, eType));
    //create the tokenizer and domain mapper
    if( eType == writerfilter::dmapper::DOCUMENT_OOXML )
    {
        writerfilter::ooxml::OOXMLStream::Pointer_t pDocStream = writerfilter::ooxml::OOXMLDocumentFactory::createStream(m_xContext, xInputStream);
        writerfilter::ooxml::OOXMLDocument::Pointer_t pDocument(writerfilter::ooxml::OOXMLDocumentFactory::createDocument(pDocStream));

        uno::Reference<frame::XModel> xModel(m_xDstDoc, uno::UNO_QUERY_THROW);
        pDocument->setModel(xModel);

        uno::Reference<drawing::XDrawPageSupplier> xDrawings
            (m_xDstDoc, uno::UNO_QUERY_THROW);
        uno::Reference<drawing::XDrawPage> xDrawPage
            (xDrawings->getDrawPage(), uno::UNO_SET_THROW);
        pDocument->setDrawPage(xDrawPage);

        pDocument->resolve(*pStream);
#if SUPD != 310
        writerfilter::ooxml::OOXMLStream::Pointer_t  pVBAProjectStream(writerfilter::ooxml::OOXMLDocumentFactory::createStream( pDocStream, writerfilter::ooxml::OOXMLStream::VBAPROJECT ));
        oox::StorageRef xVbaPrjStrg( new ::oox::ole::OleStorage( uno::Reference< lang::XMultiServiceFactory >( m_xContext->getServiceManager(), uno::UNO_QUERY_THROW ), pVBAProjectStream->getDocumentStream(), false ) );
        if( xVbaPrjStrg.get() && xVbaPrjStrg->isStorage() )
        {
            ::oox::ole::VbaProject aVbaProject( uno::Reference< lang::XMultiServiceFactory >( m_xContext->getServiceManager(), uno::UNO_QUERY_THROW ), xModel, rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Writer" ) ) );
            uno::Reference< frame::XFrame > xFrame = aMediaDesc.getUnpackedValueOrDefault(  MediaDescriptor::PROP_FRAME(), uno::Reference< frame::XFrame > () );

            // if no XFrame try fallback to what we can glean from the Model
            if ( !xFrame.is() )
            {
                uno::Reference< frame::XController > xController =  xModel->getCurrentController();
                xFrame =  xController.is() ? xController->getFrame() : NULL;
            }

            oox::GraphicHelper gHelper(  uno::Reference< lang::XMultiServiceFactory >( m_xContext->getServiceManager(), uno::UNO_QUERY_THROW ), xFrame, xVbaPrjStrg );
            aVbaProject.importVbaProject( *xVbaPrjStrg, gHelper );
        }
#endif	// SUPD != 310
    }
    else
    {
        writerfilter::doctok::WW8Stream::Pointer_t pDocStream = writerfilter::doctok::WW8DocumentFactory::createStream(m_xContext, xInputStream);
        writerfilter::doctok::WW8Document::Pointer_t pDocument(writerfilter::doctok::WW8DocumentFactory::createDocument(pDocStream));

        pDocument->resolve(*pStream);
    }

#ifdef DEBUG_ELEMENT
    writerfilter::TagLogger::dump("DOMAINMAPPER");
    dmapperLogger->endDocument();
    writerfilter::TagLogger::dump("DEBUG");
    debugLogger->endDocument();
#endif

    return sal_True;
    }
    return sal_False;
}
/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
void WriterFilter::cancel(  ) throw (uno::RuntimeException)
{
}

/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
void WriterFilter::setTargetDocument( const uno::Reference< lang::XComponent >& xDoc )
   throw (lang::IllegalArgumentException, uno::RuntimeException)
{
   m_xDstDoc = xDoc;
}

void WriterFilter::setSourceDocument( const uno::Reference< lang::XComponent >& xDoc )
   throw (lang::IllegalArgumentException, uno::RuntimeException)
{
   m_xSrcDoc = xDoc;
}

/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
void WriterFilter::initialize( const uno::Sequence< uno::Any >& aArguments ) throw (uno::Exception, uno::RuntimeException)
{
   uno::Sequence < beans::PropertyValue > aAnySeq;
   sal_Int32 nLength = aArguments.getLength();
   if ( nLength && ( aArguments[0] >>= aAnySeq ) )
   {
       const beans::PropertyValue * pValue = aAnySeq.getConstArray();
       nLength = aAnySeq.getLength();
       for ( sal_Int32 i = 0 ; i < nLength; i++)
       {
           if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "Type" ) ) )
           {
               pValue[i].Value >>= m_sFilterName;
               break;
           }
       }
   }
}
/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
OUString WriterFilter_getImplementationName () throw (uno::RuntimeException)
{
   return OUString ( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.comp.Writer.WriterFilter" ) );
}

#define SERVICE_NAME1 "com.sun.star.document.ImportFilter"
#define SERVICE_NAME2 "com.sun.star.document.ExportFilter"
/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
sal_Bool WriterFilter_supportsService( const OUString& ServiceName ) throw (uno::RuntimeException)
{
   return (ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME1 ) ) ||
           ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME1 ) ));
}
/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
uno::Sequence< OUString > WriterFilter_getSupportedServiceNames(  ) throw (uno::RuntimeException)
{
   uno::Sequence < OUString > aRet(2);
   OUString* pArray = aRet.getArray();
   pArray[0] =  OUString ( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME1 ) );
   pArray[1] =  OUString ( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME2 ) );
   return aRet;
}
#undef SERVICE_NAME1
#undef SERVICE_NAME2

/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
uno::Reference< uno::XInterface > WriterFilter_createInstance( const uno::Reference< uno::XComponentContext >& xContext)
                throw( uno::Exception )
{
   return (cppu::OWeakObject*) new WriterFilter( xContext );
}

/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
OUString WriterFilter::getImplementationName(  ) throw (uno::RuntimeException)
{
   return WriterFilter_getImplementationName();
}
/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
sal_Bool WriterFilter::supportsService( const OUString& rServiceName ) throw (uno::RuntimeException)
{
    return WriterFilter_supportsService( rServiceName );
}
/*-- 09.06.2006 10:15:20---------------------------------------------------

  -----------------------------------------------------------------------*/
uno::Sequence< OUString > WriterFilter::getSupportedServiceNames(  ) throw (uno::RuntimeException)
{
    return WriterFilter_getSupportedServiceNames();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
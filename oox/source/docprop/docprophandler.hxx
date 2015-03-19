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

#ifndef INCLUDED_OOX_SOURCE_DOCPROP_DOCPROPHANDLER_HXX
#define INCLUDED_OOX_SOURCE_DOCPROP_DOCPROPHANDLER_HXX

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/xml/sax/XFastDocumentHandler.hpp>

#include <cppuhelper/implbase1.hxx>

#include "oox/token/namespaces.hxx"
#include "oox/token/tokens.hxx"

#if SUPD == 310
#include <oox/dllapi.h>
#endif	// SUPD == 310

namespace oox {
namespace docprop {

#define COREPR_TOKEN( token )   (::oox::NMSP_packageMetaCorePr | XML_##token)
#define CUSTPR_TOKEN( token )   (::oox::NMSP_officeCustomPr | XML_##token)
#define EXTPR_TOKEN( token )    (::oox::NMSP_officeExtPr | XML_##token)
#define VT_TOKEN( token )       (::oox::NMSP_officeDocPropsVT | XML_##token)
#define DC_TOKEN( token )       (::oox::NMSP_dc | XML_##token)
#define DCT_TOKEN( token )      (::oox::NMSP_dcTerms | XML_##token)

class OOXMLDocPropHandler : public ::cppu::WeakImplHelper1< ::com::sun::star::xml::sax::XFastDocumentHandler >
{
    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > m_xContext;
    ::com::sun::star::uno::Reference< ::com::sun::star::document::XDocumentProperties > m_xDocProp;

    sal_Int32 m_nState;
    sal_Int32 m_nBlock;
    sal_Int32 m_nType;

    sal_Int32 m_nInBlock;

    OUString m_aCustomPropertyName;

public:
    explicit            OOXMLDocPropHandler( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext >& xContext, const ::com::sun::star::uno::Reference< ::com::sun::star::document::XDocumentProperties > xDocProp );

    virtual             ~OOXMLDocPropHandler();

    void InitNew();
    void AddCustomProperty( const ::com::sun::star::uno::Any& aAny );

    ::com::sun::star::util::DateTime GetDateTimeFromW3CDTF( const OUString& aChars );
    ::com::sun::star::uno::Sequence< OUString > GetKeywordsSet( const OUString& aChars );
#if SUPD == 310
    ::com::sun::star::lang::Locale GetLanguage( const OUString& aChars );
#endif	// SUPD == 310
    void UpdateDocStatistic( const OUString& aChars );

    // com.sun.star.xml.sax.XFastDocumentHandler

#if SUPD == 310
    virtual void SAL_CALL startDocument() throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual void SAL_CALL endDocument() throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual void SAL_CALL setDocumentLocator( const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XLocator >& rxLocator ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
    virtual void SAL_CALL startDocument() throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL endDocument() throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL setDocumentLocator( const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XLocator >& rxLocator ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310

    // com.sun.star.xml.sax.XFastContextHandler

#if SUPD == 310
    virtual void SAL_CALL startFastElement( ::sal_Int32 Element, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual void SAL_CALL startUnknownElement( const OUString& Namespace, const OUString& Name, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual void SAL_CALL endFastElement( ::sal_Int32 Element ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual void SAL_CALL endUnknownElement( const OUString& Namespace, const OUString& Name ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext( ::sal_Int32 Element, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastContextHandler > SAL_CALL createUnknownChildContext( const OUString& Namespace, const OUString& Name, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
    virtual void SAL_CALL characters( const OUString& aChars ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
    virtual void SAL_CALL startFastElement( ::sal_Int32 Element, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL startUnknownElement( const OUString& Namespace, const OUString& Name, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL endFastElement( ::sal_Int32 Element ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL endUnknownElement( const OUString& Namespace, const OUString& Name ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext( ::sal_Int32 Element, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastContextHandler > SAL_CALL createUnknownChildContext( const OUString& Namespace, const OUString& Name, const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& Attribs ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL characters( const OUString& aChars ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310
    virtual void SAL_CALL ignorableWhitespace( const OUString& aWhitespaces ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL processingInstruction( const OUString& aTarget, const OUString& aData ) throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException);

};



} // namespace core
} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
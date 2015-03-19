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

// FIXME? This file is nearly identical to xmloff/source/chart/ColorPropertySet.cxx

#include "ColorPropertySet.hxx"

#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/drawing/FillStyle.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::RuntimeException;



namespace
{
class lcl_ColorPropertySetInfo : public ::cppu::WeakImplHelper1<
        XPropertySetInfo  >
{
public:
    lcl_ColorPropertySetInfo( bool bFillColor );

protected:
    // ____ XPropertySetInfo ____
#if SUPD == 310
    virtual Sequence< Property > SAL_CALL getProperties()                throw (RuntimeException) SAL_OVERRIDE;
    virtual Property SAL_CALL getPropertyByName( const OUString& aName ) throw (UnknownPropertyException, RuntimeException) SAL_OVERRIDE;
    virtual sal_Bool SAL_CALL hasPropertyByName( const OUString& Name )  throw (RuntimeException) SAL_OVERRIDE;
#else	// SUPD == 310
    virtual Sequence< Property > SAL_CALL getProperties()                throw (RuntimeException, std::exception) SAL_OVERRIDE;
    virtual Property SAL_CALL getPropertyByName( const OUString& aName ) throw (UnknownPropertyException, RuntimeException, std::exception) SAL_OVERRIDE;
    virtual sal_Bool SAL_CALL hasPropertyByName( const OUString& Name )  throw (RuntimeException, std::exception) SAL_OVERRIDE;
#endif	// SUPD == 310

private:
    OUString m_aColorPropName;
    Property m_aColorProp;
};

lcl_ColorPropertySetInfo::lcl_ColorPropertySetInfo( bool bFillColor ) :
        // note: length of FillColor and LineColor is 9
        m_aColorPropName( (bFillColor ? "FillColor" : "LineColor"), 9, RTL_TEXTENCODING_ASCII_US ),
        m_aColorProp( m_aColorPropName, -1,
                      ::getCppuType( reinterpret_cast< const sal_Int32 * >(0)), 0)
{}

Sequence< Property > SAL_CALL lcl_ColorPropertySetInfo::getProperties()
#if SUPD == 310
    throw (RuntimeException)
#else	// SUPD == 310
    throw (RuntimeException, std::exception)
#endif	// SUPD == 310
{

    return Sequence< Property >( & m_aColorProp, 1 );
}

Property SAL_CALL lcl_ColorPropertySetInfo::getPropertyByName( const OUString& aName )
#if SUPD == 310
    throw (UnknownPropertyException, RuntimeException)
#else	// SUPD == 310
    throw (UnknownPropertyException, RuntimeException, std::exception)
#endif	// SUPD == 310
{
    if( aName.equals( m_aColorPropName ))
        return m_aColorProp;
    throw UnknownPropertyException( m_aColorPropName, static_cast< uno::XWeak * >( this ));
}

sal_Bool SAL_CALL lcl_ColorPropertySetInfo::hasPropertyByName( const OUString& Name )
#if SUPD == 310
    throw (RuntimeException)
#else	// SUPD == 310
    throw (RuntimeException, std::exception)
#endif	// SUPD == 310
{
    return Name.equals( m_aColorPropName );
}

} // anonymous namespace



namespace oox
{
namespace drawingml
{

ColorPropertySet::ColorPropertySet( sal_Int32 nColor, bool bFillColor /* = true */ ) :
        // note: length of FillColor and LineColor is 9
        m_aColorPropName( (bFillColor ? "FillColor" : "LineColor"), 9, RTL_TEXTENCODING_ASCII_US ),
        m_nColor( nColor ),
        m_bIsFillColor( bFillColor ),
        m_nDefaultColor( 0x0099ccff )  // blue 8
{}

ColorPropertySet::~ColorPropertySet()
{}

// ____ XPropertySet ____

Reference< XPropertySetInfo > SAL_CALL ColorPropertySet::getPropertySetInfo()
#if SUPD == 310
    throw (uno::RuntimeException)
#else	// SUPD == 310
    throw (uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    if( ! m_xInfo.is())
        m_xInfo.set( new lcl_ColorPropertySetInfo( m_bIsFillColor ));

    return m_xInfo;
}

void SAL_CALL ColorPropertySet::setPropertyValue( const OUString& /* aPropertyName */, const uno::Any& aValue )
    throw (UnknownPropertyException,
           PropertyVetoException,
           lang::IllegalArgumentException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    aValue >>= m_nColor;
}

uno::Any SAL_CALL ColorPropertySet::getPropertyValue( const OUString& aPropertyName )
    throw (UnknownPropertyException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    if( aPropertyName == "FillStyle" && m_bIsFillColor )
    {
        ::com::sun::star::drawing::FillStyle aFillStyle = ::com::sun::star::drawing::FillStyle_SOLID;
        return uno::makeAny(aFillStyle);
    }
    return uno::makeAny( m_nColor );
}

void SAL_CALL ColorPropertySet::addPropertyChangeListener( const OUString& /* aPropertyName */, const Reference< XPropertyChangeListener >& /* xListener */ )
    throw (UnknownPropertyException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    OSL_FAIL( "Not Implemented" );
    return;
}

void SAL_CALL ColorPropertySet::removePropertyChangeListener( const OUString& /* aPropertyName */, const Reference< XPropertyChangeListener >& /* aListener */ )
    throw (UnknownPropertyException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    OSL_FAIL( "Not Implemented" );
    return;
}

void SAL_CALL ColorPropertySet::addVetoableChangeListener( const OUString& /* PropertyName */, const Reference< XVetoableChangeListener >& /* aListener */ )
    throw (UnknownPropertyException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    OSL_FAIL( "Not Implemented" );
    return;
}

void SAL_CALL ColorPropertySet::removeVetoableChangeListener( const OUString& /* PropertyName */, const Reference< XVetoableChangeListener >& /* aListener */ )
    throw (UnknownPropertyException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    OSL_FAIL( "Not Implemented" );
    return;
}

// ____ XPropertyState ____

PropertyState SAL_CALL ColorPropertySet::getPropertyState( const OUString& /* PropertyName */ )
    throw (UnknownPropertyException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    return PropertyState_DIRECT_VALUE;
}

Sequence< PropertyState > SAL_CALL ColorPropertySet::getPropertyStates( const Sequence< OUString >& /* aPropertyName */ )
    throw (UnknownPropertyException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    PropertyState aState = PropertyState_DIRECT_VALUE;
    return Sequence< PropertyState >( & aState, 1 );
}

void SAL_CALL ColorPropertySet::setPropertyToDefault( const OUString& PropertyName )
    throw (UnknownPropertyException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    if( PropertyName.equals( m_aColorPropName ))
        m_nColor = m_nDefaultColor;
}

uno::Any SAL_CALL ColorPropertySet::getPropertyDefault( const OUString& aPropertyName )
    throw (UnknownPropertyException,
           lang::WrappedTargetException,
#if SUPD == 310
           uno::RuntimeException)
#else	// SUPD == 310
           uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    if( aPropertyName.equals( m_aColorPropName ))
        return uno::makeAny( m_nDefaultColor );
    return uno::Any();
}

} //  namespace chart
} //  namespace xmloff

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
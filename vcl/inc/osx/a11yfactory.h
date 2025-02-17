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

#ifndef INCLUDED_VCL_INC_OSX_A11YFACTORY_H
#define INCLUDED_VCL_INC_OSX_A11YFACTORY_H

#include "osxvcltypes.h"
#include "a11ywrapper.h"
#include <com/sun/star/accessibility/XAccessibleContext.hpp>

#ifdef USE_JAVA
#include <vcl/a11y.h>
#endif	// USE_JAVA

@interface AquaA11yFactory : NSObject
{
}
#ifdef USE_JAVA
+(void)insertIntoWrapperRepository: (id<NSAccessibility>) viewElement forAccessibleContext: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessibleContext >) rxAccessibleContext;
#else	// USE_JAVA
+(void)insertIntoWrapperRepository: (NSView *) viewElement forAccessibleContext: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessibleContext >) rxAccessibleContext;
#endif	// USE_JAVA
+(AquaA11yWrapper *)wrapperForAccessible: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessible >) rxAccessible;
+(AquaA11yWrapper *)wrapperForAccessibleContext: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessibleContext >) rxAccessibleContext;
+(AquaA11yWrapper *)wrapperForAccessibleContext: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessibleContext >) rxAccessibleContext createIfNotExists:(BOOL) bCreate;
+(AquaA11yWrapper *)wrapperForAccessibleContext: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessibleContext >) rxAccessibleContext createIfNotExists:(BOOL) bCreate asRadioGroup:(BOOL) asRadioGroup;
+(void)removeFromWrapperRepositoryFor: (::com::sun::star::uno::Reference < ::com::sun::star::accessibility::XAccessibleContext >) rxAccessibleContext;
#ifdef USE_JAVA
+(void)removeFromWrapperRepositoryForWrapper: (AquaA11yWrapper *) theWrapper;
+(void)registerView: (id<NSAccessibility>) theView;
+(void)revokeView: (id<NSAccessibility>) theView;
#else	// USE_JAVA
+(void)registerView: (NSView *) theView;
+(void)revokeView: (NSView *) theViewt;
#endif	// USE_JAVA
@end

#ifdef USE_JAVA

#ifdef USE_ONLY_MAIN_THREAD_TO_CREATE_AQUAA11YWRAPPERS

@interface AquaA11yWrapperForAccessibleContext : NSObject
{
    ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessibleContext > mxAccessibleContext;
    AquaA11yWrapper*        mpWrapper;
}
+ (id)createWithAccessibleContext:(::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessibleContext >)xAccessibleContext;
- (id)initWithAccessibleContext:(::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessibleContext >)xAccessibleContext;
- (void)dealloc;
- (void)wrapperForAccessibleContext:(id)pObject;
- (AquaA11yWrapper *)wrapper;
@end

#endif // USE_ONLY_MAIN_THREAD_TO_CREATE_AQUAA11YWRAPPERS

@interface AquaA11yRemoveFromWrapperRepository : NSObject
{
    AquaA11yWrapper*        mpElement;
}
+ (id)addElementToPendingRemovalQueue:(AquaA11yWrapper *)pElement;
- (id)initWithElement:(AquaA11yWrapper *)pElement;
- (void)dealloc;
+ (void)removeFromWrapperRepository;
@end

@interface AquaA11yPostNotification : NSObject
{
    id                      mpElement;
    NSAccessibilityNotificationName mpName;
}
+ (id)addElementToPendingNotificationQueue:(id)pElement name:(NSAccessibilityNotificationName)pName;
- (id)initWithElement:(id)pElement name:(NSAccessibilityNotificationName)pName;
- (void)dealloc;
- (void)postNotification;
+ (void)postPendingNotifications;
@end

@interface AquaA11yDoAction : AquaA11yPostNotification
{
    NSAccessibilityActionName mpAction;
}
+ (id)addElementToPendingNotificationQueue:(id)pElement action:(NSAccessibilityActionName)pAction;
- (id)initWithElement:(id)pElement action:(NSAccessibilityActionName)pAction;
- (void)dealloc;
- (void)postNotification;
@end

@interface AquaA11ySetValue : AquaA11yPostNotification
{
    id mpValue;
    NSAccessibilityAttributeName mpAttribute;
}
+ (id)addElementToPendingNotificationQueue:(id)pElement value:(id)pValue attribute:(NSAccessibilityAttributeName)pAttribute;
- (id)initWithElement:(id)pElement value:(id)pValue attribute:(NSAccessibilityAttributeName)pAttribute;
- (void)dealloc;
- (void)postNotification;
@end

#endif	// USE_JAVA

#endif // INCLUDED_VCL_INC_OSX_A11YFACTORY_H

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

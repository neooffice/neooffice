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
 * Modified October 2015 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_i18npool.hxx"

#include <inputsequencechecker.hxx>
#include <com/sun/star/i18n/InputSequenceCheckMode.hpp>
#include <com/sun/star/i18n/UnicodeType.hpp>
#include <i18nutil/unicode.hxx>
#include <rtl/ustrbuf.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::rtl;

namespace com { namespace sun { namespace star { namespace i18n {

InputSequenceCheckerImpl::InputSequenceCheckerImpl( const Reference < XMultiServiceFactory >& rxMSF ) : xMSF( rxMSF )
{
        serviceName = "com.sun.star.i18n.InputSequenceCheckerImpl";
        cachedItem = NULL;
}

InputSequenceCheckerImpl::InputSequenceCheckerImpl()
{
}

InputSequenceCheckerImpl::~InputSequenceCheckerImpl()
{
        // Clear lookuptable
        for (size_t l = 0; l < lookupTable.size(); l++)
            delete lookupTable[l];

        lookupTable.clear();
}

sal_Bool SAL_CALL 
InputSequenceCheckerImpl::checkInputSequence(const OUString& Text, sal_Int32 nStartPos,
        sal_Unicode inputChar, sal_Int16 inputCheckMode) throw(RuntimeException)
{
        if (inputCheckMode == InputSequenceCheckMode::PASSTHROUGH)
            return sal_True;

        sal_Char* language = getLanguageByScripType(Text[nStartPos], inputChar);

        if (language)
            return getInputSequenceChecker(language)->checkInputSequence(Text, nStartPos, inputChar, inputCheckMode);
        else
            return sal_True; // not a checkable languages.
}

sal_Int32 SAL_CALL 
InputSequenceCheckerImpl::correctInputSequence(OUString& Text, sal_Int32 nStartPos,
        sal_Unicode inputChar, sal_Int16 inputCheckMode) throw(RuntimeException)
{
        if (inputCheckMode != InputSequenceCheckMode::PASSTHROUGH) {
            sal_Char* language = getLanguageByScripType(Text[nStartPos], inputChar);

            if (language)
                return getInputSequenceChecker(language)->correctInputSequence(Text, nStartPos, inputChar, inputCheckMode);
        }
        Text = Text.replaceAt(++nStartPos, 0, OUString(inputChar));
        return nStartPos;
}

static ScriptTypeList typeList[] = {
        //{ UnicodeScript_kHebrew,              UnicodeScript_kHebrew },        // 10,
        //{ UnicodeScript_kArabic,              UnicodeScript_kArabic },        // 11,
        { UnicodeScript_kDevanagari,UnicodeScript_kDevanagari,          UnicodeScript_kDevanagari },    // 14,
        { UnicodeScript_kThai,  UnicodeScript_kThai,                  UnicodeScript_kThai },          // 24,

        { UnicodeScript_kScriptCount,   UnicodeScript_kScriptCount,           UnicodeScript_kScriptCount }    // 88
};

sal_Char* SAL_CALL
InputSequenceCheckerImpl::getLanguageByScripType(sal_Unicode cChar, sal_Unicode nChar)
{
    sal_Int16 type = unicode::getUnicodeScriptType( cChar, typeList, UnicodeScript_kScriptCount );

    if (type != UnicodeScript_kScriptCount && 
            type == unicode::getUnicodeScriptType( nChar, typeList, UnicodeScript_kScriptCount )) {
        switch(type) {
            case UnicodeScript_kThai:           return (sal_Char*)"th";
            //case UnicodeScript_kArabic:       return (sal_Char*)"ar";
            //case UnicodeScript_kHebrew:       return (sal_Char*)"he";
#ifdef USE_JAVA
            // Disable sequence checking for Devanagari as the implementation
            // is broken. For example, typing valid Hindi words (see bug 1429)
            // using the "Devanagari - QWERTY" keyboard will fail.
#else	// USE_JAVA
            case UnicodeScript_kDevanagari:   return (sal_Char*)"hi";
#endif	// USE_JAVA
        }
    }
    return NULL;
}

Reference< XExtendedInputSequenceChecker >& SAL_CALL
InputSequenceCheckerImpl::getInputSequenceChecker(sal_Char* rLanguage) throw (RuntimeException)
{
        if (cachedItem && cachedItem->aLanguage == rLanguage) {
            return cachedItem->xISC;
        }
        else if (xMSF.is()) {
            for (size_t l = 0; l < lookupTable.size(); l++) {
                cachedItem = lookupTable[l];
                if (cachedItem->aLanguage == rLanguage)
                    return cachedItem->xISC;
            }

            Reference < uno::XInterface > xI = xMSF->createInstance(
                        OUString::createFromAscii("com.sun.star.i18n.InputSequenceChecker_") + 
                        OUString::createFromAscii(rLanguage));

            if ( xI.is() ) {
                Reference< XExtendedInputSequenceChecker > xISC;
                xI->queryInterface( getCppuType((const Reference< XExtendedInputSequenceChecker>*)0) ) >>= xISC;
                if (xISC.is()) {
                    lookupTable.push_back(cachedItem = new lookupTableItem(rLanguage, xISC));
                    return cachedItem->xISC;
                }
            }
        }
        throw RuntimeException();
}

OUString SAL_CALL
InputSequenceCheckerImpl::getImplementationName(void) throw( RuntimeException )
{
        return OUString::createFromAscii(serviceName);
}

sal_Bool SAL_CALL
InputSequenceCheckerImpl::supportsService(const OUString& rServiceName) throw( RuntimeException )
{
        return !rServiceName.compareToAscii(serviceName);
}

Sequence< OUString > SAL_CALL
InputSequenceCheckerImpl::getSupportedServiceNames(void) throw( RuntimeException )
{
        Sequence< OUString > aRet(1);
        aRet[0] = OUString::createFromAscii(serviceName);
        return aRet;
}

} } } }
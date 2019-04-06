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

#include <shellio.hxx>
#include <doc.hxx>
#include <node.hxx>

#include <cmdid.h>


SwDocFac::SwDocFac( SwDoc *pDc )
#ifdef NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
    : pDoc( pDc )
#else	// NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
    : mxDoc( pDc )
#endif	// NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
{
#ifdef NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
    if( pDoc )
        pDoc->acquire();
#endif	// NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
}


SwDocFac::~SwDocFac()
{
#ifdef NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
    if( pDoc && !pDoc->release() )
        delete pDoc;
#endif	// NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
}


SwDoc *SwDocFac::GetDoc()
{
#ifdef NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
    if( !pDoc )
    {
        pDoc = new SwDoc;
        pDoc->acquire();
    }
    return pDoc;
#else	// NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
    if( !mxDoc.is() )
    {
        mxDoc = new SwDoc;
    }
    return mxDoc.get();
#endif	// NO_LIBO_SWDOC_ACQUIRE_LEAK_FIX
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
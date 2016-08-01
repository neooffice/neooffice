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
 ************************************************************************/

#ifndef _WEBDAV_UCP_PROPERTYMAP_HXX
#define _WEBDAV_UCP_PROPERTYMAP_HXX

#include <hash_set>
#include <com/sun/star/beans/Property.hpp>

namespace webdav_ucp {

//=========================================================================

struct equalPropertyName
{
  bool operator()( const ::com::sun::star::beans::Property & p1,
  				   const ::com::sun::star::beans::Property & p2 ) const
  {
		return !!( p1.Name == p2.Name );
  }
};

struct hashPropertyName
{
	size_t operator()( const ::com::sun::star::beans::Property & p ) const
	{
		return p.Name.hashCode();
	}
};

typedef std::hash_set
<
	::com::sun::star::beans::Property,
	hashPropertyName,
	equalPropertyName
>
PropertyMap;

}

#endif
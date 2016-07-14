/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *		 - GNU General Public License Version 2.1
 *
 *  Patrick Luby, February 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

#ifndef _SV_SALSYS_H
#define _SV_SALSYS_H

#include <vcl/sv.h>

#include "salsys.hxx"

// -----------------
// - JavaSalSystem -
// -----------------

class JavaSalSystem : public SalSystem
{
public:
							JavaSalSystem();
	virtual					~JavaSalSystem();

	virtual unsigned int	GetDisplayScreenCount();
	virtual bool			IsMultiDisplay();
	virtual unsigned int	GetDefaultDisplayNumber();
	virtual Rectangle		GetDisplayScreenPosSizePixel( unsigned int nScreen );
	virtual Rectangle		GetDisplayWorkAreaPosSizePixel( unsigned int nScreen );
	virtual rtl::OUString	GetScreenName( unsigned int nScreen );
	virtual int				ShowNativeMessageBox( const String& rTitle, const String& rMessage, int nButtonCombination, int nDefaultButton );
};

#endif // _SV_SALSYS_H
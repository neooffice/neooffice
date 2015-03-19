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

#ifndef SVX_BORDERLINE_HXX
#define SVX_BORDERLINE_HXX

#include <tools/color.hxx>
#include <svtools/poolitem.hxx>
#include "svx/svxdllapi.h"
#if SUPD == 310
#include <svx/editengdllapi.h>
#endif	// SUPD == 310

// Line defaults in twips (former Writer defaults):

#define DEF_LINE_WIDTH_0        1
#define DEF_LINE_WIDTH_1        20
#define DEF_LINE_WIDTH_2        50
#define DEF_LINE_WIDTH_3        80
#define DEF_LINE_WIDTH_4        100
#define DEF_LINE_WIDTH_5        10

#define DEF_MAX_LINE_WIDHT      DEF_LINE_WIDTH_4
#define DEF_MAX_LINE_DIST       DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE0_OUT    DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE0_IN     DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE0_DIST   DEF_LINE_WIDTH_1

#define DEF_DOUBLE_LINE1_OUT    DEF_LINE_WIDTH_1
#define DEF_DOUBLE_LINE1_IN     DEF_LINE_WIDTH_1
#define DEF_DOUBLE_LINE1_DIST   DEF_LINE_WIDTH_1

#define DEF_DOUBLE_LINE2_OUT    DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE2_IN     DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE2_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE3_OUT    DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE3_IN     DEF_LINE_WIDTH_1
#define DEF_DOUBLE_LINE3_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE4_OUT    DEF_LINE_WIDTH_1
#define DEF_DOUBLE_LINE4_IN     DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE4_DIST   DEF_LINE_WIDTH_1

#define DEF_DOUBLE_LINE5_OUT    DEF_LINE_WIDTH_3
#define DEF_DOUBLE_LINE5_IN     DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE5_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE6_OUT    DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE6_IN     DEF_LINE_WIDTH_3
#define DEF_DOUBLE_LINE6_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE7_OUT    DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE7_IN     DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE7_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE8_OUT    DEF_LINE_WIDTH_1
#define DEF_DOUBLE_LINE8_IN     DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE8_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE9_OUT    DEF_LINE_WIDTH_2
#define DEF_DOUBLE_LINE9_IN     DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE9_DIST   DEF_LINE_WIDTH_2

#define DEF_DOUBLE_LINE10_OUT	DEF_LINE_WIDTH_3
#define DEF_DOUBLE_LINE10_IN	DEF_LINE_WIDTH_0
#define DEF_DOUBLE_LINE10_DIST	DEF_LINE_WIDTH_2

// ============================================================================

#if SUPD == 310
namespace editeng {

    // values from ::com::sun::star::table::BorderLineStyle
    typedef sal_Int16 SvxBorderStyle;

    // convert border style between Word formats and LO
    SvxBorderStyle EDITENG_DLLPUBLIC ConvertBorderStyleFromWord(int);
    /// convert border width in twips between Word formats and LO
    double EDITENG_DLLPUBLIC ConvertBorderWidthToWord(SvxBorderStyle, double);
    double EDITENG_DLLPUBLIC ConvertBorderWidthFromWord(SvxBorderStyle,
            double, int);
}
#endif	// SUPD == 310

class SVX_DLLPUBLIC SvxBorderLine
{
protected:
	Color  aColor;
	USHORT nOutWidth;
	USHORT nInWidth;
	USHORT nDistance;

public:
	SvxBorderLine( const Color *pCol = 0, USHORT nOut = 0, USHORT nIn = 0, USHORT nDist = 0 );
	SvxBorderLine( const SvxBorderLine& r );

	SvxBorderLine& operator=( const SvxBorderLine& r );

	const Color&	GetColor() const { return aColor; }
	USHORT 			GetOutWidth() const { return nOutWidth; }
	USHORT 			GetInWidth() const { return nInWidth; }
	USHORT 			GetDistance() const { return nDistance; }

	void 			SetColor( const Color &rColor ) { aColor = rColor; }
	void			SetOutWidth( USHORT nNew ) { nOutWidth = nNew; }
	void			SetInWidth( USHORT nNew ) { nInWidth = nNew;  }
	void			SetDistance( USHORT nNew ) { nDistance = nNew; }
	void			ScaleMetrics( long nMult, long nDiv );

	BOOL			operator==( const SvxBorderLine &rCmp ) const;

    String          GetValueString( SfxMapUnit eSrcUnit, SfxMapUnit eDestUnit,
                                    const IntlWrapper* pIntl,
									BOOL bMetricStr = FALSE ) const;

	bool			HasPriority( const SvxBorderLine& rOtherLine ) const;

	bool isEmpty() const { return (0 == nOutWidth && 0 == nInWidth && 0 == nDistance); }
	bool isDouble() const { return (0 != nOutWidth && 0 != nInWidth); }
	sal_uInt16 getWidth() const { return nOutWidth + nInWidth + nDistance; }
};

// ============================================================================

#endif

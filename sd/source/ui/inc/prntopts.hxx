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
 * Modified May 2009 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#ifndef _SD_PRNTOPTS_HXX
#define _SD_PRNTOPTS_HXX

#include <vcl/group.hxx>

#ifndef _SV_BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#include <sfx2/tabdlg.hxx>
#include <vcl/fixed.hxx>

/*
enum PrintType
{
	PT_DRAWING,
	PT_NOTES,
	PT_HANDOUT,
	PT_OUTLINE
};
*/
class SdModule;
class SdPrintOptions : public SfxTabPage
{
 friend class SdOptionsDlg;
 friend class SdModule;

private:
    FixedLine           aGrpPrint;
    CheckBox            aCbxDraw;
	CheckBox			aCbxNotes;
	CheckBox			aCbxHandout;
	CheckBox			aCbxOutline;

    FixedLine           aSeparator1FL;
    FixedLine           aGrpOutput;
    RadioButton         aRbtColor;
	RadioButton			aRbtGrayscale;
	RadioButton			aRbtBlackWhite;

    FixedLine           aGrpPrintExt;
    CheckBox            aCbxPagename;
	CheckBox			aCbxDate;
	CheckBox			aCbxTime;
    CheckBox            aCbxHiddenPages;

    FixedLine           aSeparator2FL;
    FixedLine           aGrpPageoptions;
    RadioButton         aRbtDefault;
	RadioButton 		aRbtPagesize;
	RadioButton			aRbtPagetile;
	RadioButton			aRbtBooklet;
	CheckBox			aCbxFront;
	CheckBox			aCbxBack;

	CheckBox			aCbxPaperbin;

	const SfxItemSet&	rOutAttrs;

#ifdef USE_JAVA
	bool				bImpress;
#endif	// USE_JAVA

	DECL_LINK( ClickCheckboxHdl, CheckBox * );
	DECL_LINK( ClickBookletHdl, CheckBox * );

	void updateControls();

	using OutputDevice::SetDrawMode;
public:
            SdPrintOptions( Window* pParent, const SfxItemSet& rInAttrs);
			~SdPrintOptions();

	static	SfxTabPage* Create( Window*, const SfxItemSet& );
	static	USHORT* 	GetRanges();

	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

    void    SetDrawMode();
	virtual void		PageCreated (SfxAllItemSet aSet);
};



#endif // _SD_PRNTOPTS_HXX


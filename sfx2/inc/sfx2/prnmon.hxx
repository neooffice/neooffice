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
 * Modified November 2009 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/
#ifndef _SFX_PRNMON_HXX
#define _SFX_PRNMON_HXX

#include "sal/config.h"
#include "sfx2/dllapi.h"

#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _DIALOG_HXX //autogen
#include <vcl/dialog.hxx>
#endif
#include <sfx2/printer.hxx>
#include <sfx2/progress.hxx>

class SfxViewShell;
class SfxProgress;
struct SfxPrintProgress_Impl;

// ------------------------------------------------------------------------

#define PAGE_MAX    9999        //max. Anzahl der Seiten die gedruckt werden

//--------------------------------------------------------------------

class SFX2_DLLPUBLIC SfxPrintProgress: public SfxProgress
{
	SfxPrintProgress_Impl*  pImp;

private:
//#if 0 // _SOLAR__PRIVATE
	DECL_DLLPRIVATE_LINK( PrintErrorNotify, void * );
    DECL_DLLPRIVATE_LINK( StartPrintNotify, void * );
	DECL_DLLPRIVATE_LINK( EndPrintNotify, void * );
//#endif
public:
							SfxPrintProgress( SfxViewShell* pViewSh,
											  FASTBOOL bShow = TRUE );
	virtual                 ~SfxPrintProgress();

	virtual void            SetText( const String &rText );
			BOOL            SetStateText( ULONG nVal, const String &rVal, ULONG nNewRange = 0 );
	virtual BOOL            SetState( ULONG nVal, ULONG nNewRange = 0 );

	void                    RestoreOnEndPrint( SfxPrinter *pOldPrinter );
	void                    RestoreOnEndPrint( SfxPrinter *pOldPrinter,
											   BOOL bOldEnablePrintFile );
	void                    DeleteOnEndPrint();
	void					SetCancelHdl( const Link& aCancelHdl );
	BOOL					IsAborted() const;
};

// ------------------------------------------------------------------------

struct SfxPrintOptDlg_Impl;
class SfxPrintOptionsDialog : public ModalDialog
{
private:
	OKButton                aOkBtn;
	CancelButton            aCancelBtn;
	HelpButton				aHelpBtn;
	SfxPrintOptDlg_Impl*	pDlgImpl;
	SfxViewShell*           pViewSh;
	SfxItemSet*             pOptions;
	SfxTabPage*             pPage;
#if defined USE_JAVA && defined MACOSX
	bool					bShowPrintSetupDialog;
#endif	// USE_JAVA && MACOSX

public:
							SfxPrintOptionsDialog( Window *pParent,
												   SfxViewShell *pViewShell,
												   const SfxItemSet *rOptions );
	virtual                 ~SfxPrintOptionsDialog();

	BOOL                    Construct();
	virtual short           Execute();
	virtual long			Notify( NotifyEvent& rNEvt );

	SfxTabPage*             GetTabPage() const { return pPage; }
	const SfxItemSet&       GetOptions() const { return *pOptions; }
	void					DisableHelp();
};

#endif
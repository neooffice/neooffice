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
#ifndef _SD_PRINTDIALOG_HXX_
#define _SD_PRINTDIALOG_HXX_

#include <vcl/image.hxx>
#include <svtools/printdlg.hxx>
#include <boost/shared_ptr.hpp>
#include <vector>

class SdOptionsPrintItem;

class SdPrintDialog : public PrintDialog
{
 public:
	static SdPrintDialog* Create( Window* pWindow, bool bImpress );
	virtual ~SdPrintDialog();

	void Init( const SdOptionsPrintItem* pPrintOpts );
	bool Fill( SdOptionsPrintItem* pPrintOpts );

#if defined USE_JAVA && defined USE_PRINT_DIALOG_IN_PRINT_OPTIONS
	virtual short Execute();
#endif	// USE_JAVA && USE_PRINT_DIALOG_IN_PRINT_OPTIONS

private:
	SdPrintDialog( Window* pWindow, bool bImpress );

	void UpdateStates();
	void LoadPreviewImages();

	DECL_LINK( UpdateStatesHdl, void* );

	std::vector< boost::shared_ptr< Control > > mpControls;
	std::vector< boost::shared_ptr< Image > > mpPreviews;
	std::vector< boost::shared_ptr< Image > > mpPreviewsHC;
	bool mbImpress;
};

#endif // _SD_PRINTDIALOG_HXX_

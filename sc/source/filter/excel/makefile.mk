#*************************************************************************
#
# Copyright 2008 by Sun Microsystems, Inc.
#
# $RCSfile$
#
# $Revision$
#
# This file is part of NeoOffice.
#
# NeoOffice is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3
# only, as published by the Free Software Foundation.
#
# NeoOffice is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU General Public License
# version 3 along with NeoOffice.  If not, see
# <http://www.gnu.org/licenses/gpl-3.0.txt>
# for a copy of the GPLv3 License.
#
# Modified November 2014 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

PRJ=..$/..$/..

PRJNAME=sc
TARGET=excel

AUTOSEG=true

PROJECTPCH4DLL=TRUE
PROJECTPCH=filt_pch
PROJECTPCHSOURCE=..\pch\filt_pch

# --- Settings -----------------------------------------------------

.INCLUDE :  scpre.mk
.INCLUDE :  settings.mk
.INCLUDE :  sc.mk

.IF "$(UPD)" == "310"
.INCLUDE : $(PRJ)$/inc$/sc.mk
.ENDIF		# "$(UPD)" == "310"

# --- Files --------------------------------------------------------

SLOFILES =	\
		$(SLO)$/biffdump.obj				\
		$(SLO)$/colrowst.obj				\
		$(SLO)$/excdoc.obj					\
		$(SLO)$/excel.obj					\
		$(SLO)$/excform.obj					\
		$(SLO)$/excform8.obj				\
		$(SLO)$/excimp8.obj					\
		$(SLO)$/excrecds.obj				\
		$(SLO)$/exctools.obj				\
		$(SLO)$/expop2.obj					\
		$(SLO)$/fontbuff.obj				\
		$(SLO)$/frmbase.obj					\
		$(SLO)$/impop.obj					\
		$(SLO)$/namebuff.obj				\
		$(SLO)$/read.obj					\
		$(SLO)$/tokstack.obj				\
		$(SLO)$/xechart.obj					\
		$(SLO)$/xecontent.obj				\
		$(SLO)$/xeescher.obj				\
		$(SLO)$/xeformula.obj				\
		$(SLO)$/xehelper.obj				\
		$(SLO)$/xelink.obj					\
		$(SLO)$/xename.obj					\
		$(SLO)$/xepage.obj					\
		$(SLO)$/xepivot.obj					\
		$(SLO)$/xerecord.obj				\
		$(SLO)$/xeroot.obj					\
		$(SLO)$/xestream.obj				\
		$(SLO)$/xestring.obj				\
		$(SLO)$/xestyle.obj					\
		$(SLO)$/xetable.obj					\
		$(SLO)$/xeview.obj					\
		$(SLO)$/xichart.obj					\
		$(SLO)$/xicontent.obj				\
		$(SLO)$/xiescher.obj				\
		$(SLO)$/xiformula.obj				\
		$(SLO)$/xihelper.obj				\
		$(SLO)$/xilink.obj					\
		$(SLO)$/xiname.obj					\
		$(SLO)$/xipage.obj					\
		$(SLO)$/xipivot.obj					\
		$(SLO)$/xiroot.obj					\
		$(SLO)$/xistream.obj				\
		$(SLO)$/xistring.obj				\
		$(SLO)$/xistyle.obj					\
		$(SLO)$/xiview.obj					\
		$(SLO)$/xladdress.obj				\
		$(SLO)$/xlchart.obj					\
		$(SLO)$/xlescher.obj				\
		$(SLO)$/xlformula.obj				\
		$(SLO)$/xlpage.obj					\
		$(SLO)$/xlpivot.obj					\
		$(SLO)$/xlroot.obj					\
		$(SLO)$/xlstream.obj				\
		$(SLO)$/xlstyle.obj					\
		$(SLO)$/xltools.obj					\
		$(SLO)$/xltracer.obj				\
		$(SLO)$/xlview.obj \
		$(SLO)$/xltoolbar.obj \

.IF "$(OS)$(COM)$(CPUNAME)"=="LINUXGCCSPARC"
NOOPTFILES = \
		$(SLO)$/xiescher.obj
.ENDIF

EXCEPTIONSFILES = \
		$(SLO)$/excdoc.obj					\
		$(SLO)$/excel.obj					\
		$(SLO)$/excform.obj					\
		$(SLO)$/excform8.obj				\
		$(SLO)$/excimp8.obj					\
		$(SLO)$/excrecds.obj				\
		$(SLO)$/expop2.obj					\
		$(SLO)$/namebuff.obj				\
		$(SLO)$/tokstack.obj				\
		$(SLO)$/xecontent.obj				\
		$(SLO)$/xeescher.obj				\
		$(SLO)$/xeformula.obj				\
		$(SLO)$/xehelper.obj				\
		$(SLO)$/xelink.obj					\
		$(SLO)$/xename.obj					\
		$(SLO)$/xepage.obj					\
		$(SLO)$/xepivot.obj					\
		$(SLO)$/xechart.obj					\
		$(SLO)$/xestream.obj				\
		$(SLO)$/xestring.obj				\
		$(SLO)$/xestyle.obj					\
		$(SLO)$/xetable.obj					\
		$(SLO)$/xeview.obj					\
		$(SLO)$/xichart.obj					\
		$(SLO)$/xicontent.obj				\
		$(SLO)$/xiescher.obj				\
		$(SLO)$/xihelper.obj				\
		$(SLO)$/xilink.obj					\
		$(SLO)$/xipage.obj					\
		$(SLO)$/xipivot.obj					\
		$(SLO)$/xistream.obj				\
		$(SLO)$/xistring.obj				\
		$(SLO)$/xistyle.obj					\
		$(SLO)$/xladdress.obj				\
		$(SLO)$/xiescher.obj				\
		$(SLO)$/xlchart.obj					\
		$(SLO)$/xlformula.obj				\
		$(SLO)$/xlpivot.obj					\
		$(SLO)$/xlstyle.obj					\
		$(SLO)$/xlview.obj \
		$(SLO)$/xltoolbar.obj \

# --- Targets -------------------------------------------------------

.INCLUDE :  target.mk

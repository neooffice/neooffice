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

PRJ=..$/..
PRJNAME=svtools
TARGET=items1
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/svl.pmk

.IF "$(UPD)" == "310"
INCLOCAL += -I$(PRJ)$/..$/sal$/inc
.ENDIF		# "$(UPD)" == "310"

# --- Files --------------------------------------------------------

SLOFILES=\
	$(SLO)$/bintitem.obj	\
	$(SLO)$/cenumitm.obj	\
	$(SLO)$/cintitem.obj	\
	$(SLO)$/cntwall.obj	\
	$(SLO)$/cstitem.obj	\
	$(SLO)$/ctypeitm.obj	\
	$(SLO)$/custritm.obj	\
	$(SLO)$/dateitem.obj	\
	$(SLO)$/dtritem.obj	\
	$(SLO)$/frqitem.obj	\
	$(SLO)$/ilstitem.obj    \
	$(SLO)$/itemiter.obj	\
	$(SLO)$/itempool.obj	\
	$(SLO)$/itemprop.obj	\
	$(SLO)$/itemset.obj	\
	$(SLO)$/lckbitem.obj	\
	$(SLO)$/poolio.obj	\
	$(SLO)$/stylepool.obj	\
	$(SLO)$/poolitem.obj	\
	$(SLO)$/sfontitm.obj	\
	$(SLO)$/sitem.obj	    \
	$(SLO)$/slstitm.obj	\
	$(SLO)$/tfrmitem.obj	\
	$(SLO)$/tresitem.obj	\
	$(SLO)$/whiter.obj \
	$(SLO)$/visitem.obj

.IF "$(UPD)" == "310"
SLOFILES += $(SLO)$/grabbagitem.obj
.ENDIF		# "$(UPD)" == "310"

SRS1NAME=$(TARGET)
SRC1FILES=\
	cstitem.src

# --- Targets -------------------------------------------------------

.INCLUDE :  target.mk

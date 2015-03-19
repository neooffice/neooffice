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
# Modified December 2014 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

# tk.dxp should contain all c functions that have to be exported. MT 2001/11/29

PRJ=..

PRJNAME=toolkit
TARGET=tk
TARGET2=tka
USE_DEFFILE=TRUE

# --- Settings -----------------------------------------------------------

.INCLUDE :	settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk


# --- Allgemein ----------------------------------------------------------

# ========================================================================
# = tk lib: the "classic" toolkit library

LIB1TARGET= $(SLB)$/$(TARGET).lib
LIB1FILES=	$(SLB)$/awt.lib \
			$(SLB)$/tree.lib \
			$(SLB)$/controls.lib \
			$(SLB)$/helper.lib\
			$(SLB)$/layout-core.lib \
			$(SLB)$/layout-vcl.lib

SHL1TARGET= tk$(DLLPOSTFIX)
SHL1IMPLIB= itk
SHL1USE_EXPORTS=name

SHL1STDLIBS=\
		$(VOSLIB)           \
		$(VCLLIB)			\
		$(SOTLIB)			\
		$(UNOTOOLSLIB)		\
		$(TOOLSLIB)			\
		$(COMPHELPERLIB)	\
		$(CPPUHELPERLIB)	\
		$(CPPULIB)			\
		$(SALLIB)

SHL1LIBS=	$(LIB1TARGET)
SHL1DEF=	$(MISC)$/$(SHL1TARGET).def
SHL1DEPN=$(LIB1TARGET)

DEF1NAME	=$(SHL1TARGET)
DEF1DEPN	=$(LIB1TARGET)
DEF1DES		=TK
DEFLIB1NAME	=tk

.IF "$(OS)"=="MACOSX" 

# [ed] 6/16/02 Add in X libraries if we're building X

.IF "$(GUIBASE)"=="unx"
SHL1STDLIBS +=\
	-lX11 -lXt -lXmu
.ENDIF

.ELSE
.IF "$(GUI)"=="UNX"
SHL1STDLIBS +=\
	-lX11
.ENDIF
.ENDIF

.IF "$(GUIBASE)"=="java"
SHL1STDLIBS += -lobjc
.ENDIF		# "$(GUIBASE)"=="java"

RESLIB1IMAGES=$(PRJ)$/source$/awt
RES1FILELIST=$(SRS)$/awt.srs
RESLIB1NAME=$(TARGET)
RESLIB1SRSFILES=$(RES1FILELIST)

# --- Footer -------------------------------------------------------------
.INCLUDE :	target.mk
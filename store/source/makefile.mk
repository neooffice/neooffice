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
# Modified January 2009 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

PRJ=..

PRJNAME=store
TARGET=store
ENABLE_EXCEPTIONS=TRUE

# --- Settings ---

.INCLUDE : settings.mk

.IF "$(GUIBASE)" == "java"
CDEFS+=-O0
.ENDIF

# --- Files ---

SLOFILES=	\
	$(SLO)$/object.obj		\
	$(SLO)$/memlckb.obj		\
	$(SLO)$/filelckb.obj	\
	$(SLO)$/storbase.obj	\
	$(SLO)$/storcach.obj	\
	$(SLO)$/stordata.obj	\
	$(SLO)$/storlckb.obj	\
	$(SLO)$/stortree.obj	\
	$(SLO)$/storpage.obj	\
	$(SLO)$/store.obj

.IF "$(debug)" != ""
OBJFILES=	\
	$(OBJ)$/object.obj		\
	$(OBJ)$/memlckb.obj		\
	$(OBJ)$/filelckb.obj	\
	$(OBJ)$/storbase.obj	\
	$(OBJ)$/storcach.obj	\
	$(OBJ)$/stordata.obj	\
	$(OBJ)$/storlckb.obj	\
	$(OBJ)$/stortree.obj	\
	$(OBJ)$/storpage.obj	\
	$(OBJ)$/store.obj
.ENDIF # debug

# --- Targets ---

.INCLUDE : target.mk

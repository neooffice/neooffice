##########################################################################
# 
#   $RCSfile$
# 
#   $Revision$
# 
#   last change: $Author$ $Date$
# 
#   The Contents of this file are made available subject to the terms of
#   either of the following licenses
# 
#          - GNU General Public License Version 2.1
# 
#   Patrick Luby, November 2014
# 
#   GNU General Public License Version 2.1
#   =============================================
#   Copyright 2014 Planamesa Inc.
# 
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public
#   License version 2.1, as published by the Free Software Foundation.
# 
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
# 
#   You should have received a copy of the GNU General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#   MA  02111-1307  USA
# 
##########################################################################

.IF "$(UPD)" == "310"
# Avoid conflicting header file names by putting this project first in the list
INCLOCAL += \
	-I$(PRJ)$/inc \
	-I$(PRJ)$/..$/comphelper$/inc \
	-I$(PRJ)$/..$/offapi$/$(INPATH)$/inc$/csstext \
	-I$(PRJ)$/..$/oox$/inc \
	-I$(PRJ)$/..$/oox$/$(INPATH)$/inc \
	-I$(PRJ)$/..$/sal$/inc \
	-I$(PRJ)$/..$/sax$/inc \
	-I$(PRJ)$/..$/sfx2$/inc \
	-I$(PRJ)$/..$/svtools$/inc \
	-I$(PRJ)$/..$/svx$/inc
.ENDIF		# "$(UPD)" == "310"
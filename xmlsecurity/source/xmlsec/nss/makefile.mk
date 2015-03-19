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
# Modified February 2015 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

PRJ=..$/..$/..

PRJNAME = xmlsecurity
TARGET = xs_nss

ENABLE_EXCEPTIONS = TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :	$(PRJ)$/util$/target.pmk

.IF "$(SYSTEM_LIBXML)" == "YES"
CFLAGS+=-DSYSTEM_LIBXML $(LIBXML_CFLAGS)
.ENDIF

.IF "$(CRYPTO_ENGINE)" != "nss"
LIBTARGET=NO
.ENDIF

.IF "$(UPD)" == "310"
INCLOCAL+= \
	-I$(PRJ)$/..$/offapi$/$(INPATH)$/inc$/cssxmlcrypto \
	-I$(PRJ)$/..$/sal$/inc
.ENDIF		# "$(UPD)" == "310"

.IF "$(CRYPTO_ENGINE)" == "nss"

.IF "$(WITH_MOZILLA)" == "NO"
@all:
	@echo "No mozilla -> no nss -> no libxmlsec -> no xmlsecurity/nss"
.ENDIF

.IF "$(SYSTEM_MOZILLA)" != "YES"
MOZ_INC = $(SOLARVERSION)$/$(INPATH)$/inc$(UPDMINOREXT)$/mozilla
NSS_INC = $(MOZ_INC)$/nss
NSPR_INC = $(MOZ_INC)$/nspr
.ELSE
# MOZ_INC already defined from environment
NSS_INC = $(MOZ_NSS_CFLAGS)
NSPR_INC = $(MOZ_INC)$/nspr
.ENDIF

.IF "$(GUI)"=="UNX"
.IF "$(COMNAME)"=="sunpro5"
CFLAGS += -features=tmplife
#This flag is needed to build mozilla 1.7 code
.ENDIF		# "$(COMNAME)"=="sunpro5"
.ENDIF

.IF "$(GUI)" == "WNT"
.IF "$(DBG_LEVEL)" == "0"
INCPRE += \
-I$(MOZ_INC)$/profile \
-I$(MOZ_INC)$/string \
-I$(MOZ_INC)$/embed_base
CFLAGS +=   -GR- -W3 -Gy -MD -UDEBUG
.ELSE
INCPRE += \
-I$(MOZ_INC)$/profile \
-I$(MOZ_INC)$/string \
-I$(MOZ_INC)$/embed_base
CFLAGS += -Zi -GR- -W3 -Gy -MDd -UNDEBUG
.ENDIF
.ENDIF
.IF "$(GUI)" == "UNX"
INCPOST += \
$(MOZ_INC)$/profile \
-I$(MOZ_INC)$/string \
-I$(MOZ_INC)$/embed_base
#.IF "$(OS)" == "LINUX"
#CFLAGS +=   -fPIC -g
#CFLAGSCXX += \
#            -fno-rtti -Wall -Wconversion -Wpointer-arith \
#            -Wbad-function-cast -Wcast-align -Woverloaded-virtual -Wsynth \
#            -Wno-long-long -pthread
#CDEFS     += -DTRACING
#.ELIF "$(OS)" == "NETBSD"
#CFLAGS +=   -fPIC
#CFLAGSCXX += \
#            -fno-rtti -Wall -Wconversion -Wpointer-arith \
#            -Wbad-function-cast -Wcast-align -Woverloaded-virtual -Wsynth \
#            -Wno-long-long
#CDEFS     += -DTRACING
#.ENDIF
.ENDIF

CDEFS += -DXMLSEC_CRYPTO_NSS -DXMLSEC_NO_XSLT

# --- Files --------------------------------------------------------

SOLARINC += \
 -I$(MOZ_INC) \
-I$(NSPR_INC) \
-I$(PRJ)$/source$/xmlsec

.IF "$(SYSTEM_MOZILLA)" == "YES"
SOLARINC += -DSYSTEM_MOZILLA $(NSS_INC)
.ELSE
SOLARINC += -I$(NSS_INC)
.ENDIF

SLOFILES = \
	$(SLO)$/securityenvironment_nssimpl.obj \
	$(SLO)$/xmlencryption_nssimpl.obj \
	$(SLO)$/xmlsecuritycontext_nssimpl.obj \
	$(SLO)$/xmlsignature_nssimpl.obj \
	$(SLO)$/x509certificate_nssimpl.obj \
	$(SLO)$/seinitializer_nssimpl.obj \
	$(SLO)$/xsec_nss.obj

.IF "$(UPD)" == "310"
SLOFILES += \
	$(SLO)$/ciphercontext.obj \
	$(SLO)$/digestcontext.obj \
	$(SLO)$/nssinitializer.obj
.ENDIF		# "$(UPD)" == "310"
	
.ENDIF

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk
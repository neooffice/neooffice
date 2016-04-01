#**************************************************************
#  
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#  
#    http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#  
#  This file incorporates work covered by the following license notice:
# 
#    Modified February 2016 by Patrick Luby. NeoOffice is only distributed
#    under the GNU General Public License, Version 3 as allowed by Section 4
#    of the Apache License, Version 2.0.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#  
#**************************************************************



PRJ=..$/..$/..

PRJNAME=filter
TARGET=ipict
DEPTARGET=vipict

# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk

# --- Allgemein ----------------------------------------------------------

.IF "$(editdebug)"!="" || "$(EDITDEBUG)"!=""
CDEFS+= -DEDITDEBUG
.ENDIF
.IF "$(L10N_framework)"==""
SLOFILES =  $(SLO)$/ipict.obj $(SLO)$/shape.obj

# ==========================================================================

SHL1TARGET=     ipt$(DLLPOSTFIX)
SHL1IMPLIB=     ipict
SHL1STDLIBS=    $(VCLLIB) $(TOOLSLIB) $(SALLIB) $(BASEGFXLIB)
.IF "$(GUI)" == "OS2"
SHL1STDLIBS+=   $(CPPULIB)
.ENDIF
SHL1LIBS=       $(SLB)$/ipict.lib

.IF "$(GUI)" != "UNX"
.IF "$(COM)" != "GCC"
SHL1OBJS=       $(SLOFILES)
.ENDIF
.ENDIF

.IF "$(GUIBASE)" == "java"
SHL1STDLIBS += -framework ApplicationServices
.ENDIF

SHL1VERSIONMAP=exports.map
SHL1DEF=        $(MISC)$/$(SHL1TARGET).def

DEF1NAME=$(SHL1TARGET)
.ENDIF

# ==========================================================================

.INCLUDE :  target.mk

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
# Modified April 2006 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

# Builds the Java Canvas implementation.

PRJNAME	= canvas
PRJ		= ..$/..
TARGET	= javacanvas
PACKAGE = canvas

USE_JAVAVER:=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE: settings.mk

.IF "$(SOLAR_JAVA)"=="TRUE"
# Since Canvas needs newer features like
# e.g. java.awt.image.BufferStrategy,
# disabled for now for everything <1.4
.IF "$(JAVANUMVER:s/.//)" >= "000100040000"

JAVAFILES  = \
	SpriteBase.java \
	JavaCanvas.java \
	CanvasGraphicDevice.java \
	CanvasUtils.java \
	CanvasFont.java \
	CanvasBitmap.java \
	CanvasSprite.java \
	CanvasCustomSprite.java \
	CanvasClonedSprite.java \
	TextLayout.java \
	BackBuffer.java \
	LinePolyPolygon.java \
	BezierPolyPolygon.java \
	SpriteRunner.java

.IF "$(GUIBASE)"=="unx"

JAVAFILES += x11/WindowAdapter.java

.ELIF "$(GUIBASE)"=="aqua" || "$(GUIBASE)"=="java"

JAVAFILES += aqua/WindowAdapter.java

.ELSE

JAVAFILES += win/WindowAdapter.java

.ENDIF		# "$(GUIBASE)"=="unx"

JARFILES 		= jurt.jar unoil.jar ridl.jar juh.jar java_uno.jar
#JAVACLASSFILES	= $(foreach,i,$(JAVAFILES) $(CLASSDIR)$/$(PACKAGE)$/$(i:b).class)

JARTARGET               = $(TARGET).uno.jar
JARCOMPRESS             = TRUE
#JARCLASSDIRS            = $(PACKAGE)
CUSTOMMANIFESTFILE      = manifest

.ENDIF # "$(JAVANUMVER:s/.//)" >= "000100040000"
.ENDIF # "$(SOLAR_JAVA)"=="TRUE"

# --- Targets ------------------------------------------------------

.INCLUDE: target.mk

#dist: $(JAVA_FILES:b:+".class")
#	+jar -cvfm $(CLASSDIR)/JavaCanvas.jar $(JARMANIFEST) $(JAVACLASSFILES)
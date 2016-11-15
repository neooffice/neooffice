/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 * 
 *   Modified November 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_FPICKER_SOURCE_AQUA_FPSERVICEINFO_HXX
#define INCLUDED_FPICKER_SOURCE_AQUA_FPSERVICEINFO_HXX

// the service names
#define FILE_PICKER_SERVICE_NAME "com.sun.star.ui.dialogs.AquaFilePicker"
#define FOLDER_PICKER_SERVICE_NAME "com.sun.star.ui.dialogs.AquaFolderPicker"

// the implementation names
#ifdef USE_JAVA
#define FILE_PICKER_IMPL_NAME    "com.sun.star.ui.dialogs.JavaFilePicker"
#define FOLDER_PICKER_IMPL_NAME    "com.sun.star.ui.dialogs.JavaFolderPicker"
#else	// USE_JAVA
#define FILE_PICKER_IMPL_NAME    "com.sun.star.ui.dialogs.SalAquaFilePicker"
#define FOLDER_PICKER_IMPL_NAME    "com.sun.star.ui.dialogs.SalAquaFolderPicker"
#endif	// USE_JAVA

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
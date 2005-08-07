/***************************************************************************
	begin                : Aug 2005
	copyright            : (C) 2005 by Craig Bradney	
	email                : cbradney@zip.com.au
***************************************************************************/

/***************************************************************************
*                                                                         *
*   ScApp program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "commonstrings.h"
#include "commonstrings.moc"

QString CommonStrings::_Apply      = "";
QString CommonStrings::tr_Apply    = "";
QString CommonStrings::_Cancel     = "";
QString CommonStrings::tr_Cancel   = "";
QString CommonStrings::None        = "";
QString CommonStrings::trNone      = "";
QString CommonStrings::_OK         = "";
QString CommonStrings::tr_OK       = "";
QString CommonStrings::_Save       = "";
QString CommonStrings::tr_Save     = "";

void CommonStrings::languageChange()
{
	CommonStrings::_Apply    = "&Apply";
	CommonStrings::tr_Apply  = tr( "&Apply" );
	
	CommonStrings::_Cancel    = "&Cancel";
	CommonStrings::tr_Cancel  = tr( "&Cancel" );
	
	CommonStrings::None       = "None";
	CommonStrings::trNone     = QT_TR_NOOP( "None" );
	
	CommonStrings::_OK        = "&OK";
	CommonStrings::tr_OK      = tr( "&OK" );
	
	CommonStrings::_Save      = "&Save";
	CommonStrings::tr_Save    = QT_TR_NOOP( "&Save" );
}


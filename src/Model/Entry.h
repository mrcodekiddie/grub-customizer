/*
 * Copyright (C) 2010-2011 Daniel Richter <danielrichter2007@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GRUB_CUSTOMIZER_ENTRY_INCLUDED
#define GRUB_CUSTOMIZER_ENTRY_INCLUDED
#include <cstdio>
#include <string>
#include <list>
#include "../lib/CommonClass.h"
#include "../lib/str_replace.h"

struct Model_Entry_Row {
	Model_Entry_Row(FILE* sourceFile);
	Model_Entry_Row();
	std::string text;
	bool eof;
	bool is_loaded;
	operator bool();
};

struct Model_Entry : public CommonClass {
	enum EntryType {
		MENUENTRY,
		SUBMENU,
		SCRIPT_ROOT,
		PLAINTEXT
	} type;
	bool isValid, isModified;
	std::string name, extension, content;
	char quote;
	std::list<Model_Entry> subEntries;
	Model_Entry();
	Model_Entry(std::string name, std::string extension, std::string content = "", EntryType type = MENUENTRY);
	Model_Entry(FILE* sourceFile, Model_Entry_Row firstRow = Model_Entry_Row(), Logger* logger = NULL, std::string* plaintextBuffer = NULL);
	std::list<Model_Entry>& getSubEntries();
	operator bool() const;
};

#endif

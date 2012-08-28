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

#ifndef THREADCONTROLLER_H_INCLUDED
#define THREADCONTROLLER_H_INCLUDED
#include <string>
#include "../lib/Exception.h"

class ThreadController {
public:
	virtual inline ~ThreadController() {};

	virtual void syncEntryList() = 0;
	virtual void updateSaveProgress() = 0;
	virtual void updateSettingsDlgResolutionList() = 0;
	virtual void showThreadDiedError() = 0;
	virtual void enableSettings() = 0;
	virtual void startLoadThread(bool preserveConfig) = 0;
	virtual void startSaveThread() = 0;
	virtual void startFramebufferResolutionLoader() = 0;
	virtual void startGrubInstallThread(std::string const& device) = 0;
	virtual void stopApplication() = 0;
	virtual void showException(Exception const& e) = 0;
};

#endif
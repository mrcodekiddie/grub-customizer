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

#include "entryEditDlgGtk.h"

EntryEditDlgGtk::EntryEditDlgGtk()
	: rulePtr(NULL), lblType(gettext("_Type:"), true), lock_state(false)
{
	this->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	this->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

	this->set_default_size(400, 300);
	this->set_title(Glib::ustring() + gettext("Entry editor") + " - Grub Customizer");
	this->set_icon_name("grub-customizer");

	Gtk::VBox& vbMain = *this->get_vbox();
	vbMain.add(this->tabbox);

	tabbox.append_page(this->scrOptions, gettext("Options"));
	tabbox.append_page(this->scrSource, gettext("Source"));
	scrOptions.add(this->tblOptions);
	scrOptions.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrSource.add(this->tvSource);
	scrSource.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	this->tblOptions.attach(this->lblType, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
	this->tblOptions.attach(this->cbType, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 5, 5);
	lblType.set_mnemonic_widget(cbType);
	lblType.set_alignment(Gtk::ALIGN_RIGHT);

	this->signal_response().connect(sigc::mem_fun(this, &EntryEditDlgGtk::signal_response_action));
	this->cbType.signal_changed().connect(sigc::mem_fun(this, &EntryEditDlgGtk::signal_typeModified));

	this->tvSource.signal_key_release_event().connect(sigc::mem_fun(this, &EntryEditDlgGtk::signal_sourceModified));
}

void EntryEditDlgGtk::setEventListener(EventListener_entryEditDlg& eventListener) {
	this->eventListener = &eventListener;
}

void EntryEditDlgGtk::setDeviceDataList(DeviceDataList_Iface& deviceDataList) {
	this->deviceDataList = &deviceDataList;
}

std::string EntryEditDlgGtk::mapOptionName(std::string const& name) {
	if (name == "partition_uuid")
		return gettext("_Partition");
	else if (name == "initramfs")
		return gettext("_Initial ramdisk");
	else if (name == "linux_image")
		return gettext("_Linux image");
	else if (name == "memtest_image")
		return gettext("_Memtest image");
	else if (name == "iso_path")
		return gettext("Path to iso file");
	else if (name == "locale")
		return gettext("Locale");
	else if (name == "other_params")
		return gettext("Kernel params");
	else
		return name;
}

void EntryEditDlgGtk::setSourcecode(std::string const& source) {
	std::string optimizedSource = str_replace("\n\t", "\n", source);
	if (optimizedSource[0] == '\t') {
		optimizedSource = optimizedSource.substr(1);
	}
	this->tvSource.get_buffer()->set_text(optimizedSource);
}

std::string EntryEditDlgGtk::getSourcecode() {
	std::string optimizedSourcecode = this->tvSource.get_buffer()->get_text();
	std::string withIndent = str_replace("\n", "\n\t", optimizedSourcecode);
	if (withIndent.size() >= 2 && withIndent.substr(withIndent.size() - 2) == "\n\t") {
		withIndent.replace(withIndent.size() - 2, 2, "\n");
	} else if (withIndent.size() >= 1 && withIndent[withIndent.size() - 1] != '\n') {
		withIndent += '\n'; // add trailing slash
	}
	return "\t" + withIndent;
}

void EntryEditDlgGtk::addOption(std::string const& name, std::string const& value) {
	int pos = name == "partition_uuid" ? 1 : this->optionMap.size() + 2; // partition should be the first option
	Gtk::Label* label = Gtk::manage(new Gtk::Label(this->mapOptionName(name) + ":", true));
	label->set_alignment(Gtk::ALIGN_RIGHT);
	this->tblOptions.attach(*label, 0, 1, pos, pos+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
	this->labelMap[name] = label;

	Gtk::Widget* addedWidget = NULL;
	if (name != "partition_uuid" || this->deviceDataList == NULL) {
		Gtk::Entry* entry = Gtk::manage(new Gtk::Entry());
		entry->set_text(value);
		entry->signal_changed().connect(sigc::mem_fun(this, &EntryEditDlgGtk::signal_optionsModified));
		addedWidget = entry;
	} else {
		PartitionChooser_DropDown* pChooserDD = Gtk::manage(new PartitionChooser_DropDown(value, *this->deviceDataList));
		pChooserDD->signal_changed().connect(sigc::mem_fun(this, &EntryEditDlgGtk::signal_optionsModified));
		addedWidget = pChooserDD;
	}

	this->tblOptions.attach(*addedWidget, 1, 2, pos, pos+1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 5, 5);
	label->set_mnemonic_widget(*addedWidget);

	this->optionMap[name] = addedWidget;
	if (this->is_visible()) {
		this->tblOptions.show_all();
	}
}

void EntryEditDlgGtk::setOptions(std::map<std::string, std::string> options) {
	this->removeOptions();
	for (std::map<std::string, std::string>::iterator iter = options.begin(); iter != options.end(); iter++) {
		this->addOption(iter->first, iter->second);
	}
}

std::map<std::string, std::string> EntryEditDlgGtk::getOptions() const {
	std::map<std::string, std::string> result;
	for (std::map<std::string, Gtk::Widget*>::const_iterator iter = this->optionMap.begin(); iter != this->optionMap.end(); iter++) {
		try {
			result[iter->first] = dynamic_cast<PartitionChooser_DropDown&>(*iter->second).getSelectedUuid();
		} catch (std::bad_cast e) {
			result[iter->first] = dynamic_cast<Gtk::Entry&>(*iter->second).get_text();
		}
	}
	return result;
}

void EntryEditDlgGtk::removeOptions() {
	for (std::map<std::string, Gtk::Label*>::iterator iter = this->labelMap.begin(); iter != this->labelMap.end(); iter++) {
		this->tblOptions.remove(*iter->second);
	}
	this->labelMap.clear();
	for (std::map<std::string, Gtk::Widget*>::iterator iter = this->optionMap.begin(); iter != this->optionMap.end(); iter++) {
		this->tblOptions.remove(*iter->second);
	}
	this->optionMap.clear();
}

void EntryEditDlgGtk::setRulePtr(void* rulePtr) {
	this->rulePtr = rulePtr;
}

void* EntryEditDlgGtk::getRulePtr() {
	return this->rulePtr;
}

void EntryEditDlgGtk::show() {
	Gtk::Window::show_all();
}

void EntryEditDlgGtk::hide() {
	Gtk::Window::hide();
}

void EntryEditDlgGtk::setAvailableEntryTypes(std::list<std::string> const& names) {
	this->cbType.clear();
	for (std::list<std::string>::const_iterator iter = names.begin(); iter != names.end(); iter++) {
		this->cbType.append_text(*iter);
	}
	this->cbType.append_text(gettext("Other"));
}

void EntryEditDlgGtk::selectType(std::string const& name) {
	std::string name2 = name;
	if (name2 == "") {
		name2 = gettext("Other");
	}

	this->lock_state = true;
	this->cbType.set_active_text(name2);
	this->lock_state = false;
}

std::string EntryEditDlgGtk::getSelectedType() const {
	return this->cbType.get_active_text();
}

void EntryEditDlgGtk::signal_response_action(int response_id) {
	if (response_id == Gtk::RESPONSE_OK){
		eventListener->entryEditDlg_applied();
	}
	this->hide();
}

bool EntryEditDlgGtk::signal_sourceModified(GdkEventKey* event) {
	if (!this->lock_state) {
		this->eventListener->entryEditDlg_sourceModified();
	}
	return true;
}

void EntryEditDlgGtk::signal_optionsModified() {
	if (!this->lock_state) {
		this->eventListener->entryEditDlg_optionsModified();
	}
}

void EntryEditDlgGtk::signal_typeModified() {
	if (!this->lock_state) {
		this->eventListener->entryEditDlg_typeModified(this->cbType.get_active_text() != gettext("Other") ? this->cbType.get_active_text() : "");
	}
}

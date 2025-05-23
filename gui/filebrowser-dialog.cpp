/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "gui/filebrowser-dialog.h"

#include "common/system.h"
#include "common/algorithm.h"
#include "common/savefile.h"
#include "common/str-array.h"

#include "common/translation.h"

#include "gui/widgets/list.h"
#include "gui/gui-manager.h"
#include "gui/message.h"

namespace GUI {

enum {
	kChooseCmd = 'Chos'
};

FileBrowserDialog::FileBrowserDialog(const char *title, const char *fileExtension, int mode, const char *fileMask, const char *initialFilename)
	: Dialog("FileBrowser"), _mode(mode), _fileExt(fileExtension) {

	if (fileMask == NULL) {
		_fileMask = "*.";
		_fileMask += fileExtension;
	} else {
		_fileMask = fileMask;
	}
	_fileList = nullptr;

	new StaticTextWidget(this, "FileBrowser.Headline", title ? Common::convertToU32String(title) :
					mode == kFBModeLoad ? _("Choose file for loading") : _("Enter filename for saving"));

	_fileName = new EditTextWidget(this, "FileBrowser.Filename", Common::U32String(initialFilename));

	if (mode == kFBModeLoad)
		_fileName->setEnabled(false);

	// Add file list
	_fileList = new ListWidget(this, "FileBrowser.List");
	_fileList->setNumberingMode(kListNumberingOff);
	_fileList->setEditable(false);

	_backgroundType = GUI::ThemeEngine::kDialogBackgroundPlain;

	// Buttons
	new ButtonWidget(this, "FileBrowser.Cancel", _("Cancel"), Common::U32String(), kCloseCmd);
	new ButtonWidget(this, "FileBrowser.Choose", _("Choose"), Common::U32String(), kChooseCmd);
}

void FileBrowserDialog::open() {
	// Call super implementation
	Dialog::open();

	updateListing();
}

void FileBrowserDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kChooseCmd:
		if (_fileName->getEditString().empty())
			break;

		normalieFileName();

		if (!isProceedSave())
			break;

		setResult(1);
		close();
		break;
	case kListSelectionChangedCmd:
		_fileName->setEditString(_fileList->getList().operator[](_fileList->getSelected()));
		break;
	case kListItemActivatedCmd:
	case kListItemDoubleClickedCmd:
		normalieFileName();

		if (!isProceedSave())
			break;

		setResult(1);
		close();
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}

void FileBrowserDialog::normalieFileName() {
	Common::String filename = Common::convertFromU32String(_fileName->getEditString());

	if (filename.matchString(_fileMask, true))
		return;

	_fileName->setEditString(filename + "." + _fileExt);
}


bool FileBrowserDialog::isProceedSave() {
	bool matched = false;

	if (_mode == kFBModeLoad)
		return true;

	for (Common::U32StringArray::const_iterator file = _fileList->getList().begin(); file != _fileList->getList().end(); ++file) {
		if (*file == _fileName->getEditString()) {
			matched = true;
			break;
		}
	}

	if (matched) {
		GUI::MessageDialog alert(_("Do you really want to overwrite the file?"), _("Yes"), _("No"));

		if (alert.runModal() != GUI::kMessageOK)
			return false;
	}

	return true;
}

void FileBrowserDialog::updateListing() {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();

	Common::U32StringArray list;

	Common::StringArray filenames = saveFileMan->listSavefiles(_fileMask);
	Common::sort(filenames.begin(), filenames.end());

	for (const auto &file : filenames) {
		list.push_back(Common::U32String(file));
	}

	_fileList->setList(list);
	_fileList->scrollTo(0);

	// Finally, redraw
	g_gui.scheduleTopDialogRedraw();
}

} // End of namespace GUI

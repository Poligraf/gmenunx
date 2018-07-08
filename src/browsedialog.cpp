#include "browsedialog.h"
#include "FastDelegate.h"
// #include "debug.h"
#include <algorithm>

using namespace fastdelegate;
using namespace std;

BrowseDialog::BrowseDialog(GMenu2X *gmenu2x, const string &title, const string &subtitle)
: Dialog(gmenu2x), title(title), subtitle(subtitle), ts_pressed(false), buttonBox(gmenu2x) {
	IconButton *btn;

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/start.png", gmenu2x->tr["Exit"]);
	btn->setAction(MakeDelegate(this, &BrowseDialog::cancel));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/b.png", gmenu2x->tr["Folder up"]);
	btn->setAction(MakeDelegate(this, &BrowseDialog::directoryUp));
	buttonBox.add(btn);

	btn = new IconButton(gmenu2x, "skin:imgs/buttons/a.png", gmenu2x->tr["Select"]);
	btn->setAction(MakeDelegate(this, &BrowseDialog::directoryEnter));
	buttonBox.add(btn);

	iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	iconFile = gmenu2x->sc.skinRes("imgs/file.png");

	fl = new FileLister(CARD_ROOT, true, false);
}

BrowseDialog::~BrowseDialog() {
	delete fl;
}

bool BrowseDialog::exec() {
	// gmenu2x->initBG();

	// moved out of the loop to fix weird scroll behavior
	uint32_t i, iY, firstElement = 0, offsetY, animation = 0, padding = 6;

	if (!fl)
		return false;

	string path = fl->getPath();
	if (path.empty() || !dirExists(path) || path.compare(0,CARD_ROOT_LEN,CARD_ROOT)!=0)
		setPath(CARD_ROOT);

	fl->browse();

	rowHeight = gmenu2x->font->getHeight()+1;
	numRows = gmenu2x->listRect.h/rowHeight - 1;

	selected = 0;
	close = false;

	drawTopBar(this->bg, title, subtitle, "icons/explorer.png");
	drawBottomBar(this->bg);
	this->bg->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

	// this->bg->setClipRect(gmenu2x->listRect);
	uint32_t tickStart = SDL_GetTicks();

	while (!close) {

		this->bg->blit(gmenu2x->s, 0, 0);

		buttonBox.paint(5);

		// beforeFileList(); // imagedialog.cpp

		if (selected > firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		offsetY = gmenu2x->listRect.y;

		iY = selected - firstElement;
		iY = offsetY + iY * rowHeight;

		//Selection
		gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);

		//Files & Directories
		for (i = firstElement; i < fl->size() && i <= firstElement + numRows; i++) {
			if (fl->isDirectory(i)) {
				if ((*fl)[i] == "..")
					iconGoUp->blit(gmenu2x->s, gmenu2x->listRect.x + 10, offsetY + rowHeight/2, HAlignCenter | VAlignMiddle);
				else
					iconFolder->blit(gmenu2x->s, gmenu2x->listRect.x + 10, offsetY + rowHeight/2, HAlignCenter | VAlignMiddle);
			} else{
				iconFile->blit(gmenu2x->s, gmenu2x->listRect.x + 10, offsetY + rowHeight/2, HAlignCenter | VAlignMiddle);
			}
			gmenu2x->s->write(gmenu2x->font, (*fl)[i], gmenu2x->listRect.x + 21, offsetY + 4, VAlignMiddle);

			if (gmenu2x->f200 && gmenu2x->ts.pressed() && gmenu2x->ts.inRect(gmenu2x->listRect.x, offsetY + 3, gmenu2x->listRect.w, rowHeight)) {
				ts_pressed = true;
				selected = i;
			}

			offsetY += rowHeight;
		}

		// preview
		string filename = fl->getPath() + "/" + getFile();
		string ext = getExt();

		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif") {
			gmenu2x->s->box(320 - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);

			gmenu2x->sc[filename]->softStretch(gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding, true, false);
			gmenu2x->sc[filename]->blit(gmenu2x->s, {320 - animation + padding, gmenu2x->listRect.y + padding, gmenu2x->skinConfInt["previewWidth"] - 2 * padding, gmenu2x->listRect.h - 2 * padding}, HAlignCenter | VAlignMiddle, 240);

			if (animation < gmenu2x->skinConfInt["previewWidth"]) {
				animation = intTransition(0, gmenu2x->skinConfInt["previewWidth"], tickStart, 110);
				gmenu2x->s->flip();
				gmenu2x->input.setWakeUpInterval(45);
				continue;
			}
		} else {
			if (animation > 0) {
				gmenu2x->s->box(320 - animation, gmenu2x->listRect.y, gmenu2x->skinConfInt["previewWidth"], gmenu2x->listRect.h, gmenu2x->skinConfColors[COLOR_TOP_BAR_BG]);
				animation = gmenu2x->skinConfInt["previewWidth"] - intTransition(0, gmenu2x->skinConfInt["previewWidth"], tickStart, 80);
				gmenu2x->s->flip();
				gmenu2x->input.setWakeUpInterval(45);
				continue;
			}
		}
		gmenu2x->input.setWakeUpInterval(1000);


		gmenu2x->drawScrollBar(numRows, fl->size(), firstElement, gmenu2x->listRect);
		gmenu2x->s->flip();


		// handleInput();
		BrowseDialog::Action action;

		bool inputAction = gmenu2x->input.update();
	// if (gmenu2x->powerManager(inputAction)) return;
		if (gmenu2x->inputCommonActions(inputAction)) continue;

		if (ts_pressed && !gmenu2x->ts.pressed()) {
			action = BrowseDialog::ACT_SELECT;
			ts_pressed = false;
		} else {
			action = getAction();
		}

		if (gmenu2x->f200 && gmenu2x->ts.pressed() && !gmenu2x->ts.inRect(gmenu2x->listRect)) ts_pressed = false;
	// if (action == BrowseDialog::ACT_SELECT && (*fl)[selected] == "..")
		// action = BrowseDialog::ACT_GOUP;
		switch (action) {
			case BrowseDialog::ACT_CLOSE:
			if (allowSelectDirectory && fl->isDirectory(selected)) {
				confirm();
			} else {
				cancel();
			}
			break;
			case BrowseDialog::ACT_UP:
			tickStart = SDL_GetTicks();
			if (selected == 0)
				selected = fl->size() - 1;
			else
				selected -= 1;
			break;
			case BrowseDialog::ACT_DOWN:
			tickStart = SDL_GetTicks();
			if (fl->size() - 1 <= selected)
				selected = 0;
			else
				selected += 1;
			break;
			case BrowseDialog::ACT_SCROLLUP:
			tickStart = SDL_GetTicks();
			if (selected < numRows)
				selected = 0;
			else
				selected -= numRows;
			break;
			case BrowseDialog::ACT_SCROLLDOWN:
			tickStart = SDL_GetTicks();
			if (selected + numRows >= fl->size())
				selected = fl->size()-1;
			else
				selected += numRows;
			break;
			case BrowseDialog::ACT_GOUP:
			directoryUp();
			break;
			case BrowseDialog::ACT_SELECT:
			if (fl->isDirectory(selected)) {
				directoryEnter();
				break;
			}
		/* Falltrough */
			case BrowseDialog::ACT_CONFIRM:
			confirm();
			break;
			default:
			break;
		}

		if (gmenu2x->f200) gmenu2x->ts.poll();
		buttonBox.handleTS();
	}
	// gmenu2x->s->clearClipRect();
	return result;
}

BrowseDialog::Action BrowseDialog::getAction() {
	BrowseDialog::Action action = BrowseDialog::ACT_NONE;

	// if (gmenu2x->inputCommonActions()) return action;

	if (gmenu2x->input[MENU])
		action = BrowseDialog::ACT_CLOSE;
	else if (gmenu2x->input[UP])
		action = BrowseDialog::ACT_UP;
	else if (gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT])
		action = BrowseDialog::ACT_SCROLLUP;
	else if (gmenu2x->input[DOWN])
		action = BrowseDialog::ACT_DOWN;
	else if (gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT])
		action = BrowseDialog::ACT_SCROLLDOWN;
	else if (gmenu2x->input[CANCEL])
		action = BrowseDialog::ACT_GOUP;
	else if (gmenu2x->input[CONFIRM])
		action = BrowseDialog::ACT_SELECT;
	else if (gmenu2x->input[SETTINGS]) {
		action = BrowseDialog::ACT_CLOSE;
	}
	return action;
}

void BrowseDialog::handleInput() {
	// BrowseDialog::Action action;

	// bool inputAction = gmenu2x->input.update();
	// // if (gmenu2x->powerManager(inputAction)) return;
	// if (gmenu2x->inputCommonActions(inputAction)) return;

	// if (ts_pressed && !gmenu2x->ts.pressed()) {
	// 	action = BrowseDialog::ACT_SELECT;
	// 	ts_pressed = false;
	// } else {
	// 	action = getAction();
	// }

	// if (gmenu2x->f200 && gmenu2x->ts.pressed() && !gmenu2x->ts.inRect(gmenu2x->listRect)) ts_pressed = false;
	// // if (action == BrowseDialog::ACT_SELECT && (*fl)[selected] == "..")
	// 	// action = BrowseDialog::ACT_GOUP;
	// switch (action) {
	// case BrowseDialog::ACT_CLOSE:
	// 	if (fl->isDirectory(selected)) {
	// 		confirm();
	// 	} else {
	// 		cancel();
	// 	}
	// 	break;
	// case BrowseDialog::ACT_UP:
	// 	if (selected == 0)
	// 		selected = fl->size() - 1;
	// 	else
	// 		selected -= 1;
	// 	break;
	// case BrowseDialog::ACT_SCROLLUP:
	// 	if (selected < numRows)
	// 		selected = 0;
	// 	else
	// 		selected -= numRows;
	// 	break;
	// case BrowseDialog::ACT_DOWN:
	// 	if (fl->size() - 1 <= selected)
	// 		selected = 0;
	// 	else
	// 		selected += 1;
	// 	break;
	// case BrowseDialog::ACT_SCROLLDOWN:
	// 	if (selected + numRows >= fl->size())
	// 		selected = fl->size()-1;
	// 	else
	// 		selected += numRows;
	// 	break;
	// case BrowseDialog::ACT_GOUP:
	// 	directoryUp();
	// 	break;
	// case BrowseDialog::ACT_SELECT:
	// 	if (fl->isDirectory(selected)) {
	// 		directoryEnter();
	// 		break;
	// 	}
	// 	/* Falltrough */
	// case BrowseDialog::ACT_CONFIRM:
	// 	confirm();
	// 	break;
	// default:
	// 	break;
	// }

	// buttonBox.handleTS();
}

void BrowseDialog::directoryUp() {
	string path = fl->getPath();
	string::size_type p = path.rfind("/");

	if (p == path.size() - 1)
		p = path.rfind("/", p - 1);

	if (p == string::npos || p < 4 || path.compare(0, CARD_ROOT_LEN, CARD_ROOT) != 0) {
		close = true;
		result = false;
	} else {
		selected = 0;
		setPath("/"+path.substr(0, p));
	}
}

void BrowseDialog::directoryEnter() {
	string path = fl->getPath();
	setPath(path + "/" + fl->at(selected));
	selected = 0;
}

void BrowseDialog::confirm() {
	result = true;
	close = true;
}

void BrowseDialog::cancel() {
	result = false;
	close = true;
}

const std::string BrowseDialog::getExt() {
	string filename = (*fl)[selected];
	string ext = "";
	string::size_type pos = filename.rfind(".");
	if (pos != string::npos && pos > 0) {
		ext = filename.substr(pos, filename.length());
		transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) tolower);
	}
	return ext;
}
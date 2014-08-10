/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo                           *
 *   massimiliano.torromeo@gmail.com                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "settingsdialog.h"

#include "gmenu2x.h"
#include "menusetting.h"

#include <SDL.h>

using namespace std;

SettingsDialog::SettingsDialog(
		GMenu2X *gmenu2x_, InputManager &inputMgr_, Touchscreen &ts_,
		const string &text_, const string &icon)
	: Dialog(gmenu2x_)
	, inputMgr(inputMgr_)
	, ts(ts_)
	, text(text_)
{
	if (!icon.empty() && gmenu2x->sc[icon] != NULL) {
		this->icon = icon;
	} else {
		this->icon = "icons/generic.png";
	}
}

SettingsDialog::~SettingsDialog() {
	for (vector<MenuSetting *>::iterator it = voices.begin();
			it != voices.end(); ++it) {
		delete *it;
	}
}

bool SettingsDialog::exec() {
	OffscreenSurface bg(gmenu2x->bg);
	bg.convertToDisplayFormat();

	bool close = false, ts_pressed = false;
	uint i, sel = 0, firstElement = 0;

	const int topBarHeight = gmenu2x->skinConfInt["topBarHeight"];
	SDL_Rect clipRect = {
		0,
		static_cast<Sint16>(topBarHeight + 1),
		static_cast<Uint16>(gmenu2x->resX - 9),
		static_cast<Uint16>(gmenu2x->resY - topBarHeight - 25)
	};
	SDL_Rect touchRect = {
		2,
		static_cast<Sint16>(topBarHeight + 4),
		static_cast<Uint16>(gmenu2x->resX - 12),
		static_cast<Uint16>(clipRect.h)
	};
	uint rowHeight = gmenu2x->font->getLineSpacing() + 1; // gp2x=15+1 / pandora=19+1
	uint numRows = (gmenu2x->resY - topBarHeight - 20) / rowHeight;

	uint maxNameWidth = 0;
	for (auto it = voices.begin(); it != voices.end(); it++) {
		maxNameWidth = max(maxNameWidth, (uint) gmenu2x->font->getTextWidth((*it)->getName()));
	}

	while (!close) {
		OutputSurface& s = *gmenu2x->s;

		if (ts.available()) ts.poll();

		bg.blit(s, 0, 0);

		gmenu2x->drawTopBar(s);
		//link icon
		drawTitleIcon(s, icon);
		writeTitle(s, text);

		gmenu2x->drawBottomBar(s);

		if (sel>firstElement+numRows-1) firstElement=sel-numRows+1;
		if (sel<firstElement) firstElement=sel;

		//selection
		uint iY = topBarHeight + 2 + (sel - firstElement) * rowHeight;

		//selected option
		voices[sel]->drawSelected(maxNameWidth + 15, iY, rowHeight);

		if (ts_pressed && !ts.pressed()) {
			ts_pressed = false;
		}
		if (ts.available() && ts.pressed() && !ts.inRect(touchRect)) {
			ts_pressed = false;
		}
		for (i=firstElement; i<voices.size() && i<firstElement+numRows; i++) {
			iY = i-firstElement;
			voices[i]->draw(maxNameWidth + 15, iY * rowHeight + topBarHeight + 2, rowHeight);
			if (ts.available() && ts.pressed() && ts.inRect(
					touchRect.x, touchRect.y + (iY * rowHeight),
					touchRect.w, rowHeight
					)) {
				ts_pressed = true;
				sel = i;
			}
		}

		gmenu2x->drawScrollBar(numRows, voices.size(), firstElement);

		//description
		writeSubTitle(s, voices[sel]->getDescription());

		s.flip();
		voices[sel]->handleTS(maxNameWidth + 15, iY, rowHeight);

		InputManager::Button button = inputMgr.waitForPressedButton();
		if (!voices[sel]->handleButtonPress(button)) {
			switch (button) {
				case InputManager::SETTINGS:
					close = true;
					break;
				case InputManager::UP:
					if (sel == 0) {
						sel = voices.size() - 1;
					} else {
						sel -= 1;
					}
					break;
				case InputManager::DOWN:
					sel += 1;
					if (sel>=voices.size()) sel = 0;
				default:
					break;
			}
		}
	}

	return true;
}

void SettingsDialog::addSetting(MenuSetting *set) {
	voices.push_back(set);
}

bool SettingsDialog::edited() {
	for (uint i=0; i < voices.size(); i++) {
		if (voices[i]->edited()) return true;
	}
	return false;
}

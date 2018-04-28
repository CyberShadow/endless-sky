/* PreferencesPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PreferencesPanel.h"

#include "Audio.h"
#include "Color.h"
#include "Dialog.h"
#include "Files.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Preferences.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "Table.h"
#include "UI.h"
#include "WrappedText.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;

namespace {
	// Settings that require special handling.

	enum {
		ZOOM_FACTOR = Preferences::MAX,
		VIEW_ZOOM_FACTOR,
		REACTIVATE_HELP,
		SCROLL_SPEED,
	};

	const int ZOOM_FACTOR_MIN = 100;
	const int ZOOM_FACTOR_MAX = 200;
	const int ZOOM_FACTOR_INCREMENT = 10;
}



PreferencesPanel::PreferencesPanel()
	: editing(-1), selected(0), hover(-1)
{
	if(!GameData::PluginAboutText().empty())
		selectedPlugin = GameData::PluginAboutText().begin()->first;
	
	SetIsFullScreen(true);
	hoverPreference = -1;
}



// Draw this panel.
void PreferencesPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	info.SetBar("volume", Audio::Volume());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	string pageName = (page == 'c' ? "controls" : page == 's' ? "settings" : "plugins");
	GameData::Interfaces().Get(pageName)->Draw(info, this);
	GameData::Interfaces().Get("preferences")->Draw(info, this);
	
	zones.clear();
	prefZones.clear();
	pluginZones.clear();
	if(page == 'c')
		DrawControls();
	else if(page == 's')
		DrawSettings();
	else if(page == 'p')
		DrawPlugins();
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(static_cast<unsigned>(editing) < zones.size())
	{
		Command::SetKey(zones[editing].Value(), key);
		EndEditing();
		return true;
	}
	
	if(key == SDLK_DOWN && static_cast<unsigned>(selected + 1) < zones.size())
		++selected;
	else if(key == SDLK_UP && selected > 0)
		--selected;
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		Exit();
	else if(key == 'c' || key == 's' || key == 'p')
		page = key;
	else
		return false;
	
	return true;
}



bool PreferencesPanel::Click(int x, int y, int clicks)
{
	EndEditing();
	
	if(x >= 265 && x < 295 && y >= -220 && y < 70)
	{
		Audio::SetVolume((20 - y) / 200.);
		Audio::Play(Audio::Get("warder"));
		return true;
	}
	
	Point point(x, y);
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			editing = selected = index;
	
	for(const auto &zone : prefZones)
		if(zone.Contains(point))
		{
			if(zone.Value().first == ZOOM_FACTOR)
			{
				int newZoom = Screen::Zoom() + ZOOM_FACTOR_INCREMENT;
				if(newZoom > ZOOM_FACTOR_MAX)
					newZoom = ZOOM_FACTOR_MIN;
				Screen::SetZoom(newZoom);
				// Make sure there is enough vertical space for the full UI.
				if(Screen::Height() < 700)
				{
					// Notify the user why setting the zoom any higher isn't permitted.
					// Only show this if it's not possible to zoom the view at all, as
					// otherwise the dialog will show every time, which is annoying.
					if(newZoom == ZOOM_FACTOR_MIN + ZOOM_FACTOR_INCREMENT)
						GetUI()->Push(new Dialog(
							"Your screen resolution is too low to support a zoom level above 100%."));
					Screen::SetZoom(ZOOM_FACTOR_MIN);
				}
				// Convert to raw window coordinates, at the new zoom level.
				point *= Screen::Zoom() / 100.;
				point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
				SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
			}
			else if(zone.Value().first == VIEW_ZOOM_FACTOR)
			{
				// Increase the zoom factor unless it is at the maximum. In that
				// case, cycle around to the lowest zoom factor.
				if(!Preferences::ZoomViewIn())
					while(Preferences::ZoomViewOut()) {}
			}
			else if(zone.Value().first == Preferences::ESCORTS_EXPEND_AMMO)
				Preferences::ToggleAmmoUsage();
			else if(zone.Value().first == REACTIVATE_HELP)
				Help::Reactivate();
			else if(zone.Value().first == SCROLL_SPEED)
			{
				// Toogle between three different speeds.
				int speed = Preferences::ScrollSpeed() + 20;
				if(speed > 60)
					speed = 20;
				Preferences::SetScrollSpeed(speed);
			}
			else
			{
				Preferences::Preference p = (Preferences::Preference)zone.Value().first;
				Preferences::Set(p, !Preferences::Has(p));
			}
			break;
		}
	
	for(const auto &zone : pluginZones)
		if(zone.Contains(point))
			selectedPlugin = zone.Value();
	
	return true;
}



bool PreferencesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	
	hover = -1;
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(hoverPoint))
			hover = index;
	
	hoverPreference = -1;
	for(const auto &zone : prefZones)
		if(zone.Contains(hoverPoint))
			hoverPreference = zone.Value().first;
	
	hoverPlugin.clear();
	for(const auto &zone : pluginZones)
		if(zone.Contains(hoverPoint))
			hoverPlugin = zone.Value();
	
	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool PreferencesPanel::Scroll(double dx, double dy)
{
	if(!dy || hoverPreference < 0)
		return false;
	
	if(hoverPreference == ZOOM_FACTOR)
	{
		int zoom = Screen::Zoom();
		if(dy < 0. && zoom > ZOOM_FACTOR_MIN)
			zoom -= ZOOM_FACTOR_INCREMENT;
		if(dy > 0. && zoom < ZOOM_FACTOR_MAX)
			zoom += ZOOM_FACTOR_INCREMENT;
		
		Screen::SetZoom(zoom);
		// Make sure there is enough vertical space for the full UI.
		while(Screen::Height() < 700 && zoom > ZOOM_FACTOR_MIN)
		{
			zoom -= ZOOM_FACTOR_INCREMENT;
			Screen::SetZoom(zoom);
		}
		
		// Convert to raw window coordinates, at the new zoom level.
		Point point = hoverPoint * (Screen::Zoom() / 100.);
		point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
		SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
	}
	else if(hoverPreference == VIEW_ZOOM_FACTOR)
	{
		if(dy < 0.)
			Preferences::ZoomViewOut();
		else
			Preferences::ZoomViewIn();
	}
	else if(hoverPreference == SCROLL_SPEED)
	{
		int speed = Preferences::ScrollSpeed();
		if(dy < 0.)
			speed = max(20, speed - 20);
		else
			speed = min(60, speed + 20);
		Preferences::SetScrollSpeed(speed);
	}
	return true;
}



void PreferencesPanel::EndEditing()
{
	editing = -1;
}



void PreferencesPanel::DrawControls()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	// Check for conflicts.
	Color red(.3, 0., 0., .3);
	
	Table table;
	table.AddColumn(-115, Table::LEFT);
	table.AddColumn(115, Table::RIGHT);
	table.SetUnderline(-120, 120);
	
	int firstY = -248;
	table.DrawAt(Point(-130, firstY));
	
	static const string CATEGORIES[] = {
		"Navigation",
		"Weapons",
		"Targeting",
		"Menus",
		"Fleet"
	};
	const string *category = CATEGORIES;
	static const Command COMMANDS[] = {
		Command::NONE,
		Command::FORWARD,
		Command::LEFT,
		Command::RIGHT,
		Command::BACK,
		Command::AFTERBURNER,
		Command::LAND,
		Command::JUMP,
		Command::NONE,
		Command::PRIMARY,
		Command::SELECT,
		Command::SECONDARY,
		Command::CLOAK,
		Command::NONE,
		Command::NEAREST,
		Command::TARGET,
		Command::HAIL,
		Command::BOARD,
		Command::SCAN,
		Command::NONE,
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::NONE,
		Command::DEPLOY,
		Command::FIGHT,
		Command::GATHER,
		Command::HOLD,
		Command::AMMO
	};
	static const Command *BREAK = &COMMANDS[19];
	for(const Command &command : COMMANDS)
	{
		// The "BREAK" line is where to go to the next column.
		if(&command == BREAK)
			table.DrawAt(Point(130, firstY));
		
		if(!command)
		{
			table.DrawGap(10);
			table.DrawUnderline(medium);
			if(category != end(CATEGORIES))
				table.Draw(*category++, bright);
			else
				table.Advance();
			table.Draw("Key", bright);
			table.DrawGap(5);
		}
		else
		{
			int index = zones.size();
			// Mark conflicts.
			bool isConflicted = command.HasConflict();
			bool isEditing = (index == editing);
			if(isConflicted || isEditing)
			{
				table.SetHighlight(66, 120);
				table.DrawHighlight(isEditing ? dim: red);
			}
			
			// Mark the selected row.
			bool isHovering = (index == hover && !isEditing);
			if(!isHovering && index == selected)
			{
				table.SetHighlight(-120, 64);
				table.DrawHighlight(back);
			}
			
			// Highlight whichever row the mouse hovers over.
			table.SetHighlight(-120, 120);
			if(isHovering)
				table.DrawHighlight(back);
			
			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);
			
			table.Draw(command.Description(), medium);
			table.Draw(command.KeyName(), isEditing ? bright : medium);
		}
	}
	
	Table shiftTable;
	shiftTable.AddColumn(125, Table::RIGHT);
	shiftTable.SetUnderline(0, 130);
	shiftTable.DrawAt(Point(-400, 52));
	
	shiftTable.DrawUnderline(medium);
	shiftTable.Draw("With <shift> key", bright);
	shiftTable.DrawGap(5);
	shiftTable.Draw("Select nearest ship", medium);
	shiftTable.Draw("Select next escort", medium);
	shiftTable.Draw("Talk to planet", medium);
	shiftTable.Draw("Board disabled escort", medium);
}



void PreferencesPanel::DrawSettings()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(-115, Table::LEFT);
	table.AddColumn(115, Table::RIGHT);
	table.SetUnderline(-120, 120);
	
	int firstY = -248;
	table.DrawAt(Point(-130, firstY));
	
	static const pair<int, const char*> SETTINGS[] = {
		make_pair(                                      -1, "Display"),
		make_pair(                             ZOOM_FACTOR, "Main zoom factor"),
		make_pair(                        VIEW_ZOOM_FACTOR, "View zoom factor"),
		make_pair(       Preferences::SHOW_STATUS_OVERLAYS, nullptr),
		make_pair( Preferences::HIGHLIGHT_PLAYERS_FLAGSHIP, nullptr),
		make_pair(     Preferences::ROTATE_FLAGSHIP_IN_HUD, nullptr),
		make_pair(         Preferences::SHOW_PLANET_LABELS, nullptr),
		make_pair(               Preferences::SHOW_MINIMAP, nullptr),
		make_pair(                                      -1, ""),
		make_pair(                                      -1, "AI"),
		make_pair(           Preferences::AUTOMATIC_AIMING, nullptr),
		make_pair(           Preferences::AUTOMATIC_FIRING, nullptr),
		make_pair(        Preferences::ESCORTS_EXPEND_AMMO, nullptr),
		make_pair(         Preferences::TURRETS_FOCUS_FIRE, "Turret tracking"),
		make_pair(                                      -1, ""),
		make_pair(                                      -1, "Performance"),
		make_pair(          Preferences::SHOW_CPU_GPU_LOAD, nullptr),
		make_pair(         Preferences::RENDER_MOTION_BLUR, nullptr),
		make_pair(      Preferences::REDUCE_LARGE_GRAPHICS, nullptr),
		make_pair(       Preferences::DRAW_BACKGROUND_HAZE, nullptr),
		make_pair(      Preferences::SHOW_HYPERSPACE_FLASH, nullptr),
		make_pair(                                      -1, "\n"),
		make_pair(                                      -1, "Other"),
		make_pair(    Preferences::CLICKABLE_RADAR_DISPLAY, nullptr),
		make_pair(Preferences::HIDE_UNEXPLORED_MAP_REGIONS, nullptr),
		make_pair(                         REACTIVATE_HELP, "Reactivate first-time help"),
		make_pair(Preferences::REHIRE_EXTRA_CREW_WHEN_LOST, nullptr),
		make_pair(                            SCROLL_SPEED, "Scroll speed"),
		make_pair( Preferences::SHOW_ESCORT_SYSTEMS_ON_MAP, nullptr),
		make_pair(              Preferences::WARNING_SIREN, nullptr),
	};
	bool isCategory = true;
	for(const pair<int, const char*> &setting : SETTINGS)
	{
		// Check if this is a category break or column break.
		Preferences::Preference preference = (Preferences::Preference)setting.first;
		string title = setting.second ? setting.second : Preferences::Name(preference);
		if(title.empty() || title == "\n")
		{
			isCategory = true;
			if(!title.empty())
				table.DrawAt(Point(130, firstY));
			continue;
		}
		
		if(isCategory)
		{
			isCategory = false;
			table.DrawGap(10);
			table.DrawUnderline(medium);
			table.Draw(title, bright);
			table.Advance();
			table.DrawGap(5);
			continue;
		}
		
		// Record where this setting is displayed, so the user can click on it.
		prefZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), make_pair(preference, title));
		
		// Get the "on / off" text for this setting.
		bool isOn = false;
		string text;
		if((int)preference == ZOOM_FACTOR)
		{
			isOn = true;
			text = to_string(Screen::Zoom());
		}
		else if((int)preference == VIEW_ZOOM_FACTOR)
		{
			isOn = true;
			text = to_string(static_cast<int>(100. * Preferences::ViewZoom()));
		}
		else if(preference == Preferences::ESCORTS_EXPEND_AMMO)
		{
			isOn = Preferences::Has(preference);
			text = Preferences::AmmoUsage();
		}
		else if(preference == Preferences::TURRETS_FOCUS_FIRE)
		{
			isOn = true;
			text = Preferences::Has(Preferences::TURRETS_FOCUS_FIRE) ? "focused" : "opportunistic";
		}
		else if((int)preference == REACTIVATE_HELP)
		{
			// Check how many help messages have been displayed.
			int shown = 0;
			int total = 0;
			for(int h = 0; h < Help::MAX; h++)
			{
				// Don't count certain special help messages that are always
				// active for new players.
				if((h >= Help::BASICS_1 && h <= Help::BASICS_2) ||
					(h >= Help::LOST_1 && h <= Help::LOST_7))
					continue;

				++total;
				shown += Help::Seen((Help::Topic)h);
			}
			
			if(shown)
				text = to_string(shown) + " / " + to_string(Help::MAX);
			else
			{
				isOn = true;
				text = "done";
			}
		}
		else if((int)preference == SCROLL_SPEED)
		{
			isOn = true;
			text = to_string(Preferences::ScrollSpeed());
		}
		else
		{
			isOn = Preferences::Has(preference);
			text = isOn ? "on" : "off";
		}
		
		if(preference == hoverPreference)
			table.DrawHighlight(back);
		table.Draw(title, isOn ? medium : dim);
		table.Draw(text, isOn ? bright : medium);
	}
}



void PreferencesPanel::DrawPlugins()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(-115, Table::LEFT);
	table.SetUnderline(-120, 120);
	
	int firstY = -238;
	table.DrawAt(Point(-130, firstY));
	table.DrawUnderline(medium);
	table.Draw("Installed plugins:", bright);
	table.DrawGap(5);
	
	const int MAX_TEXT_WIDTH = 230;
	const Font &font = FontSet::Get(14);
	for(const pair<string, string> &plugin : GameData::PluginAboutText())
	{
		pluginZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), plugin.first);
		
		bool isSelected = (plugin.first == selectedPlugin);
		if(isSelected || plugin.first == hoverPlugin)
			table.DrawHighlight(back);
		table.Draw(font.TruncateMiddle(plugin.first, MAX_TEXT_WIDTH), isSelected ? bright : medium);
		
		if(isSelected)
		{
			const Sprite *sprite = SpriteSet::Get(plugin.first);
			Point top(15., firstY);
			if(sprite)
			{
				Point center(130., top.Y() + .5 * sprite->Height());
				SpriteShader::Draw(sprite, center);
				top.Y() += sprite->Height() + 10.;
			}
			
			WrappedText wrap(font);
			wrap.SetWrapWidth(MAX_TEXT_WIDTH);
			static const string EMPTY = "(No description given.)";
			wrap.Wrap(plugin.second.empty() ? EMPTY : plugin.second);
			wrap.Draw(top, medium);
		}
	}
}



void PreferencesPanel::Exit()
{
	Command::SaveSettings(Files::Config() + "keys.txt");
	
	GetUI()->Pop(this);
}

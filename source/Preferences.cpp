/* Preferences.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Preferences.h"

#include "Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "Screen.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {
	bool settings[Preferences::MAX_BOOLEAN];
	int scrollSpeed = 60;
	
	const vector<double> ZOOMS = {.25, .35, .50, .70, 1.00, 1.40, 2.00};
	int zoomIndex = 4;

	static const char *BOOLEAN_NAMES[Preferences::MAX_NAMED_BOOLEAN] =
	{
		// Boolean preferences

		// Display
		"Show status overlays",
		"Highlight player's flagship",
		"Rotate flagship in HUD",
		"Show planet labels",
		"Show mini-map",
		"fullscreen", // Implicit
		"maximized", // Implicit

		// AI
		"Automatic aiming",
		"Automatic firing",
		"Escorts expend ammo",
		"Escorts use ammo frugally",
		"Turrets focus fire",
		"Damaged fighters retreat", // Hidden

		// Performance
		"Show CPU / GPU load",
		"Render motion blur",
		"Reduce large graphics",
		"Draw background haze",
		"Show hyperspace flash",

		// Other
		"Clickable radar display",
		"Hide unexplored map regions",
		"Rehire extra crew when lost",
		"Show escort systems on map",
		"Warning siren",
	};
}



void Preferences::Load()
{
	// These settings should be on by default. There is no need to specify
	// values for settings that are off by default.
	settings[AUTOMATIC_AIMING] = true;
	settings[RENDER_MOTION_BLUR] = true;
	settings[FRUGAL_ESCORTS] = true;
	settings[ESCORTS_EXPEND_AMMO] = true;
	settings[DAMAGED_FIGHTERS_RETREAT] = true;
	settings[WARNING_SIREN] = true;
	settings[SHOW_ESCORT_SYSTEMS_ON_MAP] = true;
	settings[SHOW_MINIMAP] = true;
	settings[SHOW_PLANET_LABELS] = true;
	settings[SHOW_HYPERSPACE_FLASH] = true;
	settings[DRAW_BACKGROUND_HAZE] = true;
	settings[HIDE_UNEXPLORED_MAP_REGIONS] = true;
	settings[TURRETS_FOCUS_FIRE] = true;
	
	DataFile prefs(Files::Config() + "preferences.txt");
	for(const DataNode &node : prefs)
	{
		if(node.Token(0) == "window size" && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2));
		else if(node.Token(0) == "zoom" && node.Size() >= 2)
			Screen::SetZoom(node.Value(1));
		else if(node.Token(0) == "volume" && node.Size() >= 2)
			Audio::SetVolume(node.Value(1));
		else if(node.Token(0) == "scroll speed" && node.Size() >= 2)
			scrollSpeed = node.Value(1);
		else if(node.Token(0) == "view zoom")
			zoomIndex = node.Value(1);
		else
		{
			for(int p = 0; p < MAX_BOOLEAN; p++)
				if(node.Token(0) == Name((Preference)p))
				{
					settings[p] = (node.Size() == 1 || node.Value(1));
					break;
				}
		}
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() + "preferences.txt");
	
	out.Write("volume", Audio::Volume());
	out.Write("window size", Screen::RawWidth(), Screen::RawHeight());
	out.Write("zoom", Screen::Zoom());
	out.Write("scroll speed", scrollSpeed);
	out.Write("view zoom", zoomIndex);
	
	for(int p = 0; p < MAX_BOOLEAN; p++)
		out.Write(Name((Preference)p), settings[p]);
}



bool Preferences::Has(Preference p)
{
	if (p >= Preferences::MAX_BOOLEAN)
		throw runtime_error("Unknown preference index!");
	return settings[p];
}



void Preferences::Set(Preference p, bool on)
{
	if (p >= Preferences::MAX_BOOLEAN)
		throw runtime_error("Unknown preference index!");
	settings[p] = on;
}



string Preferences::Name(Preference p)
{
	if(p < MAX_NAMED_BOOLEAN)
		return BOOLEAN_NAMES[p];
	else if(p >= SEEN_HELP_0 && p < SEEN_HELP_MAX)
		return "help: " + string(Help::TopicName((Help::Topic)(p - SEEN_HELP_0)));
	else
		throw runtime_error("Unknown preference index!");
}



void Preferences::ToggleAmmoUsage()
{
	bool expend = Has(ESCORTS_EXPEND_AMMO);
	bool frugal = Has(FRUGAL_ESCORTS);
	Preferences::Set(ESCORTS_EXPEND_AMMO, !(expend && !frugal));
	Preferences::Set(FRUGAL_ESCORTS, !expend);
}



string Preferences::AmmoUsage()
{
	return Has(ESCORTS_EXPEND_AMMO) ? Has(FRUGAL_ESCORTS) ? "frugally" : "always" : "never";
}



// Scroll speed preference.
int Preferences::ScrollSpeed()
{
	return scrollSpeed;
}



void Preferences::SetScrollSpeed(int speed)
{
	scrollSpeed = speed;
}



// View zoom.
double Preferences::ViewZoom()
{
	return ZOOMS[zoomIndex];
}



bool Preferences::ZoomViewIn()
{
	if(zoomIndex == static_cast<int>(ZOOMS.size() - 1))
		return false;
	
	++zoomIndex;
	return true;
}



bool Preferences::ZoomViewOut()
{
	if(zoomIndex == 0)
		return false;
	
	--zoomIndex;
	return true;
}

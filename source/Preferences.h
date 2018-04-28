/* Preferences.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include "Help.h"

#include <string>


class Preferences {
public:
	enum Preference {
		// Boolean preferences

		// Display
		SHOW_STATUS_OVERLAYS,
		HIGHLIGHT_PLAYERS_FLAGSHIP,
		ROTATE_FLAGSHIP_IN_HUD,
		SHOW_PLANET_LABELS,
		SHOW_MINIMAP,
		FULLSCREEN, // Implicit
		MAXIMIZED, // Implicit

		// AI
		AUTOMATIC_AIMING,
		AUTOMATIC_FIRING,
		ESCORTS_EXPEND_AMMO,
		FRUGAL_ESCORTS,
		TURRETS_FOCUS_FIRE,
		DAMAGED_FIGHTERS_RETREAT, // Hidden

		// Performance
		SHOW_CPU_GPU_LOAD,
		RENDER_MOTION_BLUR,
		REDUCE_LARGE_GRAPHICS,
		DRAW_BACKGROUND_HAZE,
		SHOW_HYPERSPACE_FLASH,

		// Other
		CLICKABLE_RADAR_DISPLAY,
		HIDE_UNEXPLORED_MAP_REGIONS,
		REHIRE_EXTRA_CREW_WHEN_LOST,
		SHOW_ESCORT_SYSTEMS_ON_MAP,
		WARNING_SIREN,

		MAX_NAMED_BOOLEAN,
		SEEN_HELP_0 = MAX_NAMED_BOOLEAN,
		SEEN_HELP_MAX = SEEN_HELP_0 + Help::MAX,

		MAX_BOOLEAN = SEEN_HELP_MAX,
		MAX = MAX_BOOLEAN,
	};

	static void Load();
	static void Save();
	
	static bool Has(Preference);
	static void Set(Preference, bool on = true);

	static std::string Name(Preference);

	// Toogle the ammo usage preferences, cycling between "never," "frugally,"
	// and "always."
	static void ToggleAmmoUsage();
	static std::string AmmoUsage();
	
	// Scroll speed preference.
	static int ScrollSpeed();
	static void SetScrollSpeed(int speed);
	
	// View zoom.
	static double ViewZoom();
	static bool ZoomViewIn();
	static bool ZoomViewOut();
};



#endif

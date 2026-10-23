/*
Multi-Stream MS
Copyright (C) 2026 Multi-Stream MS contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QPointer>

#include "multi-stream-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
OBS_MODULE_AUTHOR("Multi-Stream MS contributors")

namespace {
constexpr const char *kDockId = "multi-stream-ms";

/* QPointer turns to nullptr automatically if OBS destroys the dock first. */
QPointer<MultiStreamDock> g_dock;
bool g_dockRegistered = false;

/* Frontend events are delivered on the UI thread. */
void on_frontend_event(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
		if (g_dock)
			g_dock->reloadFromProfile();
		break;

	case OBS_FRONTEND_EVENT_PROFILE_CHANGING:
		/* Outputs of the old profile must not outlive it. */
		if (g_dock)
			g_dock->stopAllNow();
		break;

	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		/* Our outputs hold references to the shared encoders. If they
		 * kept running, the encoders would keep encoding after the main
		 * stream ended, so destinations always stop with it. */
		if (g_dock)
			g_dock->stopAllGraceful();
		break;

	case OBS_FRONTEND_EVENT_EXIT:
		/* Stop everything while the frontend is still alive, then remove
		 * the dock. Doing this in obs_module_unload is too late. */
		if (g_dock)
			g_dock->stopAllNow();
		if (g_dockRegistered) {
			obs_frontend_remove_dock(kDockId);
			g_dockRegistered = false;
		}
		break;

	default:
		break;
	}
}
} // namespace

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Multi-Stream MS";
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Send the main stream to additional RTMP destinations by sharing "
	       "the main stream's encoders.";
}

bool obs_module_load(void)
{
	auto *dock = new MultiStreamDock();
	if (obs_frontend_add_dock_by_id(kDockId, obs_module_text("MultiStreamMS.DockTitle"), dock)) {
		g_dock = dock;
		g_dockRegistered = true;
		obs_frontend_add_event_callback(on_frontend_event, nullptr);
	} else {
		/* The frontend did not adopt the widget, so we still own it. */
		delete dock;
		obs_log(LOG_WARNING, "failed to register dock '%s'", kDockId);
	}

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	/* No frontend calls here: the frontend is already gone at this point. */
	obs_log(LOG_INFO, "plugin unloaded");
}

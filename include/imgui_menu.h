#pragma once

#include "app_info.h"
#include "simulation_metadata.h"
#include "playback_state.h"
#include "render_settings.h"
#include "camera.h"
#include "imgui.h"

void draw_imgui_menu(AppInfo& info, PlaybackState& playback_state, SimulationMetadata& metadata, RenderSettings& settings, Camera& camera);
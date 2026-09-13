#pragma once

#include "app_info.h"
#include "simulation_metadata.h"
#include "playback_state.h"
#include "render_settings.h"
#include "imgui.h"

void draw_imgui_menu(AppInfo& info, PlaybackState& playback_state, SimulationMetadata& metadata, RenderSettings& settings, Camera& camera) {
    // Camera data
    float yaw{camera.getYaw()};
    float pitch{camera.getPitch()};
    float distance{camera.getDistance()};
    
    // Simulation data
    float duration{playback_state.frameDuration};
    int qtd_frames{info.get_qtd_frames()};
    uint32_t count{playback_state.currentFrame};
    float ratio{static_cast<float>(count) / static_cast<float>(qtd_frames)};
    uint32_t current_frame{playback_state.currentFrame};

    ImGui::NewFrame();

    ImGui::Begin("Shallow Water Equations - Vulkan Renderer");

    ImGui::Text("Playback controls");
    if (ImGui::Button(playback_state.isPaused ? "Play" : "Pause")) {
        playback_state.isPaused = !playback_state.isPaused;
    }
    ImGui::InputFloat("Minimum time on each simulation frame", &duration);
    ImGui::InputScalar("Current Frame", ImGuiDataType_U32, &current_frame);

    ImGui::Separator();

    ImGui::Text("Simulation data");
    ImGui::Text("Rows: %d", metadata.rows);
    ImGui::Text("Cols: %d", metadata.cols);
    ImGui::Text("Bathymetry Z Min: %f", metadata.bathymetryZMin);
    ImGui::Text("Bathymetry Z Max: %f", metadata.bathymetryZMax);
    ImGui::Text("X Extent: %f", metadata.xExtent);
    ImGui::Text("Y Extent: %f", metadata.yExtent);

    ImGui::Separator();

    ImGui::Text("Frame rate: %d FPS", info.get_fps());
    ImGui::Text("Amount of simulation frames: %d", info.get_qtd_frames());
    ImGui::Text("Current simulation frame: %d", count);
    ImGui::Text("Simulation Progress");
    ImGui::ProgressBar(ratio);

    ImGui::Separator();
    
    ImGui::Text("Camera settings");
    ImGui::InputFloat("Yaw", &yaw);
    ImGui::InputFloat("Pitch", &pitch);
    ImGui::InputFloat("Distance", &distance);

    ImGui::Separator();

    ImGui::Text("Rendering settings");
    ImGui::InputFloat("Surface Z Scale", &settings.surfaceZScale);
    ImGui::InputFloat("Bathymetry Z Scale", &settings.bathymetryZScale);
    ImGui::InputInt("Surface Render Mode", &settings.surfaceRenderMode);
    ImGui::InputInt("Bathymetry Render Mode", &settings.bathymetryRenderMode);

    ImGui::End();

    ImGui::Render();

    playback_state.frameDuration = duration;
    playback_state.currentFrame = current_frame;

    camera.setYaw(yaw);
    camera.setPitch(pitch);
    camera.setDistance(distance);
}
#include "gui/gui_manager.h"
#include "gui/pedal_board.h"
#include "gui/file_dialog.h"
#include "gui/theme.h"
#include "preset_manager.h"
#include <imgui.h>
#include <SDL2/SDL.h>
#include <cstdio>
#include <string>

namespace Amplitron {

void GuiManager::render_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Preset...")) {
                gui_presets_.begin_new_preset();
                show_save_preset_ = true;
            }
            if (ImGui::MenuItem("Save Preset...", "Ctrl+S")) {
                gui_presets_.begin_save_preset();
                show_save_preset_ = true;
            }
            if (ImGui::MenuItem("Load Preset...", "Ctrl+O")) {
                show_load_preset_ = true;
                gui_presets_.ensure_factory_presets();
                gui_presets_.refresh_presets(true);
            }
            bool has_selected_preset = gui_presets_.selected_preset_index() >= 0 &&
                                       gui_presets_.selected_preset_index() < gui_presets_.preset_count();
            if (ImGui::MenuItem("Delete Selected Preset", nullptr, false, has_selected_preset)) {
                gui_presets_.delete_preset_by_index(gui_presets_.selected_preset_index());
            }
            ImGui::Separator();
#ifndef AMPLITRON_NO_DESKTOP_SHELL
            if (ImGui::MenuItem("Change Presets Directory...")) {
                std::string chosen = show_folder_dialog("Select Presets Directory");
                if (!chosen.empty()) {
                    PresetManager::set_presets_dir(chosen);
                    PresetManager::save_config();
                    gui_presets_.refresh_presets(false);
                }
            }
            if (ImGui::MenuItem("Reset to Default Presets Directory")) {
                PresetManager::set_presets_dir("");
                PresetManager::save_config();
                gui_presets_.refresh_presets(false);
            }
#endif
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) show_settings_ = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                SDL_Event quit_event;
                quit_event.type = SDL_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            bool can_undo = command_history_.can_undo();
            bool can_redo = command_history_.can_redo();

            const char* undo_label = command_history_.undo_description();
            char undo_buf[64] = "Undo";
            if (undo_label) snprintf(undo_buf, sizeof(undo_buf), "Undo %s", undo_label);

            const char* redo_label = command_history_.redo_description();
            char redo_buf[64] = "Redo";
            if (redo_label) snprintf(redo_buf, sizeof(redo_buf), "Redo %s", redo_label);

            if (ImGui::MenuItem(undo_buf, "Ctrl+Z", false, can_undo)) {
                if (command_history_.undo() && pedal_board_) {
                    pedal_board_->rebuild_widgets();
                }
            }
            if (ImGui::MenuItem(redo_buf, "Ctrl+Shift+Z", false, can_redo)) {
                if (command_history_.redo() && pedal_board_) {
                    pedal_board_->rebuild_widgets();
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Audio")) {
            if (engine_.is_running()) {
                if (ImGui::MenuItem("Stop Audio")) engine_.stop();
            } else {
                if (ImGui::MenuItem("Start Audio")) {
                    engine_.restart();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Restart Audio")) {
                engine_.restart();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Utilities")) {
            if (ImGui::MenuItem("Open Tuner", nullptr, show_tuner_)) {
                gui_tuner_.toggle(show_tuner_);
            }
            if (ImGui::MenuItem("MIDI Settings", nullptr, show_midi_)) {
                show_midi_ = !show_midi_;
            }
            ImGui::EndMenu();
        }

        // Status bar (right side)
        float bar_w = ImGui::GetWindowWidth();

        // MIDI status indicator
        ImGui::SameLine(bar_w - 450);
        if (midi_manager_.is_port_open()) {
            ImGui::TextColored(Theme::Live(), "MIDI");
        } else {
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "MIDI");
        }

        // Recording indicator
        ImGui::SameLine(bar_w - 400);
        if (engine_.recorder().is_recording()) {
            float t = static_cast<float>(ImGui::GetTime());
            ImGui::TextColored(Theme::RecBlink(t), "REC");
            ImGui::SameLine();
            ImGui::Text("%.1fs", engine_.recorder().get_duration());
        }

        // Audio status
        ImGui::SameLine(bar_w - 200);
        if (engine_.is_running()) {
            ImGui::TextColored(Theme::Live(), "LIVE");
        } else {
            ImGui::TextColored(Theme::Stopped(), "STOPPED");
        }
        ImGui::SameLine();
        ImGui::Text("%dHz", engine_.get_sample_rate());

        bool show_update = false;
        std::string update_version;
        std::string update_url;
        {
            std::lock_guard<std::mutex> lock(update_mutex_);
            if (has_new_release_) {
                show_update = true;
                update_version = new_release_version_;
                update_url = new_release_url_;
            }
        }

        if (show_update) {
            ImGui::SameLine(bar_w - 600);
            ImGui::TextColored(Theme::GoldHot(), "New Release Available: %s", update_version.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to open release in browser");
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            if (ImGui::IsItemClicked()) {
#if defined(_WIN32)
                std::string cmd = "start " + update_url;
                std::system(cmd.c_str());
#elif defined(__APPLE__) && !TARGET_OS_IOS
                std::string cmd = "open " + update_url;
                std::system(cmd.c_str());
#elif defined(__linux__)
                std::string cmd = "xdg-open " + update_url;
                std::system(cmd.c_str());
#endif
            }
        }

        ImGui::EndMainMenuBar();
    }

    // Error banner when audio is stopped
    if (!engine_.is_running()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.35f, 0.08f, 0.08f, 0.95f));
        ImGui::BeginChild("AudioErrorBanner", ImVec2(0, 36), true);
        ImGui::TextColored(Theme::Stopped(), "Audio stream is STOPPED.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Restart Audio")) {
            engine_.restart();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Settings")) {
            show_settings_ = true;
        }
        std::string err = engine_.get_last_error();
        if (!err.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(Theme::GoldHot(), "  %s", err.c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

} // namespace Amplitron

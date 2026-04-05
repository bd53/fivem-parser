#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "imgui.h"
#include <commdlg.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <string>

extern "C" {
#include "resource.h"
#include "parser.h"
}

static char s_path[MAX_PATH] = "";
static std::string s_output;
static bool s_remove_ts = false;
static int s_chars = 0, s_lines = 0;

static bool s_show_filter = false;
static char s_kw_buf[4096] = "";
static std::string s_flt_source;
static std::string s_flt_output;
static int s_flt_chars = 0, s_flt_lines = 0;

static bool s_open_backup = false;
static bool s_bkp_enabled = false;
static char s_bkp_path[MAX_PATH] = "";
static bool s_bkp_interval_en = false;
static int s_bkp_interval_min = 10;

static bool s_open_about = false;

static void count_text(const std::string &t, int &c, int &l) {
        c = (int)t.size();
        l = 0;
        for (size_t i = 0; i < t.size(); i++)
                if (t[i] == '\n') l++;
}

static const char *stristr(const char *h, const char *n) {
        if (!*n) return h;
        for (; *h; h++) {
                const char *a = h, *b = n;
                while (*a && *b &&
                       tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
                        a++; b++;
                }
                if (!*b) return h;
        }
        return NULL;
}

static void do_find_latest(GLFWwindow *w) {
        char path[MAX_PATH * 2];
        if (!find_latest_log(path, sizeof(path))) {
                MessageBoxA(glfwGetWin32Window(w), "No FiveM session logs found.\n\n" "Expected location:\n" "  %LOCALAPPDATA%\\FiveM\\FiveM.app\\logs\\\n\n" "Launch FiveM and join a server first.", "No Logs Found", MB_ICONWARNING);
                return;
        }
        snprintf(s_path, sizeof(s_path), "%s", path);
}

static void do_browse(GLFWwindow *w) {
        char initdir[MAX_PATH + 32] = "";
        get_fivem_logs_dir(initdir, sizeof(initdir));
        OPENFILENAMEA ofn = {};
        char fname[MAX_PATH] = "";
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window(w);
        ofn.lpstrFilter = "FiveM Session Logs\0CitizenFX_log_*.log\0" "All Log Files\0*.log\0" "All Files\0*.*\0";
        ofn.lpstrFile = fname;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST;
        ofn.lpstrTitle = "Select a FiveM Session Log";
        ofn.lpstrInitialDir = initdir[0] ? initdir : NULL;
        if (GetOpenFileNameA(&ofn))
                snprintf(s_path, sizeof(s_path), "%s", fname);
}

static void do_parse(GLFWwindow *w) {
        if (!s_path[0]) {
                MessageBoxA(glfwGetWin32Window(w), "Please select or auto-detect a log file first.", "No file", MB_ICONWARNING);
                return;
        }
        char *result = parse_log_file(s_path, s_remove_ts ? 1 : 0);
        if (!result) {
                MessageBoxA(glfwGetWin32Window(w), "Failed to open log file.\n\n" "Make sure FiveM has been launched at least once " "and the file exists at the specified path.", "Error", MB_ICONERROR);
                return;
        }
        s_output = result;
        free(result);
        count_text(s_output, s_chars, s_lines);
}

static void do_save(GLFWwindow *w, const std::string &text, const char *default_name) {
        if (text.empty()) {
                MessageBoxA(glfwGetWin32Window(w), "Nothing to save - parse a log first.", "Empty", MB_ICONINFORMATION);
                return;
        }
        OPENFILENAMEA ofn = {};
        char fname[MAX_PATH];
        strncpy(fname, default_name, MAX_PATH - 1);
        fname[MAX_PATH - 1] = '\0';
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window(w);
        ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files\0*.*\0";
        ofn.lpstrFile = fname;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "txt";
        if (!GetSaveFileNameA(&ofn))
                return;
        FILE *f = fopen(fname, "wb");
        if (f) {
                fwrite(text.c_str(), 1, text.size(), f);
                fclose(f);
                MessageBoxA(glfwGetWin32Window(w), "Saved.", "Saved", MB_ICONINFORMATION);
        } else {
                MessageBoxA(glfwGetWin32Window(w), "Failed to save file.", "Error", MB_ICONERROR);
        }
}

static void do_copy(GLFWwindow *w, const std::string &text) {
        if (!text.empty())
                glfwSetClipboardString(w, text.c_str());
}

static void apply_filter(void) {
        if (s_flt_source.empty()) {
                s_flt_output.clear();
                s_flt_chars = s_flt_lines = 0;
                return;
        }
        if (!s_kw_buf[0]) {
                s_flt_output = s_flt_source;
                count_text(s_flt_output, s_flt_chars, s_flt_lines);
                return;
        }
        char tmp[4096];
        strncpy(tmp, s_kw_buf, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        const char *kw_list[256];
        int kw_count = 0;
        char *tok = strtok(tmp, "\r\n");
        while (tok && kw_count < 256) {
                while (*tok == ' ') tok++;
                if (*tok) kw_list[kw_count++] = tok;
                tok = strtok(NULL, "\r\n");
        }
        if (kw_count == 0) {
                s_flt_output = s_flt_source;
                count_text(s_flt_output, s_flt_chars, s_flt_lines);
                return;
        }
        std::string result;
        result.reserve(s_flt_source.size());
        const char *ls = s_flt_source.c_str();
        while (*ls) {
                const char *le = strstr(ls, "\r\n");
                int ll = le ? (int)(le - ls) : (int)strlen(ls);
                char lc[8192];
                int cl = ll < (int)sizeof(lc) - 1 ? ll : (int)sizeof(lc) - 1;
                memcpy(lc, ls, (size_t)cl);
                lc[cl] = '\0';
                bool match = false;
                for (int i = 0; i < kw_count; i++) {
                        if (stristr(lc, kw_list[i])) {
                                match = true;
                                break;
                        }
                }
                if (match) {
                        result.append(ls, (size_t)ll);
                        if (le) result.append("\r\n");
                }
                ls = le ? le + 2 : ls + ll;
        }
        s_flt_output = result;
        count_text(s_flt_output, s_flt_chars, s_flt_lines);
}

void ui_init(GLFWwindow *w) {
        (void)w;
        s_remove_ts = (g_config.remove_timestamps != 0);
        char path[MAX_PATH * 2];
        if (find_latest_log(path, sizeof(path)))
                snprintf(s_path, sizeof(s_path), "%s", path);
}

bool ui_get_remove_timestamps(void) {
        return s_remove_ts;
}

void ui_shutdown(void) {}

void ui_render(GLFWwindow *w) {
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::Begin("##main", nullptr, wf);
        if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Filter Chat Log")) {
                        s_show_filter = true;
                        s_flt_source = s_output;
                        s_flt_output = s_output;
                        s_kw_buf[0] = '\0';
                        count_text(s_flt_output, s_flt_chars, s_flt_lines);
                }
                if (ImGui::MenuItem("Backup Settings")) {
                        s_open_backup = true;
                        s_bkp_enabled = (g_config.backup_enabled != 0);
                        snprintf(s_bkp_path, sizeof(s_bkp_path), "%s", g_config.backup_path);
                        s_bkp_interval_en = (g_config.interval_enabled != 0);
                        s_bkp_interval_min = g_config.interval_minutes;
                }
                if (ImGui::MenuItem("About"))
                        s_open_about = true;
                ImGui::EndMenuBar();
        }
        ImGui::Text("Log File:");
        ImGui::SameLine();
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(avail - 170);
        ImGui::InputText("##path", s_path, sizeof(s_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(75, 0)))
                do_browse(w);
        ImGui::SameLine();
        if (ImGui::Button("Find Latest", ImVec2(85, 0)))
                do_find_latest(w);
        float footer = ImGui::GetFrameHeightWithSpacing() * 2 + 8;
        ImGui::BeginChild("##output", ImVec2(0, -footer),
                ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        if (!s_output.empty())
                ImGui::TextUnformatted(s_output.c_str(), s_output.c_str() + s_output.size());
        ImGui::EndChild();
        ImGui::Text("%d characters and %d lines", s_chars, s_lines);
        ImGui::Checkbox("Remove timestamps", &s_remove_ts);
        ImGui::SameLine();
        float bw = 72;
        float sp = ImGui::GetStyle().ItemSpacing.x;
        float total = bw * 3 + sp * 2;
        ImGui::SameLine(ImGui::GetWindowWidth() - total - ImGui::GetStyle().WindowPadding.x);
        if (ImGui::Button("PARSE", ImVec2(bw, 0)))
                do_parse(w);
        ImGui::SameLine();
        if (ImGui::Button("SAVE AS", ImVec2(bw, 0)))
                do_save(w, s_output, "chat_log.txt");
        ImGui::SameLine();
        if (ImGui::Button("COPY", ImVec2(bw, 0)))
                do_copy(w, s_output);
        if (s_open_backup) {
                ImGui::OpenPopup("Backup Settings");
                s_open_backup = false;
        }
        if (s_open_about) {
                ImGui::OpenPopup("About");
                s_open_about = false;
        }
        if (ImGui::BeginPopupModal("Backup Settings", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Checkbox("Enable automatic backup on game close", &s_bkp_enabled);
                ImGui::Spacing();
                ImGui::Text("Backup folder:");
                ImGui::SetNextItemWidth(380);
                ImGui::InputText("##bkp_path", s_bkp_path, sizeof(s_bkp_path));
                ImGui::Spacing();
                ImGui::Checkbox("Enable interval backup while playing", &s_bkp_interval_en);
                if (!s_bkp_interval_en) ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt("Interval (minutes)", &s_bkp_interval_min);
                if (!s_bkp_interval_en) ImGui::EndDisabled();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("OK", ImVec2(80, 0))) {
                        if (s_bkp_interval_min < 1) s_bkp_interval_min = 1;
                        if (s_bkp_interval_min > 60) s_bkp_interval_min = 60;
                        g_config.backup_enabled = s_bkp_enabled ? 1 : 0;
                        snprintf(g_config.backup_path, sizeof(g_config.backup_path), "%s", s_bkp_path);
                        g_config.interval_enabled = s_bkp_interval_en ? 1 : 0;
                        g_config.interval_minutes = s_bkp_interval_min;
                        config_save();
                        ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(80, 0)))
                        ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("About", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text(APP_TITLE);
                ImGui::Text("Version " APP_VERSION);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("https://t.me/enclaimed");
                ImGui::Text("https://github.com/bd53");
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("OK", ImVec2(120, 0)))
                        ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
        }
        ImGui::End();
        if (s_show_filter) {
                ImGui::SetNextWindowSize(ImVec2(640, 490), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Filter Chat Log", &s_show_filter)) {
                        ImGui::Text("Keywords (one per line):");
                        ImGui::InputTextMultiline("##flt_kw", s_kw_buf, sizeof(s_kw_buf), ImVec2(-1, 80));
                        if (ImGui::Button("FILTER", ImVec2(90, 0)))
                                apply_filter();
                        ImGui::SameLine();
                        if (ImGui::Button("CLEAR", ImVec2(90, 0))) {
                                s_kw_buf[0] = '\0';
                                s_flt_output = s_flt_source;
                                count_text(s_flt_output, s_flt_chars, s_flt_lines);
                        }
                        float flt_footer = ImGui::GetFrameHeightWithSpacing() + 8;
                        ImGui::BeginChild("##flt_out", ImVec2(0, -flt_footer), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
                        if (!s_flt_output.empty())
                                ImGui::TextUnformatted(s_flt_output.c_str(), s_flt_output.c_str() + s_flt_output.size());
                        ImGui::EndChild();
                        ImGui::Text("%d characters and %d lines", s_flt_chars, s_flt_lines);
                        ImGui::SameLine();
                        float fw = 90;
                        float fsp = ImGui::GetStyle().ItemSpacing.x;
                        ImGui::SameLine(ImGui::GetWindowWidth() - fw * 2 - fsp - ImGui::GetStyle().WindowPadding.x);
                        if (ImGui::Button("SAVE AS##flt", ImVec2(fw, 0)))
                                do_save(w, s_flt_output, "filtered_chat.txt");
                        ImGui::SameLine();
                        if (ImGui::Button("COPY##flt", ImVec2(fw, 0)))
                                do_copy(w, s_flt_output);
                }
                ImGui::End();
        }
}

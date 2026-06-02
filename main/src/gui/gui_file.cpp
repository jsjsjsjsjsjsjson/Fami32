#include "gui_file.h"
#include "gui_input.h"
#include "gui_menu.h"
#include "vgm_exporter.h"
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>

namespace {

constexpr int kScreenW = 128;
constexpr int kHeaderH = 9;
constexpr int kPathY = 10;
constexpr int kListY = 21;
constexpr int kRowH = 9;
constexpr int kRowsPerPage = 5;
constexpr int kListBottomY = 64;
constexpr int kMaxDirEntries = 256;

struct FileEntry {
    char name[256];
    bool is_dir;
};

class ExportTaskFreeze {
public:
    ExportTaskFreeze() {
        if (SOUND_TASK_HD != NULL) {
            vTaskSuspend(SOUND_TASK_HD);
            sound_suspended_ = true;
        }
        if (KEYPAD_TASK_HD != NULL) {
            vTaskSuspend(KEYPAD_TASK_HD);
            keypad_suspended_ = true;
        }
    }

    ~ExportTaskFreeze() {
        if (keypad_suspended_) {
            vTaskResume(KEYPAD_TASK_HD);
        }
        if (sound_suspended_) {
            vTaskResume(SOUND_TASK_HD);
        }
    }

private:
    bool sound_suspended_ = false;
    bool keypad_suspended_ = false;
};

void free_namelist(struct dirent **namelist, int n) {
    if (!namelist) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        free(namelist[i]);
    }
    free(namelist);
}

int compare_file_entries(const void *lhs, const void *rhs) {
    const FileEntry *a = static_cast<const FileEntry *>(lhs);
    const FileEntry *b = static_cast<const FileEntry *>(rhs);

    if (a->is_dir != b->is_dir) {
        return a->is_dir ? -1 : 1;
    }

    return strcmp(a->name, b->name);
}

void copy_tail_text(char *dest, size_t dest_size, const char *src, size_t max_chars) {
    if (dest_size == 0) {
        return;
    }

    size_t src_len = strlen(src);
    if (src_len <= max_chars || max_chars < 4) {
        snprintf(dest, dest_size, "%s", src);
        return;
    }

    size_t tail_len = max_chars - 3;
    snprintf(dest, dest_size, "...%s", src + src_len - tail_len);
}

void append_text(char *dest, size_t dest_size, const char *src) {
    if (dest_size == 0) {
        return;
    }

    size_t dest_len = strlen(dest);
    if (dest_len >= dest_size - 1) {
        return;
    }

    size_t write_pos = dest_len;
    while (*src != '\0' && write_pos < dest_size - 1) {
        dest[write_pos++] = *src++;
    }
    dest[write_pos] = '\0';
}

void build_child_path(char *dest, size_t dest_size, const char *dir, const char *name) {
    if (dest_size == 0) {
        return;
    }

    dest[0] = '\0';
    if (strcmp(dir, "/") == 0) {
        append_text(dest, dest_size, "/");
    } else {
        append_text(dest, dest_size, dir);
        append_text(dest, dest_size, "/");
    }
    append_text(dest, dest_size, name);
}

const char *path_basename(const char *path) {
    if (path == nullptr || path[0] == '\0') {
        return "";
    }

    const char *last_slash = strrchr(path, '/');
    return last_slash == nullptr ? path : last_slash + 1;
}

void draw_right_aligned_text(const char *text, int right_x, int y) {
    int16_t x1, y1;
    uint16_t text_width, text_height;
    display.getTextBounds(text, 0, y, &x1, &y1, &text_width, &text_height);
    display.setCursor(right_x - text_width, y);
    display.print(text);
}

void draw_file_icon(int x, int y, bool is_dir, uint16_t color) {
    if (is_dir) {
        display.drawRect(x, y + 2, 7, 5, color);
        display.drawFastHLine(x + 1, y + 1, 3, color);
        display.drawPixel(x + 4, y + 2, color);
    } else {
        display.drawRect(x + 1, y, 5, 7, color);
        display.drawPixel(x + 5, y + 1, !color);
        display.drawPixel(x + 4, y, !color);
        display.drawPixel(x + 5, y, color);
    }
}

void draw_scrollbar(int total, int page_start) {
    if (total <= kRowsPerPage) {
        return;
    }

    const int track_y = kListY;
    const int track_h = kListBottomY - kListY - 1;
    display.drawFastVLine(126, track_y, track_h, 1);

    int thumb_h = (track_h * kRowsPerPage) / total;
    if (thumb_h < 4) {
        thumb_h = 4;
    }
    if (thumb_h > track_h) {
        thumb_h = track_h;
    }

    int max_start = total - kRowsPerPage;
    int thumb_y = track_y;
    if (max_start > 0) {
        thumb_y += ((track_h - thumb_h) * page_start) / max_start;
    }
    display.fillRect(125, thumb_y, 3, thumb_h, 1);
}

void draw_file_selector(const char *current_path, const FileEntry entries[], int entry_count, int selected, int page_start) {
    display.clearDisplay();
    display.setTextWrap(false);

    display.fillRect(0, 0, kScreenW, kHeaderH, 1);
    display.setFont(&rismol57);
    display.setTextColor(0);
    display.setCursor(1, 1);
    display.print("FILES");

    char counter[24];
    snprintf(counter, sizeof(counter), "%02d/%02d", entry_count > 0 ? selected + 1 : 0, entry_count);
    display.setFont(&font4x6);
    draw_right_aligned_text(counter, 126, 2);
    display.setTextColor(1);

    char path_text[36];
    copy_tail_text(path_text, sizeof(path_text), current_path, 29);
    draw_file_icon(1, kPathY, true, 1);
    display.setFont(&font4x6);
    display.setCursor(11, kPathY + 1);
    display.print(path_text);
    display.drawFastHLine(0, 19, kScreenW, 1);

    if (entry_count == 0) {
        display.setFont(&rismol57);
        display.setCursor(25, 34);
        display.print("EMPTY FOLDER");
    } else {
        for (int row = 0; row < kRowsPerPage; ++row) {
            int index = page_start + row;
            if (index >= entry_count) {
                break;
            }

            int y = kListY + (row * kRowH);
            bool is_selected = index == selected;
            uint16_t color = is_selected ? 0 : 1;

            if (is_selected) {
                display.fillRect(0, y - 1, 124, kRowH, 1);
                display.setTextColor(0);
            } else {
                display.setTextColor(1);
            }

            draw_file_icon(2, y, entries[index].is_dir, color);

            char name_text[32];
            if (entries[index].is_dir) {
                char dir_name[260];
                snprintf(dir_name, sizeof(dir_name), "%s/", entries[index].name);
                copy_tail_text(name_text, sizeof(name_text), dir_name, 28);
            } else {
                copy_tail_text(name_text, sizeof(name_text), entries[index].name, 28);
            }

            display.setFont(&rismol35);
            display.setCursor(12, y + 1);
            display.print(name_text);
        }
    }

    display.setTextColor(1);
    draw_scrollbar(entry_count, page_start);
    display.display();
}

bool open_selected_ftm_file(const char *file, void *user) {
    (void)user;
    drawPopupBox("READING...");
    int ret = ftm.open_ftm(file);
    ESP_LOGI("OPEN_FILE", "Open file %s...", file);
    if (ret) {
        // Handle file open error codes
        switch (ret) {
            case FILE_OPEN_ERROR:
                drawPopupBox("OPEN FILE ERROR");
                ESP_LOGE("OPEN_FILE", "Cannot open %s", file);
                break;
            case FTM_NOT_VALID:
                drawPopupBox("NOT A VALID FTM FILE");
                ESP_LOGE("OPEN_FILE", "%s is not a valid FTM file", file);
                break;
            case NO_SUPPORT_VERSION:
                drawPopupBox("UNSUPPORTED .FTM VERSION");
                ESP_LOGE("OPEN_FILE", "%s version not supported", file);
                break;
            default:
                break;
        }
        display.display();
        ftm.new_ftm();
        vTaskDelay(1024);
        return false;
    }

    // File opened successfully, now read all content
    ret = ftm.read_ftm_all();
    ESP_LOGI("OPEN_FILE", "Reading FTM content...");
    if (ret) {
        // Handle file content read errors (e.g., unsupported features)
        switch (ret) {
            case NO_SUPPORT_EXTCHIP:
                drawPopupBox("EXT-CHIP NOT SUPPORTED");
                ESP_LOGE("OPEN_FILE", "File uses unsupported extension chip");
                break;
            case NO_SUPPORT_MULTITRACK:
                drawPopupBox("MULTI-TRACK NOT SUPPORTED");
                ESP_LOGE("OPEN_FILE", "File uses unsupported multi-track");
                break;
            default:
                break;
        }
        display.display();
        ftm.new_ftm();
        vTaskDelay(1024);
        return false;
    }
    return true;
}

}

// File selection UI: Browse files and directories starting from basePath. Returns chosen file path or NULL if cancelled.
const char* file_select(const char *basePath, file_select_accept_fn accept, void *user) {
    display.setFont(&rismol35);
    struct dirent **namelist = NULL;
    int n = 0;

    static char current_path[PATH_MAX];
    strncpy(current_path, basePath, PATH_MAX - 1);
    current_path[PATH_MAX - 1] = '\0';

    static char fullpath[1024];
    static char selected_file[PATH_MAX];

    int selected = 0;
    int top = 0;
    bool directory_changed = true;

    // Directory cache to store entries (max 256 entries)
    static FileEntry dir_cache[kMaxDirEntries];
    int dir_cache_size = 0;

    for (;;) {
        if (directory_changed) {
            // Free previous scan results
            if (namelist) {
                free_namelist(namelist, n);
                namelist = NULL;
                n = 0;
            }
            // Open current directory
            DIR *dir = opendir(current_path);
            if (!dir) {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(1);
                display.setCursor(0, 0);
                display.print("ERROR OPENING DIRECTORY:");
                display.setCursor(0, 8);
                display.print(current_path);
                display.display();
                vTaskDelay(1024);
                return NULL;
            }
            // Scan directory entries
            n = scandir(current_path, &namelist, NULL, alphasort);
            closedir(dir);
            if (n < 0) {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(1);
                display.setCursor(0, 0);
                display.print("ERROR READING DIRECTORY");
                display.display();
                vTaskDelay(1024);
                return NULL;
            }
            // Populate cache
            dir_cache_size = 0;
            for (int i = 0; i < n && dir_cache_size < kMaxDirEntries; ++i) {
                if (strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
                    continue;
                }
                snprintf(dir_cache[dir_cache_size].name, sizeof(dir_cache[dir_cache_size].name), "%s", namelist[i]->d_name);
                build_child_path(fullpath, sizeof(fullpath), current_path, namelist[i]->d_name);
                struct stat st;
                dir_cache[dir_cache_size].is_dir = (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode));
                dir_cache_size++;
            }
            qsort(dir_cache, dir_cache_size, sizeof(dir_cache[0]), compare_file_entries);
            selected = 0;
            top = 0;
            directory_changed = false;
        }

        // Adjust scroll window based on selected index
        if (selected < top) {
            top = selected;
        } else if (selected >= top + kRowsPerPage) {
            top = selected - kRowsPerPage + 1;
        }

        draw_file_selector(current_path, dir_cache, dir_cache_size, selected, top);

        // Handle keypad input for navigation and actions
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                switch (e.bit.KEY) {
                    case KEY_UP:
                        if (dir_cache_size > 0) {
                            selected = (selected > 0) ? selected - 1 : dir_cache_size - 1;
                        }
                        break;
                    case KEY_DOWN:
                        if (dir_cache_size > 0) {
                            selected = (selected < dir_cache_size - 1) ? selected + 1 : 0;
                        }
                        break;
                    case KEY_MENU: {
                        if (dir_cache_size <= 0) {
                            break;
                        }
                        // Options menu for file entry (rename/delete)
                        static const char *opt_str[2] = {"RENAME", "DELETE"};
                        int ret = menu("FILE OPTION", opt_str, 2, NULL, 54, 29);
                        drawPopupBox("PLEASE WAIT...");
                        char *selName = dir_cache[selected].name;
                        static char selPath[PATH_MAX];
                        build_child_path(selPath, PATH_MAX, current_path, selName);
                        strncpy(selected_file, selPath, PATH_MAX - 1);
                        selected_file[PATH_MAX - 1] = '\0';
                        if (ret == 0) {
                            // Rename option
                            char new_name[256];
                            snprintf(new_name, sizeof(new_name), "%s", dir_cache[selected].name);
                            displayKeyboard("Rename", new_name, 255);
                            char new_path[PATH_MAX];
                            build_child_path(new_path, PATH_MAX, current_path, new_name);
                            rename(selected_file, new_path);
                            directory_changed = true;
                        } else if (ret == 1) {
                            // Delete option
                            drawPopupBox("PLEASE WAIT...");
                            remove(selected_file);
                            directory_changed = true;
                        }
                        directory_changed = true;
                        } break;
                    case KEY_OK:
                        if (dir_cache_size <= 0) {
                            break;
                        } else {
                            // Open directory or select file
                            char *selName = dir_cache[selected].name;
                            static char selPath[PATH_MAX];
                            build_child_path(selPath, PATH_MAX, current_path, selName);
                            if (dir_cache[selected].is_dir) {
                                // Enter directory
                                strncpy(current_path, selPath, PATH_MAX - 1);
                                current_path[PATH_MAX - 1] = '\0';
                                directory_changed = true;
                            } else {
                                // File selected
                                strncpy(selected_file, selPath, PATH_MAX - 1);
                                selected_file[PATH_MAX - 1] = '\0';
                                if (accept && !accept(selected_file, user)) {
                                    break;
                                }
                                // Clean up name list memory
                                free_namelist(namelist, n);
                                namelist = NULL;
                                n = 0;
                                return selected_file;
                            }
                        }
                        break;
                    case KEY_BACK:
                        // Go up to parent directory or exit if at base
                        if (strcmp(current_path, basePath) == 0) {
                            free_namelist(namelist, n);
                            return NULL;
                        }
                        // Remove last path component
                        {
                            char *lastSlash = strrchr(current_path, '/');
                            if (lastSlash && lastSlash != current_path) {
                                *lastSlash = '\0';
                            } else {
                                // Reached root
                                strcpy(current_path, "/");
                            }
                            selected = 0;
                            top = 0;
                            directory_changed = true;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        // Allow touch notes while browsing (optional, not affecting file selection)
        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }
        vTaskDelay(1);
    }
}

// Open File dialog: stops sound, opens file selector, loads the selected file.
void open_file_page() {
    drawPopupBox("LOADING...");
    pause_sound();
    const char* file = file_select("/flash", open_selected_ftm_file, NULL);
    if (file == NULL) {
        return; // user cancelled
    }
    player.reload(); // Reload player with new file data
}

static bool vgm_export_progress(uint32_t ticks, uint32_t samples, void *user) {
    (void)user;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 9, 1);
    display.setTextColor(0);
    display.setCursor(1, 1);
    display.setFont(&rismol57);
    display.print("EXPORTING VGM...");
    display.setTextColor(1);
    display.setCursor(0, 11);
    display.printf("%lu ticks\n%.02fs", (unsigned long)ticks, (float)samples / 44100.0f);
    display.display();
    vTaskDelay(1);
    return true;
}

void export_vgm_page() {
    pause_sound();

    char current_name[256];
    char target_path[256];

    if (strlen(ftm.current_file) == 0) {
        strcpy(current_name, "Untitled");
    } else {
        strcpy(current_name, path_basename(ftm.current_file));
        if (strlen(current_name) > 4) {
            current_name[strlen(current_name) - 4] = '\0';
        }
    }

    displayKeyboard("EXPORT VGM...", current_name, 255);
    snprintf(target_path, sizeof(target_path), "/flash/%s.vgm", current_name);
    ESP_LOGI("VGM_EXPORT", "Export NES VGM to %s", target_path);

    drawPopupBox("EXPORTING VGM...");
    display.display();

    display.setFont(&rismol57);

    vgm_export_result_t result;
    {
        ExportTaskFreeze freeze;
        result = export_nes_vgm(&ftm, target_path, vgm_export_progress, NULL);
    }

    display.setFont(&rismol35);

    if (result == VGM_EXPORT_OK) {
        drawPopupBox("VGM EXPORTED");
    } else {
        drawPopupBox(vgm_export_result_str(result));
        ESP_LOGE("VGM_EXPORT", "Export failed: %s", vgm_export_result_str(result));
    }
    display.display();
    vTaskDelay(1024);

    player.reload();
}

// File menu: provides options to create new, open existing, save, save as, and export NES VGM.
void menu_file() {
    static const char *file_menu_str[5] = {"NEW", "OPEN", "SAVE", "SAVE AS", "EXPORT VGM"};
    int ret = menu("FILE", file_menu_str, 5, NULL, 64, 45);
    static char current_name[256];
    static char target_path[256];
    switch (ret) {
        case 0:  // NEW
            inst_sel_pos = 0;
            pause_sound();
            ftm.new_ftm();
            player.reload();
            break;
        case 1:  // OPEN
            inst_sel_pos = 0;
            open_file_page();
            break;
        case 2:  // SAVE
            pause_sound();
            if (strlen(ftm.current_file) == 0) {
                // If no current filename, perform Save As
                strcpy(current_name, "Untitled");
                displayKeyboard("SAVE...", current_name, 255);
                ESP_LOGI("FILE", "Save name: %s", current_name);
                snprintf(target_path, sizeof(target_path), "/flash/%s.ftm", current_name);
                drawPopupBox("WRITING...");
                display.display();
                ftm.save_as_ftm(target_path);
            } else {
                // Save to existing file
                drawPopupBox("WRITING...");
                display.display();
                ftm.save_ftm();
            }
            break;
        case 3:  // SAVE AS
            pause_sound();
            strcpy(current_name, path_basename(ftm.current_file));
            if (strlen(current_name) > 4) {
                current_name[strlen(current_name) - 4] = '\0'; // remove extension
            }
            displayKeyboard("SAVE AS...", current_name, 255);
            ESP_LOGI("FILE", "Save As name: %s", current_name);
            snprintf(target_path, sizeof(target_path), "/flash/%s.ftm", current_name);
            drawPopupBox("WRITING...");
            display.display();
            ftm.save_as_ftm(target_path);
            break;
        case 4:
            export_vgm_page();
            break;
        default:
            break;
    }
}

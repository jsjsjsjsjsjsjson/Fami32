#include "gui_file.h"
#include "gui_input.h"
#include "gui_menu.h"
#include <dirent.h>
#include <sys/stat.h>

// File selection UI: Browse files and directories starting from basePath. Returns chosen file path or NULL if cancelled.
const char* file_select(const char *basePath) {
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
    const int display_lines = 8;
    bool directory_changed = true;

    // Directory cache to store entries (max 256 entries)
    static struct {
        char name[256];
        bool is_dir;
    } dir_cache[256];
    int dir_cache_size = 0;

    // Draw header
    display.setTextColor(SSD1306_BLACK);
    display.fillRect(0, 0, 128, 6, SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("FILE SELECTOR/MANAGER");
    display.setTextColor(SSD1306_WHITE);

    for (;;) {
        if (directory_changed) {
            // Free previous scan results
            if (namelist) {
                for (int i = 0; i < n; ++i) {
                    free(namelist[i]);
                }
                free(namelist);
                namelist = NULL;
                n = 0;
            }
            // Open current directory
            DIR *dir = opendir(current_path);
            if (!dir) {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
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
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(0, 0);
                display.print("ERROR READING DIRECTORY");
                display.display();
                vTaskDelay(1024);
                return NULL;
            }
            // Populate cache
            dir_cache_size = 0;
            for (int i = 0; i < n && dir_cache_size < 256; ++i) {
                snprintf(dir_cache[dir_cache_size].name, sizeof(dir_cache[dir_cache_size].name), "%s", namelist[i]->d_name);
                snprintf(fullpath, sizeof(fullpath), "%.250s/%.250s", current_path, namelist[i]->d_name);
                struct stat st;
                dir_cache[dir_cache_size].is_dir = (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode));
                dir_cache_size++;
            }
            selected = 0;
            top = 0;
            directory_changed = false;
        }

        // Adjust scroll window based on selected index
        if (selected < top) {
            top = selected;
        } else if (selected >= top + display_lines) {
            top = selected - display_lines + 1;
        }

        // Draw file list region
        display.fillRect(0, 6, 128, 58, SSD1306_BLACK);
        display.setCursor(0, 7);
        display.printf(":%s", current_path);
        display.drawFastHLine(0, 13, 128, SSD1306_WHITE);
        if (dir_cache_size > 0) {
            // List directory entries
            for (int i = top; i < top + display_lines && i < dir_cache_size; ++i) {
                int rowIndex = i - top;
                if (i == selected) {
                    // Highlight selected entry
                    display.fillRect(0, 15 + rowIndex * 6, 128, 7, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                display.setCursor(0, 16 + rowIndex * 6);
                if (dir_cache[i].is_dir) {
                    // Mark directories with a trailing '/'
                    static char display_name[260];
                    snprintf(display_name, sizeof(display_name), "%s/", dir_cache[i].name);
                    display.print(display_name);
                } else {
                    display.print(dir_cache[i].name);
                }
                if (i == selected) {
                    display.setTextColor(SSD1306_WHITE);
                }
            }
        } else {
            // Directory is empty
            display.setCursor(22, 36);
            display.print("There's nothing here!");
        }
        display.display();

        // Handle keypad input for navigation and actions
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                switch (e.bit.KEY) {
                    case KEY_UP:
                        selected = (selected > 0) ? selected - 1 : dir_cache_size - 1;
                        break;
                    case KEY_DOWN:
                        selected = (selected < dir_cache_size - 1) ? selected + 1 : 0;
                        break;
                    case KEY_MENU: {
                        // Options menu for file entry (rename/delete)
                        static const char *opt_str[2] = {"RENAME", "DELETE"};
                        int ret = menu("FILE OPTION", opt_str, 2, NULL, 54, 29);
                        drawPopupBox("PLEASE WAIT...");
                        char *selName = dir_cache[selected].name;
                        static char selPath[PATH_MAX];
                        snprintf(selPath, PATH_MAX, "%s/%s", current_path, selName);
                        strncpy(selected_file, selPath, PATH_MAX - 1);
                        selected_file[PATH_MAX - 1] = '\0';
                        if (ret == 0) {
                            // Rename option
                            char new_name[256];
                            snprintf(new_name, sizeof(new_name), "%s", dir_cache[selected].name);
                            displayKeyboard("Rename", new_name, 255);
                            char new_path[PATH_MAX];
                            snprintf(new_path, PATH_MAX, "%s/%s", current_path, new_name);
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
                            snprintf(selPath, PATH_MAX, "%s/%s", current_path, selName);
                            if (dir_cache[selected].is_dir) {
                                // Enter directory
                                strncpy(current_path, selPath, PATH_MAX - 1);
                                current_path[PATH_MAX - 1] = '\0';
                                directory_changed = true;
                            } else {
                                // File selected
                                strncpy(selected_file, selPath, PATH_MAX - 1);
                                selected_file[PATH_MAX - 1] = '\0';
                                // Clean up name list memory
                                for (int i = 0; i < n; ++i) {
                                    free(namelist[i]);
                                }
                                free(namelist);
                                namelist = NULL;
                                n = 0;
                                return selected_file;
                            }
                        }
                        break;
                    case KEY_BACK:
                        // Go up to parent directory or exit if at base
                        if (strcmp(current_path, basePath) == 0) {
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
        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(NULL, NULL, e);
        }
        vTaskDelay(1);
    }
}

// Open File dialog: stops sound, opens file selector, loads the selected file.
void open_file_page() {
    drawPopupBox("LOADING...");
    pause_sound();
    const char* file = file_select("/flash");
    if (file == NULL) {
        return; // user cancelled
    }
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
        return;
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
        return;
    }
    player.reload(); // Reload player with new file data
}

// File menu: provides options to create new, open existing, save, save as, and record (if implemented)
void menu_file() {
    static const char *file_menu_str[5] = {"NEW", "OPEN", "SAVE", "SAVE AS", "RECORD"};
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
            strcpy(current_name, basename(ftm.current_file));
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
        case 4:  // RECORD (if implemented)
        default:
            break;
    }
}

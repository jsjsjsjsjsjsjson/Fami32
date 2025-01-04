#ifndef Serial0_TERMINAL_H
#define Serial0_TERMINAL_H

#include <Arduino.h>

class SerialTerminal {
public:
    static const uint8_t CMD_BUF_SIZE = 64;
    static const uint8_t CMD_LIST_SIZE = 16;

    typedef struct {
        const char* name;
        void (*func)(int argc, const char* argv[]);
    } Command;

    SerialTerminal() 
        : cmdBufferIndex(0), cmdListSize(0) {
        instance = this; // 在构造函数中设置instance
    }

    void begin(long baudRate, const char *termPrompt) {
        Serial0.begin(baudRate);
        strncpy(prompt, termPrompt, 31);
        Serial0.println("Serial0 Terminal Started");
        printPrompt();
        // 添加内置的帮助命令
        addCommand("help", helpCmd);
    }

    void update() {
        // 读取串口输入
        if (Serial0.available() > 0) {
            char c = Serial0.read();

            if (c == 127) { // 处理退格键
                if (cmdBufferIndex > 0) {
                    cmdBufferIndex--;
                    Serial0.print("\b \b"); // 光标左移一位并覆盖字符
                }
            } else if (c == '\n' || c == '\r') { // 处理换行符
                if (cmdBufferIndex > 0) {
                    cmdBuffer[cmdBufferIndex] = '\0';
                    Serial0.println(); // 回显换行
                    executeCommand(cmdBuffer);
                    cmdBufferIndex = 0;
                } else {
                    Serial0.println(); // 直接回车时换行
                }
                printPrompt(); // 打印提示行
            } else if (cmdBufferIndex < CMD_BUF_SIZE - 1) {
                // 记录命令字符并回显
                cmdBuffer[cmdBufferIndex++] = c;
                Serial0.print(c);
            }
        }
    }

    void addCommand(const char* name, void (*func)(int argc, const char* argv[])) {
        if (cmdListSize < CMD_LIST_SIZE) {
            cmdList[cmdListSize].name = name;
            cmdList[cmdListSize].func = func;
            cmdListSize++;
        }
    }

    static void helpCmd(int argc, const char* argv[]) {
        if (instance != nullptr) { // 检查instance是否为空
            Serial0.println("Available commands:");
            for (int i = 0; i < instance->cmdListSize; i++) {
                Serial0.println(instance->cmdList[i].name);
            }
        } else {
            Serial0.println("Error: instance is not set.");
        }
    }

    void executeCommand(const char* cmd) {
        const char* argv[CMD_BUF_SIZE / 2];
        int argc = 0;
        char cmdCopy[CMD_BUF_SIZE];
        strncpy(cmdCopy, cmd, CMD_BUF_SIZE);
        char* token = strtok(cmdCopy, " ");
        while (token != NULL && argc < CMD_BUF_SIZE / 2) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (argc > 0) {
            for (int i = 0; i < cmdListSize; i++) {
                if (strcmp(argv[0], cmdList[i].name) == 0) {
                    cmdList[i].func(argc, argv);
                    return;
                }
            }
            Serial0.println("Unknown command");
        }
    }

    static SerialTerminal* instance;

private:
    void printPrompt() {
        Serial0.printf("%s>>> ", prompt);
    }
    char prompt[32];
    char cmdBuffer[CMD_BUF_SIZE];
    int cmdBufferIndex;
    Command cmdList[CMD_LIST_SIZE];
    int cmdListSize;
};

// 定义静态成员变量
SerialTerminal* SerialTerminal::instance = nullptr;

#endif

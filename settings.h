#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SETTINGS_FILE ".\\settings.txt"
#define SETTING_OPTIONS 6

struct settings {
    char IP[16];
    uint32_t PORT;
    struct task task_limits;
    uint16_t workers;
    uint16_t *worker_weights;
};

struct settings * load_settings_file();
void settings_print(struct settings * setting);
char * get_current_setting(char * string);
void settings_worker_weights(uint16_t * weights, uint16_t workers,const char* str);

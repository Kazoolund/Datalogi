#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "shared.h"
#include "settings.h"

/* How to use:
 * Simply initialize a struct settings variable with load_settings_file()
 * Eksample: struct settings * settings_variable = load_settings_file();
 * 
 * Structure:
 *          char IP[16];
 *          uint32_t PORT;
 *          struct task task_limits; - From the shared.h header
 *                  uint32_t from;
 *                  uint32_t to;
 *                  uint16_t task_number;
 *          uint16_t workers;
 *          uint16_t *worker_weights;
 * 
 * NOTE: Remember to free() the struct pointer AND the weights array pointer when it isn't in use anymore.
 *       free(setting->worker_weights);
 *       free(setting);
 */


/*
 * void settings_print(struct settings * setting)
 * Takes 1 variable: setting
 * setting represents the setting struct which is initialized through the load_settings_file() function.
 * 
 * Prints out all active settings in the variable.
 */
void settings_print(struct settings * setting) {
    printf("Settings: \n");
    printf("\nIP: %s", setting->IP);
    printf("\nPORT: %d", setting->PORT);
    printf("\nTASKSIZE: %d", setting->task_limits.task_number);
    printf("\nTASKRANGE: %d - %d", setting->task_limits.from, setting->task_limits.to);
    printf("\nWORKERS: %d", setting->workers);
    for (int i = 1; i <= setting->workers; i++)
    {
        printf("\nWORKER %d Weight: %d", i, setting->worker_weights[i]);
    }
}

/*
 * struct settings * load_settings_file()
 * Takes 0 variables:
 * This function simply initializes the settings variable based on the settings.txt file.
 * 
 * Returns the a settings pointer based on the settings.txt file.
 * NOTE: Remember to free() the pointer AND the weights array pointer when it isn't in use anymore.
 *       free(setting->worker_weights);
 *       free(setting);
 */
struct settings * load_settings_file() {
    struct settings * setting_vars = malloc(sizeof(struct settings));
    FILE * file_pointer = fopen(SETTINGS_FILE, "r");
    char str[50];


    if (file_pointer != NULL) {
        while (fgets(str, 50, file_pointer))
        {
            char* option = get_current_setting(str);
            if (strcmp(option, "IP") == 0) {
                sscanf(str, "IP=%s", setting_vars->IP);

            } else if (strcmp(option, "PORT") == 0) {   
                const char temp_holder[15];
                sscanf(str, "PORT=%s", temp_holder);
                setting_vars->PORT = atoi(temp_holder);

            } else if (strcmp(option, "TASKSIZE") == 0) {
                const char temp_holder[15];
                sscanf(str, "TASKSIZE=%s", temp_holder);
                setting_vars->task_limits.task_number = atoi(temp_holder);
            
            } else if (strcmp(option, "TASKRANGE") == 0) {
                const char temp_int_lower[2];
                const char temp_int_upper[25];
                sscanf(str, "TASKRANGE=%s %s", temp_int_lower, temp_int_upper);
                setting_vars->task_limits.from  = atoi(temp_int_lower);
                setting_vars->task_limits.to    = atoi(temp_int_upper);

            } else if (strcmp(option, "WORKERS") == 0) {
                const char temp_holder[15];
                sscanf(str, "WORKERS=%s", temp_holder);
                setting_vars->workers = atoi(temp_holder);

            } else if (strcmp(option, "WORKERWEIGHT") == 0) {
                setting_vars->worker_weights = calloc(setting_vars->workers, sizeof(uint16_t));
                settings_worker_weights(setting_vars->worker_weights, setting_vars->workers, str);
            }
        }
        
    }

    fclose(file_pointer);
    return setting_vars;
}

/*
 * char * get_current_setting(char * string)
 * Takes 1 variable: string
 * string represents the string line currently being analyzed from the settings.txt file
 * 
 * Returns the current settings header, eks: IP, PORT, etc.
 */
char * get_current_setting(char * string) {
    static char output[15];
    sscanf(string, "%[A-Z]=", output);

    return output; 
}

/*
 * void settings_worker_weights(uint16_t * weights, uint16_t workers,const char* str)
 * Takes 3 variables: weights, workers and str
 * weights represents the weight array for each worker expected based on the settings.txt file.
 * workers represents the amount of workers expected based on the settings.txt file.
 * str represents the weight string read from the settings.txt file.
 * 
 * The function sets all weights in the order specified in the settings.txt file. Left to right.
 */
void settings_worker_weights(uint16_t * weights, uint16_t workers, char* str) {
    char* temp = str;
    const char delim[2] = ",";
    int i = 0;

    temp = strtok(temp, "=");
    while (temp != NULL)
    {
        if(strcmp(temp, "WORKERWEIGHT")) {
            weights[i] = atoi(temp);
        }
        temp = strtok(NULL, delim);
        i++;
    }
}
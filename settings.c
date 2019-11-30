#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "shared.h"
#include "settings.h"

/* How to use:
 * Simply initialize a struct settings variable with load_settings_file(filename)
 * Example: struct settings * settings_variable = load_settings_file(filename);
 * 
 * Structure:
 *          char IP[16];
 *          uint32_t PORT;
 *          struct task task_limits; - From the shared.h header
 *                  uint32_t from;
 *                  uint32_t to;
 *                  uint16_t task_number;
 *          uint16_t workers;
 *          weight_t *worker_weights;
 *          enum balance_type balance_type;
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
	printf("Settings:");
	printf("\nIP: %s", setting->IP);
	printf("\nPORT: %d", setting->PORT);
	printf("\nBALANCETYPE: %s", algorithm_names[setting->balance_type]);
	printf("\nTASKSIZE: %d", setting->task_limits.task_number);
	printf("\nTASKRANGE: %d - %d", setting->task_limits.from, setting->task_limits.to);
	printf("\nWORKERS: %d", setting->workers);
	for (int i = 0; i < setting->workers; i++)
	{
		printf("\nWORKER %d Weight: %d", i, setting->worker_weights[i]);
	}
	printf("\n");
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
struct settings * load_settings_file(const char *filename) {
	struct settings * setting_vars = malloc(sizeof(struct settings));
	FILE * file_pointer = fopen(filename, "r");
	char str[50];


	if (file_pointer != NULL) {
		while (fgets(str, 50, file_pointer))
		{
			char* option = get_current_setting(str);
			if (strcmp(option, "IP") == 0) {
				sscanf(str, "IP=%s", setting_vars->IP);

			} else if (strcmp(option, "PORT") == 0) {   
				sscanf(str, "PORT=%" SCNu32, &setting_vars->PORT);
			} else if (strcmp(option, "TASKSIZE") == 0) {
				sscanf(str, "TASKSIZE=%" SCNu16,
				       &setting_vars->task_limits.task_number);            
			} else if (strcmp(option, "TASKRANGE") == 0) {
				sscanf(str, "TASKRANGE=%" SCNu32 " %" SCNu32,
				       &setting_vars->task_limits.from,
				       &setting_vars->task_limits.to);
			} else if (strcmp(option, "WORKERS") == 0) {
				sscanf(str, "WORKERS=%" SCNu16, &setting_vars->workers);
			} else if (strcmp(option, "WORKERWEIGHT") == 0) {
				setting_vars->worker_weights = calloc(setting_vars->workers, sizeof(weight_t));
				settings_worker_weights(setting_vars->worker_weights, str);
			} else if (strcmp(option, "BALANCETYPE") == 0) {
				sscanf(str, "BALANCETYPE=%d", (int *)&setting_vars->balance_type);
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
 * void settings_worker_weights(weight_t *weights, const char* str)
 * Takes 2 variables: weights and str
 * weights represents the weight array for each worker expected based on the settings.txt file.
 * str represents the weight string read from the settings.txt file.
 * 
 * The function sets all weights in the order specified in the settings.txt file. Left to right.
 */
void settings_worker_weights(weight_t *weights, char* str) {
	char* temp = str;
	const char delim[2] = ",";
	int i = 0;

	temp = strtok(temp, "=");
	while (temp != NULL)
	{
		if(strcmp(temp, "WORKERWEIGHT")) {
			weights[i-1] = atoi(temp);
		}
		temp = strtok(NULL, delim);
		i++;
	}
}

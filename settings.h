struct settings {
	char IP[16];
	uint32_t PORT;
	struct task task_limits;
	uint16_t workers;
	weight_t *worker_weights;
	enum balance_type balance_type;
};

struct settings *load_settings_file(const char *filename);
void  settings_print(struct settings *setting);
void  settings_worker_weights(weight_t *weights, char *str);
char *get_current_setting(char *string);

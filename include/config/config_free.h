#ifndef CONFIG_FREE_H
#define CONFIG_FREE_H

#include "config_parse.h"

void free_circle_layout(Config *config) {
	if (config->circle_layout) {
		// Free each string
		for (int32_t i = 0; i < config->circle_layout_count; i++) {
			if (config->circle_layout[i]) {
				free(config->circle_layout[i]);	 // Release a single string
				config->circle_layout[i] = NULL; // Prevent wild pointers
			}
		}
		// Release the circle_layout array itself
		free(config->circle_layout);
		config->circle_layout = NULL; // Prevent wild pointers
	}
	config->circle_layout_count = 0; //Reset count
}

void free_baked_points(void) {
	if (baked_points_move) {
		free(baked_points_move);
		baked_points_move = NULL;
	}
	if (baked_points_open) {
		free(baked_points_open);
		baked_points_open = NULL;
	}
	if (baked_points_close) {
		free(baked_points_close);
		baked_points_close = NULL;
	}
	if (baked_points_tag) {
		free(baked_points_tag);
		baked_points_tag = NULL;
	}
	if (baked_points_focus) {
		free(baked_points_focus);
		baked_points_focus = NULL;
	}
	if (baked_points_opafadein) {
		free(baked_points_opafadein);
		baked_points_opafadein = NULL;
	}
	if (baked_points_opafadeout) {
		free(baked_points_opafadeout);
		baked_points_opafadeout = NULL;
	}
}

void free_config(void) {
	// release memory
	int32_t i;

	// Release window_rules
	if (config.window_rules) {
		for (int32_t i = 0; i < config.window_rules_count; i++) {
			ConfigWinRule *rule = &config.window_rules[i];
			if (rule->id)
				free((void *)rule->id);
			if (rule->title)
				free((void *)rule->title);
			if (rule->animation_type_open)
				free((void *)rule->animation_type_open);
			if (rule->animation_type_close)
				free((void *)rule->animation_type_close);
			if (rule->monitor)
				free((void *)rule->monitor);
			rule->id = NULL;
			rule->title = NULL;
			rule->animation_type_open = NULL;
			rule->animation_type_close = NULL;
			rule->monitor = NULL;
			// Release arg.v of globalkeybinding (if dynamically allocated)
			if (rule->globalkeybinding.arg.v) {
				free((void *)rule->globalkeybinding.arg.v);
			}
		}
		free(config.window_rules);
		config.window_rules = NULL;
		config.window_rules_count = 0;
	}

	// Release key_bindings
	if (config.key_bindings) {
		for (i = 0; i < config.key_bindings_count; i++) {
			if (config.key_bindings[i].arg.v) {
				free((void *)config.key_bindings[i].arg.v);
				config.key_bindings[i].arg.v = NULL;
			}
			if (config.key_bindings[i].arg.v2) {
				free((void *)config.key_bindings[i].arg.v2);
				config.key_bindings[i].arg.v2 = NULL;
			}
			if (config.key_bindings[i].arg.v3) {
				free((void *)config.key_bindings[i].arg.v3);
				config.key_bindings[i].arg.v3 = NULL;
			}
		}
		free(config.key_bindings);
		config.key_bindings = NULL;
		config.key_bindings_count = 0;
	}

	// Release mouse_bindings
	if (config.mouse_bindings) {
		for (i = 0; i < config.mouse_bindings_count; i++) {
			if (config.mouse_bindings[i].arg.v) {
				free((void *)config.mouse_bindings[i].arg.v);
				config.mouse_bindings[i].arg.v = NULL;
			}
			if (config.mouse_bindings[i].arg.v2) {
				free((void *)config.mouse_bindings[i].arg.v2);
				config.mouse_bindings[i].arg.v2 = NULL;
			}
			if (config.mouse_bindings[i].arg.v3) {
				free((void *)config.mouse_bindings[i].arg.v3);
				config.mouse_bindings[i].arg.v3 = NULL;
			}
		}
		free(config.mouse_bindings);
		config.mouse_bindings = NULL;
		config.mouse_bindings_count = 0;
	}

	// Release axis_bindings
	if (config.axis_bindings) {
		for (i = 0; i < config.axis_bindings_count; i++) {
			if (config.axis_bindings[i].arg.v) {
				free((void *)config.axis_bindings[i].arg.v);
				config.axis_bindings[i].arg.v = NULL;
			}
			if (config.axis_bindings[i].arg.v2) {
				free((void *)config.axis_bindings[i].arg.v2);
				config.axis_bindings[i].arg.v2 = NULL;
			}
			if (config.axis_bindings[i].arg.v3) {
				free((void *)config.axis_bindings[i].arg.v3);
				config.axis_bindings[i].arg.v3 = NULL;
			}
		}
		free(config.axis_bindings);
		config.axis_bindings = NULL;
		config.axis_bindings_count = 0;
	}

	// Release switch_bindings
	if (config.switch_bindings) {
		for (i = 0; i < config.switch_bindings_count; i++) {
			if (config.switch_bindings[i].arg.v) {
				free((void *)config.switch_bindings[i].arg.v);
				config.switch_bindings[i].arg.v = NULL;
			}
			if (config.switch_bindings[i].arg.v2) {
				free((void *)config.switch_bindings[i].arg.v2);
				config.switch_bindings[i].arg.v2 = NULL;
			}
			if (config.switch_bindings[i].arg.v3) {
				free((void *)config.switch_bindings[i].arg.v3);
				config.switch_bindings[i].arg.v3 = NULL;
			}
		}
		free(config.switch_bindings);
		config.switch_bindings = NULL;
		config.switch_bindings_count = 0;
	}

	// Release gesture_bindings
	if (config.gesture_bindings) {
		for (i = 0; i < config.gesture_bindings_count; i++) {
			if (config.gesture_bindings[i].arg.v) {
				free((void *)config.gesture_bindings[i].arg.v);
				config.gesture_bindings[i].arg.v = NULL;
			}
			if (config.gesture_bindings[i].arg.v2) {
				free((void *)config.gesture_bindings[i].arg.v2);
				config.gesture_bindings[i].arg.v2 = NULL;
			}
			if (config.gesture_bindings[i].arg.v3) {
				free((void *)config.gesture_bindings[i].arg.v3);
				config.gesture_bindings[i].arg.v3 = NULL;
			}
		}
		free(config.gesture_bindings);
		config.gesture_bindings = NULL;
		config.gesture_bindings_count = 0;
	}

	// Release tag_rules
	if (config.tag_rules) {
		for (int32_t i = 0; i < config.tag_rules_count; i++) {
			if (config.tag_rules[i].layout_name)
				free((void *)config.tag_rules[i].layout_name);
			if (config.tag_rules[i].monitor_name)
				free((void *)config.tag_rules[i].monitor_name);
			if (config.tag_rules[i].monitor_make)
				free((void *)config.tag_rules[i].monitor_make);
			if (config.tag_rules[i].monitor_model)
				free((void *)config.tag_rules[i].monitor_model);
			if (config.tag_rules[i].monitor_serial)
				free((void *)config.tag_rules[i].monitor_serial);
		}
		free(config.tag_rules);
		config.tag_rules = NULL;
		config.tag_rules_count = 0;
	}

	// Release monitor_rules
	if (config.monitor_rules) {
		for (int32_t i = 0; i < config.monitor_rules_count; i++) {
			if (config.monitor_rules[i].name)
				free((void *)config.monitor_rules[i].name);
			if (config.monitor_rules[i].make)
				free((void *)config.monitor_rules[i].make);
			if (config.monitor_rules[i].model)
				free((void *)config.monitor_rules[i].model);
			if (config.monitor_rules[i].serial)
				free((void *)config.monitor_rules[i].serial);
		}
		free(config.monitor_rules);
		config.monitor_rules = NULL;
		config.monitor_rules_count = 0;
	}

	// Release layer_rules
	if (config.layer_rules) {
		for (int32_t i = 0; i < config.layer_rules_count; i++) {
			if (config.layer_rules[i].layer_name)
				free((void *)config.layer_rules[i].layer_name);
			if (config.layer_rules[i].animation_type_open)
				free((void *)config.layer_rules[i].animation_type_open);
			if (config.layer_rules[i].animation_type_close)
				free((void *)config.layer_rules[i].animation_type_close);
		}
		free(config.layer_rules);
		config.layer_rules = NULL;
		config.layer_rules_count = 0;
	}

	// release env
	if (config.env) {
		for (int32_t i = 0; i < config.env_count; i++) {
			if (config.env[i]->type) {
				free((void *)config.env[i]->type);
			}
			if (config.env[i]->value) {
				free((void *)config.env[i]->value);
			}
			free(config.env[i]);
		}
		free(config.env);
		config.env = NULL;
		config.env_count = 0;
	}

	// release exec
	if (config.exec) {
		for (i = 0; i < config.exec_count; i++) {
			free(config.exec[i]);
		}
		free(config.exec);
		config.exec = NULL;
		config.exec_count = 0;
	}

	// Release exec_once
	if (config.exec_once) {
		for (i = 0; i < config.exec_once_count; i++) {
			free(config.exec_once[i]);
		}
		free(config.exec_once);
		config.exec_once = NULL;
		config.exec_once_count = 0;
	}

	// Release scroller_proportion_preset
	if (config.scroller_proportion_preset) {
		free(config.scroller_proportion_preset);
		config.scroller_proportion_preset = NULL;
		config.scroller_proportion_preset_count = 0;
	}

	if (config.cursor_theme) {
		free(config.cursor_theme);
		config.cursor_theme = NULL;
	}

	// Release circle_layout
	free_circle_layout(&config);

	// Release animation resources
	free_baked_points();

	// Clean up the keymap used to parse keys
	cleanup_config_keymap();
}

#endif /* CONFIG_FREE_H */

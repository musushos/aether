#ifndef CONFIG_INPUT_H
#define CONFIG_INPUT_H

#include "config_utils.h"

uint32_t parse_mod(const char *mod_str) {
	if (!mod_str || !*mod_str) {
		return UINT32_MAX;
	}

	uint32_t mod = 0;
	char input_copy[256];
	char *token;
	char *saveptr = NULL;
	bool match_success = false;

	// Copy and convert to lowercase
	strncpy(input_copy, mod_str, sizeof(input_copy) - 1);
	input_copy[sizeof(input_copy) - 1] = '\0';
	for (char *p = input_copy; *p; p++) {
		*p = tolower(*p);
	}

	// Split and process each part
	token = strtok_r(input_copy, "+", &saveptr);
	while (token != NULL) {
		//Remove leading and trailing whitespace
		trim_whitespace(token);

		// Skip if token becomes empty string
		if (*token == '\0') {
			token = strtok_r(NULL, "+", &saveptr);
			continue;
		}

		if (strncmp(token, "code:", 5) == 0) {
			// Process code: form
			char *endptr;
			long keycode = strtol(token + 5, &endptr, 10);
			if (endptr != token + 5 && (*endptr == '\0' || *endptr == ' ')) {
				switch (keycode) {
				case 133:
				case 134:
					mod |= WLR_MODIFIER_LOGO;
					break;
				case 37:
				case 105:
					mod |= WLR_MODIFIER_CTRL;
					break;
				case 50:
				case 62:
					mod |= WLR_MODIFIER_SHIFT;
					break;
				case 64:
				case 108:
					mod |= WLR_MODIFIER_ALT;
					break;
				default:
					fprintf(stderr,
							"unknown modifier keycode: \033[1m\033[31m%s\n",
							token);
					break;
				}
			}
		} else {
			if (!strcmp(token, "super") || !strcmp(token, "super_l") ||
				!strcmp(token, "super_r")) {
				mod |= WLR_MODIFIER_LOGO;
				match_success = true;
			}
			if (!strcmp(token, "ctrl") || !strcmp(token, "ctrl_l") ||
				!strcmp(token, "ctrl_r")) {
				mod |= WLR_MODIFIER_CTRL;
				match_success = true;
			}
			if (!strcmp(token, "shift") || !strcmp(token, "shift_l") ||
				!strcmp(token, "shift_r")) {
				mod |= WLR_MODIFIER_SHIFT;
				match_success = true;
			}
			if (!strcmp(token, "alt") || !strcmp(token, "alt_l") ||
				!strcmp(token, "alt_r")) {
				mod |= WLR_MODIFIER_ALT;
				match_success = true;
			}
			if (!strcmp(token, "hyper") || !strcmp(token, "hyper_l") ||
				!strcmp(token, "hyper_r")) {
				mod |= WLR_MODIFIER_MOD3;
				match_success = true;
			}
			if (!strcmp(token, "none")) {
				match_success = true;
			}
		}

		token = strtok_r(NULL, "+", &saveptr);
	}

	if (!match_success) {
		mod = UINT32_MAX;
		fprintf(stderr,
				"\033[1m\033[31m[ERROR]:\033[33m Unknown modifier: "
				"\033[1m\033[31m%s\n",
				mod_str);
	}

	return mod;
}

//Define auxiliary function: find multiple keycodes corresponding to keysym in keymap
static int32_t find_keycodes_for_keysym(struct xkb_keymap *keymap,
										xkb_keysym_t sym,
										MultiKeycode *multi_kc) {
	xkb_keycode_t min_keycode = xkb_keymap_min_keycode(keymap);
	xkb_keycode_t max_keycode = xkb_keymap_max_keycode(keymap);

	multi_kc->keycode1 = 0;
	multi_kc->keycode2 = 0;
	multi_kc->keycode3 = 0;

	int32_t found_count = 0;

	for (xkb_keycode_t keycode = min_keycode;
		 keycode <= max_keycode && found_count < 3; keycode++) {
		// Use layout 0 and level 0
		const xkb_keysym_t *syms;
		int32_t num_syms =
			xkb_keymap_key_get_syms_by_level(keymap, keycode, 0, 0, &syms);

		for (int32_t i = 0; i < num_syms; i++) {
			if (syms[i] == sym) {
				switch (found_count) {
				case 0:
					multi_kc->keycode1 = keycode;
					break;
				case 1:
					multi_kc->keycode2 = keycode;
					break;
				case 2:
					multi_kc->keycode3 = keycode;
					break;
				}
				found_count++;
				break;
			}
		}
	}

	return found_count;
}

void cleanup_config_keymap(void) {
	if (config.keymap != NULL) {
		xkb_keymap_unref(config.keymap);
		config.keymap = NULL;
	}
	if (config.ctx != NULL) {
		xkb_context_unref(config.ctx);
		config.ctx = NULL;
	}
}

void create_config_keymap(void) {
	//Initialize xkb context and keymap

	if (config.ctx == NULL) {
		config.ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	}

	if (config.keymap == NULL) {
		config.keymap = xkb_keymap_new_from_names(
			config.ctx, &xkb_fallback_rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
	}
}

KeySymCode parse_key(const char *key_str, bool isbindsym) {
	KeySymCode kc = {0}; // initialized to 0

	if (config.keymap == NULL || config.ctx == NULL) {
		// Handle errors
		kc.type = KEY_TYPE_SYM;
		kc.keysym = XKB_KEY_NoSymbol;
		return kc;
	}

	// Handle the case of code: prefix
	if (strncmp(key_str, "code:", 5) == 0) {
		char *endptr;
		xkb_keycode_t keycode = (xkb_keycode_t)strtol(key_str + 5, &endptr, 10);
		kc.type = KEY_TYPE_CODE;
		kc.keycode.keycode1 = keycode; //Only set the first one
		kc.keycode.keycode2 = 0;
		kc.keycode.keycode3 = 0;
		return kc;
	}

	// change key string to keysym, case insensitive
	xkb_keysym_t sym =
		xkb_keysym_from_name(key_str, XKB_KEYSYM_CASE_INSENSITIVE);

	if (isbindsym) {
		kc.type = KEY_TYPE_SYM;
		kc.keysym = sym;
		return kc;
	}

	if (sym != XKB_KEY_NoSymbol) {
		//Try to find multiple corresponding keycodes
		int32_t found_count =
			find_keycodes_for_keysym(config.keymap, sym, &kc.keycode);
		if (found_count > 0) {
			kc.type = KEY_TYPE_CODE;
			kc.keysym = sym; // Still saving keysym for reference
		} else {
			kc.type = KEY_TYPE_SYM;
			kc.keysym = sym;
			// keycode field remains 0
		}
	} else {
		//Unresolvable key name
		kc.type = KEY_TYPE_SYM;
		kc.keysym = XKB_KEY_NoSymbol;
		fprintf(
			stderr,
			"\033[1m\033[31m[ERROR]:\033[33m Unknown key: \033[1m\033[31m%s\n",
			key_str);
		// keycode field remains 0
	}

	return kc;
}

uint32_t parse_button(const char *str) {
	//Convert the input string to lowercase
	char lowerStr[20];
	int32_t i = 0;
	while (str[i] && i < 19) {
		lowerStr[i] = tolower(str[i]);
		i++;
	}
	lowerStr[i] = '\0'; // Make sure the string is terminated correctly

	//Return the corresponding button number based on the converted lowercase string
	if (strcmp(lowerStr, "btn_left") == 0) {
		return BTN_LEFT;
	} else if (strcmp(lowerStr, "btn_right") == 0) {
		return BTN_RIGHT;
	} else if (strcmp(lowerStr, "btn_middle") == 0) {
		return BTN_MIDDLE;
	} else if (strcmp(lowerStr, "btn_side") == 0) {
		return BTN_SIDE;
	} else if (strcmp(lowerStr, "btn_extra") == 0) {
		return BTN_EXTRA;
	} else if (strcmp(lowerStr, "btn_forward") == 0) {
		return BTN_FORWARD;
	} else if (strcmp(lowerStr, "btn_back") == 0) {
		return BTN_BACK;
	} else if (strcmp(lowerStr, "btn_task") == 0) {
		return BTN_TASK;
	} else {
		fprintf(stderr,
				"\033[1m\033[31m[ERROR]:\033[33m Unknown button: "
				"\033[1m\033[31m%s\n",
				str);
		return UINT32_MAX;
	}
}

int32_t parse_mouse_action(const char *str) {
	//Convert the input string to lowercase
	char lowerStr[20];
	int32_t i = 0;
	while (str[i] && i < 19) {
		lowerStr[i] = tolower(str[i]);
		i++;
	}
	lowerStr[i] = '\0'; // Make sure the string is terminated correctly

	//Return the corresponding button number based on the converted lowercase string
	if (strcmp(lowerStr, "curmove") == 0) {
		return CurMove;
	} else if (strcmp(lowerStr, "curresize") == 0) {
		return CurResize;
	} else if (strcmp(lowerStr, "curnormal") == 0) {
		return CurNormal;
	} else if (strcmp(lowerStr, "curpressed") == 0) {
		return CurPressed;
	} else {
		return 0;
	}
}

void convert_hex_to_rgba(float *color, uint32_t hex) {
	color[0] = ((hex >> 24) & 0xFF) / 255.0f;
	color[1] = ((hex >> 16) & 0xFF) / 255.0f;
	color[2] = ((hex >> 8) & 0xFF) / 255.0f;
	color[3] = (hex & 0xFF) / 255.0f;
}

uint32_t parse_num_type(char *str) {
	switch (str[0]) {
	case '-':
		return NUM_TYPE_MINUS;
	case '+':
		return NUM_TYPE_PLUS;
	default:
		return NUM_TYPE_DEFAULT;
	}
}

FuncType parse_func_name(char *func_name, Arg *arg, char *arg_value,
						 char *arg_value2, char *arg_value3, char *arg_value4,
						 char *arg_value5) {

	FuncType func = NULL;
	(*arg).i = 0;
	(*arg).i2 = 0;
	(*arg).f = 0.0f;
	(*arg).f2 = 0.0f;
	(*arg).ui = 0;
	(*arg).ui2 = 0;
	(*arg).v = NULL;
	(*arg).v2 = NULL;
	(*arg).v3 = NULL;

	if (strcmp(func_name, "focusstack") == 0) {
		func = focusstack;
		(*arg).i = parse_circle_direction(arg_value);
	} else if (strcmp(func_name, "focusdir") == 0) {
		func = focusdir;
		(*arg).i = parse_direction(arg_value);
	} else if (strcmp(func_name, "incnmaster") == 0) {
		func = incnmaster;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "setmfact") == 0) {
		func = setmfact;
		(*arg).f = atof(arg_value);
	} else if (strcmp(func_name, "zoom") == 0) {
		func = zoom;
	} else if (strcmp(func_name, "exchange_client") == 0) {
		func = exchange_client;
		(*arg).i = parse_direction(arg_value);
	} else if (strcmp(func_name, "exchange_stack_client") == 0) {
		func = exchange_stack_client;
		(*arg).i = parse_circle_direction(arg_value);
	} else if (strcmp(func_name, "toggleglobal") == 0) {
		func = toggleglobal;
	} else if (strcmp(func_name, "toggleoverview") == 0) {
		func = toggleoverview;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "set_proportion") == 0) {
		func = set_proportion;
		(*arg).f = atof(arg_value);
	} else if (strcmp(func_name, "switch_proportion_preset") == 0) {
		func = switch_proportion_preset;
		(*arg).i = parse_circle_direction(arg_value);
	} else if (strcmp(func_name, "viewtoleft") == 0) {
		func = viewtoleft;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "viewtoright") == 0) {
		func = viewtoright;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "tagsilent") == 0) {
		func = tagsilent;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
	} else if (strcmp(func_name, "tagtoleft") == 0) {
		func = tagtoleft;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "tagtoright") == 0) {
		func = tagtoright;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "killclient") == 0) {
		func = killclient;
	} else if (strcmp(func_name, "centerwin") == 0) {
		func = centerwin;
	} else if (strcmp(func_name, "focuslast") == 0) {
		func = focuslast;
	} else if (strcmp(func_name, "toggle_trackpad_enable") == 0) {
		func = toggle_trackpad_enable;
	} else if (strcmp(func_name, "setoption") == 0) {
		func = setoption;

		(*arg).v = strdup(arg_value);

		// Collect parameters that need to be spliced
		const char *non_empty_params[4] = {NULL};
		int32_t param_index = 0;

		if (arg_value2 && arg_value2[0] != '\0')
			non_empty_params[param_index++] = arg_value2;
		if (arg_value3 && arg_value3[0] != '\0')
			non_empty_params[param_index++] = arg_value3;
		if (arg_value4 && arg_value4[0] != '\0')
			non_empty_params[param_index++] = arg_value4;
		if (arg_value5 && arg_value5[0] != '\0')
			non_empty_params[param_index++] = arg_value5;

		// handle splicing
		if (param_index == 0) {
			(*arg).v2 = strdup("");
		} else {
			// Calculate the total length
			size_t len = 0;
			for (int32_t i = 0; i < param_index; i++) {
				len += strlen(non_empty_params[i]);
			}
			len += (param_index - 1) + 1; // Number of commas + null terminator

			char *temp = malloc(len);
			if (temp) {
				char *cursor = temp;
				for (int32_t i = 0; i < param_index; i++) {
					if (i > 0) {
						*cursor++ = ',';
					}
					size_t param_len = strlen(non_empty_params[i]);
					memcpy(cursor, non_empty_params[i], param_len);
					cursor += param_len;
				}
				*cursor = '\0';
				(*arg).v2 = temp;
			}
		}
	} else if (strcmp(func_name, "setkeymode") == 0) {
		func = setkeymode;
		(*arg).v = strdup(arg_value);
	} else if (strcmp(func_name, "switch_keyboard_layout") == 0) {
		func = switch_keyboard_layout;
		(*arg).i = CLAMP_INT(atoi(arg_value), 0, 100);
	} else if (strcmp(func_name, "setlayout") == 0) {
		func = setlayout;
		(*arg).v = strdup(arg_value);
	} else if (strcmp(func_name, "switch_layout") == 0) {
		func = switch_layout;
	} else if (strcmp(func_name, "togglefloating") == 0) {
		func = togglefloating;
	} else if (strcmp(func_name, "togglefullscreen") == 0) {
		func = togglefullscreen;
	} else if (strcmp(func_name, "togglefakefullscreen") == 0) {
		func = togglefakefullscreen;
	} else if (strcmp(func_name, "toggleoverlay") == 0) {
		func = toggleoverlay;
	} else if (strcmp(func_name, "minimized") == 0) {
		func = minimized;
	} else if (strcmp(func_name, "restore_minimized") == 0) {
		func = restore_minimized;
	} else if (strcmp(func_name, "toggle_scratchpad") == 0) {
		func = toggle_scratchpad;
	} else if (strcmp(func_name, "toggle_render_border") == 0) {
		func = toggle_render_border;
	} else if (strcmp(func_name, "focusmon") == 0) {
		func = focusmon;
		(*arg).i = parse_direction(arg_value);
		if ((*arg).i == UNDIR) {
			(*arg).v = strdup(arg_value);
		}
	} else if (strcmp(func_name, "tagmon") == 0) {
		func = tagmon;
		(*arg).i = parse_direction(arg_value);
		(*arg).i2 = atoi(arg_value2);
		if ((*arg).i == UNDIR) {
			(*arg).v = strdup(arg_value);
		};
	} else if (strcmp(func_name, "incgaps") == 0) {
		func = incgaps;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "togglegaps") == 0) {
		func = togglegaps;
	} else if (strcmp(func_name, "chvt") == 0) {
		func = chvt;
		(*arg).ui = atoi(arg_value);
	} else if (strcmp(func_name, "spawn") == 0) {
		func = spawn;
		char *values[] = {arg_value, arg_value2, arg_value3, arg_value4,
						  arg_value5};
		(*arg).v = combine_args_until_empty(values, 5);
	} else if (strcmp(func_name, "spawn_shell") == 0) {
		func = spawn_shell;
		char *values[] = {arg_value, arg_value2, arg_value3, arg_value4,
						  arg_value5};
		(*arg).v = combine_args_until_empty(values, 5);
	} else if (strcmp(func_name, "spawn_on_empty") == 0) {
		func = spawn_on_empty;
		(*arg).v = strdup(arg_value); // Note: This memory needs to be released later
		(*arg).ui = 1 << (atoi(arg_value2) - 1);
	} else if (strcmp(func_name, "quit") == 0) {
		func = quit;
	} else if (strcmp(func_name, "create_virtual_output") == 0) {
		func = create_virtual_output;
	} else if (strcmp(func_name, "destroy_all_virtual_output") == 0) {
		func = destroy_all_virtual_output;
	} else if (strcmp(func_name, "moveresize") == 0) {
		func = moveresize;
		(*arg).ui = parse_mouse_action(arg_value);
	} else if (strcmp(func_name, "togglemaximizescreen") == 0) {
		func = togglemaximizescreen;
	} else if (strcmp(func_name, "viewtoleft_have_client") == 0) {
		func = viewtoleft_have_client;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "viewtoright_have_client") == 0) {
		func = viewtoright_have_client;
		(*arg).i = atoi(arg_value);
	} else if (strcmp(func_name, "reload_config") == 0) {
		func = reload_config;
	} else if (strcmp(func_name, "tag") == 0) {
		func = tag;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
		(*arg).i = atoi(arg_value2);
	} else if (strcmp(func_name, "view") == 0) {
		func = bind_to_view;

		uint32_t mask = 0;
		char *token;
		char *arg_copy = strdup(arg_value);

		if (arg_copy != NULL) {
			char *saveptr = NULL;
			token = strtok_r(arg_copy, "|", &saveptr);

			while (token != NULL) {
				int32_t num = atoi(token);
				if (num > 0 && num <= LENGTH(tags)) {
					mask |= (1 << (num - 1));
				}
				token = strtok_r(NULL, "|", &saveptr);
			}

			free(arg_copy);
		}

		if (mask) {
			(*arg).ui = mask;
		} else {
			(*arg).ui = atoi(arg_value);
		}
		(*arg).i = atoi(arg_value2);
	} else if (strcmp(func_name, "viewcrossmon") == 0) {
		func = viewcrossmon;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
		(*arg).v = strdup(arg_value2);
	} else if (strcmp(func_name, "tagcrossmon") == 0) {
		func = tagcrossmon;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
		(*arg).v = strdup(arg_value2);
	} else if (strcmp(func_name, "toggletag") == 0) {
		func = toggletag;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
	} else if (strcmp(func_name, "toggleview") == 0) {
		func = toggleview;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
	} else if (strcmp(func_name, "comboview") == 0) {
		func = comboview;
		(*arg).ui = 1 << (atoi(arg_value) - 1);
	} else if (strcmp(func_name, "smartmovewin") == 0) {
		func = smartmovewin;
		(*arg).i = parse_direction(arg_value);
	} else if (strcmp(func_name, "smartresizewin") == 0) {
		func = smartresizewin;
		(*arg).i = parse_direction(arg_value);
	} else if (strcmp(func_name, "resizewin") == 0) {
		func = resizewin;
		(*arg).ui = parse_num_type(arg_value);
		(*arg).ui2 = parse_num_type(arg_value2);
		(*arg).i = (*arg).ui == NUM_TYPE_DEFAULT ? atoi(arg_value)
												 : atoi(arg_value + 1);
		(*arg).i2 = (*arg).ui2 == NUM_TYPE_DEFAULT ? atoi(arg_value2)
												   : atoi(arg_value2 + 1);
	} else if (strcmp(func_name, "movewin") == 0) {
		func = movewin;
		(*arg).ui = parse_num_type(arg_value);
		(*arg).ui2 = parse_num_type(arg_value2);
		(*arg).i = (*arg).ui == NUM_TYPE_DEFAULT ? atoi(arg_value)
												 : atoi(arg_value + 1);
		(*arg).i2 = (*arg).ui2 == NUM_TYPE_DEFAULT ? atoi(arg_value2)
												   : atoi(arg_value2 + 1);
	} else if (strcmp(func_name, "toggle_named_scratchpad") == 0) {
		func = toggle_named_scratchpad;
		(*arg).v = strdup(arg_value);
		(*arg).v2 = strdup(arg_value2);
		(*arg).v3 = strdup(arg_value3);
	} else if (strcmp(func_name, "disable_monitor") == 0) {
		func = disable_monitor;
		(*arg).v = strdup(arg_value);
	} else if (strcmp(func_name, "enable_monitor") == 0) {
		func = enable_monitor;
		(*arg).v = strdup(arg_value);
	} else if (strcmp(func_name, "toggle_monitor") == 0) {
		func = toggle_monitor;
		(*arg).v = strdup(arg_value);
	} else if (strcmp(func_name, "scroller_stack") == 0) {
		func = scroller_stack;
		(*arg).i = parse_direction(arg_value);
	} else if (strcmp(func_name, "toggle_all_floating") == 0) {
		func = toggle_all_floating;
	} else if (strcmp(func_name, "toggle_floating_compositor_mode") == 0) {
		func = toggle_floating_compositor_mode;
	} else if (strcmp(func_name, "switcher_forward") == 0) {
		func = switcher_forward;
	} else if (strcmp(func_name, "switcher_backward") == 0) {
		func = switcher_backward;
	} else if (strcmp(func_name, "switcher_commit_bind") == 0) {
		func = switcher_commit_bind;
	} else if (strcmp(func_name, "switcher_cancel_bind") == 0) {
		func = switcher_cancel_bind;
	} else {
		return NULL;
	}
	return func;
}

#endif /* CONFIG_INPUT_H */

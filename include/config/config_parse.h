#ifndef CONFIG_PARSE_H
#define CONFIG_PARSE_H

#include "config_exec.h"

bool parse_option(Config *config, char *key, char *value) {
	if (strcmp(key, "keymode") == 0) {
		snprintf(config->keymode, sizeof(config->keymode), "%.27s", value);
	} else if (strcmp(key, "animations") == 0) {
		config->animations = atoi(value);
	} else if (strcmp(key, "layer_animations") == 0) {
		config->layer_animations = atoi(value);
	} else if (strcmp(key, "animation_type_open") == 0) {
		snprintf(config->animation_type_open,
				 sizeof(config->animation_type_open), "%.9s",
				 value); // string limit to 9 char
	} else if (strcmp(key, "animation_type_close") == 0) {
		snprintf(config->animation_type_close,
				 sizeof(config->animation_type_close), "%.9s",
				 value); // string limit to 9 char
	} else if (strcmp(key, "layer_animation_type_open") == 0) {
		snprintf(config->layer_animation_type_open,
				 sizeof(config->layer_animation_type_open), "%.9s",
				 value); // string limit to 9 char
	} else if (strcmp(key, "layer_animation_type_close") == 0) {
		snprintf(config->layer_animation_type_close,
				 sizeof(config->layer_animation_type_close), "%.9s",
				 value); // string limit to 9 char
	} else if (strcmp(key, "animation_fade_in") == 0) {
		config->animation_fade_in = atoi(value);
	} else if (strcmp(key, "animation_fade_out") == 0) {
		config->animation_fade_out = atoi(value);
	} else if (strcmp(key, "tag_animation_direction") == 0) {
		config->tag_animation_direction = atoi(value);
	} else if (strcmp(key, "zoom_initial_ratio") == 0) {
		config->zoom_initial_ratio = atof(value);
	} else if (strcmp(key, "zoom_end_ratio") == 0) {
		config->zoom_end_ratio = atof(value);
	} else if (strcmp(key, "fadein_begin_opacity") == 0) {
		config->fadein_begin_opacity = atof(value);
	} else if (strcmp(key, "fadeout_begin_opacity") == 0) {
		config->fadeout_begin_opacity = atof(value);
	} else if (strcmp(key, "animation_duration_move") == 0) {
		config->animation_duration_move = atoi(value);
	} else if (strcmp(key, "animation_duration_open") == 0) {
		config->animation_duration_open = atoi(value);
	} else if (strcmp(key, "animation_duration_tag") == 0) {
		config->animation_duration_tag = atoi(value);
	} else if (strcmp(key, "animation_duration_close") == 0) {
		config->animation_duration_close = atoi(value);
	} else if (strcmp(key, "animation_duration_focus") == 0) {
		config->animation_duration_focus = atoi(value);
	} else if (strcmp(key, "animation_curve_move") == 0) {
		int32_t num =
			parse_double_array(value, config->animation_curve_move, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_move: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "animation_curve_open") == 0) {
		int32_t num =
			parse_double_array(value, config->animation_curve_open, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_open: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "animation_curve_tag") == 0) {
		int32_t num = parse_double_array(value, config->animation_curve_tag, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_tag: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "animation_curve_close") == 0) {
		int32_t num =
			parse_double_array(value, config->animation_curve_close, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_close: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "animation_curve_focus") == 0) {
		int32_t num =
			parse_double_array(value, config->animation_curve_focus, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_focus: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "animation_curve_opafadein") == 0) {
		int32_t num =
			parse_double_array(value, config->animation_curve_opafadein, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_opafadein: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "animation_curve_opafadeout") == 0) {
		int32_t num =
			parse_double_array(value, config->animation_curve_opafadeout, 4);
		if (num != 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to parse "
					"animation_curve_opafadeout: %s\n",
					value);
			return false;
		}
	} else if (strcmp(key, "scroller_structs") == 0) {
		config->scroller_structs = atoi(value);
	} else if (strcmp(key, "scroller_default_proportion") == 0) {
		config->scroller_default_proportion = atof(value);
	} else if (strcmp(key, "scroller_default_proportion_single") == 0) {
		config->scroller_default_proportion_single = atof(value);
	} else if (strcmp(key, "scroller_ignore_proportion_single") == 0) {
		config->scroller_ignore_proportion_single = atoi(value);
	} else if (strcmp(key, "scroller_focus_center") == 0) {
		config->scroller_focus_center = atoi(value);
	} else if (strcmp(key, "scroller_prefer_center") == 0) {
		config->scroller_prefer_center = atoi(value);
	} else if (strcmp(key, "scroller_prefer_overspread") == 0) {
		config->scroller_prefer_overspread = atoi(value);
	} else if (strcmp(key, "edge_scroller_pointer_focus") == 0) {
		config->edge_scroller_pointer_focus = atoi(value);
	} else if (strcmp(key, "focus_cross_monitor") == 0) {
		config->focus_cross_monitor = atoi(value);
	} else if (strcmp(key, "exchange_cross_monitor") == 0) {
		config->exchange_cross_monitor = atoi(value);
	} else if (strcmp(key, "scratchpad_cross_monitor") == 0) {
		config->scratchpad_cross_monitor = atoi(value);
	} else if (strcmp(key, "focus_cross_tag") == 0) {
		config->focus_cross_tag = atoi(value);
	} else if (strcmp(key, "view_current_to_back") == 0) {
		config->view_current_to_back = atoi(value);
	} else if (strcmp(key, "blur") == 0) {
		config->blur = atoi(value);
	} else if (strcmp(key, "blur_layer") == 0) {
		config->blur_layer = atoi(value);
	} else if (strcmp(key, "blur_optimized") == 0) {
		config->blur_optimized = atoi(value);
	} else if (strcmp(key, "border_radius") == 0) {
		config->border_radius = atoi(value);
	} else if (strcmp(key, "blur_params_num_passes") == 0) {
		config->blur_params.num_passes = atoi(value);
	} else if (strcmp(key, "blur_params_radius") == 0) {
		config->blur_params.radius = atoi(value);
	} else if (strcmp(key, "blur_params_noise") == 0) {
		config->blur_params.noise = atof(value);
	} else if (strcmp(key, "blur_params_brightness") == 0) {
		config->blur_params.brightness = atof(value);
	} else if (strcmp(key, "blur_params_contrast") == 0) {
		config->blur_params.contrast = atof(value);
	} else if (strcmp(key, "blur_params_saturation") == 0) {
		config->blur_params.saturation = atof(value);
	} else if (strcmp(key, "shadows") == 0) {
		config->shadows = atoi(value);
	} else if (strcmp(key, "shadow_only_floating") == 0) {
		config->shadow_only_floating = atoi(value);
	} else if (strcmp(key, "layer_shadows") == 0) {
		config->layer_shadows = atoi(value);
	} else if (strcmp(key, "shadows_size") == 0) {
		config->shadows_size = atoi(value);
	} else if (strcmp(key, "shadows_blur") == 0) {
		config->shadows_blur = atof(value);
	} else if (strcmp(key, "shadows_position_x") == 0) {
		config->shadows_position_x = atoi(value);
	} else if (strcmp(key, "shadows_position_y") == 0) {
		config->shadows_position_y = atoi(value);
	} else if (strcmp(key, "single_scratchpad") == 0) {
		config->single_scratchpad = atoi(value);
	} else if (strcmp(key, "xwayland_persistence") == 0) {
		config->xwayland_persistence = atoi(value);
	} else if (strcmp(key, "syncobj_enable") == 0) {
		config->syncobj_enable = atoi(value);
	} else if (strcmp(key, "drag_tile_refresh_interval") == 0) {
		config->drag_tile_refresh_interval = atof(value);
	} else if (strcmp(key, "drag_floating_refresh_interval") == 0) {
		config->drag_floating_refresh_interval = atof(value);
	} else if (strcmp(key, "allow_tearing") == 0) {
		config->allow_tearing = atoi(value);
	} else if (strcmp(key, "allow_shortcuts_inhibit") == 0) {
		config->allow_shortcuts_inhibit = atoi(value);
	} else if (strcmp(key, "allow_lock_transparent") == 0) {
		config->allow_lock_transparent = atoi(value);
	} else if (strcmp(key, "no_border_when_single") == 0) {
		config->no_border_when_single = atoi(value);
	} else if (strcmp(key, "no_radius_when_single") == 0) {
		config->no_radius_when_single = atoi(value);
	} else if (strcmp(key, "snap_distance") == 0) {
		config->snap_distance = atoi(value);
	} else if (strcmp(key, "enable_floating_snap") == 0) {
		config->enable_floating_snap = atoi(value);
	} else if (strcmp(key, "enable_edge_snap") == 0) {
		config->enable_edge_snap = atoi(value);
	} else if (strcmp(key, "edge_snap_threshold") == 0) {
		config->edge_snap_threshold = atoi(value);
	} else if (strcmp(key, "edge_snap_corner_size") == 0) {
		config->edge_snap_corner_size = atoi(value);
	} else if (strcmp(key, "edge_snap_preview_alpha") == 0) {
		config->edge_snap_preview_alpha = atof(value);
	} else if (strcmp(key, "edge_snap_preview_color") == 0) {
		int64_t col = parse_color(value);
		if (col >= 0) {
			convert_hex_to_rgba(config->edge_snap_preview_color, col);
			config->edge_snap_preview_color[3] = config->edge_snap_preview_alpha;
		}
	} else if (strcmp(key, "drag_tile_to_tile") == 0) {
		config->drag_tile_to_tile = atoi(value);
	} else if (strcmp(key, "drag_tile_small") == 0) {
		config->drag_tile_small = atoi(value);
	} else if (strcmp(key, "swipe_min_threshold") == 0) {
		config->swipe_min_threshold = atoi(value);
	} else if (strcmp(key, "focused_opacity") == 0) {
		config->focused_opacity = atof(value);
	} else if (strcmp(key, "unfocused_opacity") == 0) {
		config->unfocused_opacity = atof(value);
	} else if (strcmp(key, "xkb_rules_rules") == 0) {
		strncpy(config->xkb_rules_rules, value,
				sizeof(config->xkb_rules_rules) - 1);
		config->xkb_rules_rules[sizeof(config->xkb_rules_rules) - 1] = '\0';
	} else if (strcmp(key, "xkb_rules_model") == 0) {
		strncpy(config->xkb_rules_model, value,
				sizeof(config->xkb_rules_model) - 1);
		config->xkb_rules_model[sizeof(config->xkb_rules_model) - 1] = '\0';
	} else if (strcmp(key, "xkb_rules_layout") == 0) {
		strncpy(config->xkb_rules_layout, value,
				sizeof(config->xkb_rules_layout) - 1);
		config->xkb_rules_layout[sizeof(config->xkb_rules_layout) - 1] = '\0';
	} else if (strcmp(key, "xkb_rules_variant") == 0) {
		strncpy(config->xkb_rules_variant, value,
				sizeof(config->xkb_rules_variant) - 1);
		config->xkb_rules_variant[sizeof(config->xkb_rules_variant) - 1] = '\0';
	} else if (strcmp(key, "xkb_rules_options") == 0) {
		strncpy(config->xkb_rules_options, value,
				sizeof(config->xkb_rules_options) - 1);
		config->xkb_rules_options[sizeof(config->xkb_rules_options) - 1] = '\0';
	} else if (strcmp(key, "scroller_proportion_preset") == 0) {
		// 1. Count how many commas there are in value and determine the number of floating point numbers that need to be parsed.
		int32_t count = 0; // initialized to 0
		for (const char *p = value; *p; p++) {
			if (*p == ',')
				count++;
		}
		int32_t float_count = count + 1; // The number of floating point numbers is the number of commas plus 1

		// 2. Dynamically allocate memory and store floating point numbers
		config->scroller_proportion_preset =
			(float *)malloc(float_count * sizeof(float));
		if (!config->scroller_proportion_preset) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Memory "
							"allocation failed\n");
			return false;
		}

		// 3. Parse the floating point number in value
		char *value_copy =
			strdup(value); //Copy value, because strtok will modify the original string
		char *token = strtok(value_copy, ",");
		int32_t i = 0;
		float value_set;

		while (token != NULL && i < float_count) {
			if (sscanf(token, "%f", &value_set) != 1) {
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m Invalid float "
						"value in "
						"scroller_proportion_preset: %s\n",
						token);
				free(value_copy);
				free(config->scroller_proportion_preset);
				config->scroller_proportion_preset = NULL;
				return false;
			}

			// Clamp the value between 0.0 and 1.0 (or your desired
			// range)
			config->scroller_proportion_preset[i] =
				CLAMP_FLOAT(value_set, 0.1f, 1.0f);

			token = strtok(NULL, ",");
			i++;
		}

		// 4. Check whether the number of parsed floating point numbers matches
		if (i != float_count) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid "
					"scroller_proportion_preset format: %s\n",
					value);
			free(value_copy);
			free(config->scroller_proportion_preset);  // Release allocated memory
			config->scroller_proportion_preset = NULL; // Prevent wild pointers
			config->scroller_proportion_preset_count = 0;
			return false;
		}
		config->scroller_proportion_preset_count = float_count;

		// 5. Release the temporarily copied string
		free(value_copy);
	} else if (strcmp(key, "circle_layout") == 0) {
		// 1. Count how many commas there are in value to determine the number of strings that need to be parsed
		int32_t count = 0; // initialized to 0
		for (const char *p = value; *p; p++) {
			if (*p == ',')
				count++;
		}
		int32_t string_count = count + 1; // The number of strings is the number of commas plus 1

		// 2. Dynamically allocate memory and store string pointers
		config->circle_layout = (char **)malloc(string_count * sizeof(char *));
		memset(config->circle_layout, 0, string_count * sizeof(char *));
		if (!config->circle_layout) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Memory "
							"allocation failed\n");
			return false;
		}

		// 3. Parse the string in value
		char *value_copy =
			strdup(value); //Copy value, because strtok will modify the original string
		char *token = strtok(value_copy, ",");
		int32_t i = 0;
		char *cleaned_token;
		while (token != NULL && i < string_count) {
			// Allocate memory for each string and copy the contents
			cleaned_token = sanitize_string(token);
			config->circle_layout[i] = strdup(cleaned_token);
			if (!config->circle_layout[i]) {
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m Memory allocation "
						"failed for "
						"string: %s\n",
						token);
				// Release previously allocated memory
				for (int32_t j = 0; j < i; j++) {
					free(config->circle_layout[j]);
				}
				free(config->circle_layout);
				free(value_copy);
				config->circle_layout = NULL; // Prevent wild pointers
				config->circle_layout_count = 0;
				return false;
			}
			token = strtok(NULL, ",");
			i++;
		}

		// 4. Check whether the number of parsed strings matches
		if (i != string_count) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid circle_layout "
					"format: %s\n",
					value);
			// Release previously allocated memory
			for (int32_t j = 0; j < i; j++) {
				free(config->circle_layout[j]);
			}
			free(config->circle_layout);
			free(value_copy);
			config->circle_layout = NULL; // Prevent wild pointers
			config->circle_layout_count = 0;
			return false;
		}
		config->circle_layout_count = string_count;

		// 5. Release the temporarily copied string
		free(value_copy);
	} else if (strcmp(key, "new_is_master") == 0) {
		config->new_is_master = atoi(value);
	} else if (strcmp(key, "default_mfact") == 0) {
		config->default_mfact = atof(value);
	} else if (strcmp(key, "default_nmaster") == 0) {
		config->default_nmaster = atoi(value);
	} else if (strcmp(key, "center_master_overspread") == 0) {
		config->center_master_overspread = atoi(value);
	} else if (strcmp(key, "center_when_single_stack") == 0) {
		config->center_when_single_stack = atoi(value);
	} else if (strcmp(key, "dwindle_vsplit") == 0) {
		config->dwindle_vsplit = atoi(value);
	} else if (strcmp(key, "dwindle_hsplit") == 0) {
		config->dwindle_hsplit = atoi(value);
	} else if (strcmp(key, "dwindle_preserve_split") == 0) {
		config->dwindle_preserve_split = atoi(value);
	} else if (strcmp(key, "dwindle_smart_split") == 0) {
		config->dwindle_smart_split = atoi(value);
	} else if (strcmp(key, "dwindle_smart_resize") == 0) {
		config->dwindle_smart_resize = atoi(value);
	} else if (strcmp(key, "dwindle_drop_simple_split") == 0) {
		config->dwindle_drop_simple_split = atoi(value);
	} else if (strcmp(key, "dwindle_split_ratio") == 0) {
		config->dwindle_split_ratio = atof(value);
	} else if (strcmp(key, "hotarea_size") == 0) {
		config->hotarea_size = atoi(value);
	} else if (strcmp(key, "hotarea_corner") == 0) {
		config->hotarea_corner = atoi(value);
	} else if (strcmp(key, "enable_hotarea") == 0) {
		config->enable_hotarea = atoi(value);
	} else if (strcmp(key, "ov_tab_mode") == 0) {
		config->ov_tab_mode = atoi(value);
	} else if (strcmp(key, "overviewgappi") == 0) {
		config->overviewgappi = atoi(value);
	} else if (strcmp(key, "overviewgappo") == 0) {
		config->overviewgappo = atoi(value);
	} else if (strcmp(key, "cursor_hide_timeout") == 0) {
		config->cursor_hide_timeout = atoi(value);
	} else if (strcmp(key, "axis_bind_apply_timeout") == 0) {
		config->axis_bind_apply_timeout = atoi(value);
	} else if (strcmp(key, "focus_on_activate") == 0) {
		config->focus_on_activate = atoi(value);
	} else if (strcmp(key, "numlockon") == 0) {
		config->numlockon = atoi(value);
	} else if (strcmp(key, "idleinhibit_ignore_visible") == 0) {
		config->idleinhibit_ignore_visible = atoi(value);
	} else if (strcmp(key, "sloppyfocus") == 0) {
		config->sloppyfocus = atoi(value);
	} else if (strcmp(key, "warpcursor") == 0) {
		config->warpcursor = atoi(value);
	} else if (strcmp(key, "drag_corner") == 0) {
		config->drag_corner = atoi(value);
	} else if (strcmp(key, "drag_warp_cursor") == 0) {
		config->drag_warp_cursor = atoi(value);
	} else if (strcmp(key, "smartgaps") == 0) {
		config->smartgaps = atoi(value);
	} else if (strcmp(key, "repeat_rate") == 0) {
		config->repeat_rate = atoi(value);
	} else if (strcmp(key, "repeat_delay") == 0) {
		config->repeat_delay = atoi(value);
	} else if (strcmp(key, "disable_trackpad") == 0) {
		config->disable_trackpad = atoi(value);
	} else if (strcmp(key, "tap_to_click") == 0) {
		config->tap_to_click = atoi(value);
	} else if (strcmp(key, "tap_and_drag") == 0) {
		config->tap_and_drag = atoi(value);
	} else if (strcmp(key, "drag_lock") == 0) {
		config->drag_lock = atoi(value);
	} else if (strcmp(key, "mouse_natural_scrolling") == 0) {
		config->mouse_natural_scrolling = atoi(value);
	} else if (strcmp(key, "trackpad_natural_scrolling") == 0) {
		config->trackpad_natural_scrolling = atoi(value);
	} else if (strcmp(key, "cursor_size") == 0) {
		config->cursor_size = atoi(value);
	} else if (strcmp(key, "cursor_theme") == 0) {
		config->cursor_theme = strdup(value);
	} else if (strcmp(key, "disable_while_typing") == 0) {
		config->disable_while_typing = atoi(value);
	} else if (strcmp(key, "left_handed") == 0) {
		config->left_handed = atoi(value);
	} else if (strcmp(key, "middle_button_emulation") == 0) {
		config->middle_button_emulation = atoi(value);
	} else if (strcmp(key, "mouse_accel_profile") == 0) {
		config->mouse_accel_profile = atoi(value);
	} else if (strcmp(key, "mouse_accel_speed") == 0) {
		config->mouse_accel_speed = atof(value);
	} else if (strcmp(key, "trackpad_accel_profile") == 0) {
		config->trackpad_accel_profile = atoi(value);
	} else if (strcmp(key, "trackpad_accel_speed") == 0) {
		config->trackpad_accel_speed = atof(value);
	} else if (strcmp(key, "scroll_method") == 0) {
		config->scroll_method = atoi(value);
	} else if (strcmp(key, "scroll_button") == 0) {
		config->scroll_button = atoi(value);
	} else if (strcmp(key, "click_method") == 0) {
		config->click_method = atoi(value);
	} else if (strcmp(key, "send_events_mode") == 0) {
		config->send_events_mode = atoi(value);
	} else if (strcmp(key, "button_map") == 0) {
		config->button_map = atoi(value);
	} else if (strcmp(key, "axis_scroll_factor") == 0) {
		config->axis_scroll_factor = atof(value);
	} else if (strcmp(key, "trackpad_scroll_factor") == 0) {
		config->trackpad_scroll_factor = atof(value);
	} else if (strcmp(key, "gappih") == 0) {
		config->gappih = atoi(value);
	} else if (strcmp(key, "gappiv") == 0) {
		config->gappiv = atoi(value);
	} else if (strcmp(key, "gap_outer_left") == 0) {
		config->gap_outer_left = atoi(value);
	} else if (strcmp(key, "gap_outer_right") == 0) {
		config->gap_outer_right = atoi(value);
	} else if (strcmp(key, "gappoh") == 0) {
		/* backward compatibility: set both left and right outer horizontal gaps */
		config->gap_outer_left  = atoi(value);
		config->gap_outer_right = atoi(value);
	} else if (strcmp(key, "gap_outer_top") == 0) {
		config->gap_outer_top = atoi(value);
	} else if (strcmp(key, "gap_outer_bottom") == 0) {
		config->gap_outer_bottom = atoi(value);
	} else if (strcmp(key, "scratchpad_width_ratio") == 0) {
		config->scratchpad_width_ratio = atof(value);
	} else if (strcmp(key, "scratchpad_height_ratio") == 0) {
		config->scratchpad_height_ratio = atof(value);
	} else if (strcmp(key, "borderpx") == 0) {
		config->borderpx = atoi(value);
	} else if (strcmp(key, "rootcolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid rootcolor "
					"format: "
					"%s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->rootcolor, color);
		}

	} else if (strcmp(key, "shadowscolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid shadowscolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->shadowscolor, color);
		}
	} else if (strcmp(key, "bordercolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid bordercolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->bordercolor, color);
		}
	} else if (strcmp(key, "dropcolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid dropcolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->dropcolor, color);
		}
	} else if (strcmp(key, "focuscolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid focuscolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->focuscolor, color);
		}
	} else if (strcmp(key, "maximizescreencolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid "
					"maximizescreencolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->maximizescreencolor, color);
		}
	} else if (strcmp(key, "urgentcolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid urgentcolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->urgentcolor, color);
		}
	} else if (strcmp(key, "switcher_card_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_card_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_card_color, color);
	} else if (strcmp(key, "switcher_panel_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_panel_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_panel_color, color);
	} else if (strcmp(key, "switcher_outline_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_outline_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_outline_color, color);
	} else if (strcmp(key, "switcher_select_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_select_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_select_color, color);
	} else if (strcmp(key, "switcher_backdrop_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_backdrop_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_backdrop_color, color);
	} else if (strcmp(key, "switcher_minimize_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_minimize_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_minimize_color, color);
	} else if (strcmp(key, "switcher_maximize_color") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Invalid switcher_maximize_color: %s\n", value);
			return false;
		}
		convert_hex_to_rgba(config->switcher_maximize_color, color);
	} else if (strcmp(key, "switcher_max_cards") == 0) {
		config->switcher_max_cards = (uint32_t)atoi(value);
	} else if (strcmp(key, "switcher_card_w") == 0) {
		config->switcher_card_w_px = (uint32_t)atoi(value);
	} else if (strcmp(key, "switcher_card_h") == 0) {
		config->switcher_card_h_px = (uint32_t)atoi(value);
	} else if (strcmp(key, "switcher_pad") == 0) {
		config->switcher_pad_px = (uint32_t)atoi(value);
	} else if (strcmp(key, "switcher_gap") == 0) {
		config->switcher_gap_px = (uint32_t)atoi(value);
	} else if (strcmp(key, "switcher_modifier") == 0) {
		uint32_t m = parse_mod(value);
		if (m != UINT32_MAX)
			config->switcher_modifier = m;
	} else if (strcmp(key, "scratchpadcolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid "
					"scratchpadcolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->scratchpadcolor, color);
		}
	} else if (strcmp(key, "globalcolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid globalcolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->globalcolor, color);
		}
	} else if (strcmp(key, "overlaycolor") == 0) {
		int64_t color = parse_color(value);
		if (color == -1) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid overlaycolor "
					"format: %s\n",
					value);
			return false;
		} else {
			convert_hex_to_rgba(config->overlaycolor, color);
		}
	} else if (strcmp(key, "monitorrule") == 0) {
		config->monitor_rules =
			realloc(config->monitor_rules, (config->monitor_rules_count + 1) *
											   sizeof(ConfigMonitorRule));
		if (!config->monitor_rules) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for monitor rules\n");
			return false;
		}

		ConfigMonitorRule *rule =
			&config->monitor_rules[config->monitor_rules_count];
		memset(rule, 0, sizeof(ConfigMonitorRule));

		//Set default value
		rule->name = NULL;
		rule->make = NULL;
		rule->model = NULL;
		rule->serial = NULL;
		rule->rr = 0;
		rule->scale = 1.0f;
		rule->x = INT32_MAX;
		rule->y = INT32_MAX;
		rule->width = -1;
		rule->height = -1;
		rule->refresh = 0.0f;
		rule->vrr = 0;
		rule->custom = 0;

		bool parse_error = false;
		char *token = strtok(value, ",");
		while (token != NULL) {
			char *colon = strchr(token, ':');
			if (colon != NULL) {
				*colon = '\0';
				char *key = token;
				char *val = colon + 1;

				trim_whitespace(key);
				trim_whitespace(val);

				if (strcmp(key, "name") == 0) {
					rule->name = strdup(val);
				} else if (strcmp(key, "make") == 0) {
					rule->make = strdup(val);
				} else if (strcmp(key, "model") == 0) {
					rule->model = strdup(val);
				} else if (strcmp(key, "serial") == 0) {
					rule->serial = strdup(val);
				} else if (strcmp(key, "rr") == 0) {
					rule->rr = CLAMP_INT(atoi(val), 0, 7);
				} else if (strcmp(key, "scale") == 0) {
					rule->scale = CLAMP_FLOAT(atof(val), 0.001f, 1000.0f);
				} else if (strcmp(key, "x") == 0) {
					rule->x = atoi(val);
				} else if (strcmp(key, "y") == 0) {
					rule->y = atoi(val);
				} else if (strcmp(key, "width") == 0) {
					rule->width = CLAMP_INT(atoi(val), 1, INT32_MAX);
				} else if (strcmp(key, "height") == 0) {
					rule->height = CLAMP_INT(atoi(val), 1, INT32_MAX);
				} else if (strcmp(key, "refresh") == 0) {
					rule->refresh = CLAMP_FLOAT(atof(val), 0.001f, 1000.0f);
				} else if (strcmp(key, "vrr") == 0) {
					rule->vrr = CLAMP_INT(atoi(val), 0, 1);
				} else if (strcmp(key, "custom") == 0) {
					rule->custom = CLAMP_INT(atoi(val), 0, 1);
				} else {
					fprintf(stderr,
							"\033[1m\033[31m[ERROR]:\033[33m Unknown "
							"monitor rule "
							"option:\033[1m\033[31m %s\n",
							key);
					parse_error = true;
				}
			}
			token = strtok(NULL, ",");
		}

		if (!rule->name && !rule->make && !rule->model && !rule->serial) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Monitor rule "
							"must have at least one of the following "
							"options: name, make, model, serial\n");
			return false;
		}

		config->monitor_rules_count++;
		return !parse_error;
	} else if (strcmp(key, "tagrule") == 0) {
		config->tag_rules =
			realloc(config->tag_rules,
					(config->tag_rules_count + 1) * sizeof(ConfigTagRule));
		if (!config->tag_rules) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for tag rules\n");
			return false;
		}

		ConfigTagRule *rule = &config->tag_rules[config->tag_rules_count];
		memset(rule, 0, sizeof(ConfigTagRule));

		//Set default value
		rule->id = 0;
		rule->layout_name = NULL;
		rule->monitor_name = NULL;
		rule->monitor_make = NULL;
		rule->monitor_model = NULL;
		rule->monitor_serial = NULL;
		rule->nmaster = 0;
		rule->mfact = 0.0f;
		rule->no_render_border = 0;
		rule->open_as_floating = 0;
		rule->no_hide = 0;

		bool parse_error = false;
		char *token = strtok(value, ",");
		while (token != NULL) {
			char *colon = strchr(token, ':');
			if (colon != NULL) {
				*colon = '\0';
				char *key = token;
				char *val = colon + 1;

				trim_whitespace(key);
				trim_whitespace(val);

				if (strcmp(key, "id") == 0) {
					rule->id = CLAMP_INT(atoi(val), 0, LENGTH(tags));
				} else if (strcmp(key, "layout_name") == 0) {
					rule->layout_name = strdup(val);
				} else if (strcmp(key, "monitor_name") == 0) {
					rule->monitor_name = strdup(val);
				} else if (strcmp(key, "monitor_make") == 0) {
					rule->monitor_make = strdup(val);
				} else if (strcmp(key, "monitor_model") == 0) {
					rule->monitor_model = strdup(val);
				} else if (strcmp(key, "monitor_serial") == 0) {
					rule->monitor_serial = strdup(val);
				} else if (strcmp(key, "no_render_border") == 0) {
					rule->no_render_border = CLAMP_INT(atoi(val), 0, 1);
				} else if (strcmp(key, "open_as_floating") == 0) {
					rule->open_as_floating = CLAMP_INT(atoi(val), 0, 1);
				} else if (strcmp(key, "no_hide") == 0) {
					rule->no_hide = CLAMP_INT(atoi(val), 0, 1);
				} else if (strcmp(key, "nmaster") == 0) {
					rule->nmaster = CLAMP_INT(atoi(val), 1, 99);
				} else if (strcmp(key, "mfact") == 0) {
					rule->mfact = CLAMP_FLOAT(atof(val), 0.1f, 0.9f);
				} else {
					fprintf(stderr,
							"\033[1m\033[31m[ERROR]:\033[33m Unknown "
							"tag rule "
							"option:\033[1m\033[31m %s\n",
							key);
					parse_error = true;
				}
			}
			token = strtok(NULL, ",");
		}

		config->tag_rules_count++;
		return !parse_error;
	} else if (strcmp(key, "layerrule") == 0) {
		config->layer_rules =
			realloc(config->layer_rules,
					(config->layer_rules_count + 1) * sizeof(ConfigLayerRule));
		if (!config->layer_rules) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for layer rules\n");
			return false;
		}

		ConfigLayerRule *rule = &config->layer_rules[config->layer_rules_count];
		memset(rule, 0, sizeof(ConfigLayerRule));

		//Set default value
		rule->layer_name = NULL;
		rule->animation_type_open = NULL;
		rule->animation_type_close = NULL;
		rule->noblur = 0;
		rule->noanim = 0;
		rule->noshadow = 0;

		bool parse_error = false;
		char *token = strtok(value, ",");
		while (token != NULL) {
			char *colon = strchr(token, ':');
			if (colon != NULL) {
				*colon = '\0';
				char *key = token;
				char *val = colon + 1;

				trim_whitespace(key);
				trim_whitespace(val);

				if (strcmp(key, "layer_name") == 0) {
					rule->layer_name = strdup(val);
				} else if (strcmp(key, "animation_type_open") == 0) {
					rule->animation_type_open = strdup(val);
				} else if (strcmp(key, "animation_type_close") == 0) {
					rule->animation_type_close = strdup(val);
				} else if (strcmp(key, "noblur") == 0) {
					rule->noblur = CLAMP_INT(atoi(val), 0, 1);
				} else if (strcmp(key, "noanim") == 0) {
					rule->noanim = CLAMP_INT(atoi(val), 0, 1);
				} else if (strcmp(key, "noshadow") == 0) {
					rule->noshadow = CLAMP_INT(atoi(val), 0, 1);
				} else {
					fprintf(stderr,
							"\033[1m\033[31m[ERROR]:\033[33m Unknown "
							"layer rule "
							"option:\033[1m\033[31m %s\n",
							key);
					parse_error = true;
				}
			}
			token = strtok(NULL, ",");
		}

		// If no layout name is specified, the default value is used
		if (rule->layer_name == NULL) {
			rule->layer_name = strdup("default");
		}

		config->layer_rules_count++;
		return !parse_error;
	} else if (strcmp(key, "windowrule") == 0) {
		config->window_rules =
			realloc(config->window_rules,
					(config->window_rules_count + 1) * sizeof(ConfigWinRule));
		if (!config->window_rules) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for window rules\n");
			return false;
		}

		ConfigWinRule *rule = &config->window_rules[config->window_rules_count];
		memset(rule, 0, sizeof(ConfigWinRule));

		// int32_t rule value, relay to a client property
		rule->isfloating = -1;
		rule->isfullscreen = -1;
		rule->isfakefullscreen = -1;
		rule->isnoborder = -1;
		rule->isnoshadow = -1;
		rule->isnoradius = -1;
		rule->isnoanimation = -1;
		rule->isopensilent = -1;
		rule->istagsilent = -1;
		rule->isnamedscratchpad = -1;
		rule->isunglobal = -1;
		rule->isglobal = -1;
		rule->isoverlay = -1;
		rule->allow_shortcuts_inhibit = -1;
		rule->ignore_maximize = -1;
		rule->ignore_minimize = -1;
		rule->isnosizehint = -1;
		rule->indleinhibit_when_focus = -1;
		rule->isterm = -1;
		rule->allow_csd = -1;
		rule->force_fakemaximize = -1;
		rule->force_tiled_state = -1;
		rule->force_tearing = -1;
		rule->noswallow = -1;
		rule->noblur = -1;
		rule->nofocus = -1;
		rule->nofadein = -1;
		rule->nofadeout = -1;
		rule->no_force_center = -1;

		// string rule value, relay to a client property
		rule->animation_type_open = NULL;
		rule->animation_type_close = NULL;

		// float rule value, relay to a client property
		rule->focused_opacity = 0;
		rule->unfocused_opacity = 0;
		rule->scroller_proportion_single = 0.0f;
		rule->scroller_proportion = 0;

		// special rule value,not directly set to client property
		rule->tags = 0;
		rule->offsetx = 0;
		rule->offsety = 0;
		rule->width = 0;
		rule->height = 0;
		rule->monitor = NULL;
		rule->id = NULL;
		rule->title = NULL;

		rule->globalkeybinding = (KeyBinding){0};

		bool parse_error = false;
		char *token = strtok(value, ",");
		while (token != NULL) {
			char *colon = strchr(token, ':');
			if (colon != NULL) {
				*colon = '\0';
				char *key = token;
				char *val = colon + 1;

				trim_whitespace(key);
				trim_whitespace(val);

				if (strcmp(key, "isfloating") == 0) {
					rule->isfloating = atoi(val);
				} else if (strcmp(key, "title") == 0) {
					rule->title = strdup(val);
				} else if (strcmp(key, "appid") == 0) {
					rule->id = strdup(val);
				} else if (strcmp(key, "animation_type_open") == 0) {
					rule->animation_type_open = strdup(val);
				} else if (strcmp(key, "animation_type_close") == 0) {
					rule->animation_type_close = strdup(val);
				} else if (strcmp(key, "tags") == 0) {
					rule->tags = 1 << (atoi(val) - 1);
				} else if (strcmp(key, "monitor") == 0) {
					rule->monitor = strdup(val);
				} else if (strcmp(key, "offsetx") == 0) {
					rule->offsetx = atoi(val);
				} else if (strcmp(key, "offsety") == 0) {
					rule->offsety = atoi(val);
				} else if (strcmp(key, "nofocus") == 0) {
					rule->nofocus = atoi(val);
				} else if (strcmp(key, "nofadein") == 0) {
					rule->nofadein = atoi(val);
				} else if (strcmp(key, "nofadeout") == 0) {
					rule->nofadeout = atoi(val);
				} else if (strcmp(key, "no_force_center") == 0) {
					rule->no_force_center = atoi(val);
				} else if (strcmp(key, "width") == 0) {
					rule->width = atof(val);
				} else if (strcmp(key, "height") == 0) {
					rule->height = atof(val);
				} else if (strcmp(key, "isnoborder") == 0) {
					rule->isnoborder = atoi(val);
				} else if (strcmp(key, "isnoshadow") == 0) {
					rule->isnoshadow = atoi(val);
				} else if (strcmp(key, "isnoradius") == 0) {
					rule->isnoradius = atoi(val);
				} else if (strcmp(key, "isnoanimation") == 0) {
					rule->isnoanimation = atoi(val);
				} else if (strcmp(key, "isopensilent") == 0) {
					rule->isopensilent = atoi(val);
				} else if (strcmp(key, "istagsilent") == 0) {
					rule->istagsilent = atoi(val);
				} else if (strcmp(key, "isnamedscratchpad") == 0) {
					rule->isnamedscratchpad = atoi(val);
				} else if (strcmp(key, "isunglobal") == 0) {
					rule->isunglobal = atoi(val);
				} else if (strcmp(key, "isglobal") == 0) {
					rule->isglobal = atoi(val);
				} else if (strcmp(key, "scroller_proportion_single") == 0) {
					rule->scroller_proportion_single = atof(val);
				} else if (strcmp(key, "unfocused_opacity") == 0) {
					rule->unfocused_opacity = atof(val);
				} else if (strcmp(key, "focused_opacity") == 0) {
					rule->focused_opacity = atof(val);
				} else if (strcmp(key, "isoverlay") == 0) {
					rule->isoverlay = atoi(val);
				} else if (strcmp(key, "allow_shortcuts_inhibit") == 0) {
					rule->allow_shortcuts_inhibit = atoi(val);
				} else if (strcmp(key, "ignore_maximize") == 0) {
					rule->ignore_maximize = atoi(val);
				} else if (strcmp(key, "ignore_minimize") == 0) {
					rule->ignore_minimize = atoi(val);
				} else if (strcmp(key, "isnosizehint") == 0) {
					rule->isnosizehint = atoi(val);
				} else if (strcmp(key, "indleinhibit_when_focus") == 0) {
					rule->indleinhibit_when_focus = atoi(val);
				} else if (strcmp(key, "isterm") == 0) {
					rule->isterm = atoi(val);
				} else if (strcmp(key, "allow_csd") == 0) {
					rule->allow_csd = atoi(val);
				} else if (strcmp(key, "force_fakemaximize") == 0) {
					rule->force_fakemaximize = atoi(val);
				} else if (strcmp(key, "force_tiled_state") == 0) {
					rule->force_tiled_state = atoi(val);
				} else if (strcmp(key, "force_tearing") == 0) {
					rule->force_tearing = atoi(val);
				} else if (strcmp(key, "noswallow") == 0) {
					rule->noswallow = atoi(val);
				} else if (strcmp(key, "noblur") == 0) {
					rule->noblur = atoi(val);
				} else if (strcmp(key, "scroller_proportion") == 0) {
					rule->scroller_proportion = atof(val);
				} else if (strcmp(key, "isfullscreen") == 0) {
					rule->isfullscreen = atoi(val);
				} else if (strcmp(key, "isfakefullscreen") == 0) {
					rule->isfakefullscreen = atoi(val);
				} else if (strcmp(key, "globalkeybinding") == 0) {
					char mod_str[256], keysym_str[256];
					sscanf(val, "%255[^-]-%255[a-zA-Z]", mod_str, keysym_str);
					trim_whitespace(mod_str);
					trim_whitespace(keysym_str);
					rule->globalkeybinding.mod = parse_mod(mod_str);
					rule->globalkeybinding.keysymcode =
						parse_key(keysym_str, false);
					if (rule->globalkeybinding.mod == UINT32_MAX) {
						return false;
					}
					if (rule->globalkeybinding.keysymcode.type ==
							KEY_TYPE_SYM &&
						rule->globalkeybinding.keysymcode.keysym ==
							XKB_KEY_NoSymbol) {
						return false;
					}
				} else {
					fprintf(stderr,
							"\033[1m\033[31m[ERROR]:\033[33m Unknown "
							"window rule "
							"option:\033[1m\033[31m %s\n",
							key);
					parse_error = true;
				}
			}
			token = strtok(NULL, ",");
		}
		config->window_rules_count++;
		return !parse_error;
	} else if (strncmp(key, "env", 3) == 0) {

		char env_type[256], env_value[256];
		if (sscanf(value, "%255[^,],%255[^\n]", env_type, env_value) < 2) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid bind format: "
					"\033[1m\033[31m%s\n",
					value);
			return false;
		}
		trim_whitespace(env_type);
		trim_whitespace(env_value);

		ConfigEnv *env = calloc(1, sizeof(ConfigEnv));
		env->type = strdup(env_type);
		env->value = strdup(env_value);

		config->env =
			realloc(config->env, (config->env_count + 1) * sizeof(ConfigEnv));
		if (!config->env) {
			free(env->type);
			free(env->value);
			free(env);
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Failed to "
							"allocate memory for env\n");
			return false;
		}

		config->env[config->env_count] = env;
		config->env_count++;

	} else if (strncmp(key, "exec", 9) == 0) {
		char **new_exec =
			realloc(config->exec, (config->exec_count + 1) * sizeof(char *));
		if (!new_exec) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for exec\n");
			return false;
		}
		config->exec = new_exec;

		config->exec[config->exec_count] = strdup(value);
		if (!config->exec[config->exec_count]) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m Failed to "
							"duplicate exec string\n");
			return false;
		}

		config->exec_count++;

	} else if (strncmp(key, "exec-once", 9) == 0) {

		char **new_exec_once = realloc(
			config->exec_once, (config->exec_once_count + 1) * sizeof(char *));
		if (!new_exec_once) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for exec_once\n");
			return false;
		}
		config->exec_once = new_exec_once;

		config->exec_once[config->exec_once_count] = strdup(value);
		if (!config->exec_once[config->exec_once_count]) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to duplicate "
					"exec_once string\n");
			return false;
		}

		config->exec_once_count++;

	} else if (regex_match("^bind[s|l|r|p]*$", key)) {
		config->key_bindings =
			realloc(config->key_bindings,
					(config->key_bindings_count + 1) * sizeof(KeyBinding));
		if (!config->key_bindings) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for key bindings\n");
			return false;
		}

		KeyBinding *binding = &config->key_bindings[config->key_bindings_count];
		memset(binding, 0, sizeof(KeyBinding));

		char mod_str[256], keysym_str[256], func_name[256],
			arg_value[256] = "0\0", arg_value2[256] = "0\0",
			arg_value3[256] = "0\0", arg_value4[256] = "0\0",
			arg_value5[256] = "0\0";
		if (sscanf(value,
				   "%255[^,],%255[^,],%255[^,],%255[^,],%255[^,],%255[^"
				   ",],%255["
				   "^,],%255[^\n]",
				   mod_str, keysym_str, func_name, arg_value, arg_value2,
				   arg_value3, arg_value4, arg_value5) < 3) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid bind format: "
					"\033[1m\033[31m%s\n",
					value);
			return false;
		}
		trim_whitespace(mod_str);
		trim_whitespace(keysym_str);
		trim_whitespace(func_name);
		trim_whitespace(arg_value);
		trim_whitespace(arg_value2);
		trim_whitespace(arg_value3);
		trim_whitespace(arg_value4);
		trim_whitespace(arg_value5);

		strcpy(binding->mode, config->keymode);
		if (strcmp(binding->mode, "common") == 0) {
			binding->iscommonmode = true;
			binding->isdefaultmode = false;
		} else if (strcmp(binding->mode, "default") == 0) {
			binding->isdefaultmode = true;
			binding->iscommonmode = false;
		} else {
			binding->isdefaultmode = false;
			binding->iscommonmode = false;
		}

		parse_bind_flags(key, binding);
		binding->keysymcode =
			parse_key(keysym_str, binding->keysymcode.type == KEY_TYPE_SYM);
		binding->mod = parse_mod(mod_str);
		binding->arg.i = 0;
		binding->arg.i2 = 0;
		binding->arg.f = 0.0f;
		binding->arg.f2 = 0.0f;
		binding->arg.ui = 0;
		binding->arg.ui2 = 0;
		binding->arg.v = NULL;
		binding->arg.v2 = NULL;
		binding->arg.v3 = NULL;
		binding->func =
			parse_func_name(func_name, &binding->arg, arg_value, arg_value2,
							arg_value3, arg_value4, arg_value5);
		if (!binding->func || binding->mod == UINT32_MAX ||
			(binding->keysymcode.type == KEY_TYPE_SYM &&
			 binding->keysymcode.keysym == XKB_KEY_NoSymbol)) {
			if (binding->arg.v) {
				free(binding->arg.v);
				binding->arg.v = NULL;
			}
			if (binding->arg.v2) {
				free(binding->arg.v2);
				binding->arg.v2 = NULL;
			}
			if (binding->arg.v3) {
				free(binding->arg.v3);
				binding->arg.v3 = NULL;
			}
			if (!binding->func)
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m Unknown "
						"dispatch in bind: "
						"\033[1m\033[31m%s\n",
						func_name);
			return false;
		} else {
			config->key_bindings_count++;
		}

	} else if (strncmp(key, "mousebind", 9) == 0) {
		config->mouse_bindings =
			realloc(config->mouse_bindings,
					(config->mouse_bindings_count + 1) * sizeof(MouseBinding));
		if (!config->mouse_bindings) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for mouse bindings\n");
			return false;
		}

		MouseBinding *binding =
			&config->mouse_bindings[config->mouse_bindings_count];
		memset(binding, 0, sizeof(MouseBinding));

		char mod_str[256], button_str[256], func_name[256],
			arg_value[256] = "0\0", arg_value2[256] = "0\0",
			arg_value3[256] = "0\0", arg_value4[256] = "0\0",
			arg_value5[256] = "0\0";
		if (sscanf(value,
				   "%255[^,],%255[^,],%255[^,],%255[^,],%255[^,],%255[^"
				   ",],%255["
				   "^,],%255[^\n]",
				   mod_str, button_str, func_name, arg_value, arg_value2,
				   arg_value3, arg_value4, arg_value5) < 3) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid mousebind "
					"format: "
					"%s\n",
					value);
			return false;
		}
		trim_whitespace(mod_str);
		trim_whitespace(button_str);
		trim_whitespace(func_name);
		trim_whitespace(arg_value);
		trim_whitespace(arg_value2);
		trim_whitespace(arg_value3);
		trim_whitespace(arg_value4);
		trim_whitespace(arg_value5);

		binding->mod = parse_mod(mod_str);
		binding->button = parse_button(button_str);
		binding->arg.i = 0;
		binding->arg.i2 = 0;
		binding->arg.f = 0.0f;
		binding->arg.f2 = 0.0f;
		binding->arg.ui = 0;
		binding->arg.ui2 = 0;
		binding->arg.v = NULL;
		binding->arg.v2 = NULL;
		binding->arg.v3 = NULL;

		// TODO: remove this in next version
		if (binding->mod == 0 &&
			(binding->button == BTN_LEFT || binding->button == BTN_RIGHT)) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m \033[31m%s\033[33m can't "
					"bind to \033[31m%s\033[33m mod key\n",
					button_str, mod_str);
			return false;
		}

		binding->func =
			parse_func_name(func_name, &binding->arg, arg_value, arg_value2,
							arg_value3, arg_value4, arg_value5);
		if (!binding->func || binding->mod == UINT32_MAX ||
			binding->button == UINT32_MAX) {
			if (binding->arg.v) {
				free(binding->arg.v);
				binding->arg.v = NULL;
			}
			if (binding->arg.v2) {
				free(binding->arg.v2);
				binding->arg.v2 = NULL;
			}
			if (binding->arg.v3) {
				free(binding->arg.v3);
				binding->arg.v3 = NULL;
			}

			if (!binding->func)
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m Unknown "
						"dispatch in "
						"mousebind: \033[1m\033[31m%s\n",
						func_name);
			return false;
		} else {
			config->mouse_bindings_count++;
		}
	} else if (strncmp(key, "axisbind", 8) == 0) {
		config->axis_bindings =
			realloc(config->axis_bindings,
					(config->axis_bindings_count + 1) * sizeof(AxisBinding));
		if (!config->axis_bindings) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for axis bindings\n");
			return false;
		}

		AxisBinding *binding =
			&config->axis_bindings[config->axis_bindings_count];
		memset(binding, 0, sizeof(AxisBinding));

		char mod_str[256], dir_str[256], func_name[256],
			arg_value[256] = "0\0", arg_value2[256] = "0\0",
			arg_value3[256] = "0\0", arg_value4[256] = "0\0",
			arg_value5[256] = "0\0";
		if (sscanf(value,
				   "%255[^,],%255[^,],%255[^,],%255[^,],%255[^,],%255[^"
				   ",],%255["
				   "^,],%255[^\n]",
				   mod_str, dir_str, func_name, arg_value, arg_value2,
				   arg_value3, arg_value4, arg_value5) < 3) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid axisbind "
					"format: %s\n",
					value);
			return false;
		}

		trim_whitespace(mod_str);
		trim_whitespace(dir_str);
		trim_whitespace(func_name);
		trim_whitespace(arg_value);
		trim_whitespace(arg_value2);
		trim_whitespace(arg_value3);
		trim_whitespace(arg_value4);
		trim_whitespace(arg_value5);

		binding->mod = parse_mod(mod_str);
		binding->dir = parse_direction(dir_str);
		binding->arg.v = NULL;
		binding->arg.v2 = NULL;
		binding->arg.v3 = NULL;
		binding->func =
			parse_func_name(func_name, &binding->arg, arg_value, arg_value2,
							arg_value3, arg_value4, arg_value5);

		if (!binding->func || binding->mod == UINT32_MAX) {
			if (binding->arg.v) {
				free(binding->arg.v);
				binding->arg.v = NULL;
			}
			if (binding->arg.v2) {
				free(binding->arg.v2);
				binding->arg.v2 = NULL;
			}
			if (binding->arg.v3) {
				free(binding->arg.v3);
				binding->arg.v3 = NULL;
			}
			if (!binding->func)
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m Unknown "
						"dispatch in "
						"axisbind: \033[1m\033[31m%s\n",
						func_name);
			return false;
		} else {
			config->axis_bindings_count++;
		}

	} else if (strncmp(key, "switchbind", 10) == 0) {
		config->switch_bindings = realloc(config->switch_bindings,
										  (config->switch_bindings_count + 1) *
											  sizeof(SwitchBinding));
		if (!config->switch_bindings) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for switch bindings\n");
			return false;
		}

		SwitchBinding *binding =
			&config->switch_bindings[config->switch_bindings_count];
		memset(binding, 0, sizeof(SwitchBinding));

		char fold_str[256], func_name[256],
			arg_value[256] = "0\0", arg_value2[256] = "0\0",
			arg_value3[256] = "0\0", arg_value4[256] = "0\0",
			arg_value5[256] = "0\0";
		if (sscanf(value,
				   "%255[^,],%255[^,],%255[^,],%255[^,],%255[^,],%255[^"
				   ",],%255["
				   "^\n]",
				   fold_str, func_name, arg_value, arg_value2, arg_value3,
				   arg_value4, arg_value5) < 3) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid switchbind "
					"format: %s\n",
					value);
			return false;
		}
		trim_whitespace(fold_str);
		trim_whitespace(func_name);
		trim_whitespace(arg_value);
		trim_whitespace(arg_value2);
		trim_whitespace(arg_value3);
		trim_whitespace(arg_value4);
		trim_whitespace(arg_value5);

		binding->fold = parse_fold_state(fold_str);
		binding->func =
			parse_func_name(func_name, &binding->arg, arg_value, arg_value2,
							arg_value3, arg_value4, arg_value5);

		if (!binding->func) {
			if (binding->arg.v) {
				free(binding->arg.v);
				binding->arg.v = NULL;
			}
			if (binding->arg.v2) {
				free(binding->arg.v2);
				binding->arg.v2 = NULL;
			}
			if (binding->arg.v3) {
				free(binding->arg.v3);
				binding->arg.v3 = NULL;
			}

			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Unknown dispatch in "
					"switchbind: "
					"\033[1m\033[31m%s\n",
					func_name);
			return false;
		} else {
			config->switch_bindings_count++;
		}

	} else if (strncmp(key, "gesturebind", 11) == 0) {
		config->gesture_bindings = realloc(
			config->gesture_bindings,
			(config->gesture_bindings_count + 1) * sizeof(GestureBinding));
		if (!config->gesture_bindings) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Failed to allocate "
					"memory for axis gesturebind\n");
			return false;
		}

		GestureBinding *binding =
			&config->gesture_bindings[config->gesture_bindings_count];
		memset(binding, 0, sizeof(GestureBinding));

		char mod_str[256], motion_str[256], fingers_count_str[256],
			func_name[256], arg_value[256] = "0\0", arg_value2[256] = "0\0",
							arg_value3[256] = "0\0", arg_value4[256] = "0\0",
							arg_value5[256] = "0\0";
		if (sscanf(value,
				   "%255[^,],%255[^,],%255[^,],%255[^,],%255[^,],%255[^"
				   ",],%255["
				   "^,],%255[^,],%255[^\n]",
				   mod_str, motion_str, fingers_count_str, func_name, arg_value,
				   arg_value2, arg_value3, arg_value4, arg_value5) < 4) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m Invalid gesturebind "
					"format: %s\n",
					value);
			return false;
		}

		trim_whitespace(mod_str);
		trim_whitespace(motion_str);
		trim_whitespace(fingers_count_str);
		trim_whitespace(func_name);
		trim_whitespace(arg_value);
		trim_whitespace(arg_value2);
		trim_whitespace(arg_value3);
		trim_whitespace(arg_value4);
		trim_whitespace(arg_value5);

		binding->mod = parse_mod(mod_str);
		binding->motion = parse_direction(motion_str);
		binding->fingers_count = atoi(fingers_count_str);
		binding->arg.i = 0;
		binding->arg.i2 = 0;
		binding->arg.f = 0.0f;
		binding->arg.f2 = 0.0f;
		binding->arg.ui = 0;
		binding->arg.ui2 = 0;
		binding->arg.v = NULL;
		binding->arg.v2 = NULL;
		binding->arg.v3 = NULL;
		binding->func =
			parse_func_name(func_name, &binding->arg, arg_value, arg_value2,
							arg_value3, arg_value4, arg_value5);

		if (!binding->func || binding->mod == UINT32_MAX) {
			if (binding->arg.v) {
				free(binding->arg.v);
				binding->arg.v = NULL;
			}
			if (binding->arg.v2) {
				free(binding->arg.v2);
				binding->arg.v2 = NULL;
			}
			if (binding->arg.v3) {
				free(binding->arg.v3);
				binding->arg.v3 = NULL;
			}
			if (!binding->func)
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m Unknown "
						"dispatch in "
						"axisbind: \033[1m\033[31m%s\n",
						func_name);
			return false;
		} else {
			config->gesture_bindings_count++;
		}

	} else if (strncmp(key, "source-optional", 15) == 0) {
		parse_config_file(config, value, false);
	} else if (strncmp(key, "source", 6) == 0) {
		parse_config_file(config, value, true);
	} else {
		fprintf(stderr,
				"\033[1m\033[31m[ERROR]:\033[33m Unknown keyword: "
				"\033[1m\033[31m%s\n",
				key);
		return false;
	}

	return true;
}

bool parse_config_line(Config *config, const char *line) {
	char key[256], value[256];
	if (sscanf(line, "%255[^=]=%255[^\n]", key, value) != 2) {
		fprintf(stderr,
				"\033[1m\033[31m[ERROR]:\033[33m Invalid line format: %s",
				line);
		return false;
	}

	// Then trim each part separately
	trim_whitespace(key);
	trim_whitespace(value);

	return parse_option(config, key, value);
}

bool parse_config_file(Config *config, const char *file_path, bool must_exist) {
	FILE *file;
	char full_path[1024];

	if (file_path[0] == '.' && file_path[1] == '/') {
		// Relative path

		if (cli_config_path) {
			char *config_path = strdup(cli_config_path);
			char *config_dir = dirname(config_path);
			snprintf(full_path, sizeof(full_path), "%s/%s", config_dir,
					 file_path + 1);
			free(config_path);
		} else {
			const char *home = getenv("HOME");
			if (!home) {
				fprintf(stderr,
						"\033[1m\033[31m[ERROR]:\033[33m HOME environment "
						"variable not set.\n");
				return false;
			}
			snprintf(full_path, sizeof(full_path), "%s/.config/aether/%s", home,
					 file_path + 1);
		}
		file = fopen(full_path, "r");

	} else if (file_path[0] == '~' &&
			   (file_path[1] == '/' || file_path[1] == '\0')) {
		// Home directory

		const char *home = getenv("HOME");
		if (!home) {
			fprintf(stderr, "\033[1m\033[31m[ERROR]:\033[33m HOME environment "
							"variable not set.\n");
			return false;
		}
		snprintf(full_path, sizeof(full_path), "%s%s", home, file_path + 1);
		file = fopen(full_path, "r");

	} else {
		// Absolute path
		file = fopen(file_path, "r");
	}

	if (!file) {
		if (must_exist) {
			fprintf(stderr,
					"\033[1;31m\033[1;33m[ERROR]:\033[0m Failed to open "
					"config file: %s\n",
					file_path);
			return false;
		} else {
			return true;
		}
	}

	char line[512];
	bool parse_correct = true;
	bool parse_line_correct = true;
	uint32_t line_count = 0;
	while (fgets(line, sizeof(line), file)) {
		line_count++;
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}
		parse_line_correct = parse_config_line(config, line);
		if (!parse_line_correct) {
			parse_correct = false;
			fprintf(stderr,
					"\033[1;31m╰─\033[1;33m[Index]\033[0m "
					"\033[1;36m%s\033[0m:\033[1;35m%d\033[0m\n"
					"   \033[1;36m╰─\033[0;33m%s\033[0m\n\n",
					file_path, line_count, line);
		}
	}

	fclose(file);
	return parse_correct;
}

#endif /* CONFIG_PARSE_H */

#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include "config_free.h"

void override_config(void) {
	config.animations = CLAMP_INT(config.animations, 0, 1);
	config.layer_animations = CLAMP_INT(config.layer_animations, 0, 1);
	config.tag_animation_direction =
		CLAMP_INT(config.tag_animation_direction, 0, 1);
	config.animation_fade_in = CLAMP_INT(config.animation_fade_in, 0, 1);
	config.animation_fade_out = CLAMP_INT(config.animation_fade_out, 0, 1);
	config.zoom_initial_ratio =
		CLAMP_FLOAT(config.zoom_initial_ratio, 0.1f, 1.0f);
	config.zoom_end_ratio = CLAMP_FLOAT(config.zoom_end_ratio, 0.1f, 1.0f);
	config.fadein_begin_opacity =
		CLAMP_FLOAT(config.fadein_begin_opacity, 0.0f, 1.0f);
	config.fadeout_begin_opacity =
		CLAMP_FLOAT(config.fadeout_begin_opacity, 0.0f, 1.0f);
	config.animation_duration_move =
		CLAMP_INT(config.animation_duration_move, 1, 50000);
	config.animation_duration_open =
		CLAMP_INT(config.animation_duration_open, 1, 50000);
	config.animation_duration_tag =
		CLAMP_INT(config.animation_duration_tag, 1, 50000);
	config.animation_duration_close =
		CLAMP_INT(config.animation_duration_close, 1, 50000);
	config.animation_duration_focus =
		CLAMP_INT(config.animation_duration_focus, 1, 50000);
	config.scroller_default_proportion =
		CLAMP_FLOAT(config.scroller_default_proportion, 0.1f, 1.0f);
	config.scroller_default_proportion_single =
		CLAMP_FLOAT(config.scroller_default_proportion_single, 0.1f, 1.0f);
	config.scroller_ignore_proportion_single =
		CLAMP_INT(config.scroller_ignore_proportion_single, 0, 1);
	config.scroller_focus_center =
		CLAMP_INT(config.scroller_focus_center, 0, 1);
	config.scroller_prefer_center =
		CLAMP_INT(config.scroller_prefer_center, 0, 1);
	config.scroller_prefer_overspread =
		CLAMP_INT(config.scroller_prefer_overspread, 0, 1);
	config.edge_scroller_pointer_focus =
		CLAMP_INT(config.edge_scroller_pointer_focus, 0, 1);
	config.scroller_structs = CLAMP_INT(config.scroller_structs, 0, 1000);
	config.default_mfact = CLAMP_FLOAT(config.default_mfact, 0.1f, 0.9f);
	config.default_nmaster = CLAMP_INT(config.default_nmaster, 1, 1000);
	config.center_master_overspread =
		CLAMP_INT(config.center_master_overspread, 0, 1);
	config.center_when_single_stack =
		CLAMP_INT(config.center_when_single_stack, 0, 1);
	config.new_is_master = CLAMP_INT(config.new_is_master, 0, 1);
	config.dwindle_vsplit = CLAMP_INT(config.dwindle_vsplit, 0, 2);
	config.dwindle_hsplit = CLAMP_INT(config.dwindle_hsplit, 0, 2);
	config.dwindle_preserve_split =
		CLAMP_INT(config.dwindle_preserve_split, 0, 1);
	config.dwindle_smart_split = CLAMP_INT(config.dwindle_smart_split, 0, 1);
	config.dwindle_smart_resize = CLAMP_INT(config.dwindle_smart_resize, 0, 1);
	config.dwindle_drop_simple_split =
		CLAMP_INT(config.dwindle_drop_simple_split, 0, 1);
	config.dwindle_split_ratio =
		CLAMP_FLOAT(config.dwindle_split_ratio, 0.05f, 0.95f);
	config.hotarea_size = CLAMP_INT(config.hotarea_size, 1, 1000);
	config.hotarea_corner = CLAMP_INT(config.hotarea_corner, 0, 3);
	config.enable_hotarea = CLAMP_INT(config.enable_hotarea, 0, 1);
	config.ov_tab_mode = CLAMP_INT(config.ov_tab_mode, 0, 1);
	config.overviewgappi = CLAMP_INT(config.overviewgappi, 0, 1000);
	config.overviewgappo = CLAMP_INT(config.overviewgappo, 0, 1000);
	config.xwayland_persistence = CLAMP_INT(config.xwayland_persistence, 0, 1);
	config.syncobj_enable = CLAMP_INT(config.syncobj_enable, 0, 1);
	config.drag_tile_refresh_interval =
		CLAMP_FLOAT(config.drag_tile_refresh_interval, 1.0f, 16.0f);
	config.drag_floating_refresh_interval =
		CLAMP_FLOAT(config.drag_floating_refresh_interval, 0.0f, 1000.0f);
	config.drag_tile_to_tile = CLAMP_INT(config.drag_tile_to_tile, 0, 1);
	config.drag_tile_small = CLAMP_INT(config.drag_tile_small, 0, 1);
	config.allow_tearing = CLAMP_INT(config.allow_tearing, 0, 2);
	config.allow_shortcuts_inhibit =
		CLAMP_INT(config.allow_shortcuts_inhibit, 0, 1);
	config.allow_lock_transparent =
		CLAMP_INT(config.allow_lock_transparent, 0, 1);
	config.axis_bind_apply_timeout =
		CLAMP_INT(config.axis_bind_apply_timeout, 0, 1000);
	config.focus_on_activate = CLAMP_INT(config.focus_on_activate, 0, 1);
	config.idleinhibit_ignore_visible =
		CLAMP_INT(config.idleinhibit_ignore_visible, 0, 1);
	config.sloppyfocus = CLAMP_INT(config.sloppyfocus, 0, 1);
	config.warpcursor = CLAMP_INT(config.warpcursor, 0, 1);
	config.drag_corner = CLAMP_INT(config.drag_corner, 0, 4);
	config.drag_warp_cursor = CLAMP_INT(config.drag_warp_cursor, 0, 1);
	config.focus_cross_monitor = CLAMP_INT(config.focus_cross_monitor, 0, 1);
	config.exchange_cross_monitor =
		CLAMP_INT(config.exchange_cross_monitor, 0, 1);
	config.scratchpad_cross_monitor =
		CLAMP_INT(config.scratchpad_cross_monitor, 0, 1);
	config.focus_cross_tag = CLAMP_INT(config.focus_cross_tag, 0, 1);
	config.view_current_to_back = CLAMP_INT(config.view_current_to_back, 0, 1);
	config.enable_floating_snap = CLAMP_INT(config.enable_floating_snap, 0, 1);
	config.snap_distance = CLAMP_INT(config.snap_distance, 0, 99999);
	config.enable_edge_snap = CLAMP_INT(config.enable_edge_snap, 0, 1);
	config.edge_snap_threshold = CLAMP_INT(config.edge_snap_threshold, 1, 500);
	config.edge_snap_corner_size = CLAMP_INT(config.edge_snap_corner_size, 10, 1000);
	config.edge_snap_preview_alpha = CLAMP_FLOAT(config.edge_snap_preview_alpha, 0.0f, 1.0f);
	/* sync alpha channel to preview color after clamping */
	config.edge_snap_preview_color[3] = config.edge_snap_preview_alpha;
	config.cursor_size = CLAMP_INT(config.cursor_size, 4, 512);
	config.no_border_when_single =
		CLAMP_INT(config.no_border_when_single, 0, 1);
	config.no_radius_when_single =
		CLAMP_INT(config.no_radius_when_single, 0, 1);
	config.cursor_hide_timeout =
		CLAMP_INT(config.cursor_hide_timeout, 0, 36000);
	config.single_scratchpad = CLAMP_INT(config.single_scratchpad, 0, 1);
	config.repeat_rate = CLAMP_INT(config.repeat_rate, 1, 1000);
	config.repeat_delay = CLAMP_INT(config.repeat_delay, 1, 20000);
	config.numlockon = CLAMP_INT(config.numlockon, 0, 1);
	config.disable_trackpad = CLAMP_INT(config.disable_trackpad, 0, 1);
	config.tap_to_click = CLAMP_INT(config.tap_to_click, 0, 1);
	config.tap_and_drag = CLAMP_INT(config.tap_and_drag, 0, 1);
	config.drag_lock = CLAMP_INT(config.drag_lock, 0, 1);
	config.trackpad_natural_scrolling =
		CLAMP_INT(config.trackpad_natural_scrolling, 0, 1);
	config.disable_while_typing = CLAMP_INT(config.disable_while_typing, 0, 1);
	config.left_handed = CLAMP_INT(config.left_handed, 0, 1);
	config.middle_button_emulation =
		CLAMP_INT(config.middle_button_emulation, 0, 1);
	config.swipe_min_threshold = CLAMP_INT(config.swipe_min_threshold, 1, 1000);
	config.mouse_natural_scrolling =
		CLAMP_INT(config.mouse_natural_scrolling, 0, 1);
	config.mouse_accel_profile = CLAMP_INT(config.mouse_accel_profile, 0, 2);
	config.mouse_accel_speed =
		CLAMP_FLOAT(config.mouse_accel_speed, -1.0f, 1.0f);
	config.trackpad_accel_profile =
		CLAMP_INT(config.trackpad_accel_profile, 0, 2);
	config.trackpad_accel_speed =
		CLAMP_FLOAT(config.trackpad_accel_speed, -1.0f, 1.0f);
	config.scroll_method = CLAMP_INT(config.scroll_method, 0, 4);
	config.scroll_button = CLAMP_INT(config.scroll_button, 272, 279);
	config.click_method = CLAMP_INT(config.click_method, 0, 2);
	config.send_events_mode = CLAMP_INT(config.send_events_mode, 0, 2);
	config.button_map = CLAMP_INT(config.button_map, 0, 1);
	config.axis_scroll_factor =
		CLAMP_FLOAT(config.axis_scroll_factor, 0.1f, 10.0f);
	config.trackpad_scroll_factor =
		CLAMP_FLOAT(config.trackpad_scroll_factor, 0.1f, 10.0f);
	config.gappih = CLAMP_INT(config.gappih, 0, 1000);
	config.gappiv = CLAMP_INT(config.gappiv, 0, 1000);
	config.gap_outer_left   = CLAMP_INT(config.gap_outer_left,   0, 1000);
	config.gap_outer_right  = CLAMP_INT(config.gap_outer_right,  0, 1000);
	config.gap_outer_top    = CLAMP_INT(config.gap_outer_top,    0, 1000);
	config.gap_outer_bottom = CLAMP_INT(config.gap_outer_bottom, 0, 1000);
	config.scratchpad_width_ratio =
		CLAMP_FLOAT(config.scratchpad_width_ratio, 0.1f, 1.0f);
	config.scratchpad_height_ratio =
		CLAMP_FLOAT(config.scratchpad_height_ratio, 0.1f, 1.0f);
	config.borderpx = CLAMP_INT(config.borderpx, 0, 200);
	config.smartgaps = CLAMP_INT(config.smartgaps, 0, 1);
	config.blur = CLAMP_INT(config.blur, 0, 1);
	config.blur_layer = CLAMP_INT(config.blur_layer, 0, 1);
	config.blur_optimized = CLAMP_INT(config.blur_optimized, 0, 1);
	config.border_radius = CLAMP_INT(config.border_radius, 0, 100);
	config.blur_params.num_passes =
		CLAMP_INT(config.blur_params.num_passes, 0, 10);
	config.blur_params.radius = CLAMP_INT(config.blur_params.radius, 0, 100);
	config.blur_params.noise = CLAMP_FLOAT(config.blur_params.noise, 0, 1);
	config.blur_params.brightness =
		CLAMP_FLOAT(config.blur_params.brightness, 0, 1);
	config.blur_params.contrast =
		CLAMP_FLOAT(config.blur_params.contrast, 0, 1);
	config.blur_params.saturation =
		CLAMP_FLOAT(config.blur_params.saturation, 0, 1);
	config.shadows = CLAMP_INT(config.shadows, 0, 1);
	config.shadow_only_floating = CLAMP_INT(config.shadow_only_floating, 0, 1);
	config.layer_shadows = CLAMP_INT(config.layer_shadows, 0, 1);
	config.shadows_size = CLAMP_INT(config.shadows_size, 0, 100);
	config.shadows_blur = CLAMP_INT(config.shadows_blur, 0, 100);
	config.shadows_position_x =
		CLAMP_INT(config.shadows_position_x, -1000, 1000);
	config.shadows_position_y =
		CLAMP_INT(config.shadows_position_y, -1000, 1000);
	config.focused_opacity = CLAMP_FLOAT(config.focused_opacity, 0.0f, 1.0f);
	config.unfocused_opacity =
		CLAMP_FLOAT(config.unfocused_opacity, 0.0f, 1.0f);
}

void set_value_default() {
	config.animations = 1;
	config.layer_animations = 0;
	config.animation_fade_in = 1;
	config.animation_fade_out = 1;
	config.tag_animation_direction = HORIZONTAL;
	config.zoom_initial_ratio = 0.4f;
	config.zoom_end_ratio = 0.8f;
	config.fadein_begin_opacity = 0.5f;
	config.fadeout_begin_opacity = 0.5f;
	config.animation_duration_move = 500;
	config.animation_duration_open = 400;
	config.animation_duration_tag = 300;
	config.animation_duration_close = 300;
	config.animation_duration_focus = 0;

	config.axis_bind_apply_timeout = 100;
	config.focus_on_activate = 1;
	config.new_is_master = 1;
	config.default_mfact = 0.55f;
	config.default_nmaster = 1;
	config.center_master_overspread = 0;
	config.center_when_single_stack = 1;

	config.dwindle_vsplit = 0;
	config.dwindle_hsplit = 0;
	config.dwindle_preserve_split = 0;
	config.dwindle_smart_split = 0;
	config.dwindle_smart_resize = 0;
	config.dwindle_drop_simple_split = 1;
	config.dwindle_split_ratio = 0.5f;

	config.log_level = WLR_ERROR;
	config.numlockon = 0;
	config.capslock = 0;

	config.ov_tab_mode = 0;
	config.hotarea_size = 10;
	config.hotarea_corner = BOTTOM_LEFT;
	config.enable_hotarea = 1;
	config.smartgaps = 0;
	config.sloppyfocus = 1;
	config.gappih = 5;
	config.gappiv = 5;
	config.gap_outer_left   = 10;
	config.gap_outer_right  = 10;
	config.gap_outer_top    = 10;
	config.gap_outer_bottom = 10;
	config.scratchpad_width_ratio = 0.8f;
	config.scratchpad_height_ratio = 0.9f;

	config.scroller_structs = 20;
	config.scroller_default_proportion = 0.9f;
	config.scroller_default_proportion_single = 1.0f;
	config.scroller_ignore_proportion_single = 1;
	config.scroller_focus_center = 0;
	config.scroller_prefer_center = 0;
	config.scroller_prefer_overspread = 1;
	config.edge_scroller_pointer_focus = 1;
	config.focus_cross_monitor = 0;
	config.exchange_cross_monitor = 0;
	config.scratchpad_cross_monitor = 0;
	config.focus_cross_tag = 0;
	config.axis_scroll_factor = 1.0;
	config.trackpad_scroll_factor = 1.0;
	config.view_current_to_back = 0;
	config.single_scratchpad = 1;
	config.xwayland_persistence = 1;
	config.syncobj_enable = 0;
	config.drag_tile_refresh_interval = 8.0f;
	config.drag_floating_refresh_interval = 8.0f;
	config.allow_tearing = TEARING_DISABLED;
	config.allow_shortcuts_inhibit = SHORTCUTS_INHIBIT_ENABLE;
	config.allow_lock_transparent = 0;
	config.no_border_when_single = 0;
	config.no_radius_when_single = 0;
	config.snap_distance = 30;
	config.drag_tile_to_tile = 0;
	config.drag_tile_small = 1;
	config.enable_floating_snap = 0;
	config.enable_edge_snap = 1;
	config.edge_snap_threshold = 20;
	config.edge_snap_corner_size = 80;
	config.edge_snap_preview_alpha = 0.3f;
	/* default preview: semi-transparent accent blue */
	config.edge_snap_preview_color[0] = 0.33f;
	config.edge_snap_preview_color[1] = 0.53f;
	config.edge_snap_preview_color[2] = 0.80f;
	config.edge_snap_preview_color[3] = 0.30f;
	config.swipe_min_threshold = 1;

	config.idleinhibit_ignore_visible = 0;

	config.borderpx = 4;
	config.overviewgappi = 5;
	config.overviewgappo = 30;
	config.cursor_hide_timeout = 0;

	config.warpcursor = 1;
	config.drag_corner = 3;
	config.drag_warp_cursor = 1;

	config.repeat_rate = 25;
	config.repeat_delay = 600;

	config.disable_trackpad = 0;
	config.tap_to_click = 1;
	config.tap_and_drag = 1;
	config.drag_lock = 1;
	config.mouse_natural_scrolling = 0;
	config.cursor_size = 24;
	config.trackpad_natural_scrolling = 0;
	config.disable_while_typing = 1;
	config.left_handed = 0;
	config.middle_button_emulation = 0;
	config.mouse_accel_profile = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
	config.mouse_accel_speed = 0.0;
	config.trackpad_accel_profile = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
	config.trackpad_accel_speed = 0.0;
	config.scroll_method = LIBINPUT_CONFIG_SCROLL_2FG;
	config.scroll_button = 274;
	config.click_method = LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;
	config.send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;
	config.button_map = LIBINPUT_CONFIG_TAP_MAP_LRM;

	config.blur = 0;
	config.blur_layer = 0;
	config.blur_optimized = 1;
	config.border_radius = 0;
	config.border_radius_location_default = CORNER_LOCATION_ALL;
	config.blur_params.num_passes = 1;
	config.blur_params.radius = 5;
	config.blur_params.noise = 0.02f;
	config.blur_params.brightness = 0.9f;
	config.blur_params.contrast = 0.9f;
	config.blur_params.saturation = 1.2f;
	config.shadows = 0;
	config.shadow_only_floating = 1;
	config.layer_shadows = 0;
	config.shadows_size = 10;
	config.shadows_blur = 15.0f;
	config.shadows_position_x = 0;
	config.shadows_position_y = 0;
	config.focused_opacity = 1.0f;
	config.unfocused_opacity = 1.0f;

	config.shadowscolor[0] = 0.0f;
	config.shadowscolor[1] = 0.0f;
	config.shadowscolor[2] = 0.0f;
	config.shadowscolor[3] = 1.0f;

	config.animation_curve_move[0] = 0.46;
	config.animation_curve_move[1] = 1.0;
	config.animation_curve_move[2] = 0.29;
	config.animation_curve_move[3] = 0.99;
	config.animation_curve_open[0] = 0.46;
	config.animation_curve_open[1] = 1.0;
	config.animation_curve_open[2] = 0.29;
	config.animation_curve_open[3] = 0.99;
	config.animation_curve_tag[0] = 0.46;
	config.animation_curve_tag[1] = 1.0;
	config.animation_curve_tag[2] = 0.29;
	config.animation_curve_tag[3] = 0.99;
	config.animation_curve_close[0] = 0.46;
	config.animation_curve_close[1] = 1.0;
	config.animation_curve_close[2] = 0.29;
	config.animation_curve_close[3] = 0.99;
	config.animation_curve_focus[0] = 0.46;
	config.animation_curve_focus[1] = 1.0;
	config.animation_curve_focus[2] = 0.29;
	config.animation_curve_focus[3] = 0.99;
	config.animation_curve_opafadein[0] = 0.46;
	config.animation_curve_opafadein[1] = 1.0;
	config.animation_curve_opafadein[2] = 0.29;
	config.animation_curve_opafadein[3] = 0.99;
	config.animation_curve_opafadeout[0] = 0.5;
	config.animation_curve_opafadeout[1] = 0.5;
	config.animation_curve_opafadeout[2] = 0.5;
	config.animation_curve_opafadeout[3] = 0.5;

	config.rootcolor[0] = 0x32 / 255.0f;
	config.rootcolor[1] = 0x32 / 255.0f;
	config.rootcolor[2] = 0x32 / 255.0f;
	config.rootcolor[3] = 1.0f;
	config.bordercolor[0] = 0x44 / 255.0f;
	config.bordercolor[1] = 0x44 / 255.0f;
	config.bordercolor[2] = 0x44 / 255.0f;
	config.bordercolor[3] = 1.0f;
	config.dropcolor[0] = 0x8f / 255.0f;
	config.dropcolor[1] = 0xba / 255.0f;
	config.dropcolor[2] = 0x7c / 255.0f;
	config.dropcolor[3] = 0.5f;
	config.focuscolor[0] = 0xc6 / 255.0f;
	config.focuscolor[1] = 0x6b / 255.0f;
	config.focuscolor[2] = 0x25 / 255.0f;
	config.focuscolor[3] = 1.0f;
	config.maximizescreencolor[0] = 0x89 / 255.0f;
	config.maximizescreencolor[1] = 0xaa / 255.0f;
	config.maximizescreencolor[2] = 0x61 / 255.0f;
	config.maximizescreencolor[3] = 1.0f;
	config.urgentcolor[0] = 0xad / 255.0f;
	config.urgentcolor[1] = 0x40 / 255.0f;
	config.urgentcolor[2] = 0x1f / 255.0f;
	config.urgentcolor[3] = 1.0f;

	/* switcher defaults — black transparent 50% */
	config.switcher_card_color[0] = 0.0f;  /* black 50% */
	config.switcher_card_color[1] = 0.0f;
	config.switcher_card_color[2] = 0.0f;
	config.switcher_card_color[3] = 0.5f;

	config.switcher_panel_color[0] = 0.0f; /* black 50% */
	config.switcher_panel_color[1] = 0.0f;
	config.switcher_panel_color[2] = 0.0f;
	config.switcher_panel_color[3] = 0.5f;

	config.switcher_outline_color[0] = 0.0f; /* black 50% */
	config.switcher_outline_color[1] = 0.0f;
	config.switcher_outline_color[2] = 0.0f;
	config.switcher_outline_color[3] = 0.5f;

	config.switcher_select_color[0] = 1.0f; /* white 80% */
	config.switcher_select_color[1] = 1.0f;
	config.switcher_select_color[2] = 1.0f;
	config.switcher_select_color[3] = 0.8f;

	config.switcher_backdrop_color[0] = 0.0f;        /* black 50% */
	config.switcher_backdrop_color[1] = 0.0f;
	config.switcher_backdrop_color[2] = 0.0f;
	config.switcher_backdrop_color[3] = 0.5f;

	config.switcher_minimize_color[0] = 0xe6 / 255.0f; /* #E6A21E */
	config.switcher_minimize_color[1] = 0xa2 / 255.0f;
	config.switcher_minimize_color[2] = 0x1e / 255.0f;
	config.switcher_minimize_color[3] = 1.0f;

	config.switcher_maximize_color[0] = 0x32 / 255.0f; /* #32C864 */
	config.switcher_maximize_color[1] = 0xc8 / 255.0f;
	config.switcher_maximize_color[2] = 0x64 / 255.0f;
	config.switcher_maximize_color[3] = 1.0f;

	config.switcher_max_cards  = 0;    /* unlimited */
	config.switcher_card_w_px  = 240;
	config.switcher_card_h_px  = 180;
	config.switcher_pad_px     = 16;
	config.switcher_gap_px     = 12;
	config.switcher_modifier   = WLR_MODIFIER_ALT;
	config.scratchpadcolor[0] = 0x51 / 255.0f;
	config.scratchpadcolor[1] = 0x6c / 255.0f;
	config.scratchpadcolor[2] = 0x93 / 255.0f;
	config.scratchpadcolor[3] = 1.0f;
	config.globalcolor[0] = 0xb1 / 255.0f;
	config.globalcolor[1] = 0x53 / 255.0f;
	config.globalcolor[2] = 0xa7 / 255.0f;
	config.globalcolor[3] = 1.0f;
	config.overlaycolor[0] = 0x14 / 255.0f;
	config.overlaycolor[1] = 0xa5 / 255.0f;
	config.overlaycolor[2] = 0x7c / 255.0f;
	config.overlaycolor[3] = 1.0f;
}

void set_default_key_bindings(Config *config) {
	// 计算默认按键绑定的数量
	size_t default_key_bindings_count =
		sizeof(default_key_bindings) / sizeof(KeyBinding);

	// 重新分配内存以容纳新的默认按键绑定
	config->key_bindings =
		realloc(config->key_bindings,
				(config->key_bindings_count + default_key_bindings_count) *
					sizeof(KeyBinding));
	if (!config->key_bindings) {
		return;
	}

	// 将默认按键绑定复制到配置的按键绑定数组中
	for (size_t i = 0; i < default_key_bindings_count; i++) {
		config->key_bindings[config->key_bindings_count + i] =
			default_key_bindings[i];
		config->key_bindings[config->key_bindings_count + i].iscommonmode =
			true;
		config->key_bindings[config->key_bindings_count + i].islockapply = true;
	}

	// 更新按键绑定的总数
	config->key_bindings_count += default_key_bindings_count;
}

bool parse_config(void) {

	char filename[1024];

	free_config();

	memset(&config, 0, sizeof(config));

	// 重新将xkb_rules指针指向静态数组
	config.xkb_rules.layout = config.xkb_rules_layout;
	config.xkb_rules.variant = config.xkb_rules_variant;
	config.xkb_rules.options = config.xkb_rules_options;
	config.xkb_rules.rules = config.xkb_rules_rules;
	config.xkb_rules.model = config.xkb_rules_model;

	// 初始化动态数组的指针为NULL，避免野指针
	config.window_rules = NULL;
	config.window_rules_count = 0;
	config.monitor_rules = NULL;
	config.monitor_rules_count = 0;
	config.key_bindings = NULL;
	config.key_bindings_count = 0;
	config.mouse_bindings = NULL;
	config.mouse_bindings_count = 0;
	config.axis_bindings = NULL;
	config.axis_bindings_count = 0;
	config.switch_bindings = NULL;
	config.switch_bindings_count = 0;
	config.gesture_bindings = NULL;
	config.gesture_bindings_count = 0;
	config.env = NULL;
	config.env_count = 0;
	config.exec = NULL;
	config.exec_count = 0;
	config.exec_once = NULL;
	config.exec_once_count = 0;
	config.scroller_proportion_preset = NULL;
	config.scroller_proportion_preset_count = 0;
	config.circle_layout = NULL;
	config.circle_layout_count = 0;
	config.tag_rules = NULL;
	config.tag_rules_count = 0;
	config.cursor_theme = NULL;
	strcpy(config.keymode, "default");

	create_config_keymap();

	if (cli_config_path) {
		snprintf(filename, sizeof(filename), "%s", cli_config_path);
	} else {
		const char *home = getenv("HOME");
		if (!home) {
			fprintf(stderr,
					"\033[1m\033[31m[ERROR]:\033[33m HOME environment "
					"variable not set.\n");
			return false;
		}
		snprintf(filename, sizeof(filename), "%s/.config/aether/config.conf",
				 home);
	}

	bool parse_correct = true;
	set_value_default();
	parse_correct = parse_config_file(&config, filename, true);
	set_default_key_bindings(&config);
	override_config();
	return parse_correct;
}

#endif /* CONFIG_DEFAULTS_H */

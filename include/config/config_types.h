#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <ctype.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif

// 整数版本 - 截断小数部分
#define CLAMP_INT(x, min, max)                                                 \
	((int32_t)(x) < (int32_t)(min)                                             \
		 ? (int32_t)(min)                                                      \
		 : ((int32_t)(x) > (int32_t)(max) ? (int32_t)(max) : (int32_t)(x)))

// 浮点数版本 - 保留小数部分
#define CLAMP_FLOAT(x, min, max)                                               \
	((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

enum { NUM_TYPE_MINUS, NUM_TYPE_PLUS, NUM_TYPE_DEFAULT };

enum { KEY_TYPE_CODE, KEY_TYPE_SYM };

typedef struct {
	uint32_t keycode1;
	uint32_t keycode2;
	uint32_t keycode3;
} MultiKeycode;

typedef struct {
	xkb_keysym_t keysym;
	MultiKeycode keycode;
	int32_t type;
} KeySymCode;

typedef struct {
	uint32_t mod;
	KeySymCode keysymcode;
	int32_t (*func)(const Arg *);
	Arg arg;
	char mode[28];
	bool iscommonmode;
	bool isdefaultmode;
	bool islockapply;
	bool isreleaseapply;
	bool ispassapply;
} KeyBinding;

typedef struct {
	char *type;
	char *value;
} ConfigEnv;

typedef struct {
	const char *id;
	const char *title;
	uint32_t tags;
	int32_t isfloating;
	int32_t isfullscreen;
	int32_t isfakefullscreen;
	float scroller_proportion;
	const char *animation_type_open;
	const char *animation_type_close;
	const char *layer_animation_type_open;
	const char *layer_animation_type_close;
	int32_t isnoborder;
	int32_t isnoshadow;
	int32_t isnoradius;
	int32_t isnoanimation;
	int32_t isopensilent;
	int32_t istagsilent;
	int32_t isnamedscratchpad;
	int32_t isunglobal;
	int32_t isglobal;
	int32_t isoverlay;
	int32_t allow_shortcuts_inhibit;
	int32_t ignore_maximize;
	int32_t ignore_minimize;
	int32_t isnosizehint;
	int32_t indleinhibit_when_focus;
	char *monitor;
	int32_t offsetx;
	int32_t offsety;
	float width;
	float height;
	int32_t nofocus;
	int32_t nofadein;
	int32_t nofadeout;
	int32_t no_force_center;
	int32_t isterm;
	int32_t allow_csd;
	int32_t force_fakemaximize;
	int32_t force_tiled_state;
	int32_t force_tearing;
	int32_t noswallow;
	int32_t noblur;
	float focused_opacity;
	float unfocused_opacity;
	float scroller_proportion_single;
	uint32_t passmod;
	xkb_keysym_t keysym;
	KeyBinding globalkeybinding;
} ConfigWinRule;

typedef struct {
	const char *name;			 // Monitor name
	char *make, *model, *serial; // may be NULL
	int32_t rr;					 // Rotate and flip (assume integer)
	float scale;				 // Monitor scale factor
	int32_t x, y;				 // Monitor position
	int32_t width, height;		 // Monitor resolution
	float refresh;				 // Refresh rate
	int32_t vrr;				 // variable refresh rate
	int32_t custom;				 // enable custom mode
} ConfigMonitorRule;

// 修改后的宏定义
#define CHVT(n)                                                                \
	{                                                                          \
		WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT,                                  \
			{.keysym = XKB_KEY_XF86Switch_VT_##n, .type = KEY_TYPE_SYM}, chvt, \
		{                                                                      \
			.ui = (n)                                                          \
		}                                                                      \
	}

// 默认按键绑定数组
KeyBinding default_key_bindings[] = {CHVT(1), CHVT(2),	CHVT(3),  CHVT(4),
									 CHVT(5), CHVT(6),	CHVT(7),  CHVT(8),
									 CHVT(9), CHVT(10), CHVT(11), CHVT(12)};

typedef struct {
	uint32_t mod;
	uint32_t button;
	int32_t (*func)(const Arg *);
	Arg arg;
} MouseBinding;

typedef struct {
	uint32_t mod;
	uint32_t dir;
	int32_t (*func)(const Arg *);
	Arg arg;
} AxisBinding;

typedef struct {
	uint32_t fold;
	int32_t (*func)(const Arg *);
	Arg arg;
} SwitchBinding;

typedef struct {
	uint32_t mod;
	uint32_t motion;
	uint32_t fingers_count;
	int32_t (*func)(const Arg *);
	Arg arg;
} GestureBinding;

typedef struct {
	int32_t id;
	char *layout_name;
	char *monitor_name;
	char *monitor_make;
	char *monitor_model;
	char *monitor_serial;
	float mfact;
	int32_t nmaster;
	int32_t no_render_border;
	int32_t open_as_floating;
	int32_t no_hide;
} ConfigTagRule;

typedef struct {
	char *layer_name; // 布局名称
	char *animation_type_open;
	char *animation_type_close;
	int32_t noblur;
	int32_t noanim;
	int32_t noshadow;
} ConfigLayerRule;

typedef struct {
	int32_t animations;
	int32_t layer_animations;
	char animation_type_open[10];
	char animation_type_close[10];
	char layer_animation_type_open[10];
	char layer_animation_type_close[10];
	int32_t animation_fade_in;
	int32_t animation_fade_out;
	int32_t tag_animation_direction;
	float zoom_initial_ratio;
	float zoom_end_ratio;
	float fadein_begin_opacity;
	float fadeout_begin_opacity;
	uint32_t animation_duration_move;
	uint32_t animation_duration_open;
	uint32_t animation_duration_tag;
	uint32_t animation_duration_close;
	uint32_t animation_duration_focus;
	double animation_curve_move[4];
	double animation_curve_open[4];
	double animation_curve_tag[4];
	double animation_curve_close[4];
	double animation_curve_focus[4];
	double animation_curve_opafadein[4];
	double animation_curve_opafadeout[4];

	int32_t scroller_structs;
	float scroller_default_proportion;
	float scroller_default_proportion_single;
	int32_t scroller_ignore_proportion_single;
	int32_t scroller_focus_center;
	int32_t scroller_prefer_center;
	int32_t scroller_prefer_overspread;
	int32_t edge_scroller_pointer_focus;
	int32_t focus_cross_monitor;
	int32_t exchange_cross_monitor;
	int32_t scratchpad_cross_monitor;
	int32_t focus_cross_tag;
	int32_t view_current_to_back;
	int32_t no_border_when_single;
	int32_t no_radius_when_single;
	int32_t snap_distance;
	int32_t enable_floating_snap;
	/* edge snap (aero snap) for floating windows */
	int32_t enable_edge_snap;        /* 0=off 1=on */
	int32_t edge_snap_threshold;     /* px near edge to trigger snap zone */
	int32_t edge_snap_corner_size;   /* px from corner to select quarter snap */
	float   edge_snap_preview_alpha; /* 0.0-1.0 opacity of the preview rect */
	float   edge_snap_preview_color[4]; /* RGBA color of preview rect */
	int32_t drag_tile_to_tile;
	int32_t drag_tile_small;
	uint32_t swipe_min_threshold;
	float focused_opacity;
	float unfocused_opacity;
	float *scroller_proportion_preset;
	int32_t scroller_proportion_preset_count;

	char **circle_layout;
	int32_t circle_layout_count;

	uint32_t new_is_master;
	float default_mfact;
	uint32_t default_nmaster;
	int32_t center_master_overspread;
	int32_t center_when_single_stack;

	/* dwindle layout */
	int32_t dwindle_vsplit;
	int32_t dwindle_hsplit;
	int32_t dwindle_preserve_split;
	int32_t dwindle_smart_split;
	int32_t dwindle_smart_resize;
	int32_t dwindle_drop_simple_split;
	float dwindle_split_ratio;

	uint32_t hotarea_size;
	uint32_t hotarea_corner;
	uint32_t enable_hotarea;
	uint32_t ov_tab_mode;
	int32_t overviewgappi;
	int32_t overviewgappo;
	uint32_t cursor_hide_timeout;

	uint32_t axis_bind_apply_timeout;
	uint32_t focus_on_activate;
	int32_t idleinhibit_ignore_visible;
	int32_t sloppyfocus;
	int32_t warpcursor;
	int32_t drag_corner;
	int32_t drag_warp_cursor;

	/* keyboard */
	int32_t repeat_rate;
	int32_t repeat_delay;
	uint32_t numlockon;

	/* common pointer */
	int32_t disable_while_typing;
	int32_t left_handed;
	int32_t middle_button_emulation;
	uint32_t scroll_method;
	uint32_t scroll_button;
	uint32_t click_method;
	uint32_t send_events_mode;

	/* mouse */
	int32_t mouse_natural_scrolling;
	uint32_t mouse_accel_profile;
	double mouse_accel_speed;
	double axis_scroll_factor;

	/* Trackpad */
	int32_t trackpad_natural_scrolling;
	uint32_t trackpad_accel_profile;
	double trackpad_accel_speed;
	double trackpad_scroll_factor;
	int32_t disable_trackpad;
	int32_t tap_to_click;
	int32_t tap_and_drag;
	int32_t drag_lock;
	uint32_t button_map;

	/* window effects */
	int32_t blur;
	int32_t blur_layer;
	int32_t blur_optimized;
	int32_t border_radius;
	int32_t border_radius_location_default;
	struct blur_data blur_params;
	int32_t shadows;
	int32_t shadow_only_floating;
	int32_t layer_shadows;
	uint32_t shadows_size;
	float shadows_blur;
	int32_t shadows_position_x;
	int32_t shadows_position_y;
	float shadowscolor[4];

	/* appearance */
	int32_t smartgaps;
	uint32_t gappih;
	uint32_t gappiv;
	uint32_t gap_outer_left;   /* outer horizontal gap — left edge */
	uint32_t gap_outer_right;  /* outer horizontal gap — right edge */
	uint32_t gap_outer_top;    /* outer vertical gap — top edge */
	uint32_t gap_outer_bottom; /* outer vertical gap — bottom edge */
	uint32_t borderpx;
	float scratchpad_width_ratio;
	float scratchpad_height_ratio;
	float rootcolor[4];
	float bordercolor[4];
	float dropcolor[4];
	float focuscolor[4];
	float maximizescreencolor[4];
	float urgentcolor[4];
	float scratchpadcolor[4];
	float globalcolor[4];
	float overlaycolor[4];

	/* switcher (Alt+Tab style window picker) */
	float    switcher_card_color[4];      /* card background */
	float    switcher_panel_color[4];     /* panel background */
	float    switcher_outline_color[4];   /* panel outline */
	float    switcher_select_color[4];    /* selected card highlight */
	float    switcher_backdrop_color[4];  /* full-screen backdrop */
	float    switcher_minimize_color[4];  /* dot for minimized windows */
	float    switcher_maximize_color[4];  /* dot for maximized windows */
	uint32_t switcher_max_cards;          /* 0 = unlimited */
	uint32_t switcher_card_w_px;          /* card width hint */
	uint32_t switcher_card_h_px;          /* card height hint */
	uint32_t switcher_pad_px;             /* panel padding */
	uint32_t switcher_gap_px;             /* gap between cards */
	uint32_t switcher_modifier;           /* modifier that triggers commit on release */

	int32_t log_level;
	uint32_t capslock;

	ConfigTagRule *tag_rules; // 动态数组
	int32_t tag_rules_count;  // 数量

	ConfigLayerRule *layer_rules; // 动态数组
	int32_t layer_rules_count;	  // 数量

	ConfigWinRule *window_rules;
	int32_t window_rules_count;

	ConfigMonitorRule *monitor_rules; // 动态数组
	int32_t monitor_rules_count;	  // 条数

	KeyBinding *key_bindings;
	int32_t key_bindings_count;

	MouseBinding *mouse_bindings;
	int32_t mouse_bindings_count;

	AxisBinding *axis_bindings;
	int32_t axis_bindings_count;

	SwitchBinding *switch_bindings;
	int32_t switch_bindings_count;

	GestureBinding *gesture_bindings;
	int32_t gesture_bindings_count;

	ConfigEnv **env;
	int32_t env_count;

	char **exec;
	int32_t exec_count;

	char **exec_once;
	int32_t exec_once_count;

	char *cursor_theme;
	uint32_t cursor_size;

	int32_t single_scratchpad;
	int32_t xwayland_persistence;
	int32_t syncobj_enable;
	float drag_tile_refresh_interval;
	float drag_floating_refresh_interval;
	int32_t allow_tearing;
	int32_t allow_shortcuts_inhibit;
	int32_t allow_lock_transparent;

	struct xkb_rule_names xkb_rules;
	char xkb_rules_rules[128];
	char xkb_rules_model[128];
	char xkb_rules_layout[128];
	char xkb_rules_variant[128];
	char xkb_rules_options[128];

	char keymode[28];

	struct xkb_context *ctx;
	struct xkb_keymap *keymap;
} Config;

typedef int32_t (*FuncType)(const Arg *);
Config config;

bool parse_config_file(Config *config, const char *file_path, bool must_exist);
bool apply_rule_to_state(Monitor *m, const ConfigMonitorRule *rule,
						 struct wlr_output_state *state, int vrr, int custom);
bool monitor_matches_rule(Monitor *m, const ConfigMonitorRule *rule);

#endif /* CONFIG_TYPES_H */

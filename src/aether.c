#include "wlr-layer-shell-unstable-v1-protocol.h"
#include "wlr/util/box.h"
#include "wlr/util/edges.h"
#include <getopt.h>
#include <libinput.h>
#include <limits.h>
#include <linux/input-event-codes.h>
#include <scenefx/render/fx_renderer/fx_renderer.h>
#include <scenefx/types/fx/blur_data.h>
#include <scenefx/types/fx/clipped_region.h>
#include <scenefx/types/fx/corner_location.h>
#include <scenefx/types/wlr_scene.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/backend/headless.h>
#include <wlr/backend/libinput.h>
#include <wlr/backend/multi.h>
#include <wlr/backend/wayland.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_cursor_shape_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_drm_lease_v1.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_ext_data_control_v1.h>
#include <wlr/types/wlr_ext_image_capture_source_v1.h>
#include <wlr/types/wlr_ext_image_copy_capture_v1.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard_group.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_pointer_gestures_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_switch.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_foreign_registry.h>
#include <wlr/types/wlr_xdg_foreign_v1.h>
#include <wlr/types/wlr_xdg_foreign_v2.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#include <wordexp.h>
#include <xkbcommon/xkbcommon.h>
#ifdef XWAYLAND
#include <X11/Xlib.h>
#include <wlr/xwayland.h>
#include <xcb/xcb_icccm.h>
#endif
#include "common/util.h"

/* macros */
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define GEZERO(A) ((A) >= 0 ? (A) : 0)
#define CLEANMASK(mask) (mask & ~WLR_MODIFIER_CAPS)
#define INSIDEMON(A)                                                           \
	(A->geom.x >= A->mon->m.x && A->geom.y >= A->mon->m.y &&                   \
	 A->geom.x + A->geom.width <= A->mon->m.x + A->mon->m.width &&             \
	 A->geom.y + A->geom.height <= A->mon->m.y + A->mon->m.height)
#define GEOMINSIDEMON(A, M)                                                    \
	(A->x >= M->m.x && A->y >= M->m.y &&                                       \
	 A->x + A->width <= M->m.x + M->m.width &&                                 \
	 A->y + A->height <= M->m.y + M->m.height)
#define ISTILED(A)                                                             \
	(A && !(A)->isfloating && !(A)->isminimized && !(A)->iskilling &&          \
	 !(A)->ismaximizescreen && !(A)->isfullscreen && !(A)->isunglobal)
#define ISSCROLLTILED(A)                                                       \
	(A && !(A)->isfloating && !(A)->isminimized && !(A)->iskilling &&          \
	 !(A)->isunglobal)
#define VISIBLEON(C, M)                                                        \
	((C) && (M) && (C)->mon == (M) &&                                          \
	 (((C)->tags & (M)->tagset[(M)->seltags] || (C)->isglobal ||               \
	   (C)->isunglobal)))
#define LENGTH(X) (sizeof X / sizeof X[0])
#define END(A) ((A) + LENGTH(A))
#define TAGMASK ((1 << LENGTH(tags)) - 1)
#define LISTEN(E, L, H) wl_signal_add((E), ((L)->notify = (H), (L)))
#define ISFULLSCREEN(A)                                                        \
	((A)->isfullscreen || (A)->ismaximizescreen ||                             \
	 (A)->overview_ismaximizescreenbak || (A)->overview_isfullscreenbak)
#define LISTEN_STATIC(E, H)                                                    \
	do {                                                                       \
		struct wl_listener *_l = ecalloc(1, sizeof(*_l));                      \
		_l->notify = (H);                                                      \
		wl_signal_add((E), _l);                                                \
	} while (0)

#define APPLY_INT_PROP(obj, rule, prop)                                        \
	if (rule->prop >= 0)                                                       \
	obj->prop = rule->prop

#define APPLY_FLOAT_PROP(obj, rule, prop)                                      \
	if (rule->prop > 0.0f)                                                     \
	obj->prop = rule->prop

#define APPLY_STRING_PROP(obj, rule, prop)                                     \
	if (rule->prop != NULL)                                                    \
	obj->prop = rule->prop

#define BAKED_POINTS_COUNT 256

/* enums */
enum { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };

enum { VERTICAL, HORIZONTAL };
enum { SWIPE_UP, SWIPE_DOWN, SWIPE_LEFT, SWIPE_RIGHT };
enum { CurNormal, CurPressed, CurMove, CurResize }; /* cursor */
/* Edge snap zones for floating windows (Aero-Snap style) */
typedef enum {
	EDGE_SNAP_NONE = 0,
	EDGE_SNAP_LEFT,
	EDGE_SNAP_RIGHT,
	EDGE_SNAP_TOP,        /* → maximize (full work area) */
	EDGE_SNAP_TOP_LEFT,
	EDGE_SNAP_TOP_RIGHT,
	EDGE_SNAP_BOTTOM_LEFT,
	EDGE_SNAP_BOTTOM_RIGHT,
} EdgeSnapZone;
enum { XDGShell, LayerShell, X11 };					/* client types */
enum { AxisUp, AxisDown, AxisLeft, AxisRight };		//The direction in which the wheel rolls
enum {
	LyrBg,
	LyrBlur,
	LyrBottom,
	LyrTile,
	LyrFloat,
	LyrTop,
	LyrFadeOut,
	LyrOverlay,
	LyrIMPopup, // text-input layer
	LyrBlock,
	NUM_LAYERS
}; /* scene layers */
#ifdef XWAYLAND
enum {
	NetWMWindowTypeDialog,
	NetWMWindowTypeSplash,
	NetWMWindowTypeToolbar,
	NetWMWindowTypeUtility,
	NetLast
}; /* EWMH atoms */
#endif
enum { UP, DOWN, LEFT, RIGHT, UNDIR }; /* smartmovewin */
enum { NONE, OPEN, MOVE, CLOSE, TAG, FOCUS, OPAFADEIN, OPAFADEOUT };
enum { UNFOLD, FOLD, INVALIDFOLD };
enum { PREV, NEXT };
enum { STATE_UNSPECIFIED = 0, STATE_ENABLED, STATE_DISABLED };

enum tearing_mode {
	TEARING_DISABLED = 0,
	TEARING_ENABLED,
	TEARING_FULLSCREEN_ONLY,
};

enum seat_config_shortcuts_inhibit {
	SHORTCUTS_INHIBIT_DISABLE,
	SHORTCUTS_INHIBIT_ENABLE,
};

//Event mask enumeration
enum print_event_type {
	PRINT_ACTIVE = 1 << 0,
	PRINT_TAG = 1 << 1,
	PRINT_LAYOUT = 1 << 2,
	PRINT_TITLE = 1 << 3,
	PRINT_APPID = 1 << 4,
	PRINT_LAYOUT_SYMBOL = 1 << 5,
	PRINT_FULLSCREEN = 1 << 6,
	PRINT_FLOATING = 1 << 7,
	PRINT_X = 1 << 8,
	PRINT_Y = 1 << 9,
	PRINT_WIDTH = 1 << 10,
	PRINT_HEIGHT = 1 << 11,
	PRINT_LAST_LAYER = 1 << 12,
	PRINT_KB_LAYOUT = 1 << 13,
	PRINT_KEYMODE = 1 << 14,
	PRINT_SCALEFACTOR = 1 << 15,
	PRINT_FRAME = 1 << 16,
	PRINT_ALL = (1 << 17) - 1 // Set all bits to 1
};

typedef struct Pertag Pertag;
typedef struct Monitor Monitor;
typedef struct Client Client;

struct dvec2 {
	double x, y;
};

struct ivec2 {
	int32_t x, y, width, height;
};

#define AETHER_ARG_TYPEDEF
typedef struct {
	int32_t i;
	int32_t i2;
	float f;
	float f2;
	char *v;
	char *v2;
	char *v3;
	uint32_t ui;
	uint32_t ui2;
} Arg;

typedef struct {
	uint32_t mod;
	uint32_t button;
	int32_t (*func)(const Arg *);
	const Arg arg;
} Button; //Mouse buttons

typedef struct {
	char mode[28];
	bool isdefault;
} KeyMode;

typedef struct {
	uint32_t mod;
	uint32_t dir;
	int32_t (*func)(const Arg *);
	const Arg arg;
} Axis;

typedef struct {
	struct wl_list link;
	struct wlr_input_device *wlr_device;
	struct libinput_device *libinput_device;
	struct wl_listener destroy_listener; // Used to listen for device destruction events
	void *device_data;					 // New: points to device-specific data (such as Switch)
} InputDevice;

typedef struct {
	struct wl_list link;
	struct wlr_switch *wlr_switch;
	struct wl_listener toggle;
	InputDevice *input_dev;
} Switch;

struct aether_animation {
	bool should_animate;
	bool running;
	bool tagining;
	bool tagouted;
	bool tagouting;
	bool begin_fade_in;
	bool tag_from_rule;
	uint32_t time_started;
	uint32_t duration;
	struct wlr_box initial;
	struct wlr_box current;
	int32_t action;
};

struct aether_opacity_animation {
	bool running;
	float current_opacity;
	float target_opacity;
	float initial_opacity;
	uint32_t time_started;
	uint32_t duration;
	float current_border_color[4];
	float target_border_color[4];
	float initial_border_color[4];
};

typedef struct {
	float width_scale;
	float height_scale;
	int32_t width;
	int32_t height;
	enum corner_location corner_location;
	bool should_scale;
} BufferData;

struct Client {
	/* Must keep these three elements in this order */
	uint32_t type; /* XDGShell or X11* */
	struct wlr_box geom, pending, float_geom, animainit_geom,
		overview_backup_geom, current,
		drag_begin_geom; /* layout-relative, includes border */
	Monitor *mon;
	struct wlr_scene_tree *scene;
	struct wlr_scene_rect *border; /* top, bottom, left, right */
	struct wlr_scene_rect *droparea;
	struct wlr_scene_shadow *shadow;
	struct wlr_scene_tree *scene_surface;
	struct wl_list link;
	struct wl_list flink;
	struct wl_list fadeout_link;
	union {
		struct wlr_xdg_surface *xdg;
		struct wlr_xwayland_surface *xwayland;
	} surface;
	struct wl_listener commit;
	struct wl_listener map;
	struct wl_listener maximize;
	struct wl_listener minimize;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener set_title;
	struct wl_listener fullscreen;
	struct wl_listener request_move;
	struct wl_listener request_resize;
#ifdef XWAYLAND
	struct wl_listener activate;
	struct wl_listener associate;
	struct wl_listener dissociate;
	struct wl_listener configure;
	struct wl_listener set_hints;
	struct wl_listener set_geometry;
	struct wl_listener commmitx11;
#endif
	uint32_t bw;
	uint32_t tags, oldtags, mini_restore_tag;
	bool dirty;
	uint32_t configure_serial;
	struct wlr_foreign_toplevel_handle_v1 *foreign_toplevel;
	int32_t isfloating, isurgent, isfullscreen, isfakefullscreen,
		need_float_size_reduce, isminimized, isoverlay, isnosizehint,
		ignore_maximize, ignore_minimize, indleinhibit_when_focus,
		layout_forced_floating; /* set when floating layout forced this window to float */
	int32_t ismaximizescreen;
	int32_t overview_backup_bw;
	int32_t fullscreen_backup_x, fullscreen_backup_y, fullscreen_backup_w,
		fullscreen_backup_h;
	int32_t overview_isfullscreenbak, overview_ismaximizescreenbak,
		overview_isfloatingbak;

	struct wlr_xdg_toplevel_decoration_v1 *decoration;
	struct wl_listener foreign_activate_request;
	struct wl_listener foreign_fullscreen_request;
	struct wl_listener foreign_close_request;
	struct wl_listener foreign_destroy;
	struct wl_listener foreign_minimize_request;
	struct wl_listener foreign_maximize_request;
	struct wl_listener set_decoration_mode;
	struct wl_listener destroy_decoration;

	const char *animation_type_open;
	const char *animation_type_close;
	int32_t is_in_scratchpad;
	int32_t iscustomsize;
	int32_t iscustompos;
	int32_t is_scratchpad_show;
	int32_t isglobal;
	int32_t isnoborder;
	int32_t isnoshadow;
	int32_t isnoradius;
	int32_t isnoanimation;
	int32_t isopensilent;
	int32_t istagsilent;
	int32_t iskilling;
	int32_t istagswitching;
	int32_t isnamedscratchpad;
	bool is_pending_open_animation;
	bool is_restoring_from_ov;
	float scroller_proportion;
	float stack_proportion;
	float old_stack_proportion;
	bool need_output_flush;
	struct aether_animation animation;
	struct aether_opacity_animation opacity_animation;
	int32_t isterm, noswallow;
	int32_t allow_csd;
	int32_t force_fakemaximize;
	int32_t force_tiled_state;
	pid_t pid;
	Client *swallowing, *swallowedby;
	bool is_clip_to_hide;
	bool drag_to_tile;
	bool scratchpad_switching_mon;
	bool fake_no_border;
	int32_t nofocus;
	int32_t nofadein;
	int32_t nofadeout;
	int32_t no_force_center;
	int32_t isunglobal;
	float focused_opacity;
	float unfocused_opacity;
	char oldmonname[128];
	int32_t noblur;
	double master_mfact_per, master_inner_per, stack_inner_per;
	double old_master_mfact_per, old_master_inner_per, old_stack_inner_per;
	double old_scroller_pproportion;
	bool ismaster;
	bool old_ismaster;
	bool cursor_in_upper_half, cursor_in_left_half;
	bool isleftstack;
	int32_t tearing_hint;
	int32_t force_tearing;
	int32_t allow_shortcuts_inhibit;
	float scroller_proportion_single;
	bool isfocusing;
	bool enable_drop_area_draw;
	int32_t drop_direction;
	struct wlr_box drag_tile_float_backup_geom;
};

typedef struct {
	struct wl_list link;
	struct wl_resource *resource;
	Monitor *mon;
} aetherIpcOutput;

typedef struct {
	uint32_t mod;
	xkb_keysym_t keysym;
	int32_t (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	struct wlr_keyboard_group *wlr_group;

	int32_t nsyms;
	const xkb_keysym_t *keysyms; /* invalid if nsyms == 0 */
	uint32_t mods;				 /* invalid if nsyms == 0 */
	uint32_t keycode;
	struct wl_event_source *key_repeat_source;

	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
} KeyboardGroup;

typedef struct {
	struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor;
	struct wl_listener destroy;
	struct wl_list link;
} KeyboardShortcutsInhibitor;

typedef struct {
	/* Must keep these three elements in this order */
	uint32_t type; /* LayerShell */
	struct wlr_box geom, current, pending, animainit_geom;
	Monitor *mon;
	struct wlr_scene_tree *scene;
	struct wlr_scene_tree *popups;
	struct wlr_scene_shadow *shadow;
	struct wlr_scene_layer_surface_v1 *scene_layer;
	struct wl_list link;
	struct wl_list fadeout_link;
	int32_t mapped;
	struct wlr_layer_surface_v1 *layer_surface;

	struct wl_listener destroy;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener surface_commit;

	struct aether_animation animation;
	bool dirty;
	int32_t noblur;
	int32_t noanim;
	int32_t noshadow;
	char *animation_type_open;
	char *animation_type_close;
	bool need_output_flush;
	bool being_unmapped;
} LayerSurface;

typedef struct {
	struct wlr_xdg_popup *wlr_popup;
	struct wl_listener destroy;
	struct wl_listener commit;
	struct wl_listener reposition;
} Popup;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
	const char *name;
	uint32_t id;
} Layout;

struct Monitor {
	struct wl_list link;
	struct wlr_output *wlr_output;
	struct wlr_scene_output *scene_output;
	struct wlr_output_state pending;
	struct wl_listener frame;
	struct wl_listener destroy;
	struct wl_listener request_state;
	struct wl_listener destroy_lock_surface;
	struct wlr_session_lock_surface_v1 *lock_surface;
	struct wl_event_source *skip_frame_timeout;
	struct wlr_box m;		  /* monitor area, layout-relative */
	struct wlr_box w;		  /* window area, layout-relative */
	struct wl_list layers[4]; /* LayerSurface::link */
	uint32_t seltags;
	uint32_t tagset[2];
	bool skiping_frame;
	uint32_t resizing_count_pending;
	uint32_t resizing_count_current;

	struct wl_list aether_ipc_outputs;
	int32_t gappih;            /* horizontal gap between windows */
	int32_t gappiv;            /* vertical gap between windows */
	int32_t gap_outer_left;    /* outer horizontal gap — left edge */
	int32_t gap_outer_right;   /* outer horizontal gap — right edge */
	int32_t gap_outer_top;     /* outer vertical gap — top edge */
	int32_t gap_outer_bottom;  /* outer vertical gap — bottom edge */
	Pertag *pertag;
	uint32_t ovbk_current_tagset;
	uint32_t ovbk_prev_tagset;
	Client *sel, *prevsel;
	int32_t isoverview;
	int32_t is_in_hotarea;
	int32_t asleep;
	uint32_t visible_clients;
	uint32_t visible_tiling_clients;
	uint32_t visible_scroll_tiling_clients;
	struct wlr_scene_optimized_blur *blur;
	char last_surface_ws_name[256];
	struct wlr_ext_workspace_group_handle_v1 *ext_group;
	bool iscleanuping;
};

typedef struct {
	struct wlr_pointer_constraint_v1 *constraint;
	struct wl_listener destroy;
} PointerConstraint;

typedef struct {
	struct wlr_scene_tree *scene;

	struct wlr_session_lock_v1 *lock;
	struct wl_listener new_surface;
	struct wl_listener unlock;
	struct wl_listener destroy;
} SessionLock;

typedef struct DwindleNode DwindleNode;
struct ScrollerStackNode {
	Client *client;
	float scroller_proportion;
	float stack_proportion;
	float scroller_proportion_single;

	struct ScrollerStackNode *next_in_stack;
	struct ScrollerStackNode *prev_in_stack;
	struct ScrollerStackNode *all_next;
};

struct TagScrollerState {
	struct ScrollerStackNode *all_first; /* Single linked list header of all nodes */
	int count;
};

/* function declarations */
static void applybounds(
	Client *c,
	struct wlr_box *bbox); // Set boundary rules to allow some windows to have a more suitable size
static void applyrules(Client *c); // Window rule application, apply the window rules defined in config.h
static void arrange(Monitor *m, bool want_animation,
					bool from_view); //Layout function, let the window move and resize according to the tiling rules
static void arrangelayer(Monitor *m, struct wl_list *list,
						 struct wlr_box *usable_area, int32_t exclusive);
static void arrangelayers(Monitor *m);
static void handle_print_status(struct wl_listener *listener, void *data);
static void axisnotify(struct wl_listener *listener,
					   void *data); //Roller event handling
static void buttonpress(struct wl_listener *listener,
						void *data); //Mouse button event processing
static int32_t ongesture(struct wlr_pointer_swipe_end_event *event);
static void swipe_begin(struct wl_listener *listener, void *data);
static void swipe_update(struct wl_listener *listener, void *data);
static void swipe_end(struct wl_listener *listener, void *data);
static void pinch_begin(struct wl_listener *listener, void *data);
static void pinch_update(struct wl_listener *listener, void *data);
static void pinch_end(struct wl_listener *listener, void *data);
static void hold_begin(struct wl_listener *listener, void *data);
static void hold_end(struct wl_listener *listener, void *data);
static void checkidleinhibitor(struct wlr_surface *exclude);
static void cleanup(void);										  // Exit cleanup
static void cleanupmon(struct wl_listener *listener, void *data); // Exit cleanup
static void closemon(Monitor *m);
static void cleanuplisteners(void);
static void toggle_hotarea(int32_t x_root, int32_t y_root); //Trigger hot zone
static void maplayersurfacenotify(struct wl_listener *listener, void *data);
static void commitlayersurfacenotify(struct wl_listener *listener, void *data);
static void commitnotify(struct wl_listener *listener, void *data);
static void createdecoration(struct wl_listener *listener, void *data);
static void createidleinhibitor(struct wl_listener *listener, void *data);
static void createkeyboard(struct wlr_keyboard *keyboard);
static void requestmonstate(struct wl_listener *listener, void *data);
static void createlayersurface(struct wl_listener *listener, void *data);
static void createlocksurface(struct wl_listener *listener, void *data);
static void createmon(struct wl_listener *listener, void *data);
static void createnotify(struct wl_listener *listener, void *data);
static void createpointer(struct wlr_pointer *pointer);
static void configure_pointer(struct libinput_device *device);
static void destroyinputdevice(struct wl_listener *listener, void *data);
static void createswitch(struct wlr_switch *switch_device);
static void switch_toggle(struct wl_listener *listener, void *data);
static void createpointerconstraint(struct wl_listener *listener, void *data);
static void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint);
static void commitpopup(struct wl_listener *listener, void *data);
static void createpopup(struct wl_listener *listener, void *data);
static void cursorframe(struct wl_listener *listener, void *data);
static void cursorwarptohint(void);
static void destroydecoration(struct wl_listener *listener, void *data);
static void destroydragicon(struct wl_listener *listener, void *data);
static void destroyidleinhibitor(struct wl_listener *listener, void *data);
static void destroylayernodenotify(struct wl_listener *listener, void *data);
static void destroylock(SessionLock *lock, int32_t unlocked);
static void destroylocksurface(struct wl_listener *listener, void *data);
static void destroynotify(struct wl_listener *listener, void *data);
static void destroypointerconstraint(struct wl_listener *listener, void *data);
static void destroysessionlock(struct wl_listener *listener, void *data);
static void destroykeyboardgroup(struct wl_listener *listener, void *data);
static Monitor *dirtomon(enum wlr_direction dir);
static void setcursorshape(struct wl_listener *listener, void *data);

static void focusclient(Client *c, int32_t lift);

static void setborder_color(Client *c);
static Client *focustop(Monitor *m);
static void fullscreennotify(struct wl_listener *listener, void *data);
static void gpureset(struct wl_listener *listener, void *data);

static int32_t keyrepeat(void *data);

static void inputdevice(struct wl_listener *listener, void *data);
static int32_t keybinding(uint32_t state, bool locked, uint32_t mods,
						  xkb_keysym_t sym, uint32_t keycode);
static void keypress(struct wl_listener *listener, void *data);
static void keypressmod(struct wl_listener *listener, void *data);
static bool keypressglobal(struct wlr_surface *last_surface,
						   struct wlr_keyboard *keyboard,
						   struct wlr_keyboard_key_event *event, uint32_t mods,
						   xkb_keysym_t keysym, uint32_t keycode);
static void locksession(struct wl_listener *listener, void *data);
static void mapnotify(struct wl_listener *listener, void *data);
static void maximizenotify(struct wl_listener *listener, void *data);
static void requestmovenotify(struct wl_listener *listener, void *data);
static void requestresizenotify(struct wl_listener *listener, void *data);
static void minimizenotify(struct wl_listener *listener, void *data);
static void motionabsolute(struct wl_listener *listener, void *data);
static void motionnotify(uint32_t time, struct wlr_input_device *device,
						 double sx, double sy, double sx_unaccel,
						 double sy_unaccel);
static void motionrelative(struct wl_listener *listener, void *data);

static void reset_foreign_tolevel(Client *c, Monitor *oldmon, Monitor *newmon);
static void add_foreign_topleve(Client *c);
static void exchange_two_client(Client *c1, Client *c2);
static void outputmgrapply(struct wl_listener *listener, void *data);
static void outputmgrapplyortest(struct wlr_output_configuration_v1 *config,
								 int32_t test);
static void outputmgrtest(struct wl_listener *listener, void *data);
static void pointerfocus(Client *c, struct wlr_surface *surface, double sx,
						 double sy, uint32_t time);
static void printstatus(void);
static void quitsignal(int32_t signo);
static void powermgrsetmode(struct wl_listener *listener, void *data);
static void rendermon(struct wl_listener *listener, void *data);
static void requestdecorationmode(struct wl_listener *listener, void *data);
static void requestdrmlease(struct wl_listener *listener, void *data);
static void requeststartdrag(struct wl_listener *listener, void *data);
static void resize(Client *c, struct wlr_box geo, int32_t interact);
static void run(char *startup_cmd);
static void setcursor(struct wl_listener *listener, void *data);
static void setfloating(Client *c, int32_t floating);
static void setfakefullscreen(Client *c, int32_t fakefullscreen);
static void setfullscreen(Client *c, int32_t fullscreen);
static void setmaximizescreen(Client *c, int32_t maximizescreen);
static void reset_maximizescreen_size(Client *c);
static void setgaps(int32_t outer_left, int32_t outer_right, int32_t outer_top, int32_t outer_bottom, int32_t ih, int32_t iv);

static void setmon(Client *c, Monitor *m, uint32_t newtags, bool focus);
static void setpsel(struct wl_listener *listener, void *data);
static void setsel(struct wl_listener *listener, void *data);
static void setup(void);
static void startdrag(struct wl_listener *listener, void *data);

static void unlocksession(struct wl_listener *listener, void *data);
static void unmaplayersurfacenotify(struct wl_listener *listener, void *data);
static void unmapnotify(struct wl_listener *listener, void *data);
static void updatemons(struct wl_listener *listener, void *data);
static void updatetitle(struct wl_listener *listener, void *data);
static void urgent(struct wl_listener *listener, void *data);
static void view(const Arg *arg, bool want_animation);

static void handlesig(int32_t signo);
static void
handle_keyboard_shortcuts_inhibit_new_inhibitor(struct wl_listener *listener,
												void *data);
static void virtualkeyboard(struct wl_listener *listener, void *data);
static void virtualpointer(struct wl_listener *listener, void *data);
static void warp_cursor(const Client *c);
static Monitor *xytomon(double x, double y);
static void xytonode(double x, double y, struct wlr_surface **psurface,
					 Client **pc, LayerSurface **pl, double *nx, double *ny);
static void clear_fullscreen_flag(Client *c);
static pid_t getparentprocess(pid_t p);
static int32_t isdescprocess(pid_t p, pid_t c);
static Client *termforwin(Client *w);
static void swallow(Client *c, Client *w);

static void warp_cursor_to_selmon(Monitor *m);
uint32_t want_restore_fullscreen(Client *target_client);
static void overview_restore(Client *c, const Arg *arg);
static void overview_backup(Client *c);
static void set_minimized(Client *c);

static void show_scratchpad(Client *c);
static void show_hide_client(Client *c);
static void tag_client(const Arg *arg, Client *target_client);

static struct wlr_box setclient_coordinate_center(Client *c, Monitor *m,
												  struct wlr_box geom,
												  int32_t offsetx,
												  int32_t offsety);
static uint32_t get_tags_first_tag(uint32_t tags);

static struct wlr_output_mode *
get_nearest_output_mode(struct wlr_output *output, int32_t width,
						int32_t height, float refresh);

static void client_commit(Client *c);
static void layer_commit(LayerSurface *l);
static void apply_border(Client *c);
static void client_set_opacity(Client *c, double opacity);
static void init_baked_points(void);
static void scene_buffer_apply_opacity(struct wlr_scene_buffer *buffer,
									   int32_t sx, int32_t sy, void *data);

static Client *direction_select(const Arg *arg);
static void view_in_mon(const Arg *arg, bool want_animation, Monitor *m,
						bool changefocus);

static void buffer_set_effect(Client *c, BufferData buffer_data);
static void snap_scene_buffer_apply_effect(struct wlr_scene_buffer *buffer,
										   int32_t sx, int32_t sy, void *data);
static void client_set_pending_state(Client *c);
static void layer_set_pending_state(LayerSurface *l);
static void set_rect_size(struct wlr_scene_rect *rect, int32_t width,
						  int32_t height);
static Client *center_tiled_select(Monitor *m);
static void handlecursoractivity(void);
static int32_t hidecursor(void *data);
static bool check_hit_no_border(Client *c);
static void reset_keyboard_layout(void);
static void client_update_oldmonname_record(Client *c, Monitor *m);
static void pending_kill_client(Client *c);
static uint32_t get_tags_first_tag_num(uint32_t source_tags);
static void set_layer_open_animaiton(LayerSurface *l, struct wlr_box geo);
static void init_fadeout_layers(LayerSurface *l);
static void layer_actual_size(LayerSurface *l, int32_t *width, int32_t *height);
static void get_layer_target_geometry(LayerSurface *l,
									  struct wlr_box *target_box);
static void scene_buffer_apply_effect(struct wlr_scene_buffer *buffer,
									  int32_t sx, int32_t sy, void *data);
static double find_animation_curve_at(double t, int32_t type);

static void apply_opacity_to_rect_nodes(Client *c, struct wlr_scene_node *node,
										double animation_passed);
static enum corner_location set_client_corner_location(Client *c);
static double all_output_frame_duration_ms();
static struct wlr_scene_tree *
wlr_scene_tree_snapshot(struct wlr_scene_node *node,
						struct wlr_scene_tree *parent);
static bool is_scroller_layout(Monitor *m);
static bool is_centertile_layout(Monitor *m);
static void create_output(struct wlr_backend *backend, void *data);
static void get_layout_abbr(char *abbr, const char *full_name);
static void apply_named_scratchpad(Client *target_client);
static Client *get_client_by_id_or_title(const char *arg_id,
										 const char *arg_title);
static bool switch_scratchpad_client_state(Client *c);
static bool check_trackpad_disabled(struct wlr_pointer *pointer);
static uint32_t get_tag_status(uint32_t tag, Monitor *m);
static void enable_adaptive_sync(Monitor *m, struct wlr_output_state *state);
static Client *get_next_stack_client(Client *c, bool reverse);
static void set_float_malposition(Client *tc);
static void set_size_per(Monitor *m, Client *c);
static void resize_tile_client(Client *grabc, bool isdrag, int32_t offsetx,
							   int32_t offsety, uint32_t time);
static void refresh_monitors_workspaces_status(Monitor *m);
static void init_client_properties(Client *c);
static float *get_border_color(Client *c);
static void clear_fullscreen_and_maximized_state(Monitor *m);
static void request_fresh_all_monitors(void);
static Client *find_client_by_direction(Client *tc, const Arg *arg,
										bool findfloating, bool ignore_align);
static void exit_scroller_stack(Client *c);
static Client *scroll_get_stack_head_client(Client *c);
static bool client_only_in_one_tag(Client *c);
static Client *get_focused_stack_client(Client *sc);
static bool client_is_in_same_stack(Client *sc, Client *tc, Client *fc);
static void monitor_stop_skip_frame_timer(Monitor *m);
static int monitor_skip_frame_timeout_callback(void *data);
static Monitor *get_monitor_nearest_to(int32_t lx, int32_t ly);
static bool match_monitor_spec(char *spec, Monitor *m);
static void last_cursor_surface_destroy(struct wl_listener *listener,
										void *data);
static int32_t keep_idle_inhibit(void *data);
static void check_keep_idle_inhibit(Client *c);
static void pre_caculate_before_arrange(Monitor *m, bool want_animation,
										bool from_view, bool only_caculate);
static void client_pending_fullscreen_state(Client *c, int32_t isfullscreen);
static void client_pending_maximized_state(Client *c, int32_t ismaximized);
static void client_pending_minimized_state(Client *c, int32_t isminimized);
static void scroller_insert_stack(Client *c, Client *target_client,
								  bool insert_before);
static void dwindle_move_client(DwindleNode **root, Client *c, Client *target,
								float ratio, int32_t dir);
static void dwindle_resize_client_step(Monitor *m, Client *c, int32_t dx,
									   int32_t dy);
static void dwindle_resize_client(Monitor *m, Client *c);

static struct TagScrollerState *ensure_scroller_state(Monitor *m, uint32_t tag);
static struct ScrollerStackNode *find_scroller_node(struct TagScrollerState *st,
													Client *c);
static void sync_scroller_state_to_clients(Monitor *m, uint32_t tag);
static void scroller_node_remove(struct TagScrollerState *st,
								 struct ScrollerStackNode *target);
static struct ScrollerStackNode *
scroller_node_create(struct TagScrollerState *st, Client *c);
static void update_scroller_state(Monitor *m);
Client *scroll_get_stack_tail_client(Client *c);

/* Edge-snap (Aero-Snap) helpers */
static EdgeSnapZone detect_edge_snap_zone(Monitor *m, double cx, double cy);
static struct wlr_box get_edge_snap_geometry(EdgeSnapZone zone, Monitor *m);
static void update_edge_snap_preview(Client *c);
static void hide_edge_snap_preview(void);
static void apply_edge_snap(Client *c);

#include "data/static_keymap.h"
#include "switcher/switcher.h"
#include "dispatch/bind_declare.h"
#include "layout/layout.h"

/* variables */
static const char broken[] = "broken";
static pid_t child_pid = -1;
static int32_t locked;
static uint32_t locked_mods = 0;
static void *exclusive_focus;
static struct wl_display *dpy;
static struct wl_event_loop *event_loop;
static struct wlr_backend *backend;
static struct wlr_backend *headless_backend;
static struct wlr_scene *scene;
static struct wlr_scene_tree *layers[NUM_LAYERS];
static struct wlr_renderer *drw;
static struct wlr_allocator *alloc;
static struct wlr_compositor *compositor;

static struct wlr_xdg_shell *xdg_shell;
static struct wlr_xdg_activation_v1 *activation;
static struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
static struct wl_list clients; /* tiling order */
static struct wl_list fstack;  /* focus order */
static struct wl_list fadeout_clients;
static struct wl_list fadeout_layers;
static struct wlr_idle_notifier_v1 *idle_notifier;
static struct wlr_idle_inhibit_manager_v1 *idle_inhibit_mgr;
static struct wlr_layer_shell_v1 *layer_shell;
static struct wlr_output_manager_v1 *output_mgr;
static struct wlr_virtual_keyboard_manager_v1 *virtual_keyboard_mgr;
static struct wlr_keyboard_shortcuts_inhibit_manager_v1
	*keyboard_shortcuts_inhibit;
static struct wlr_virtual_pointer_manager_v1 *virtual_pointer_mgr;
static struct wlr_output_power_manager_v1 *power_mgr;
static struct wlr_pointer_gestures_v1 *pointer_gestures;
static struct wlr_drm_lease_v1_manager *drm_lease_manager;
struct aether_print_status_manager *print_status_manager;

static struct wlr_cursor *cursor;
static struct wlr_xcursor_manager *cursor_mgr;
static struct wlr_session *session;

static struct wlr_scene_rect *root_bg;
static struct wlr_session_lock_manager_v1 *session_lock_mgr;
static struct wlr_scene_rect *locked_bg;
static struct wlr_session_lock_v1 *cur_lock;
static const int32_t layermap[] = {LyrBg, LyrBottom, LyrTop, LyrOverlay};
static struct wlr_scene_tree *drag_icon;
static struct wlr_cursor_shape_manager_v1 *cursor_shape_mgr;
static struct wlr_pointer_constraints_v1 *pointer_constraints;
static struct wlr_relative_pointer_manager_v1 *relative_pointer_mgr;
static struct wlr_pointer_constraint_v1 *active_constraint;

static struct wlr_seat *seat;
static KeyboardGroup *kb_group;
static struct wl_list inputdevices;
static struct wl_list keyboard_shortcut_inhibitors;
static uint32_t cursor_mode;
static Client *grabc, *dropc;
static int32_t rzcorner;
static uint32_t resize_edges; /* WLR_EDGE_* bitmask for current resize */
static int32_t grabcx, grabcy;						   /* client-relative */
static int32_t drag_begin_cursorx, drag_begin_cursory; /* client-relative */
static bool start_drag_window = false;
static int32_t last_apply_drap_time = 0;

/* Edge-snap (Aero-Snap) state */
static EdgeSnapZone current_edge_snap_zone = EDGE_SNAP_NONE;
static struct wlr_scene_rect *edge_snap_preview_rect = NULL;

static struct wlr_output_layout *output_layout;
static struct wlr_box sgeom;
static struct wl_list mons;
static Monitor *selmon;

static int32_t enablegaps = 1; /* enables gaps, used by togglegaps */
static int32_t axis_apply_time = 0;
static int32_t axis_apply_dir = 0;
static int32_t scroller_focus_lock = 0;

static uint32_t swipe_fingers = 0;
static double swipe_dx = 0;
static double swipe_dy = 0;

bool render_border = true;

uint32_t chvt_backup_tag = 0;
bool allow_frame_scheduling = true;
char chvt_backup_selmon[32] = {0};

struct dvec2 *baked_points_move;
struct dvec2 *baked_points_open;
struct dvec2 *baked_points_tag;
struct dvec2 *baked_points_close;
struct dvec2 *baked_points_focus;
struct dvec2 *baked_points_opafadein;
struct dvec2 *baked_points_opafadeout;

static struct wl_event_source *hide_cursor_source;
static struct wl_event_source *keep_idle_inhibit_source;
/* config file auto-reload */
static struct wl_event_source *config_watch_source = NULL;
static int                     config_inotify_fd   = -1;
static int                     config_inotify_wd   = -1;
static bool cursor_hidden = false;
static bool tag_combo = false;
static const char *cli_config_path = NULL;
static bool cli_debug_log = false;
static KeyMode keymode = {
	.mode = {'d', 'e', 'f', 'a', 'u', 'l', 't', '\0'},
	.isdefault = true,
};

static char *env_vars[] = {"DISPLAY",
						   "WAYLAND_DISPLAY",
						   "XDG_CURRENT_DESKTOP",
						   "XDG_SESSION_TYPE",
						   "XCURSOR_THEME",
						   "XCURSOR_SIZE",
						   NULL};
static struct {
	enum wp_cursor_shape_device_v1_shape shape;
	struct wlr_surface *surface;
	int32_t hotspot_x;
	int32_t hotspot_y;
} last_cursor;

#include "client/client.h"
#include "config/preset.h"
struct Pertag {
	uint32_t curtag, prevtag;
	int32_t nmasters[LENGTH(tags) + 1];
	float mfacts[LENGTH(tags) + 1];
	int32_t no_hide[LENGTH(tags) + 1];
	int32_t no_render_border[LENGTH(tags) + 1];
	int32_t open_as_floating[LENGTH(tags) + 1];
	struct DwindleNode *dwindle_root[LENGTH(tags) + 1];
	const Layout *ltidxs[LENGTH(tags) + 1];
	struct TagScrollerState *scroller_state[LENGTH(tags) + 1];

};
#include "config/parse_config.h"

static struct wl_signal aether_print_status;

static struct wl_listener print_status_listener = {.notify =
													   handle_print_status};
static struct wl_listener cursor_axis = {.notify = axisnotify};
static struct wl_listener cursor_button = {.notify = buttonpress};
static struct wl_listener cursor_frame = {.notify = cursorframe};
static struct wl_listener cursor_motion = {.notify = motionrelative};
static struct wl_listener cursor_motion_absolute = {.notify = motionabsolute};
static struct wl_listener gpu_reset = {.notify = gpureset};
static struct wl_listener layout_change = {.notify = updatemons};
static struct wl_listener new_idle_inhibitor = {.notify = createidleinhibitor};
static struct wl_listener new_input_device = {.notify = inputdevice};
static struct wl_listener new_virtual_keyboard = {.notify = virtualkeyboard};
static struct wl_listener new_virtual_pointer = {.notify = virtualpointer};
static struct wl_listener new_pointer_constraint = {
	.notify = createpointerconstraint};
static struct wl_listener new_output = {.notify = createmon};
static struct wl_listener new_xdg_toplevel = {.notify = createnotify};
static struct wl_listener new_xdg_popup = {.notify = createpopup};
static struct wl_listener new_xdg_decoration = {.notify = createdecoration};
static struct wl_listener new_layer_surface = {.notify = createlayersurface};
static struct wl_listener output_mgr_apply = {.notify = outputmgrapply};
static struct wl_listener output_mgr_test = {.notify = outputmgrtest};
static struct wl_listener output_power_mgr_set_mode = {.notify =
														   powermgrsetmode};
static struct wl_listener request_activate = {.notify = urgent};
static struct wl_listener request_cursor = {.notify = setcursor};
static struct wl_listener request_set_psel = {.notify = setpsel};
static struct wl_listener request_set_sel = {.notify = setsel};
static struct wl_listener request_set_cursor_shape = {.notify = setcursorshape};
static struct wl_listener request_start_drag = {.notify = requeststartdrag};
static struct wl_listener start_drag = {.notify = startdrag};
static struct wl_listener new_session_lock = {.notify = locksession};
static struct wl_listener drm_lease_request = {.notify = requestdrmlease};
static struct wl_listener keyboard_shortcuts_inhibit_new_inhibitor = {
	.notify = handle_keyboard_shortcuts_inhibit_new_inhibitor};
static struct wl_listener last_cursor_surface_destroy_listener = {
	.notify = last_cursor_surface_destroy};

#ifdef XWAYLAND
static void fix_xwayland_unmanaged_coordinate(Client *c);
static int32_t synckeymap(void *data);
static void activatex11(struct wl_listener *listener, void *data);
static void configurex11(struct wl_listener *listener, void *data);
static void createnotifyx11(struct wl_listener *listener, void *data);
static void dissociatex11(struct wl_listener *listener, void *data);
static void commitx11(struct wl_listener *listener, void *data);
static void associatex11(struct wl_listener *listener, void *data);
static void sethints(struct wl_listener *listener, void *data);
static void xwaylandready(struct wl_listener *listener, void *data);
static void setgeometrynotify(struct wl_listener *listener, void *data);
static struct wl_listener new_xwayland_surface = {.notify = createnotifyx11};
static struct wl_listener xwayland_ready = {.notify = xwaylandready};
static struct wlr_xwayland *xwayland;
static struct wl_event_source *sync_keymap;
#endif

#include "animation/client.h"
#include "animation/common.h"
#include "animation/layer.h"
#include "animation/tag.h"
#include "dispatch/bind_define.h"
#include "ext-protocol/all.h"
#include "fetch/fetch.h"
#include "layout/arrange.h"
#include "layout/dwindle.h"
#include "layout/floating.h"
#include "layout/horizontal.h"
#include "layout/scroll.h"
#include "layout/vertical.h"


/* Included split files */
#include "core/rules_wm.c"
#include "core/client_state.c"
#include "core/input_devices.c"
#include "core/layers_popups.c"
#include "core/monitors_rendering.c"
#include "core/client_focus.c"
#include "core/compositor_setup.c"
#include "core/features_misc.c"

#include "arg.h"
#include "aether-ipc-unstable-x-protocol.h"
#include "dynarr.h"
#include <ctype.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-util.h>

#define FATAL_ERROR(fmt, ...) \
    do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)

char *argv0;

typedef enum {
    MODE_NONE = 0,
    MODE_SET = 1 << 0,
    MODE_GET = 1 << 1,
    MODE_WATCH = (1 << 2) | MODE_GET,
} RunMode;

static RunMode current_mode = MODE_NONE;

typedef struct {
    int32_t O, T, L, o, t, l, c, v, m, f, q, d, x, e, k, b, A;
} AppFlags;
static AppFlags flags;

typedef struct {
    uint32_t occ, seltags, total_clients, urg;
    char *output_name;
    int32_t tagcount;
    char *tagset;
    char *layout_name;
    int32_t layoutcount, layout_idx;
    char *client_tags;
    char *dispatch_cmd;
    char *dispatch_args[5];
} AppContext;
static AppContext ctx;

struct output {
    char *output_name;
    uint32_t name;
};
static DYNARR_DEF(struct output) outputs;

static struct wl_display *wl_dpy;
static struct zaether_ipc_manager_v2 *ipc_mgr;

/* No-op wl_output handlers */
#define NOOP(name, ...) static void noop_##name(__VA_ARGS__) {}
NOOP(geometry, void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform)
NOOP(mode, void *data, struct wl_output *wl_output, uint32_t f, int32_t w, int32_t h, int32_t r)
NOOP(done, void *data, struct wl_output *wl_output)
NOOP(scale, void *data, struct wl_output *wl_output, int32_t factor)
NOOP(description, void *data, struct wl_output *wl_output, const char *desc)

static void u32_to_bin_str(char *buf, uint32_t val, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        *buf++ = ((val >> i) & 1) ? '1' : '0';
    }
    *buf = '\0';
}

static void on_ipc_tags(void *data, struct zaether_ipc_manager_v2 *mgr, uint32_t count) {
    ctx.tagcount = count;
    if (flags.T && (current_mode & MODE_GET))
        printf("%d\n", ctx.tagcount);
}

static void on_ipc_layout(void *data, struct zaether_ipc_manager_v2 *mgr, const char *name) {
    if (flags.l && (current_mode & MODE_SET) && strcmp(ctx.layout_name, name) == 0)
        ctx.layout_idx = ctx.layoutcount;
    if (flags.L && (current_mode & MODE_GET))
        printf("%s\n", name);
    ctx.layoutcount++;
}

static const struct zaether_ipc_manager_v2_listener ipc_mgr_listener = {
    .tags = on_ipc_tags,
    .layout = on_ipc_layout
};

/* Boilerplate macros for IPC output events */
#define IPC_EV(name, cond, print_fmt, type, ...) \
    static void on_ipc_##name(void *data, struct zaether_ipc_output_v2 *out, type val) { \
        if (!(cond)) return; \
        if (data) printf("%s ", (char *)data); \
        printf(print_fmt "\n", __VA_ARGS__); \
    }

#define IPC_EV_VOID(name, cond, print_fmt, ...) \
    static void on_ipc_##name(void *data, struct zaether_ipc_output_v2 *out) { \
        if (!(cond)) return; \
        if (data) printf("%s ", (char *)data); \
        printf(print_fmt "\n", ##__VA_ARGS__); \
    }

IPC_EV_VOID(toggle_visibility, flags.v, "toggle")
IPC_EV(layout_symbol, (flags.l && (current_mode & MODE_GET)), "layout %s", const char *, val)
IPC_EV(title, (flags.c && (current_mode & MODE_GET)), "title %s", const char *, val)
IPC_EV(appid, (flags.c && (current_mode & MODE_GET)), "appid %s", const char *, val)
IPC_EV(x, flags.x, "x %d", int32_t, val)
IPC_EV(y, flags.x, "y %d", int32_t, val)
IPC_EV(width, flags.x, "width %d", int32_t, val)
IPC_EV(height, flags.x, "height %d", int32_t, val)
IPC_EV(last_layer, flags.e, "last_layer %s", const char *, val)
IPC_EV(kb_layout, flags.k, "kb_layout %s", const char *, val)
IPC_EV(scalefactor, flags.A, "scale_factor %f", const uint32_t, val / 100.0f)
IPC_EV(keymode, flags.b, "keymode %s", const char *, val)
IPC_EV(fullscreen, flags.m, "fullscreen %u", uint32_t, val)
IPC_EV(floating, flags.f, "floating %u", uint32_t, val)

static void on_ipc_active(void *data, struct zaether_ipc_output_v2 *out, uint32_t active) {
    if (!flags.o) {
        if ((current_mode & MODE_SET) && !ctx.output_name && active)
            ctx.output_name = strdup((char *)data);
        return;
    }
    if (data) printf("%s ", (char *)data);
    printf("selmon %u\n", active ? 1 : 0);
}

static void on_ipc_tag(void *data, struct zaether_ipc_output_v2 *out, uint32_t tag, uint32_t state, uint32_t clients, uint32_t focused) {
    if (!flags.t) return;
    
    if (state == ZAETHER_IPC_OUTPUT_V2_TAG_STATE_ACTIVE) ctx.seltags |= (1 << tag);
    if (state == ZAETHER_IPC_OUTPUT_V2_TAG_STATE_URGENT) ctx.urg |= (1 << tag);
    if (clients > 0) ctx.occ |= (1 << tag);
    
    ctx.total_clients += clients;

    if (!(current_mode & MODE_GET)) return;
    
    if (data) printf("%s ", (char *)data);
    printf("tag %u %u %u %u\n", tag + 1, state, clients, focused);
}

static void on_ipc_layout_ev(void *data, struct zaether_ipc_output_v2 *out, uint32_t layout) {
    // Empty intentionally
}

static void handle_set_frame(void *data, struct zaether_ipc_output_v2 *out) {
    if (data && (!ctx.output_name || strcmp(ctx.output_name, (char *)data))) return;

    if (flags.q) {
        zaether_ipc_output_v2_quit(out);
    }
    if (flags.l) {
        zaether_ipc_output_v2_set_layout(out, ctx.layout_idx);
    }
    if (flags.t) {
        uint32_t mask = ctx.seltags;
        char *t = ctx.tagset;
        int32_t i = 0;

        for (; *t && isdigit((unsigned char)*t); t++)
            i = *t - '0' + i * 10;

        if (!*t) mask = 1 << (i - 1);

        for (; *t; t++, i++) {
            switch (*t) {
                case '-': mask &= ~(1 << (i - 1)); break;
                case '+': mask |= 1 << (i - 1); break;
                case '^': mask ^= 1 << (i - 1); break;
            }
        }
        if ((i - 1) > ctx.tagcount) FATAL_ERROR("bad tagset %s", ctx.tagset);
        zaether_ipc_output_v2_set_tags(out, mask, 0);
    }
    if (flags.c) {
        uint32_t and_m = ~0, xor_m = 0;
        char *t = ctx.client_tags;
        int32_t i = 0;

        for (; *t && isdigit((unsigned char)*t); t++)
            i = *t - '0' + i * 10;

        if (!*t) t = "+";

        for (; *t; t++, i++) {
            switch (*t) {
                case '-': and_m &= ~(1 << (i - 1)); break;
                case '+': and_m &= ~(1 << (i - 1)); xor_m |= 1 << (i - 1); break;
                case '^': xor_m |= 1 << (i - 1); break;
            }
        }
        if ((i - 1) > ctx.tagcount) FATAL_ERROR("bad client tagset %s", ctx.client_tags);
        zaether_ipc_output_v2_set_client_tags(out, and_m, xor_m);
    }
    if (flags.d) {
        zaether_ipc_output_v2_dispatch(out, ctx.dispatch_cmd, ctx.dispatch_args[0], 
            ctx.dispatch_args[1], ctx.dispatch_args[2], ctx.dispatch_args[3], ctx.dispatch_args[4]);
    }
    wl_display_flush(wl_dpy);
    usleep(1000);
    exit(0);
}

static void on_ipc_frame(void *data, struct zaether_ipc_output_v2 *out) {
    if (current_mode & MODE_SET) {
        handle_set_frame(data, out);
    } else {
        if (flags.t) {
            char *out_name = (char *)data;
            printf("%s clients %u\n", out_name, ctx.total_clients);

            char occ_s[10], sel_s[10], urg_s[10];
            u32_to_bin_str(occ_s, ctx.occ, 9);
            u32_to_bin_str(sel_s, ctx.seltags, 9);
            u32_to_bin_str(urg_s, ctx.urg, 9);
            
            printf("%s tags %u %u %u\n", out_name, ctx.occ, ctx.seltags, ctx.urg);
            printf("%s tags %s %s %s\n", out_name, occ_s, sel_s, urg_s);
            
            ctx.occ = ctx.seltags = ctx.total_clients = ctx.urg = 0;
        }
    }
    fflush(stdout);
}

static const struct zaether_ipc_output_v2_listener ipc_out_listener = {
    .toggle_visibility = on_ipc_toggle_visibility,
    .active = on_ipc_active,
    .tag = on_ipc_tag,
    .layout = on_ipc_layout_ev,
    .title = on_ipc_title,
    .appid = on_ipc_appid,
    .layout_symbol = on_ipc_layout_symbol,
    .fullscreen = on_ipc_fullscreen,
    .floating = on_ipc_floating,
    .x = on_ipc_x,
    .y = on_ipc_y,
    .width = on_ipc_width,
    .height = on_ipc_height,
    .last_layer = on_ipc_last_layer,
    .kb_layout = on_ipc_kb_layout,
    .keymode = on_ipc_keymode,
    .scalefactor = on_ipc_scalefactor,
    .frame = on_ipc_frame,
};

static void parse_dispatch_args(char *arg) {
    ctx.dispatch_cmd = ctx.dispatch_args[0] = ctx.dispatch_args[1] = 
    ctx.dispatch_args[2] = ctx.dispatch_args[3] = ctx.dispatch_args[4] = "";
    
    char *tokens[6] = {0};
    int count = 0;

    while (arg && count < 6) {
        char *comma = (count < 5) ? strchr(arg, ',') : NULL;
        if (comma) {
            *comma = '\0';
            tokens[count++] = arg;
            arg = comma + 1;
        } else {
            tokens[count++] = arg;
            break;
        }
    }

    for (int i = 0; i < count; i++) {
        char *str = tokens[i];
        while (isspace((unsigned char)*str)) str++;
        if (*str) {
            char *end = str + strlen(str) - 1;
            while (end > str && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';
        }
        tokens[i] = str;
    }

    if (count > 0) ctx.dispatch_cmd = tokens[0];
    for (int i = 1; i < count; i++) {
        ctx.dispatch_args[i - 1] = tokens[i];
    }
}

static void on_wl_output_name(void *data, struct wl_output *out, const char *name) {
    if (outputs.arr) {
        struct output *o = (struct output *)data;
        o->output_name = strdup(name);
        printf("+ ");
    }
    if (flags.O) printf("%s\n", name);
    
    if (ctx.output_name && strcmp(ctx.output_name, name) != 0) {
        wl_output_release(out);
        return;
    }
    
    struct zaether_ipc_output_v2 *ipc_out = zaether_ipc_manager_v2_get_output(ipc_mgr, out);
    zaether_ipc_output_v2_add_listener(ipc_out, &ipc_out_listener, ctx.output_name ? NULL : strdup(name));
}

static const struct wl_output_listener wl_out_listener = {
    .geometry = noop_geometry,
    .mode = noop_mode,
    .done = noop_done,
    .scale = noop_scale,
    .name = on_wl_output_name,
    .description = noop_description,
};

static void on_global_add(void *data, struct wl_registry *reg, uint32_t name, const char *iface, uint32_t version) {
    if (strcmp(iface, wl_output_interface.name) == 0) {
        struct wl_output *o = wl_registry_bind(reg, name, &wl_output_interface, WL_OUTPUT_NAME_SINCE_VERSION);
        if (!outputs.arr) {
            wl_output_add_listener(o, &wl_out_listener, NULL);
        } else {
            DYNARR_PUSH(&outputs, (struct output){.name = name});
            wl_output_add_listener(o, &wl_out_listener, &outputs.arr[outputs.len - 1]);
        }
    } else if (strcmp(iface, zaether_ipc_manager_v2_interface.name) == 0) {
        ipc_mgr = wl_registry_bind(reg, name, &zaether_ipc_manager_v2_interface, 2);
        zaether_ipc_manager_v2_add_listener(ipc_mgr, &ipc_mgr_listener, NULL);
    }
}

static void on_global_remove(void *data, struct wl_registry *reg, uint32_t name) {
    if (!outputs.arr) return;
    struct output *o = outputs.arr;
    for (size_t i = 0; i < outputs.len; i++, o++) {
        if (o->name == name) {
            printf("- %s\n", o->output_name);
            free(o->output_name);
            *o = DYNARR_POP(&outputs);
        }
    }
}

static const struct wl_registry_listener reg_listener = {
    .global = on_global_add,
    .global_remove = on_global_remove,
};

static void print_usage(void) {
	fprintf(stderr,
			"AetherMessenger - AetherWM IPC\n"
			"\n"
			"SYNOPSIS:\n"
			"\tAetherMessenger [-OTLq]\n"
			"\tAetherMessenger [-o <output>] -s [-t <tags>] [-l <layout>] [-c <tags>] [-d "
			"<cmd>,<arg1>,<arg2>,<arg3>,<arg4>,<arg5>]\n"
			"\tAetherMessenger [-o <output>] (-g | -w) [-OotlcvmfxekbA]\n"
			"\n"
			"OPERATION MODES:\n"
			"\t-g           Get values (tags, layout, focused client)\n"
			"\t-s           Set values (switch tags, layouts)\n"
			"\t-w           Watch mode (stream events)\n"
			"\n"
			"GENERAL OPTIONS:\n"
			"\t-O           Get all output (monitor) information\n"
			"\t-T           Get number of tags\n"
			"\t-L           Get all available layouts\n"
			"\t-q           Quit aether\n"
			"\t-o <output>  Select output (monitor)\n"
			"\n"
			"GET OPTIONS (used with -g or -w):\n"
			"\t-O           Get output name\n"
			"\t-o           Get output (monitor) focus information\n"
			"\t-t           Get selected tags\n"
			"\t-l           Get current layout\n"
			"\t-c           Get title and appid of focused clients\n"
			"\t-v           Get visibility of statusbar\n"
			"\t-m           Get fullscreen status\n"
			"\t-f           Get floating status\n"
			"\t-x           Get focused client geometry\n"
			"\t-e           Get name of last focused layer\n"
			"\t-k           Get current keyboard layout\n"
			"\t-b           Get current keybind mode\n"
			"\t-A           Get scale factor of monitor\n"
			"\n"
			"SET OPTIONS (used with -s):\n"
			"\t-o <output>         Select output (monitor)\n"
			"\t-t <tags>           Set selected tags (can be used with [+-^.] "
			"modifiers)\n"
			"\t-l <layout>         Set current layout\n"
			"\t-c <tags>           Get title and appid of focused client\n"
			"\t-d <cmd>,<args...>  Dispatch internal command (max 5 args)\n");
	exit(2);
}

int32_t main(int32_t argc, char *argv[]) {
	ARGBEGIN {
	case 'q':
		flags.q = 1;
		if (!(current_mode & MODE_GET)) current_mode = MODE_SET;
		break;
	case 's':
		if (current_mode != MODE_NONE) print_usage();
		current_mode = MODE_SET;
		break;
	case 'g':
		if (current_mode != MODE_NONE) print_usage();
		current_mode = MODE_GET;
		break;
	case 'w':
		if (current_mode != MODE_NONE) print_usage();
		current_mode = MODE_WATCH;
		break;
	case 'o':
		if (current_mode == MODE_GET || current_mode == MODE_WATCH)
			flags.o = 1;
		else if (current_mode == MODE_SET)
			ctx.output_name = EARGF(print_usage());
		else
			ctx.output_name = ARGF();
		break;
	case 't':
		flags.t = 1;
		if (!(current_mode & MODE_GET)) {
			current_mode = MODE_SET;
			ctx.tagset = EARGF(print_usage());
		}
		break;
	case 'l':
		flags.l = 1;
		if (!(current_mode & MODE_GET)) {
			current_mode = MODE_SET;
			ctx.layout_name = EARGF(print_usage());
		}
		break;
	case 'c':
		flags.c = 1;
		if (!(current_mode & MODE_GET)) {
			current_mode = MODE_SET;
			ctx.client_tags = EARGF(print_usage());
		}
		break;
	case 'd':
		flags.d = 1;
		if (!(current_mode & MODE_GET)) {
			current_mode = MODE_SET;
			parse_dispatch_args(EARGF(print_usage()));
		}
		break;
	case 'O':
		flags.O = 1;
		if (current_mode && !(current_mode & MODE_GET)) print_usage();
		if (current_mode & MODE_WATCH) DYNARR_INIT(&outputs);
		else current_mode = MODE_GET;
		break;
	case 'T':
		flags.T = 1;
		if (current_mode && current_mode != MODE_GET) print_usage();
		current_mode = MODE_GET;
		break;
	case 'L':
		flags.L = 1;
		if (current_mode && current_mode != MODE_GET) print_usage();
		current_mode = MODE_GET;
		break;
	case 'v': flags.v = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'm': flags.m = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'f': flags.f = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'x': flags.x = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'e': flags.e = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'k': flags.k = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'b': flags.b = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	case 'A': flags.A = 1; if (current_mode == MODE_SET) print_usage(); current_mode |= MODE_GET; break;
	default:
		fprintf(stderr, "bad option %c\n", ARGC());
		print_usage();
	}
	ARGEND

	if (current_mode == MODE_NONE) print_usage();

	if ((current_mode & MODE_GET) && !ctx.output_name &&
		!(flags.o || flags.t || flags.l || flags.O || flags.T || flags.L || flags.c ||
		  flags.v || flags.m || flags.f || flags.x || flags.e || flags.k || flags.b ||
		  flags.A || flags.d)) {
		flags.o = flags.t = flags.l = flags.c = flags.v = flags.m = flags.f = flags.x =
		flags.e = flags.k = flags.b = flags.A = 1;
	}

	wl_dpy = wl_display_connect(NULL);
	if (!wl_dpy) FATAL_ERROR("bad display");

	struct wl_registry *registry = wl_display_get_registry(wl_dpy);
	wl_registry_add_listener(registry, &reg_listener, NULL);

	wl_display_dispatch(wl_dpy);
	wl_display_roundtrip(wl_dpy);

	if (!ipc_mgr) FATAL_ERROR("bad aether-ipc protocol");

	wl_display_roundtrip(wl_dpy);

	if (current_mode == MODE_WATCH)
		while (wl_display_dispatch(wl_dpy) != -1);

	return 0;
}

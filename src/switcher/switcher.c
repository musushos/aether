/* switcher.c — included as unity build from src/aether.c */

#include <stdarg.h>
#include <wlr/render/pass.h>
#include <wlr/render/swapchain.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <drm_fourcc.h>
#include <math.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SWITCHER_MAX_CLIENTS 128
#define SWITCHER_DEBUG_LOG "/tmp/aether-switcher.log"

typedef struct {
	struct wlr_scene_tree   *tree;
	struct wlr_scene_buffer *card_bg_node;
	struct wlr_buffer       *bg_normal;
	struct wlr_buffer       *bg_selected;
	struct wlr_scene_buffer *thumb;
} SwitcherSlot;

static struct {
	int active;
	int count;
	int index;
	Monitor *mon;
	Client *items[SWITCHER_MAX_CLIENTS];
	struct wlr_scene_tree *tree;
	struct wlr_scene_rect *backdrop;
	struct wlr_scene_rect *panel;
	struct wlr_scene_rect *outline;
	SwitcherSlot slots[SWITCHER_MAX_CLIENTS];
} switcher;

/* ── forward declarations ──────────────────────────────── */
static int  sw_clientexists(Client *needle);
static int  sw_collect(Monitor *m);
static int  sw_itemindex(int base, int offset, int count);
static void sw_build(void);
static void sw_destroy(void);
static void sw_log(const char *fmt, ...);
static void sw_rendernodescaled(struct wlr_render_pass *pass,
		struct wlr_scene_node *node, int x, int y,
		double sx, double sy);
static struct wlr_buffer *sw_renderthumb(Monitor *m, Client *c,
		int width, int height);
static void sw_hide(void);
static void sw_paintslot(SwitcherSlot *slot, Client *c, int selected,
		int x, int y, int w, int h);
static void sw_updatehighlight(void);

/* ── logging ───────────────────────────────────────────── */
static void
sw_log(const char *fmt, ...)
{
	FILE *fp;
	va_list ap;
	time_t now;
	struct tm tm;
	char stamp[32];

	fp = fopen(SWITCHER_DEBUG_LOG, "a");
	if (!fp) return;
	now = time(NULL);
	localtime_r(&now, &tm);
	strftime(stamp, sizeof(stamp), "%F %T", &tm);
	fprintf(fp, "[%s] ", stamp);
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fputc('\n', fp);
	fclose(fp);
}

/* ── helpers ───────────────────────────────────────────── */
static int
sw_clientexists(Client *needle)
{
	Client *c;
	wl_list_for_each(c, &clients, link) {
		if (c == needle) return 1;
	}
	return 0;
}

static int
sw_collect(Monitor *m)
{
	Client *c;
	uint32_t tags;

	switcher.count = 0;
	switcher.mon   = m;
	if (!m) return 0;

	tags = m->tagset[m->seltags];
	wl_list_for_each(c, &fstack, flink) {
		uint32_t ctags = c->isminimized ? c->mini_restore_tag : c->tags;
		if (switcher.count >= SWITCHER_MAX_CLIENTS) break;
		if (!c->mon || c->mon != m || client_is_unmanaged(c)) continue;
		if (!client_surface(c)) continue;
		/* include minimized windows even though their surface is unmapped */
		if (!client_surface(c)->mapped && !c->isminimized) continue;
		if (!(ctags & tags)) continue;
		switcher.items[switcher.count++] = c;
	}
	return switcher.count;
}

static int
sw_itemindex(int base, int offset, int count)
{
	base = (base + offset) % count;
	if (base < 0) base += count;
	return base;
}

/* ── thumbnail rendering ───────────────────────────────── */
static void
sw_rendernodescaled(struct wlr_render_pass *pass, struct wlr_scene_node *node,
		int x, int y, double sx, double sy)
{
	struct wlr_scene_node *child;

	switch (node->type) {
	case WLR_SCENE_NODE_TREE:
		wl_list_for_each(child, &wlr_scene_tree_from_node(node)->children, link)
			sw_rendernodescaled(pass, child,
					x + (int)round(node->x * sx),
					y + (int)round(node->y * sy), sx, sy);
		break;
	case WLR_SCENE_NODE_RECT: {
		struct wlr_scene_rect *rect = wlr_scene_rect_from_node(node);
		wlr_render_pass_add_rect(pass, &(struct wlr_render_rect_options){
			.box = { .x = x, .y = y,
				.width  = MAX((int)round(rect->width  * sx), 1),
				.height = MAX((int)round(rect->height * sy), 1) },
			.color = { .r=rect->color[0], .g=rect->color[1],
				.b=rect->color[2], .a=rect->color[3] },
			.blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED,
		});
		break;
	}
	case WLR_SCENE_NODE_BUFFER: {
		float alpha = 1.0f;
		struct wlr_texture *texture;
		struct wlr_scene_buffer *sb = wlr_scene_buffer_from_node(node);
		int dw, dh;
		if (!sb->buffer) break;
		texture = wlr_texture_from_buffer(drw, sb->buffer);
		if (!texture) break;
		alpha = sb->opacity;
		dw = sb->dst_width  > 0 ? sb->dst_width  : sb->buffer->width;
		dh = sb->dst_height > 0 ? sb->dst_height : sb->buffer->height;
		wlr_render_pass_add_texture(pass, &(struct wlr_render_texture_options){
			.texture  = texture,
			.src_box  = sb->src_box,
			.dst_box  = { .x = x, .y = y,
				.width  = MAX((int)round(dw * sx), 1),
				.height = MAX((int)round(dh * sy), 1) },
			.alpha      = alpha < 1.0f ? &alpha : NULL,
			.transform  = sb->transform,
			.filter_mode = sb->filter_mode,
			.blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED,
		});
		wlr_texture_destroy(texture);
		break;
	}
	default:
		break;
	}
}

static struct wlr_buffer *
sw_renderthumb(Monitor *m, Client *c, int width, int height)
{
	struct wlr_buffer    *buffer;
	struct wlr_render_pass *pass;
	struct wlr_scene_node  *child;
	double sx, sy;

	if (!m || !c || !c->scene_surface || !m->wlr_output->swapchain)
		return NULL;

	width  = MAX(width,  1);
	height = MAX(height, 1);
	buffer = wlr_allocator_create_buffer(alloc, width, height,
			&m->wlr_output->swapchain->format);
	if (!buffer) return NULL;

	pass = wlr_renderer_begin_buffer_pass(drw, buffer, NULL);
	if (!pass) { wlr_buffer_drop(buffer); return NULL; }

	wlr_render_pass_add_rect(pass, &(struct wlr_render_rect_options){
		.box = {0, 0, width, height},
		.color = {0, 0, 0, 0},
		.blend_mode = WLR_RENDER_BLEND_MODE_NONE,
	});

	sx = (double)width  / MAX(c->geom.width,  1);
	sy = (double)height / MAX(c->geom.height, 1);
	wl_list_for_each(child, &c->scene_surface->children, link)
		sw_rendernodescaled(pass, child, 0, 0, sx, sy);

	if (!wlr_render_pass_submit(pass)) {
		wlr_buffer_drop(buffer);
		return NULL;
	}
	return buffer;
}

/* ── Cairo icon helpers ────────────────────────────────── */
static cairo_surface_t *
sw_pixbuf_to_cairo(GdkPixbuf *pb)
{
	int w = gdk_pixbuf_get_width(pb);
	int h = gdk_pixbuf_get_height(pb);
	int rowstride  = gdk_pixbuf_get_rowstride(pb);
	int n_channels = gdk_pixbuf_get_n_channels(pb);
	gboolean has_alpha = gdk_pixbuf_get_has_alpha(pb);
	guchar *pixels = gdk_pixbuf_get_pixels(pb);
	cairo_surface_t *surface;
	unsigned char *data;
	int stride, x, y;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	cairo_surface_flush(surface);
	data   = cairo_image_surface_get_data(surface);
	stride = cairo_image_surface_get_stride(surface);

	for (y = 0; y < h; y++) {
		guchar  *p = pixels + y * rowstride;
		guint32 *q = (guint32 *)(data + y * stride);
		for (x = 0; x < w; x++) {
			guint8 r = p[0], g = p[1], b = p[2];
			guint8 a = has_alpha ? p[3] : 255;
			r = (r*a)/255; g = (g*a)/255; b = (b*a)/255;
			q[x] = ((guint32)a<<24)|((guint32)r<<16)|((guint32)g<<8)|b;
			p += n_channels;
		}
	}
	cairo_surface_mark_dirty(surface);
	return surface;
}

static GdkPixbuf *
sw_load_icon(const char *app_id, int size)
{
	char path[512];
	int d, e, i;
	const char *exts[] = {"svg", "png"};
	const char *dirs[] = {
		"/usr/share/icons/hicolor/scalable/apps",
		"/usr/share/icons/hicolor/48x48/apps",
		"/usr/share/pixmaps",
		"/usr/share/icons/Adwaita/scalable/apps",
		"/usr/share/icons/Papirus/64x64/apps",
		"/usr/share/icons/breeze/apps/48",
		NULL
	};
	if (!app_id) return NULL;
	for (d = 0; dirs[d]; d++) {
		for (e = 0; e < 2; e++) {
			GError *err = NULL;
			GdkPixbuf *pb;
			char lower_id[256];

			snprintf(path, sizeof(path), "%s/%s.%s", dirs[d], app_id, exts[e]);
			pb = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &err);
			if (pb) return pb;
			if (err) g_error_free(err);

			err = NULL;
			strncpy(lower_id, app_id, sizeof(lower_id)-1);
			lower_id[sizeof(lower_id)-1] = '\0';
			for (i = 0; lower_id[i]; i++) lower_id[i] = tolower(lower_id[i]);
			snprintf(path, sizeof(path), "%s/%s.%s", dirs[d], lower_id, exts[e]);
			pb = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &err);
			if (pb) return pb;
			if (err) g_error_free(err);
		}
	}
	return NULL;
}

/* ── Card background rendering ─────────────────────────── */
static struct wlr_buffer *
sw_render_card(Client *c, int width, int height, int selected)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	double r = MAX((double)config.border_radius, 0.0);
	const float *bg_col  = config.switcher_card_color;
	const float *brd_col;
	int header_h  = 36;
	int icon_size = 24;
	int text_x    = 16;
	const char *app_id, *title;
	unsigned char *pixels;
	int stride;
	struct wlr_texture *tex;
	struct wlr_buffer  *buf;
	struct wlr_render_pass *pass;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create(surface);

	/* clear */
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	/* rounded rect path */
	cairo_new_sub_path(cr);
	cairo_arc(cr, width-r, r,        r, -M_PI/2, 0);
	cairo_arc(cr, width-r, height-r, r, 0,       M_PI/2);
	cairo_arc(cr, r,       height-r, r, M_PI/2,  M_PI);
	cairo_arc(cr, r,       r,        r, M_PI,    3*M_PI/2);
	cairo_close_path(cr);

	/* fill */
	cairo_set_source_rgba(cr, bg_col[0], bg_col[1], bg_col[2], bg_col[3]);
	cairo_fill_preserve(cr);

	/* border */
	brd_col = (c && c->isurgent) ? config.urgentcolor
	        : selected            ? config.switcher_select_color
	                              : config.switcher_card_color;
	cairo_set_source_rgba(cr, brd_col[0], brd_col[1], brd_col[2], brd_col[3]);
	cairo_set_line_width(cr, selected ? 4.0 : 2.0);
	cairo_stroke(cr);

	/* state dot */
	if (c) {
		const float *dot_col =
			c->isurgent        ? config.urgentcolor
			: c->isminimized   ? config.switcher_minimize_color
			: c->ismaximizescreen ? config.switcher_maximize_color
			: c->isfloating    ? config.focuscolor
			                   : config.bordercolor;
		cairo_set_source_rgba(cr, dot_col[0], dot_col[1], dot_col[2], dot_col[3]);
		cairo_arc(cr, text_x + 6, 8 + header_h/2.0, 5, 0, 2*M_PI);
		cairo_fill(cr);
		text_x += 20;
	}

	/* icon */
	app_id = c ? client_get_appid(c) : NULL;
	if (app_id) {
		GdkPixbuf *pb = sw_load_icon(app_id, icon_size);
		if (pb) {
			cairo_surface_t *img = sw_pixbuf_to_cairo(pb);
			if (img) {
				cairo_set_source_surface(cr, img, text_x, 8+(header_h-icon_size)/2.0);
				cairo_paint(cr);
				cairo_surface_destroy(img);
			}
			g_object_unref(pb);
			text_x += icon_size + 8;
		}
	}

	/* title */
	title = c ? client_get_title(c) : NULL;
	if (title) {
		PangoLayout *layout = pango_cairo_create_layout(cr);
		PangoFontDescription *desc;
		int max_w = width - text_x - 16;
		pango_layout_set_text(layout, title, -1);
		desc = pango_font_description_from_string("sans-serif 11");
		pango_layout_set_font_description(layout, desc);
		pango_font_description_free(desc);
		if (max_w > 0) {
			int tw, th;
			pango_layout_set_width(layout, max_w * PANGO_SCALE);
			pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
			pango_layout_get_pixel_size(layout, &tw, &th);
			cairo_move_to(cr, text_x, 8 + (header_h-th)/2.0);
			cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, 1.0);
			pango_cairo_show_layout(cr, layout);
		}
		g_object_unref(layout);
	}

	cairo_destroy(cr);
	cairo_surface_flush(surface);

	pixels = cairo_image_surface_get_data(surface);
	stride = cairo_image_surface_get_stride(surface);

	tex = wlr_texture_from_pixels(drw, DRM_FORMAT_ARGB8888,
			stride, width, height, pixels);
	cairo_surface_destroy(surface);
	if (!tex) return NULL;

	buf = wlr_allocator_create_buffer(alloc, width, height,
			&switcher.mon->wlr_output->swapchain->format);
	if (!buf) { wlr_texture_destroy(tex); return NULL; }

	pass = wlr_renderer_begin_buffer_pass(drw, buf, NULL);
	wlr_render_pass_add_texture(pass, &(struct wlr_render_texture_options){
		.texture    = tex,
		.blend_mode = WLR_RENDER_BLEND_MODE_NONE,
	});
	wlr_render_pass_submit(pass);
	wlr_texture_destroy(tex);
	return buf;
}

/* ── Scene management ──────────────────────────────────── */
static void
sw_destroy(void)
{
	int i;
	if (switcher.tree) {
		wlr_scene_node_destroy(&switcher.tree->node);
		switcher.tree     = NULL;
		switcher.backdrop = NULL;
		switcher.panel    = NULL;
		switcher.outline  = NULL;
	}
	for (i = 0; i < SWITCHER_MAX_CLIENTS; i++) {
		if (switcher.slots[i].bg_normal)   wlr_buffer_drop(switcher.slots[i].bg_normal);
		if (switcher.slots[i].bg_selected) wlr_buffer_drop(switcher.slots[i].bg_selected);
		switcher.slots[i].tree         = NULL;
		switcher.slots[i].card_bg_node = NULL;
		switcher.slots[i].bg_normal    = NULL;
		switcher.slots[i].bg_selected  = NULL;
		switcher.slots[i].thumb        = NULL;
	}
}

static void
sw_hide(void)
{
	sw_destroy();
	switcher.active = 0;
	switcher.count  = 0;
	switcher.index  = 0;
	switcher.mon    = NULL;
}

static void
sw_paintslot(SwitcherSlot *slot, Client *c, int selected,
		int x, int y, int width, int height)
{
	int header_h = 36 + 8;
	int thumb_w, thumb_h, thumb_x, thumb_y;
	struct wlr_buffer *thumb_buffer;

	wlr_scene_node_set_position(&slot->tree->node, x, y);

	slot->bg_normal   = sw_render_card(c, width, height, 0);
	slot->bg_selected = sw_render_card(c, width, height, 1);
	wlr_scene_buffer_set_buffer(slot->card_bg_node,
			selected ? slot->bg_selected : slot->bg_normal);
	wlr_scene_node_set_position(&slot->card_bg_node->node, 0, 0);

	thumb_w = MAX(width  - 24, 1);
	thumb_h = MAX(height - header_h - 16, 1);
	if (c->geom.width > 0 && c->geom.height > 0) {
		if (thumb_w * c->geom.height > thumb_h * c->geom.width)
			thumb_w = thumb_h * c->geom.width / c->geom.height;
		else
			thumb_h = thumb_w * c->geom.height / c->geom.width;
	}
	thumb_x = (width  - thumb_w) / 2;
	thumb_y = header_h + (height - header_h - thumb_h) / 2;

	thumb_buffer = sw_renderthumb(switcher.mon, c, thumb_w, thumb_h);
	wlr_scene_buffer_set_buffer(slot->thumb, thumb_buffer);
	if (thumb_buffer) wlr_buffer_drop(thumb_buffer);
	wlr_scene_buffer_set_dest_size(slot->thumb, thumb_w, thumb_h);
	wlr_scene_node_set_position(&slot->thumb->node, thumb_x, thumb_y);
	wlr_scene_node_set_enabled(&slot->tree->node, 1);
}

static void
sw_updatehighlight(void)
{
	int i;
	if (!switcher.active || !switcher.tree) return;
	for (i = 0; i < switcher.count; i++) {
		struct wlr_buffer *bg = (i == switcher.index)
				? switcher.slots[i].bg_selected
				: switcher.slots[i].bg_normal;
		wlr_scene_buffer_set_buffer(switcher.slots[i].card_bg_node, bg);
	}
}

static void
sw_build(void)
{
	int i, visible, pad, gap;
	int pw, ph, px, py, iw, ih;
	Monitor *m = switcher.mon;

	if (!m) return;
	sw_destroy();

	switcher.tree     = wlr_scene_tree_create(layers[LyrOverlay]);
	switcher.backdrop = wlr_scene_rect_create(switcher.tree, 0, 0,
			config.switcher_backdrop_color);
	switcher.panel    = wlr_scene_rect_create(switcher.tree, 0, 0,
			config.switcher_panel_color);
	switcher.outline  = wlr_scene_rect_create(switcher.tree, 0, 0,
			config.switcher_outline_color);
	wlr_scene_rect_set_corner_radius(switcher.panel,  config.border_radius, CORNER_LOCATION_ALL);
	wlr_scene_rect_set_corner_radius(switcher.outline, config.border_radius, CORNER_LOCATION_ALL);

	visible = config.switcher_max_cards > 0
			? MIN(switcher.count, (int)config.switcher_max_cards)
			: switcher.count;
	visible = MIN(visible, SWITCHER_MAX_CLIENTS);
	pad = (int)config.switcher_pad_px;
	gap = (int)config.switcher_gap_px;
	iw  = MIN((int)config.switcher_card_w_px,
			MAX((m->w.width - 2*pad - (visible-1)*gap) / MAX(visible, 1), 160));
	ih  = MIN((int)config.switcher_card_h_px, MAX(iw * 10 / 16, 120));
	pw  = visible * iw + MAX(visible-1, 0) * gap + 2 * pad;
	ph  = ih + 2 * pad;
	px  = m->w.x + (m->w.width  - pw) / 2;
	py  = m->w.y + (m->w.height - ph) / 2;

	wlr_scene_rect_set_size(switcher.backdrop, m->m.width, m->m.height);
	wlr_scene_node_set_position(&switcher.backdrop->node, m->m.x, m->m.y);
	wlr_scene_rect_set_size(switcher.panel, pw, ph);
	wlr_scene_node_set_position(&switcher.panel->node, px, py);
	wlr_scene_rect_set_size(switcher.outline, pw+2, ph+2);
	wlr_scene_node_set_position(&switcher.outline->node, px-1, py-1);

	for (i = 0; i < visible; i++) {
		switcher.slots[i].tree         = wlr_scene_tree_create(switcher.tree);
		switcher.slots[i].card_bg_node = wlr_scene_buffer_create(switcher.slots[i].tree, NULL);
		switcher.slots[i].thumb        = wlr_scene_buffer_create(switcher.slots[i].tree, NULL);
		sw_paintslot(&switcher.slots[i], switcher.items[i], i == switcher.index,
				px + pad + i*(iw+gap), py + pad, iw, ih);
	}

	wlr_scene_node_raise_to_top(&switcher.tree->node);
	wlr_scene_node_set_enabled(&switcher.tree->node, 1);
}

/* ── Public API ────────────────────────────────────────── */
void
switchercancel(void)
{
	sw_log("cancel");
	sw_hide();
}

void
switchercommit(void)
{
	Client *c;
	if (!switcher.active || switcher.count <= 0) return;
	c = switcher.items[switcher.index];
	sw_log("commit index=%d title='%s'", switcher.index,
			c ? client_get_title(c) : "(null)");
	sw_hide();
	if (!c || !sw_clientexists(c)) return;

	if (c->isminimized) {
		extern void unminimize(Client *c);
		unminimize(c);
	}
	focusclient(c, 1);
}

int
switcherisactive(void)
{
	return switcher.active;
}

void
switcherstep(const Arg *arg)
{
	int dir = (arg && arg->i < 0) ? -1 : 1;
	sw_log("step dir=%d active=%d", dir, switcher.active);

	if (!switcher.active && sw_collect(selmon) < 2) {
		sw_log("step: insufficient windows");
		sw_hide();
		return;
	}
	if (!switcher.active) {
		switcher.active = 1;
		switcher.index  = dir > 0 ? 1 : switcher.count - 1;
		sw_build();
	} else {
		switcher.index = sw_itemindex(switcher.index, dir, switcher.count);
		sw_updatehighlight();
	}
	sw_log("step selected=%d title='%s'", switcher.index,
			client_get_title(switcher.items[switcher.index]));
}

void
switcherupdatemods(uint32_t mods)
{
	if (switcher.active && !(CLEANMASK(mods) & CLEANMASK(config.switcher_modifier)))
		switchercommit();
}

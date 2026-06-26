#include "aether-ipc-unstable-x-protocol.h"

static void aether_ipc_manager_bind(struct wl_client *client, void *data,
								 uint32_t version, uint32_t id);
static void aether_ipc_manager_destroy(struct wl_resource *resource);
static void aether_ipc_manager_get_output(struct wl_client *client,
									   struct wl_resource *resource,
									   uint32_t id, struct wl_resource *output);
static void aether_ipc_manager_release(struct wl_client *client,
									struct wl_resource *resource);
static void aether_ipc_output_destroy(struct wl_resource *resource);
static void aether_ipc_output_printstatus(Monitor *monitor);
static void aether_ipc_output_printstatus_to(aetherIpcOutput *ipc_output);
static void aether_ipc_output_set_client_tags(struct wl_client *client,
										   struct wl_resource *resource,
										   uint32_t and_tags,
										   uint32_t xor_tags);
static void aether_ipc_output_set_layout(struct wl_client *client,
									  struct wl_resource *resource,
									  uint32_t index);
static void aether_ipc_output_set_tags(struct wl_client *client,
									struct wl_resource *resource,
									uint32_t tagmask, uint32_t toggle_tagset);
static void aether_ipc_output_quit(struct wl_client *client,
								struct wl_resource *resource);
static void aether_ipc_output_dispatch(struct wl_client *client,
									struct wl_resource *resource,
									const char *dispatch, const char *arg1,
									const char *arg2, const char *arg3,
									const char *arg4, const char *arg5);
static void aether_ipc_output_release(struct wl_client *client,
								   struct wl_resource *resource);

/* global event handlers */
static struct zaether_ipc_manager_v2_interface aether_manager_implementation = {
	.release = aether_ipc_manager_release,
	.get_output = aether_ipc_manager_get_output};
static struct zaether_ipc_output_v2_interface aether_output_implementation = {
	.release = aether_ipc_output_release,
	.set_tags = aether_ipc_output_set_tags,
	.quit = aether_ipc_output_quit,
	.dispatch = aether_ipc_output_dispatch,
	.set_layout = aether_ipc_output_set_layout,
	.set_client_tags = aether_ipc_output_set_client_tags};

void aether_ipc_manager_bind(struct wl_client *client, void *data,
						  uint32_t version, uint32_t id) {
	struct wl_resource *manager_resource =
		wl_resource_create(client, &zaether_ipc_manager_v2_interface, version, id);
	if (!manager_resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(manager_resource,
								   &aether_manager_implementation, NULL,
								   aether_ipc_manager_destroy);

	zaether_ipc_manager_v2_send_tags(manager_resource, LENGTH(tags));

	for (uint32_t i = 0; i < LENGTH(layouts); i++)
		zaether_ipc_manager_v2_send_layout(manager_resource, layouts[i].symbol);
}

void aether_ipc_manager_destroy(struct wl_resource *resource) {
	/* No state to destroy */
}

void aether_ipc_manager_get_output(struct wl_client *client,
								struct wl_resource *resource, uint32_t id,
								struct wl_resource *output) {
	aetherIpcOutput *ipc_output;
	struct wlr_output *op = wlr_output_from_resource(output);
	if (!op)
		return;
	Monitor *monitor = op->data;
	struct wl_resource *output_resource =
		wl_resource_create(client, &zaether_ipc_output_v2_interface,
						   wl_resource_get_version(resource), id);
	if (!output_resource)
		return;

	ipc_output = ecalloc(1, sizeof(*ipc_output));
	ipc_output->resource = output_resource;
	ipc_output->mon = monitor;
	wl_resource_set_implementation(output_resource, &aether_output_implementation,
								   ipc_output, aether_ipc_output_destroy);
	wl_list_insert(&monitor->aether_ipc_outputs, &ipc_output->link);
	aether_ipc_output_printstatus_to(ipc_output);
}

void aether_ipc_manager_release(struct wl_client *client,
							 struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void aether_ipc_output_destroy(struct wl_resource *resource) {
	aetherIpcOutput *ipc_output = wl_resource_get_user_data(resource);
	wl_list_remove(&ipc_output->link);
	free(ipc_output);
}

// Modify the IPC output function to accept mask parameters
void aether_ipc_output_printstatus(Monitor *monitor) {
	aetherIpcOutput *ipc_output;
	wl_list_for_each(ipc_output, &monitor->aether_ipc_outputs, link)
		aether_ipc_output_printstatus_to(ipc_output);
}

// Modify the main IPC output function and send corresponding events according to the mask
void aether_ipc_output_printstatus_to(aetherIpcOutput *ipc_output) {
	Monitor *monitor = ipc_output->mon;
	Client *c = NULL, *focused = NULL;
	struct wlr_keyboard *keyboard;
	xkb_layout_index_t current;
	int32_t tagmask, state, numclients, focused_client, tag;
	const char *title, *appid, *symbol;
	char kb_layout[32];
	focused = focustop(monitor);
	zaether_ipc_output_v2_send_active(ipc_output->resource, monitor == selmon);

	for (tag = 0; tag < LENGTH(tags); tag++) {
		numclients = state = focused_client = 0;
		tagmask = 1 << tag;
		if ((tagmask & monitor->tagset[monitor->seltags]) != 0)
			state |= ZAETHER_IPC_OUTPUT_V2_TAG_STATE_ACTIVE;
		wl_list_for_each(c, &clients, link) {
			if (c->mon != monitor)
				continue;
			if (!(c->tags & tagmask))
				continue;
			if (c == focused)
				focused_client = 1;
			if (c->isurgent)
				state |= ZAETHER_IPC_OUTPUT_V2_TAG_STATE_URGENT;
			numclients++;
		}
		zaether_ipc_output_v2_send_tag(ipc_output->resource, tag, state,
									numclients, focused_client);
	}

	title = focused ? client_get_title(focused) : "";
	appid = focused ? client_get_appid(focused) : "";

	if (monitor->isoverview) {
		symbol = overviewlayout.symbol;
	} else {
		symbol = monitor->pertag->ltidxs[monitor->pertag->curtag]->symbol;
	}

	keyboard = &kb_group->wlr_group->keyboard;
	current = xkb_state_serialize_layout(keyboard->xkb_state,
										 XKB_STATE_LAYOUT_EFFECTIVE);
	get_layout_abbr(kb_layout,
					xkb_keymap_layout_get_name(keyboard->keymap, current));

	zaether_ipc_output_v2_send_layout(
		ipc_output->resource,
		monitor->pertag->ltidxs[monitor->pertag->curtag] - layouts);
	zaether_ipc_output_v2_send_title(ipc_output->resource, title ? title : broken);
	zaether_ipc_output_v2_send_appid(ipc_output->resource, appid ? appid : broken);
	zaether_ipc_output_v2_send_layout_symbol(ipc_output->resource, symbol);
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_FULLSCREEN_SINCE_VERSION) {
		zaether_ipc_output_v2_send_fullscreen(ipc_output->resource,
										   focused ? focused->isfullscreen : 0);
	}
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_FLOATING_SINCE_VERSION) {
		zaether_ipc_output_v2_send_floating(ipc_output->resource,
										 focused ? focused->isfloating : 0);
	}
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_X_SINCE_VERSION) {
		zaether_ipc_output_v2_send_x(ipc_output->resource,
								  focused ? focused->geom.x : 0);
	}
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_Y_SINCE_VERSION) {
		zaether_ipc_output_v2_send_y(ipc_output->resource,
								  focused ? focused->geom.y : 0);
	}
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_WIDTH_SINCE_VERSION) {
		zaether_ipc_output_v2_send_width(ipc_output->resource,
									  focused ? focused->geom.width : 0);
	}
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_HEIGHT_SINCE_VERSION) {
		zaether_ipc_output_v2_send_height(ipc_output->resource,
									   focused ? focused->geom.height : 0);
	}
	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_LAST_LAYER_SINCE_VERSION) {
		zaether_ipc_output_v2_send_last_layer(ipc_output->resource,
										   monitor->last_surface_ws_name);
	}

	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_KB_LAYOUT_SINCE_VERSION) {
		zaether_ipc_output_v2_send_kb_layout(ipc_output->resource, kb_layout);
	}

	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_KEYMODE_SINCE_VERSION) {
		zaether_ipc_output_v2_send_keymode(ipc_output->resource, keymode.mode);
	}

	if (wl_resource_get_version(ipc_output->resource) >=
		ZAETHER_IPC_OUTPUT_V2_SCALEFACTOR_SINCE_VERSION) {
		zaether_ipc_output_v2_send_scalefactor(ipc_output->resource,
											monitor->wlr_output->scale * 100);
	}

	zaether_ipc_output_v2_send_frame(ipc_output->resource);
}

void aether_ipc_output_set_client_tags(struct wl_client *client,
									struct wl_resource *resource,
									uint32_t and_tags, uint32_t xor_tags) {
	aetherIpcOutput *ipc_output;
	Monitor *monitor = NULL;
	Client *selected_client = NULL;
	uint32_t newtags = 0;

	ipc_output = wl_resource_get_user_data(resource);
	if (!ipc_output)
		return;

	monitor = ipc_output->mon;
	selected_client = focustop(monitor);
	if (!selected_client)
		return;

	newtags = (selected_client->tags & and_tags) ^ xor_tags;
	if (!newtags)
		return;

	selected_client->tags = newtags;
	if (selmon == monitor)
		focusclient(focustop(monitor), 1);
	arrange(selmon, false, false);
	printstatus();
}

void aether_ipc_output_set_layout(struct wl_client *client,
							   struct wl_resource *resource, uint32_t index) {
	aetherIpcOutput *ipc_output;
	Monitor *monitor = NULL;

	ipc_output = wl_resource_get_user_data(resource);
	if (!ipc_output)
		return;

	monitor = ipc_output->mon;
	if (index >= LENGTH(layouts))
		index = 0;

	monitor->pertag->ltidxs[monitor->pertag->curtag] = &layouts[index];
	clear_fullscreen_and_maximized_state(monitor);
	arrange(monitor, false, false);
	printstatus();
}

void aether_ipc_output_set_tags(struct wl_client *client,
							 struct wl_resource *resource, uint32_t tagmask,
							 uint32_t toggle_tagset) {
	aetherIpcOutput *ipc_output;
	Monitor *monitor = NULL;
	uint32_t newtags = tagmask & TAGMASK;

	ipc_output = wl_resource_get_user_data(resource);
	if (!ipc_output)
		return;
	monitor = ipc_output->mon;

	view_in_mon(&(Arg){.ui = newtags}, true, monitor, true);
}

void aether_ipc_output_quit(struct wl_client *client,
						 struct wl_resource *resource) {
	quit(&(Arg){0});
}

void aether_ipc_output_dispatch(struct wl_client *client,
							 struct wl_resource *resource, const char *dispatch,
							 const char *arg1, const char *arg2,
							 const char *arg3, const char *arg4,
							 const char *arg5) {

	int32_t (*func)(const Arg *);
	Arg arg;
	func = parse_func_name((char *)dispatch, &arg, (char *)arg1, (char *)arg2,
						   (char *)arg3, (char *)arg4, (char *)arg5);
	if (func) {
		func(&arg);
	}

	if (arg.v)
		free(arg.v);
	if (arg.v2)
		free(arg.v2);
	if (arg.v3)
		free(arg.v3);
}

void aether_ipc_output_release(struct wl_client *client,
							struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

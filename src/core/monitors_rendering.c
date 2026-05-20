
void // 0.6
fullscreennotify(struct wl_listener *listener, void *data) {
	Client *c = wl_container_of(listener, c, fullscreen);

	if (!c || c->iskilling)
		return;

	setfullscreen(c, client_wants_fullscreen(c));
}

void requestmonstate(struct wl_listener *listener, void *data) {
	/* This ensures nested backends can be resized */
	Monitor *m = wl_container_of(listener, m, request_state);
	const struct wlr_output_event_request_state *event = data;

	if (event->state->committed == WLR_OUTPUT_STATE_MODE) {
		switch (event->state->mode_type) {
		case WLR_OUTPUT_STATE_MODE_FIXED:
			wlr_output_state_set_mode(&m->pending, event->state->mode);
			break;
		case WLR_OUTPUT_STATE_MODE_CUSTOM:
			wlr_output_state_set_custom_mode(&m->pending,
											 event->state->custom_mode.width,
											 event->state->custom_mode.height,
											 event->state->custom_mode.refresh);
			break;
		}
		updatemons(NULL, NULL);
		wlr_output_schedule_frame(m->wlr_output);
		return;
	}

	if (!wlr_output_commit_state(m->wlr_output, event->state)) {
		wlr_log(WLR_ERROR,
				"Backend requested a new state that could not be applied");
	}
}

void inputdevice(struct wl_listener *listener, void *data) {
	/* This event is raised by the backend when a new input device becomes
	 * available.
	 * when the backend is a headless backend, this event will never be
	 * triggered.
	 */
	struct wlr_input_device *device = data;
	uint32_t caps;

	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		createkeyboard(wlr_keyboard_from_input_device(device));
		break;
	case WLR_INPUT_DEVICE_POINTER:
		createpointer(wlr_pointer_from_input_device(device));
		break;
	case WLR_INPUT_DEVICE_SWITCH:
		createswitch(wlr_switch_from_input_device(device));
		break;
	default:
		/* TODO handle other input device types */
		break;
	}

	/* We need to let the wlr_seat know what our capabilities are, which is
	 * communiciated to the client. In aether we always have a cursor, even if
	 * there are no pointer devices, so we always include that capability.
	 */
	/* TODO do we actually require a cursor? */
	caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&kb_group->wlr_group->devices))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	wlr_seat_set_capabilities(seat, caps);
}

int32_t keyrepeat(void *data) {
	KeyboardGroup *group = data;
	int32_t i;
	if (!group->nsyms || group->wlr_group->keyboard.repeat_info.rate <= 0)
		return 0;

	wl_event_source_timer_update(
		group->key_repeat_source,
		1000 / group->wlr_group->keyboard.repeat_info.rate);

	for (i = 0; i < group->nsyms; i++)
		keybinding(WL_KEYBOARD_KEY_STATE_PRESSED, false, group->mods,
				   group->keysyms[i], group->keycode);

	return 0;
}

bool is_keyboard_shortcut_inhibitor(struct wlr_surface *surface) {
	KeyboardShortcutsInhibitor *kbsinhibitor;

	wl_list_for_each(kbsinhibitor, &keyboard_shortcut_inhibitors, link) {
		if (kbsinhibitor->inhibitor->surface == surface) {
			return true;
		}
	}
	return false;
}

int32_t // 17
keybinding(uint32_t state, bool locked, uint32_t mods, xkb_keysym_t sym,
		   uint32_t keycode) {
	/*
	 * Here we handle compositor keybindings. This is when the compositor is
	 * processing keys, rather than passing them on to the client for its
	 * own processing.
	 */
	int32_t handled = 0;
	const KeyBinding *k;
	int32_t ji;
	int32_t isbreak = 0;

	if (is_keyboard_shortcut_inhibitor(seat->keyboard_state.focused_surface)) {
		return false;
	}

	for (ji = 0; ji < config.key_bindings_count; ji++) {
		if (config.key_bindings_count < 1)
			break;

		if (locked && config.key_bindings[ji].islockapply == false)
			continue;

		if (state == WL_KEYBOARD_KEY_STATE_RELEASED &&
			config.key_bindings[ji].isreleaseapply == false)
			continue;

		if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
			config.key_bindings[ji].isreleaseapply == true)
			continue;

		if (state != WL_KEYBOARD_KEY_STATE_PRESSED &&
			state != WL_KEYBOARD_KEY_STATE_RELEASED)
			continue;

		k = &config.key_bindings[ji];
		if ((k->iscommonmode || (k->isdefaultmode && keymode.isdefault) ||
			 (strcmp(keymode.mode, k->mode) == 0)) &&
			CLEANMASK(mods) == CLEANMASK(k->mod) &&
			((k->keysymcode.type == KEY_TYPE_SYM &&
			  xkb_keysym_to_lower(sym) ==
				  xkb_keysym_to_lower(k->keysymcode.keysym)) ||
			 (k->keysymcode.type == KEY_TYPE_CODE &&
			  (keycode == k->keysymcode.keycode.keycode1 ||
			   keycode == k->keysymcode.keycode.keycode2 ||
			   keycode == k->keysymcode.keycode.keycode3))) &&
			k->func) {

			if (!k->ispassapply)
				handled = 1;
			else
				handled = 0;

			isbreak = k->func(&k->arg);

			if (isbreak)
				break;
		}
	}
	return handled;
}

bool keypressglobal(struct wlr_surface *last_surface,
					struct wlr_keyboard *keyboard,
					struct wlr_keyboard_key_event *event, uint32_t mods,
					xkb_keysym_t keysym, uint32_t keycode) {
	Client *c = NULL, *lastc = focustop(selmon);
	uint32_t keycodes[32] = {0};
	int32_t reset = false;
	const char *appid = NULL;
	const char *title = NULL;
	int32_t ji;
	const ConfigWinRule *r;

	for (ji = 0; ji < config.window_rules_count; ji++) {
		if (config.window_rules_count < 1)
			break;
		r = &config.window_rules[ji];

		if (!r->globalkeybinding.mod ||
			(!r->globalkeybinding.keysymcode.keysym &&
			 !r->globalkeybinding.keysymcode.keycode.keycode1 &&
			 !r->globalkeybinding.keysymcode.keycode.keycode2 &&
			 !r->globalkeybinding.keysymcode.keycode.keycode3))
			continue;

		/* match key only (case insensitive) ignoring mods */
		if (((r->globalkeybinding.keysymcode.type == KEY_TYPE_SYM &&
			  r->globalkeybinding.keysymcode.keysym == keysym) ||
			 (r->globalkeybinding.keysymcode.type == KEY_TYPE_CODE &&
			  (r->globalkeybinding.keysymcode.keycode.keycode1 == keycode ||
			   r->globalkeybinding.keysymcode.keycode.keycode2 == keycode ||
			   r->globalkeybinding.keysymcode.keycode.keycode3 == keycode))) &&
			r->globalkeybinding.mod == mods) {
			wl_list_for_each(c, &clients, link) {
				if (c && c != lastc) {
					appid = client_get_appid(c);
					title = client_get_title(c);

					if ((r->title && regex_match(r->title, title) && !r->id) ||
						(r->id && regex_match(r->id, appid) && !r->title) ||
						(r->id && regex_match(r->id, appid) && r->title &&
						 regex_match(r->title, title))) {
						reset = true;
						wlr_seat_keyboard_enter(seat, client_surface(c),
												keycodes, 0,
												&keyboard->modifiers);
						wlr_seat_keyboard_send_key(seat, event->time_msec,
												   event->keycode,
												   event->state);
						goto done;
					}
				}
			}
		}
	}

done:
	if (reset)
		wlr_seat_keyboard_enter(seat, last_surface, keycodes, 0,
								&keyboard->modifiers);
	return reset;
}

void keypress(struct wl_listener *listener, void *data) {
	int32_t i;
	/* This event is raised when a key is pressed or released. */
	KeyboardGroup *group = wl_container_of(listener, group, key);
	struct wlr_keyboard_key_event *event = data;

	struct wlr_surface *last_surface = seat->keyboard_state.focused_surface;
	struct wlr_xdg_surface *xdg_surface =
		last_surface ? wlr_xdg_surface_try_from_wlr_surface(last_surface)
					 : NULL;
	int32_t pass = 0;
	bool hit_global = false;
#ifdef XWAYLAND
	struct wlr_xwayland_surface *xsurface =
		last_surface ? wlr_xwayland_surface_try_from_wlr_surface(last_surface)
					 : NULL;
#endif

	/* Translate libinput keycode -> xkbcommon */
	uint32_t keycode = event->keycode + 8;
	/* Get a list of keysyms based on the keymap for this keyboard */
	const xkb_keysym_t *syms;
	int32_t nsyms = xkb_state_key_get_syms(group->wlr_group->keyboard.xkb_state,
										   keycode, &syms);

	int32_t handled = 0;
	uint32_t mods = wlr_keyboard_get_modifiers(&group->wlr_group->keyboard);

	wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

	// ov tab mode detect moe key release
	if (config.ov_tab_mode && !locked && group == kb_group &&
		event->state == WL_KEYBOARD_KEY_STATE_RELEASED &&
		(keycode == 133 || keycode == 37 || keycode == 64 || keycode == 50 ||
		 keycode == 134 || keycode == 105 || keycode == 108 || keycode == 62) &&
		selmon && selmon->sel) {
		if (selmon->isoverview && selmon->sel) {
			toggleoverview(&(Arg){.i = 1});
		}
	}

	/* On _press_ if there is no active screen locker,
	 * attempt to process a compositor keybinding. */
	for (i = 0; i < nsyms; i++)
		handled =
			keybinding(event->state, locked, mods, syms[i], keycode) || handled;

	if (event->state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		tag_combo = false;
	}

	if (handled && group->wlr_group->keyboard.repeat_info.delay > 0) {
		group->mods = mods;
		group->keysyms = syms;
		group->keycode = keycode;
		group->nsyms = nsyms;
		wl_event_source_timer_update(
			group->key_repeat_source,
			group->wlr_group->keyboard.repeat_info.delay);
	} else {
		group->nsyms = 0;
		wl_event_source_timer_update(group->key_repeat_source, 0);
	}

	if (handled)
		return;

	/* don't pass when popup is focused
	 * this is better than having popups (like fuzzel or wmenu) closing
	 * while typing in a passed keybind */
	pass = (xdg_surface && xdg_surface->role != WLR_XDG_SURFACE_ROLE_POPUP) ||
		   !last_surface
#ifdef XWAYLAND
		   || xsurface
#endif
		;
	/* passed keys don't get repeated */
	if (pass && syms)
		hit_global = keypressglobal(last_surface, &group->wlr_group->keyboard,
									event, mods, syms[0], keycode);

	if (hit_global) {
		return;
	}
	if (!aether_im_keyboard_grab_forward_key(group, event)) {
		wlr_seat_set_keyboard(seat, &group->wlr_group->keyboard);
		/* Pass unhandled keycodes along to the client. */
		wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode,
									 event->state);
	}
}

void keypressmod(struct wl_listener *listener, void *data) {
	/* This event is raised when a modifier key, such as shift or alt, is
	 * pressed. We simply communicate this to the client. */
	KeyboardGroup *group = wl_container_of(listener, group, modifiers);
	static xkb_layout_index_t last_group = (xkb_layout_index_t)-1;

	if (!aether_im_keyboard_grab_forward_modifiers(group)) {

		wlr_seat_set_keyboard(seat, &group->wlr_group->keyboard);
		/* Send modifiers to the client. */
		wlr_seat_keyboard_notify_modifiers(
			seat, &group->wlr_group->keyboard.modifiers);
	}

	if (last_group != group->wlr_group->keyboard.modifiers.group) {
		last_group = group->wlr_group->keyboard.modifiers.group;
		printstatus();
	}

	/* Commit the switcher if its modifier was released */
	switcherupdatemods(
		wlr_keyboard_get_modifiers(&group->wlr_group->keyboard));
}

void pending_kill_client(Client *c) {
	if (!c || c->iskilling)
		return;
	client_send_close(c);
}

void locksession(struct wl_listener *listener, void *data) {
	struct wlr_session_lock_v1 *session_lock = data;
	SessionLock *lock;
	if (!config.allow_lock_transparent) {
		wlr_scene_node_set_enabled(&locked_bg->node, true);
	}
	if (cur_lock) {
		wlr_session_lock_v1_destroy(session_lock);
		return;
	}
	lock = session_lock->data = ecalloc(1, sizeof(*lock));
	focusclient(NULL, 0);

	lock->scene = wlr_scene_tree_create(layers[LyrBlock]);
	cur_lock = lock->lock = session_lock;
	locked = 1;

	LISTEN(&session_lock->events.new_surface, &lock->new_surface,
		   createlocksurface);
	LISTEN(&session_lock->events.destroy, &lock->destroy, destroysessionlock);
	LISTEN(&session_lock->events.unlock, &lock->unlock, unlocksession);

	wlr_session_lock_v1_send_locked(session_lock);
}

static void iter_xdg_scene_buffers(struct wlr_scene_buffer *buffer, int32_t sx,
								   int32_t sy, void *user_data) {
	Client *c = user_data;

	struct wlr_scene_surface *scene_surface =
		wlr_scene_surface_try_from_buffer(buffer);
	if (!scene_surface) {
		return;
	}

	struct wlr_surface *surface = scene_surface->surface;
	/* we dont blur subsurfaces */
	if (wlr_subsurface_try_from_wlr_surface(surface) != NULL)
		return;

	if (config.blur && c && !c->noblur) {
		wlr_scene_buffer_set_backdrop_blur(buffer, true);
		wlr_scene_buffer_set_backdrop_blur_ignore_transparent(buffer, false);
		if (config.blur_optimized) {
			wlr_scene_buffer_set_backdrop_blur_optimized(buffer, true);
		} else {
			wlr_scene_buffer_set_backdrop_blur_optimized(buffer, false);
		}
	} else {
		wlr_scene_buffer_set_backdrop_blur(buffer, false);
	}
}

void init_client_properties(Client *c) {
	c->drop_direction = UNDIR;
	c->enable_drop_area_draw = false;
	c->isfocusing = false;
	c->isfloating = 0;
	c->isfakefullscreen = 0;
	c->isnoanimation = 0;
	c->isopensilent = 0;
	c->istagsilent = 0;
	c->noswallow = 0;
	c->isterm = 0;
	c->noblur = 0;
	c->tearing_hint = 0;
	c->overview_isfullscreenbak = 0;
	c->overview_ismaximizescreenbak = 0;
	c->overview_isfloatingbak = 0;
	c->pid = 0;
	c->swallowing = NULL;
	c->swallowedby = NULL;
	c->ismaster = 0;
	c->old_ismaster = 0;
	c->isleftstack = 0;
	c->ismaximizescreen = 0;
	c->isfullscreen = 0;
	c->need_float_size_reduce = 0;
	c->iskilling = 0;
	c->istagswitching = 0;
	c->isglobal = 0;
	c->isminimized = 0;
	c->isoverlay = 0;
	c->isunglobal = 0;
	c->is_in_scratchpad = 0;
	c->isnamedscratchpad = 0;
	c->is_scratchpad_show = 0;
	c->need_float_size_reduce = 0;
	c->is_clip_to_hide = 0;
	c->is_restoring_from_ov = 0;
	c->isurgent = 0;
	c->need_output_flush = 0;
	c->scroller_proportion = config.scroller_default_proportion;
	c->is_pending_open_animation = true;
	c->drag_to_tile = false;
	c->scratchpad_switching_mon = false;
	c->fake_no_border = false;
	c->focused_opacity = config.focused_opacity;
	c->unfocused_opacity = config.unfocused_opacity;
	c->nofocus = 0;
	c->nofadein = 0;
	c->nofadeout = 0;
	c->no_force_center = 0;
	c->isnoborder = 0;
	c->isnosizehint = 0;
	c->isnoradius = 0;
	c->isnoshadow = 0;
	c->ignore_maximize = 0;
	c->ignore_minimize = 0;
	c->iscustomsize = 0;
	c->iscustompos = 0;
	c->master_mfact_per = 0.0f;
	c->master_inner_per = 0.0f;
	c->stack_inner_per = 0.0f;
	c->old_stack_inner_per = 0.0f;
	c->old_master_inner_per = 0.0f;
	c->old_master_mfact_per = 0.0f;
	c->isterm = 0;
	c->allow_csd = 0;
	c->force_fakemaximize = 0;
	c->force_tiled_state = 1;
	c->force_tearing = 0;
	c->allow_shortcuts_inhibit = SHORTCUTS_INHIBIT_ENABLE;
	c->indleinhibit_when_focus = 0;
	c->scroller_proportion_single = 0.0f;
	c->float_geom.width = 0;
	c->float_geom.height = 0;
	c->float_geom.x = 0;
	c->float_geom.y = 0;
	c->stack_proportion = 0.0f;
	memset(c->oldmonname, 0, sizeof(c->oldmonname));
	memcpy(c->opacity_animation.initial_border_color, config.bordercolor,
		   sizeof(c->opacity_animation.initial_border_color));
	memcpy(c->opacity_animation.current_border_color, config.bordercolor,
		   sizeof(c->opacity_animation.current_border_color));
	c->opacity_animation.initial_opacity = c->unfocused_opacity;
	c->opacity_animation.current_opacity = c->unfocused_opacity;
}

void // old fix to 0.5
mapnotify(struct wl_listener *listener, void *data) {
	/* Called when the surface is mapped, or ready to display on-screen. */
	Client *at_client = NULL;
	Client *c = wl_container_of(listener, c, map);
	/* Create scene tree for this client and its border */
	c->scene = client_surface(c)->data = wlr_scene_tree_create(layers[LyrTile]);
	wlr_scene_node_set_enabled(&c->scene->node, c->type != XDGShell);
	c->scene_surface =
		c->type == XDGShell
			? wlr_scene_xdg_surface_create(c->scene, c->surface.xdg)
			: wlr_scene_subsurface_tree_create(c->scene, client_surface(c));
	c->scene->node.data = c->scene_surface->node.data = c;

	client_get_geometry(c, &c->geom);

	if (client_is_x11(c))
		init_client_properties(c);

	// set special window properties
	if (client_is_unmanaged(c) || client_is_x11_popup(c)) {
		c->bw = 0;
		c->isnoborder = 1;
	} else {
		c->bw = config.borderpx;
	}

	if (client_should_global(c)) {
		c->isunglobal = 1;
	}

	// init client geom
	c->geom.width += 2 * c->bw;
	c->geom.height += 2 * c->bw;

	/* Handle unmanaged clients first so we can return prior create borders
	 */
	if (client_is_unmanaged(c)) {
		/* Unmanaged clients always are floating */
#ifdef XWAYLAND
		if (client_is_x11(c)) {
			fix_xwayland_unmanaged_coordinate(c);
			LISTEN(&c->surface.xwayland->events.set_geometry, &c->set_geometry,
				   setgeometrynotify);
		}
#endif
		wlr_scene_node_reparent(&c->scene->node, layers[LyrOverlay]);
		wlr_scene_node_set_position(&c->scene->node, c->geom.x, c->geom.y);
		if (client_wants_focus(c)) {
			focusclient(c, 1);
			exclusive_focus = c;
		}
		return;
	}

	// extra node

	c->droparea = wlr_scene_rect_create(c->scene, 0, 0, config.dropcolor);
	wlr_scene_node_lower_to_bottom(&c->droparea->node);
	wlr_scene_node_set_position(&c->droparea->node, 0, 0);
	wlr_scene_node_set_enabled(&c->droparea->node, false);

	c->border = wlr_scene_rect_create(
		c->scene, 0, 0, c->isurgent ? config.urgentcolor : config.bordercolor);
	wlr_scene_node_lower_to_bottom(&c->border->node);
	wlr_scene_node_set_position(&c->border->node, 0, 0);
	wlr_scene_rect_set_corner_radius(c->border, config.border_radius,
									 config.border_radius_location_default);
	wlr_scene_node_set_enabled(&c->border->node, true);

	c->shadow =
		wlr_scene_shadow_create(c->scene, 0, 0, config.border_radius,
								config.shadows_blur, config.shadowscolor);

	wlr_scene_node_lower_to_bottom(&c->shadow->node);
	wlr_scene_node_set_enabled(&c->shadow->node, true);

	if (config.new_is_master && selmon && !is_scroller_layout(selmon))
		// tile at the top
		wl_list_insert(&clients, &c->link); // 新窗口是master,头部入栈
	else if (selmon && is_scroller_layout(selmon) &&
			 selmon->visible_scroll_tiling_clients > 0) {

		if (selmon->sel && ISSCROLLTILED(selmon->sel) &&
			VISIBLEON(selmon->sel, selmon)) {
			at_client = scroll_get_stack_tail_client(selmon->sel);
		} else {
			at_client = center_tiled_select(selmon);
		}

		if (at_client) {
			wl_list_insert(&at_client->link, &c->link);
		} else {
			wl_list_insert(clients.prev, &c->link); // 尾部入栈
		}
	} else
		wl_list_insert(clients.prev, &c->link); // 尾部入栈

	wl_list_insert(&fstack, &c->flink);

	applyrules(c);

	if (!c->isfloating || c->force_tiled_state) {
		client_set_tiled(c, WLR_EDGE_TOP | WLR_EDGE_BOTTOM | WLR_EDGE_LEFT |
								WLR_EDGE_RIGHT);
	}

	// apply buffer effects of client
	wlr_scene_node_for_each_buffer(&c->scene_surface->node,
								   iter_xdg_scene_buffers, c);

	// set border color
	setborder_color(c);

	// make sure the animation is open type
	c->is_pending_open_animation = true;
	resize(c, c->geom, 0);
	printstatus();
}

void requestmovenotify(struct wl_listener *listener, void *data) {
	Client *c = wl_container_of(listener, c, request_move);
	if (!c || !c->isfloating || client_is_unmanaged(c) || c->iskilling || c->isfullscreen)
		return;

	grabc = c;
	cursor_mode = CurMove;
	grabcx = cursor->x - grabc->geom.x;
	grabcy = cursor->y - grabc->geom.y;
	wlr_cursor_set_xcursor(cursor, cursor_mgr, "grab");
}

void requestresizenotify(struct wl_listener *listener, void *data) {
	Client *c = wl_container_of(listener, c, request_resize);
	if (!c || !c->isfloating || client_is_unmanaged(c) || c->iskilling || c->isfullscreen)
		return;

	grabc = c;
	cursor_mode = CurResize;

	/* Read the requested edges from the XDG event */
	struct wlr_xdg_toplevel_resize_event *event = data;
	resize_edges = event ? event->edges : (WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT);

	/* Convert WLR_EDGE_* to rzcorner bitmask:
	 *   bit 0 (1) = right side  (otherwise left)
	 *   bit 1 (2) = bottom side (otherwise top)
	 * For edge-only drags, we use the closest corner on the perpendicular axis. */
	bool has_right  = !!(resize_edges & WLR_EDGE_RIGHT);
	bool has_bottom = !!(resize_edges & WLR_EDGE_BOTTOM);
	bool has_left   = !!(resize_edges & WLR_EDGE_LEFT);
	bool has_top    = !!(resize_edges & WLR_EDGE_TOP);

	grabcx = (int)round(cursor->x);
	grabcy = (int)round(cursor->y);

	if (has_right)
		rzcorner = 1;
	else if (has_left)
		rzcorner = 0;
	else
		/* horizontal edge only: pick closest X side */
		rzcorner = (grabcx - grabc->geom.x >=
					grabc->geom.x + grabc->geom.width - grabcx) ? 1 : 0;

	if (has_bottom)
		rzcorner |= 2;
	else if (has_top)
		rzcorner |= 0; /* top => bit1 stays 0 */
	else
		/* vertical edge only: pick closest Y side */
		rzcorner |= (grabcy - grabc->geom.y >=
					 grabc->geom.y + grabc->geom.height - grabcy) ? 2 : 0;

	/* Cursor shape based on which edges are active */
	const char *cursor_name;
	if ((has_top || has_bottom) && (has_left || has_right)) {
		/* Corner resize */
		const char *corners[4] = {"nw-resize", "ne-resize", "sw-resize", "se-resize"};
		cursor_name = corners[rzcorner];
	} else if (has_left || has_right) {
		cursor_name = "ew-resize";
	} else if (has_top || has_bottom) {
		cursor_name = "ns-resize";
	} else {
		cursor_name = "se-resize"; /* fallback */
	}

	if (config.drag_warp_cursor) {
		grabcx = rzcorner & 1 ? grabc->geom.x + grabc->geom.width
							  : grabc->geom.x;
		grabcy = rzcorner & 2 ? grabc->geom.y + grabc->geom.height
							  : grabc->geom.y;
		wlr_cursor_warp_closest(cursor, NULL, grabcx, grabcy);
	}
	wlr_cursor_set_xcursor(cursor, cursor_mgr, cursor_name);
}

void maximizenotify(struct wl_listener *listener, void *data) {

	Client *c = wl_container_of(listener, c, maximize);

	if (!c || !c->mon || c->iskilling || c->ignore_maximize)
		return;

	if (!client_is_x11(c) && !c->surface.xdg->initialized) {
		return;
	}

	if (client_request_maximize(c, data)) {
		setmaximizescreen(c, 1);
	} else {
		setmaximizescreen(c, 0);
	}
}

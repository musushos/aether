
/* --- Edge Snap Implementation --- */

static EdgeSnapZone detect_edge_snap_zone(Monitor *m, double cx, double cy) {
	if (!m) return EDGE_SNAP_NONE;
	
	int32_t threshold = config.edge_snap_threshold;
	int32_t corner_size = config.edge_snap_corner_size;
	
	bool near_left = cx <= m->m.x + threshold;
	bool near_right = cx >= m->m.x + m->m.width - threshold;
	bool near_top = cy <= m->m.y + threshold;
	
	bool near_top_corner = cy <= m->m.y + corner_size;
	bool near_bottom_corner = cy >= m->m.y + m->m.height - corner_size;
	bool near_left_corner = cx <= m->m.x + corner_size;
	bool near_right_corner = cx >= m->m.x + m->m.width - corner_size;

	if (near_left && near_top_corner) return EDGE_SNAP_TOP_LEFT;
	if (near_left && near_bottom_corner) return EDGE_SNAP_BOTTOM_LEFT;
	if (near_right && near_top_corner) return EDGE_SNAP_TOP_RIGHT;
	if (near_right && near_bottom_corner) return EDGE_SNAP_BOTTOM_RIGHT;
	
	if (near_top && near_left_corner) return EDGE_SNAP_TOP_LEFT;
	if (near_top && near_right_corner) return EDGE_SNAP_TOP_RIGHT;

	if (near_left) return EDGE_SNAP_LEFT;
	if (near_right) return EDGE_SNAP_RIGHT;
	if (near_top) return EDGE_SNAP_TOP;
	
	return EDGE_SNAP_NONE;
}

static struct wlr_box get_edge_snap_geometry(EdgeSnapZone zone, Monitor *m) {
	struct wlr_box box = m->w; // Use usable area (excluding panels)

	/* Apply outer gaps so snapped windows respect gap_outer_top/bottom and gap_outer_left/right */
	int32_t got  = enablegaps ? m->gap_outer_top    : 0;
	int32_t gob  = enablegaps ? m->gap_outer_bottom : 0;
	int32_t gol  = enablegaps ? m->gap_outer_left   : 0;
	int32_t gor  = enablegaps ? m->gap_outer_right  : 0;
	box.x      += gol;
	box.width  -= gol + gor;
	box.y      += got;
	box.height -= got + gob;

	int32_t half_w = box.width / 2;
	int32_t half_h = box.height / 2;

	switch (zone) {
	case EDGE_SNAP_LEFT:
		box.width = half_w;
		break;
	case EDGE_SNAP_RIGHT:
		box.x += box.width - half_w; // Adjust precisely
		box.width = half_w;
		break;
	case EDGE_SNAP_TOP:
		// Full maximize box area (gaps already applied above)
		break;
	case EDGE_SNAP_TOP_LEFT:
		box.width = half_w;
		box.height = half_h;
		break;
	case EDGE_SNAP_TOP_RIGHT:
		box.x += box.width - half_w;
		box.width = half_w;
		box.height = half_h;
		break;
	case EDGE_SNAP_BOTTOM_LEFT:
		box.width = half_w;
		box.y += box.height - half_h;
		box.height = half_h;
		break;
	case EDGE_SNAP_BOTTOM_RIGHT:
		box.x += box.width - half_w;
		box.y += box.height - half_h;
		box.width = half_w;
		box.height = half_h;
		break;
	default:
		break;
	}
	return box;
}

static void update_edge_snap_preview(Client *c) {
	if (!c || !c->mon || !config.enable_edge_snap || !c->isfloating) {
		hide_edge_snap_preview();
		return;
	}
	
	EdgeSnapZone new_zone = detect_edge_snap_zone(c->mon, cursor->x, cursor->y);
	
	if (new_zone == current_edge_snap_zone) return; // No change
	
	current_edge_snap_zone = new_zone;
	
	if (new_zone == EDGE_SNAP_NONE) {
		hide_edge_snap_preview();
		return;
	}
	
	struct wlr_box geom = get_edge_snap_geometry(new_zone, c->mon);
	
	if (!edge_snap_preview_rect) {
		// Create a floating scene rect for the preview
		edge_snap_preview_rect = wlr_scene_rect_create(layers[LyrFloat], 0, 0, config.edge_snap_preview_color);
		if (!edge_snap_preview_rect) return;
	} else {
		wlr_scene_rect_set_color(edge_snap_preview_rect, config.edge_snap_preview_color);
	}
	
	wlr_scene_rect_set_size(edge_snap_preview_rect, geom.width, geom.height);
	wlr_scene_node_set_position(&edge_snap_preview_rect->node, geom.x, geom.y);
	wlr_scene_node_set_enabled(&edge_snap_preview_rect->node, true);
	wlr_scene_node_raise_to_top(&edge_snap_preview_rect->node);
}

static void hide_edge_snap_preview(void) {
	current_edge_snap_zone = EDGE_SNAP_NONE;
	if (edge_snap_preview_rect) {
		wlr_scene_node_set_enabled(&edge_snap_preview_rect->node, false);
	}
}

static void apply_edge_snap(Client *c) {
	if (!c || !c->mon || current_edge_snap_zone == EDGE_SNAP_NONE || !config.enable_edge_snap || !c->isfloating) return;
	
	if (current_edge_snap_zone == EDGE_SNAP_TOP) {
		setmaximizescreen(c, 1);
	} else {
		struct wlr_box geom = get_edge_snap_geometry(current_edge_snap_zone, c->mon);
		// If maximizing was set, remove it before tiling
		if (c->isfullscreen) {
			setfullscreen(c, 0);
		}
		c->float_geom = geom;
		c->geom = geom;
		resize(c, geom, 0);
	}
	hide_edge_snap_preview();
}


void focuslayer(LayerSurface *l) {
	focusclient(NULL, 0);
	aether_im_relay_set_focus(aether_input_method_relay, l->layer_surface->surface);
	client_notify_enter(l->layer_surface->surface, wlr_seat_get_keyboard(seat));
}

void reset_exclusive_layers_focus(Monitor *m) {
	LayerSurface *l = NULL;
	int32_t i;
	bool neet_change_focus_to_client = false;
	uint32_t layers_above_shell[] = {
		ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
		ZWLR_LAYER_SHELL_V1_LAYER_TOP,
		ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
	};

	if (!m)
		return;

	for (i = 0; i < (int32_t)LENGTH(layers_above_shell); i++) {
		wl_list_for_each(l, &m->layers[layers_above_shell[i]], link) {
			if (l == exclusive_focus &&
				l->layer_surface->current.keyboard_interactive !=
					ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE) {

				exclusive_focus = NULL;

				neet_change_focus_to_client = true;
			}

			if (l->layer_surface->surface ==
					seat->keyboard_state.focused_surface &&
				l->being_unmapped) {
				neet_change_focus_to_client = true;
			}

			if (l->layer_surface->current.keyboard_interactive ==
					ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE &&
				l->layer_surface->surface ==
					seat->keyboard_state.focused_surface) {
				neet_change_focus_to_client = true;
			}

			if (locked ||
				l->layer_surface->current.keyboard_interactive !=
					ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE ||
				l->being_unmapped)
				continue;
			/* Deactivate the focused client. */
			exclusive_focus = l;
			neet_change_focus_to_client = false;
			if (l->layer_surface->surface !=
				seat->keyboard_state.focused_surface)
				focuslayer(l);
			return;
		}
	}

	if (neet_change_focus_to_client) {
		focusclient(focustop(selmon), 1);
	}
}

void arrangelayers(Monitor *m) {
	int32_t i;
	struct wlr_box usable_area = m->m;

	if (!m->wlr_output->enabled)
		return;

	if (m->iscleanuping)
		return;

	/* Arrange exclusive surfaces from top->bottom */
	for (i = 3; i >= 0; i--)
		arrangelayer(m, &m->layers[i], &usable_area, 1);

	if (!wlr_box_equal(&usable_area, &m->w)) {
		m->w = usable_area;
		arrange(m, false, false);
	}

	/* Arrange non-exlusive surfaces from top->bottom */
	for (i = 3; i >= 0; i--)
		arrangelayer(m, &m->layers[i], &usable_area, 0);
}

bool pointer_is_trackpad(struct wlr_pointer *pointer) {
	struct libinput_device *device;

	if (wlr_input_device_is_libinput(&pointer->base) &&
		(device = wlr_libinput_get_device_handle(&pointer->base))) {
		if (libinput_device_config_tap_get_finger_count(device) > 0) {
			return true;
		}
	}

	return false;
}

void //Mouse wheel event
axisnotify(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits an axis event,
	 * for example when you move the scroll wheel. */
	struct wlr_pointer_axis_event *event = data;
	struct wlr_keyboard *keyboard, *hard_keyboard;
	uint32_t mods, hard_mods;
	AxisBinding *a;
	int32_t ji;
	uint32_t adir;
	double target_scroll_factor;
	// IDLE_NOTIFY_ACTIVITY;
	handlecursoractivity();
	wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

	if (check_trackpad_disabled(event->pointer)) {
		return;
	}

	hard_keyboard = &kb_group->wlr_group->keyboard;
	hard_mods = hard_keyboard ? wlr_keyboard_get_modifiers(hard_keyboard) : 0;

	keyboard = wlr_seat_get_keyboard(seat);
	mods = keyboard ? wlr_keyboard_get_modifiers(keyboard) : 0;

	mods = mods | hard_mods;

	if (event->orientation == WL_POINTER_AXIS_VERTICAL_SCROLL)
		adir = event->delta > 0 ? AxisDown : AxisUp;
	else
		adir = event->delta > 0 ? AxisRight : AxisLeft;

	for (ji = 0; ji < config.axis_bindings_count; ji++) {
		if (config.axis_bindings_count < 1)
			break;
		a = &config.axis_bindings[ji];
		if (CLEANMASK(mods) == CLEANMASK(a->mod) && //The keys are consistent
			adir == a->dir && a->func) { // The wheel direction judgment is consistent and the processing function exists
			if (event->time_msec - axis_apply_time >
					config.axis_bind_apply_timeout ||
				axis_apply_dir * event->delta < 0) {
				a->func(&a->arg);
				axis_apply_time = event->time_msec;
				axis_apply_dir = event->delta > 0 ? 1 : -1;
				return; // If the match is successful, the wheel event will not be sent to the client.
			} else {
				axis_apply_dir = event->delta > 0 ? 1 : -1;
				axis_apply_time = event->time_msec;
				return;
			}
		}
	}

	/* TODO: allow usage of scroll whell for mousebindings, it can be
	 * implemented checking the event's orientation and the delta of the event
	 */
	/* Notify the client with pointer focus of the axis event. */

	target_scroll_factor = pointer_is_trackpad(event->pointer)
							   ? config.trackpad_scroll_factor
							   : config.axis_scroll_factor;

	wlr_seat_pointer_notify_axis(
		seat, //The scroll wheel event is sent to the client, which is the window
		event->time_msec, event->orientation,
		event->delta * target_scroll_factor,
		roundf(event->delta_discrete * target_scroll_factor), event->source,
		event->relative_direction);
}

int32_t ongesture(struct wlr_pointer_swipe_end_event *event) {
	struct wlr_keyboard *keyboard, *hard_keyboard;
	uint32_t mods, hard_mods;
	const GestureBinding *g;
	uint32_t motion;
	uint32_t adx = (int32_t)round(fabs(swipe_dx));
	uint32_t ady = (int32_t)round(fabs(swipe_dy));
	int32_t handled = 0;
	int32_t ji;

	if (event->cancelled) {
		return handled;
	}

	// Require absolute distance movement beyond a small thresh-hold
	if (adx * adx + ady * ady <
		config.swipe_min_threshold * config.swipe_min_threshold) {
		return handled;
	}

	if (adx > ady) {
		motion = swipe_dx < 0 ? SWIPE_LEFT : SWIPE_RIGHT;
	} else {
		motion = swipe_dy < 0 ? SWIPE_UP : SWIPE_DOWN;
	}

	hard_keyboard = &kb_group->wlr_group->keyboard;
	hard_mods = hard_keyboard ? wlr_keyboard_get_modifiers(hard_keyboard) : 0;

	keyboard = wlr_seat_get_keyboard(seat);
	mods = keyboard ? wlr_keyboard_get_modifiers(keyboard) : 0;

	mods = mods | hard_mods;

	for (ji = 0; ji < config.gesture_bindings_count; ji++) {
		if (config.gesture_bindings_count < 1)
			break;
		g = &config.gesture_bindings[ji];
		if (CLEANMASK(mods) == CLEANMASK(g->mod) &&
			swipe_fingers == g->fingers_count && motion == g->motion &&
			g->func) {
			g->func(&g->arg);
			handled = 1;
		}
	}
	return handled;
}

void swipe_begin(struct wl_listener *listener, void *data) {
	struct wlr_pointer_swipe_begin_event *event = data;

	// Forward swipe begin event to client
	wlr_pointer_gestures_v1_send_swipe_begin(pointer_gestures, seat,
											 event->time_msec, event->fingers);
}

void swipe_update(struct wl_listener *listener, void *data) {
	struct wlr_pointer_swipe_update_event *event = data;

	swipe_fingers = event->fingers;
	// Accumulate swipe distance
	swipe_dx += event->dx;
	swipe_dy += event->dy;

	// Forward swipe update event to client
	wlr_pointer_gestures_v1_send_swipe_update(
		pointer_gestures, seat, event->time_msec, event->dx, event->dy);
}

void swipe_end(struct wl_listener *listener, void *data) {
	struct wlr_pointer_swipe_end_event *event = data;
	ongesture(event);
	swipe_dx = 0;
	swipe_dy = 0;
	// Forward swipe end event to client
	wlr_pointer_gestures_v1_send_swipe_end(pointer_gestures, seat,
										   event->time_msec, event->cancelled);
}

void pinch_begin(struct wl_listener *listener, void *data) {
	struct wlr_pointer_pinch_begin_event *event = data;

	// Forward pinch begin event to client
	wlr_pointer_gestures_v1_send_pinch_begin(pointer_gestures, seat,
											 event->time_msec, event->fingers);
}

void pinch_update(struct wl_listener *listener, void *data) {
	struct wlr_pointer_pinch_update_event *event = data;

	// Forward pinch update event to client
	wlr_pointer_gestures_v1_send_pinch_update(
		pointer_gestures, seat, event->time_msec, event->dx, event->dy,
		event->scale, event->rotation);
}

void pinch_end(struct wl_listener *listener, void *data) {
	struct wlr_pointer_pinch_end_event *event = data;

	// Forward pinch end event to client
	wlr_pointer_gestures_v1_send_pinch_end(pointer_gestures, seat,
										   event->time_msec, event->cancelled);
}

void hold_begin(struct wl_listener *listener, void *data) {
	struct wlr_pointer_hold_begin_event *event = data;

	// Forward hold begin event to client
	wlr_pointer_gestures_v1_send_hold_begin(pointer_gestures, seat,
											event->time_msec, event->fingers);
}

void hold_end(struct wl_listener *listener, void *data) {
	struct wlr_pointer_hold_end_event *event = data;

	// Forward hold end event to client
	wlr_pointer_gestures_v1_send_hold_end(pointer_gestures, seat,
										  event->time_msec, event->cancelled);
}

Client *find_closest_tiled_client(Client *c) {
	Client *tc, *closest = NULL;
	long min_dist = LONG_MAX;

	wl_list_for_each(tc, &clients, link) {
		if (tc == c || !ISTILED(tc) || !VISIBLEON(tc, c->mon))
			continue;

		if (cursor->x >= tc->geom.x &&
			cursor->x < tc->geom.x + tc->geom.width &&
			cursor->y >= tc->geom.y &&
			cursor->y < tc->geom.y + tc->geom.height) {
			return tc;
		}

		int32_t dx = tc->geom.x + (int32_t)(tc->geom.width / 2) - cursor->x;
		int32_t dy = tc->geom.y + (int32_t)(tc->geom.height / 2) - cursor->y;
		long dist = (long)dx * dx + (long)dy * dy;

		if (dist < min_dist) {
			min_dist = dist;
			closest = tc;
		}
	}

	return closest;
}

void place_drag_tile_client(Client *c) {
	Client *closest = find_closest_tiled_client(c);

	if (closest && closest->mon) {
		const Layout *layout =
			closest->mon->pertag->ltidxs[closest->mon->pertag->curtag];

		if (closest->drop_direction == UNDIR) {
			setfloating(c, 0);
			exchange_two_client(c, closest);
			return;
		}

		if (layout->id == SCROLLER) {
			scroller_drop_tile(c, closest, 0);
			return;
		}
		if (layout->id == VERTICAL_SCROLLER) {
			scroller_drop_tile(c, closest, 1);
			return;
		}
		if (layout->id == DWINDLE) {
			uint32_t tag = c->mon->pertag->curtag;
			bool insert_before = (closest->drop_direction == LEFT ||
								  closest->drop_direction == UP);
			bool split_h = (closest->drop_direction == LEFT ||
							closest->drop_direction == RIGHT);
			dwindle_insert(&c->mon->pertag->dwindle_root[tag], c, closest,
						   config.dwindle_split_ratio, insert_before, split_h,
						   !config.dwindle_drop_simple_split);
			setfloating(c, 0);
			return;
		}

		if (closest->drop_direction == LEFT || closest->drop_direction == UP) {
			wl_list_remove(&c->link);
			wl_list_insert(closest->link.prev, &c->link);
		} else {
			wl_list_remove(&c->link);
			wl_list_insert(&closest->link, &c->link);
		}
	}

	setfloating(c, 0);
}

bool check_trackpad_disabled(struct wlr_pointer *pointer) {
	if (!config.disable_trackpad) {
		return false;
	}

	return pointer_is_trackpad(pointer);
}

void //Mouse button event
buttonpress(struct wl_listener *listener, void *data) {
	struct wlr_pointer_button_event *event = data;
	struct wlr_keyboard *hard_keyboard, *keyboard;
	uint32_t hard_mods, mods;
	Client *c = NULL;
	LayerSurface *l = NULL;
	struct wlr_surface *surface;
	Client *tmpc = NULL;
	int32_t ji;
	const MouseBinding *m;
	struct wlr_surface *old_pointer_focus_surface =
		seat->pointer_state.focused_surface;

	handlecursoractivity();
	wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

	if (check_trackpad_disabled(event->pointer)) {
		return;
	}

	switch (event->state) {
	case WL_POINTER_BUTTON_STATE_PRESSED:
		cursor_mode = CurPressed;
		selmon = xytomon(cursor->x, cursor->y);
		if (locked)
			break;

		xytonode(cursor->x, cursor->y, &surface, NULL, NULL, NULL, NULL);
		if (toplevel_from_wlr_surface(surface, &c, &l) >= 0) {
			if (c && c->scene->node.enabled &&
				(!client_is_unmanaged(c) || client_wants_focus(c)))
				focusclient(c, 1);

			if (surface != old_pointer_focus_surface) {
				wlr_seat_pointer_notify_clear_focus(seat);
				motionnotify(0, NULL, 0, 0, 0, 0);
			}

			// Focus on the layer that requires interactive focus, but be careful not to preempt the layer that has exclusive focus.
			if (l && !exclusive_focus &&
				l->layer_surface->current.keyboard_interactive ==
					ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND) {
				focuslayer(l);
			}
		}

		// When the mouse focus is on the layer, the mod status of the virtual keyboard is not detected.
		// Prevent the layer virtual keyboard from locking the mod key state
		hard_keyboard = &kb_group->wlr_group->keyboard;
		hard_mods =
			hard_keyboard ? wlr_keyboard_get_modifiers(hard_keyboard) : 0;

		keyboard = wlr_seat_get_keyboard(seat);
		mods = keyboard && !l ? wlr_keyboard_get_modifiers(keyboard) : 0;

		mods = mods | hard_mods;

		for (ji = 0; ji < config.mouse_bindings_count; ji++) {
			if (config.mouse_bindings_count < 1)
				break;
			m = &config.mouse_bindings[ji];

			if (selmon->isoverview && event->button == BTN_LEFT && c) {
				toggleoverview(&(Arg){.i = 1});
				return;
			}

			if (selmon->isoverview && event->button == BTN_RIGHT && c) {
				pending_kill_client(c);
				return;
			}

			if (CLEANMASK(mods) == CLEANMASK(m->mod) &&
				event->button == m->button && m->func &&
				(CLEANMASK(m->mod) != 0 ||
				 (event->button != BTN_LEFT && event->button != BTN_RIGHT))) {
				m->func(&m->arg);
				return;
			}
		}
		break;
	case WL_POINTER_BUTTON_STATE_RELEASED:
		/* If you released any buttons, we exit interactive move/resize mode. */
		if (!locked && cursor_mode != CurNormal && cursor_mode != CurPressed) {
			cursor_mode = CurNormal;
			/* Clear the pointer focus, this way if the cursor is over a surface
			 * we will send an enter event after which the client will provide
			 * us a cursor surface */
			wlr_seat_pointer_clear_focus(seat);
			motionnotify(0, NULL, 0, 0, 0, 0);
			/* Drop the window off on its new monitor */
			if (grabc == selmon->sel) {
				selmon->sel = NULL;
			}
			selmon = xytomon(cursor->x, cursor->y);
			client_update_oldmonname_record(grabc, selmon);
			setmon(grabc, selmon, 0, true);
			selmon->prevsel = ISTILED(selmon->sel) ? selmon->sel : NULL;
			selmon->sel = grabc;
			tmpc = grabc;
			grabc = NULL;
			start_drag_window = false;
			last_apply_drap_time = 0;
			if (tmpc->drag_to_tile && config.drag_tile_to_tile) {
				place_drag_tile_client(tmpc);
				tmpc->float_geom = tmpc->drag_tile_float_backup_geom;
			} else {
				if (current_edge_snap_zone != EDGE_SNAP_NONE) {
					apply_edge_snap(tmpc);
				} else {
					apply_window_snap(tmpc);
				}
			}
			hide_edge_snap_preview();
			tmpc->drag_to_tile = false;
			if (dropc) {
				dropc->enable_drop_area_draw = false;
				client_set_drop_area(dropc);
				dropc = NULL;
			}
			return;
		} else {
			cursor_mode = CurNormal;
		}
		break;
	}
	/* If the event wasn't handled by the compositor, notify the client with
	 * pointer focus that a button press has occurred */
	wlr_seat_pointer_notify_button(seat, event->time_msec, event->button,
								   event->state);
}

void checkidleinhibitor(struct wlr_surface *exclude) {
	int32_t inhibited = 0;
	Client *c = NULL;
	struct wlr_surface *surface = NULL;
	struct wlr_idle_inhibitor_v1 *inhibitor;

	wl_list_for_each(inhibitor, &idle_inhibit_mgr->inhibitors, link) {
		surface = wlr_surface_get_root_surface(inhibitor->surface);

		if (exclude == surface) {
			continue;
		}

		toplevel_from_wlr_surface(inhibitor->surface, &c, NULL);

		if (config.idleinhibit_ignore_visible) {
			inhibited = 1;
			break;
		}

		struct wlr_scene_tree *tree = surface->data;
		if (!tree || (tree->node.enabled && (!c || !c->animation.tagouting))) {
			inhibited = 1;
			break;
		}
	}

	wlr_idle_notifier_v1_set_inhibited(idle_notifier, inhibited);
}

void last_cursor_surface_destroy(struct wl_listener *listener, void *data) {
	last_cursor.surface = NULL;
	wl_list_remove(&listener->link);
}

void setcursorshape(struct wl_listener *listener, void *data) {
	struct wlr_cursor_shape_manager_v1_request_set_shape_event *event = data;
	if (cursor_mode != CurNormal && cursor_mode != CurPressed)
		return;
	/* This can be sent by any client, so we check to make sure this one is
	 * actually has pointer focus first. If so, we can tell the cursor to
	 * use the provided cursor shape. */
	if (event->seat_client == seat->pointer_state.focused_client) {
		/* Remove surface destroy listener if active */
		if (last_cursor.surface &&
			last_cursor_surface_destroy_listener.link.prev != NULL)
			wl_list_remove(&last_cursor_surface_destroy_listener.link);

		last_cursor.shape = event->shape;
		last_cursor.surface = NULL;
		if (!cursor_hidden)
			wlr_cursor_set_xcursor(cursor, cursor_mgr,
								   wlr_cursor_shape_v1_name(event->shape));
	}
}

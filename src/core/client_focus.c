
void unminimize(Client *c) {
	if (c && c->is_in_scratchpad && c->is_scratchpad_show) {
		client_pending_minimized_state(c, 0);
		c->is_scratchpad_show = 0;
		c->is_in_scratchpad = 0;
		c->isnamedscratchpad = 0;
		setborder_color(c);
		return;
	}

	if (c && c->isminimized) {
		show_hide_client(c);
		c->is_scratchpad_show = 0;
		c->is_in_scratchpad = 0;
		c->isnamedscratchpad = 0;
		setborder_color(c);
		arrange(c->mon, false, false);
		return;
	}
}

void set_minimized(Client *c) {

	if (!c || !c->mon)
		return;

	c->isglobal = 0;
	c->oldtags = c->mon->tagset[c->mon->seltags];
	c->mini_restore_tag = c->tags;
	c->tags = 0;
	client_pending_minimized_state(c, 1);
	c->is_in_scratchpad = 1;
	c->is_scratchpad_show = 0;
	focusclient(focustop(selmon), 1);
	arrange(c->mon, false, false);

	if (c->foreign_toplevel)
		wlr_foreign_toplevel_handle_v1_set_activated(c->foreign_toplevel,
													 false);

	wl_list_remove(&c->link);				//Remove from original position
	wl_list_insert(clients.prev, &c->link); //Insert the tail
}

void minimizenotify(struct wl_listener *listener, void *data) {

	Client *c = wl_container_of(listener, c, minimize);

	if (!c || !c->mon || c->iskilling || c->isminimized)
		return;

	if (client_request_minimize(c, data) && !c->ignore_minimize) {
		if (!c->isminimized)
			set_minimized(c);
		client_set_minimized(c, true);
	} else {
		if (c->isminimized)
			unminimize(c);
		client_set_minimized(c, false);
	}
}

void motionabsolute(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits an
	 * _absolute_ motion event, from 0..1 on each axis. This happens, for
	 * example, when wlroots is running under a Wayland window rather than
	 * KMS+DRM, and you move the mouse over the window. You could enter the
	 * window from any edge, so we have to warp the mouse there. There is
	 * also some hardware which emits these events. */
	struct wlr_pointer_motion_absolute_event *event = data;
	double lx, ly, dx, dy;

	if (check_trackpad_disabled(event->pointer)) {
		return;
	}

	if (!event->time_msec) /* this is 0 with virtual pointer */
		wlr_cursor_warp_absolute(cursor, &event->pointer->base, event->x,
								 event->y);

	wlr_cursor_absolute_to_layout_coords(cursor, &event->pointer->base,
										 event->x, event->y, &lx, &ly);
	dx = lx - cursor->x;
	dy = ly - cursor->y;
	motionnotify(event->time_msec, &event->pointer->base, dx, dy, dx, dy);
}

void resize_floating_window(Client *grabc) {
	int cdx = (int)round(cursor->x) - grabcx;
	int cdy = (int)round(cursor->y) - grabcy;

	/* Determine which axes are active based on resize_edges.
	 * For a pure horizontal edge (left/right only), Y is locked.
	 * For a pure vertical edge (top/bottom only), X is locked.
	 * Corners and unspecified edges use both axes (rzcorner bits). */
	bool x_active = !!(resize_edges & (WLR_EDGE_LEFT | WLR_EDGE_RIGHT)) ||
					!(resize_edges & (WLR_EDGE_TOP | WLR_EDGE_BOTTOM));
	bool y_active = !!(resize_edges & (WLR_EDGE_TOP | WLR_EDGE_BOTTOM)) ||
					!(resize_edges & (WLR_EDGE_LEFT | WLR_EDGE_RIGHT));

	if (!x_active)
		cdx = 0;
	if (!y_active)
		cdy = 0;

	/* Prevent collapsing the window to zero size */
	if (x_active) {
		cdx = !(rzcorner & 1) && grabc->geom.width - 2 * (int)grabc->bw - cdx < 1
				  ? 0
				  : cdx;
	}
	if (y_active) {
		cdy = !(rzcorner & 2) && grabc->geom.height - 2 * (int)grabc->bw - cdy < 1
				  ? 0
				  : cdy;
	}

	const struct wlr_box box = {
		.x = grabc->geom.x + (rzcorner & 1 ? 0 : cdx),
		.y = grabc->geom.y + (rzcorner & 2 ? 0 : cdy),
		.width  = grabc->geom.width  + (x_active ? (rzcorner & 1 ? cdx : -cdx) : 0),
		.height = grabc->geom.height + (y_active ? (rzcorner & 2 ? cdy : -cdy) : 0)};

	grabc->float_geom = box;

	resize(grabc, box, 1);
	if (x_active)
		grabcx += cdx;
	if (y_active)
		grabcy += cdy;
}

void motionnotify(uint32_t time, struct wlr_input_device *device, double dx,
				  double dy, double dx_unaccel, double dy_unaccel) {
	double sx = 0, sy = 0, sx_confined, sy_confined;
	Client *c = NULL, *w = NULL;
	Client *closet_drop_client = NULL;
	LayerSurface *l = NULL;
	struct wlr_surface *surface = NULL;
	bool should_lock = false;

	/* time is 0 in internal calls meant to restore pointer focus. */
	if (time) {
		wlr_relative_pointer_manager_v1_send_relative_motion(
			relative_pointer_mgr, seat, (uint64_t)time * 1000, dx, dy,
			dx_unaccel, dy_unaccel);

		if (active_constraint && cursor_mode != CurResize &&
			cursor_mode != CurMove) {
			if (active_constraint->surface ==
				seat->pointer_state.focused_surface) {

				if (active_constraint->type == WLR_POINTER_CONSTRAINT_V1_LOCKED)
					return;

				toplevel_from_wlr_surface(active_constraint->surface, &c, NULL);
				if (c) {
					sx = cursor->x - c->geom.x - c->bw;
					sy = cursor->y - c->geom.y - c->bw;
					if (wlr_region_confine(&active_constraint->region, sx, sy,
										   sx + dx, sy + dy, &sx_confined,
										   &sy_confined)) {
						dx = sx_confined - sx;
						dy = sy_confined - sy;
					}
				}
			}
		}

		wlr_cursor_move(cursor, device, dx, dy);
		handlecursoractivity();
		wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

		/* Update selmon (even while dragging a window) */
		if (config.sloppyfocus)
			selmon = xytomon(cursor->x, cursor->y);
	}

	/* Find the client under the pointer and send the event along. */
	xytonode(cursor->x, cursor->y, &surface, &c, NULL, &sx, &sy);

	if (cursor_mode == CurPressed && !seat->drag &&
		surface != seat->pointer_state.focused_surface &&
		toplevel_from_wlr_surface(seat->pointer_state.focused_surface, &w,
								  &l) >= 0) {
		c = w;
		surface = seat->pointer_state.focused_surface;
		sx = cursor->x - (l ? l->scene->node.x : w->geom.x);
		sy = cursor->y - (l ? l->scene->node.y : w->geom.y);
	}

	/* Update drag icon's position */
	wlr_scene_node_set_position(&drag_icon->node, (int32_t)round(cursor->x),
								(int32_t)round(cursor->y));

	/* If we are currently grabbing the mouse, handle and return */
	if (cursor_mode == CurMove) {
		/* Move the grabbed client to the new position. */
		grabc->iscustomsize = 1;
		grabc->float_geom =
			(struct wlr_box){.x = (int32_t)round(cursor->x) - grabcx,
							 .y = (int32_t)round(cursor->y) - grabcy,
							 .width = grabc->geom.width,
							 .height = grabc->geom.height};
		if (config.drag_tile_to_tile && grabc->drag_to_tile) {
			closet_drop_client = find_closest_tiled_client(grabc);
			if (closet_drop_client && dropc && closet_drop_client != dropc) {
				dropc->enable_drop_area_draw = false;
				client_set_drop_area(dropc);
				dropc = closet_drop_client;
				dropc->enable_drop_area_draw = true;
				client_set_drop_area(dropc);
			} else if (closet_drop_client) {
				dropc = closet_drop_client;
				dropc->enable_drop_area_draw = true;
				client_set_drop_area(dropc);
			} else if (dropc) {
				dropc->enable_drop_area_draw = false;
				client_set_drop_area(dropc);
				dropc = NULL;
			}
		}
		if (config.enable_edge_snap && grabc->isfloating) {
			update_edge_snap_preview(grabc);
		}
		resize(grabc, grabc->float_geom, 1);
		return;
	} else if (cursor_mode == CurResize) {
		if (grabc->isfloating) {
			grabc->iscustomsize = 1;
			if (last_apply_drap_time == 0 ||
				time - last_apply_drap_time >
					config.drag_floating_refresh_interval) {
				resize_floating_window(grabc);
				last_apply_drap_time = time;
			}
			return;
		} else {
			resize_tile_client(grabc, true, 0, 0, time);
		}
	}

	/* If there's no client surface under the cursor, set the cursor image
	 * to a default. This is what makes the cursor image appear when you
	 * move it off of a client or over its border. */
	if (!surface && !seat->drag && !cursor_hidden)
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");

	if (c && c->mon && !c->animation.running && (INSIDEMON(c) || !ISTILED(c))) {
		scroller_focus_lock = 0;
	}

	should_lock = false;
	if (!scroller_focus_lock || !(c && c->mon && !INSIDEMON(c))) {
		if (c && c->mon && is_scroller_layout(c->mon) && !INSIDEMON(c)) {
			should_lock = true;
		}

		if (!(!config.edge_scroller_pointer_focus && c && c->mon &&
			  is_scroller_layout(c->mon) && !INSIDEMON(c)))
			pointerfocus(c, surface, sx, sy, time);

		if (should_lock && c && c->mon && ISTILED(c) && c == c->mon->sel) {
			scroller_focus_lock = 1;
		}
	}
}

void motionrelative(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits a
	 * _relative_ pointer motion event (i.e. a delta) */
	struct wlr_pointer_motion_event *event = data;
	/* The cursor doesn't move unless we tell it to. The cursor
	 * automatically handles constraining the motion to the output layout,
	 * as well as any special configuration applied for the specific input
	 * device which generated the event. You can pass NULL for the device if
	 * you want to move the cursor around without any input. */

	if (check_trackpad_disabled(event->pointer)) {
		return;
	}

	motionnotify(event->time_msec, &event->pointer->base, event->delta_x,
				 event->delta_y, event->unaccel_dx, event->unaccel_dy);
	toggle_hotarea(cursor->x, cursor->y);
}

void outputmgrapply(struct wl_listener *listener, void *data) {
	struct wlr_output_configuration_v1 *config = data;
	outputmgrapplyortest(config, 0);
}

void // 0.7 custom
outputmgrapplyortest(struct wlr_output_configuration_v1 *config, int32_t test) {
	/*
	 * Called when a client such as wlr-randr requests a change in output
	 * configuration. This is only one way that the layout can be changed,
	 * so any Monitor information should be updated by updatemons() after an
	 * output_layout.change event, not here.
	 */
	struct wlr_output_configuration_head_v1 *config_head;
	int32_t ok = 1;

	wl_list_for_each(config_head, &config->heads, link) {
		struct wlr_output *wlr_output = config_head->state.output;
		Monitor *m = wlr_output->data;
		struct wlr_output_state state;

		/* Ensure displays previously disabled by
		 * wlr-output-power-management-v1 are properly handled*/
		m->asleep = 0;

		wlr_output_state_init(&state);
		wlr_output_state_set_enabled(&state, config_head->state.enabled);
		if (!config_head->state.enabled)
			goto apply_or_test;

		if (config_head->state.mode)
			wlr_output_state_set_mode(&state, config_head->state.mode);
		else
			wlr_output_state_set_custom_mode(
				&state, config_head->state.custom_mode.width,
				config_head->state.custom_mode.height,
				config_head->state.custom_mode.refresh);

		wlr_output_state_set_transform(&state, config_head->state.transform);
		wlr_output_state_set_scale(&state, config_head->state.scale);
		wlr_output_state_set_adaptive_sync_enabled(
			&state, config_head->state.adaptive_sync_enabled);

	apply_or_test:
		ok &= test ? wlr_output_test_state(wlr_output, &state)
				   : wlr_output_commit_state(wlr_output, &state);

		/* Don't move monitors if position wouldn't change, this to avoid
		 * wlroots marking the output as manually configured.
		 * wlr_output_layout_add does not like disabled outputs */
		if (!test && wlr_output->enabled &&
			(m->m.x != config_head->state.x || m->m.y != config_head->state.y))
			wlr_output_layout_add(output_layout, wlr_output,
								  config_head->state.x, config_head->state.y);

		wlr_output_state_finish(&state);
	}

	if (ok)
		wlr_output_configuration_v1_send_succeeded(config);
	else
		wlr_output_configuration_v1_send_failed(config);
	wlr_output_configuration_v1_destroy(config);

	/* https://codeberg.org/aether/aether/issues/577 */
	updatemons(NULL, NULL);
}

void outputmgrtest(struct wl_listener *listener, void *data) {
	struct wlr_output_configuration_v1 *config = data;
	outputmgrapplyortest(config, 1);
}

void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
				  uint32_t time) {
	struct timespec now;

	if (config.sloppyfocus && !start_drag_window && c && time && c->scene &&
		c->scene->node.enabled && !c->animation.tagining &&
		(surface != seat->pointer_state.focused_surface) &&
		!client_is_unmanaged(c) && VISIBLEON(c, c->mon))
		focusclient(c, 0);

	/* If surface is NULL, clear pointer focus */
	if (!surface) {
		wlr_seat_pointer_notify_clear_focus(seat);
		return;
	}

	if (!time) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		time = now.tv_sec * 1000 + now.tv_nsec / 1000000;
	}

	/* Let the client know that the mouse cursor has entered one
	 * of its surfaces, and make keyboard focus follow if desired.
	 * wlroots makes this a no-op if surface is already focused */
	wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
	wlr_seat_pointer_notify_motion(seat, time, sx, sy);
}

// Modify the printstatus function to accept mask parameters
void printstatus(void) { wl_signal_emit(&aether_print_status, NULL); }

void powermgrsetmode(struct wl_listener *listener, void *data) {
	struct wlr_output_power_v1_set_mode_event *event = data;
	struct wlr_output_state state = {0};
	Monitor *m = event->output->data;

	if (!m)
		return;

	wlr_output_state_set_enabled(&state, event->mode);
	wlr_output_commit_state(m->wlr_output, &state);

	m->asleep = !event->mode;
	updatemons(NULL, NULL);
}

void quitsignal(int32_t signo) { quit(NULL); }

void scene_buffer_apply_opacity(struct wlr_scene_buffer *buffer, int32_t sx,
								int32_t sy, void *data) {
	wlr_scene_buffer_set_opacity(buffer, *(double *)data);
}

void client_set_opacity(Client *c, double opacity) {
	opacity = CLAMP_FLOAT(opacity, 0.0f, 1.0f);
	wlr_scene_node_for_each_buffer(&c->scene_surface->node,
								   scene_buffer_apply_opacity, &opacity);
}

void monitor_stop_skip_frame_timer(Monitor *m) {
	if (m->skip_frame_timeout)
		wl_event_source_timer_update(m->skip_frame_timeout, 0);
	m->skiping_frame = false;
	m->resizing_count_pending = 0;
	m->resizing_count_current = 0;
}

static int monitor_skip_frame_timeout_callback(void *data) {
	Monitor *m = data;
	Client *c, *tmp;

	wl_list_for_each_safe(c, tmp, &clients, link) { c->configure_serial = 0; }

	monitor_stop_skip_frame_timer(m);
	wlr_output_schedule_frame(m->wlr_output);

	return 1;
}

void monitor_check_skip_frame_timeout(Monitor *m) {
	if (m->skiping_frame &&
		m->resizing_count_pending == m->resizing_count_current) {
		return;
	}

	if (m->skip_frame_timeout) {
		m->resizing_count_current = m->resizing_count_pending;
		m->skiping_frame = true;
		wl_event_source_timer_update(m->skip_frame_timeout, 100); // 100ms
	}
}

void rendermon(struct wl_listener *listener, void *data) {
	Monitor *m = wl_container_of(listener, m, frame);
	Client *c = NULL, *tmp = NULL;
	LayerSurface *l = NULL, *tmpl = NULL;
	int32_t i;
	struct wl_list *layer_list;
	bool frame_allow_tearing = false;
	struct timespec now;
	bool need_more_frames = false;

	if (session && !session->active) {
		return;
	}

	if (!m->wlr_output->enabled || !allow_frame_scheduling)
		return;

	frame_allow_tearing = check_tearing_frame_allow(m);

	// Draw layer and fade out effect
	for (i = 0; i < LENGTH(m->layers); i++) {
		layer_list = &m->layers[i];
		wl_list_for_each_safe(l, tmpl, layer_list, link) {
			need_more_frames = layer_draw_frame(l) || need_more_frames;
		}
	}

	wl_list_for_each_safe(c, tmp, &fadeout_clients, fadeout_link) {
		need_more_frames = client_draw_fadeout_frame(c) || need_more_frames;
	}

	wl_list_for_each_safe(l, tmpl, &fadeout_layers, fadeout_link) {
		need_more_frames = layer_draw_fadeout_frame(l) || need_more_frames;
	}

	// draw client
	wl_list_for_each(c, &clients, link) {
		need_more_frames = client_draw_frame(c) || need_more_frames;
		if (!config.animations && !grabc && c->configure_serial &&
			client_is_rendered_on_mon(c, m)) {
			monitor_check_skip_frame_timeout(m);
			goto skip;
		}
	}

	if (m->skiping_frame) {
		monitor_stop_skip_frame_timer(m);
	}

	// Build and commit state only when frames are needed
	if (config.allow_tearing && frame_allow_tearing) {
		apply_tear_state(m);
	} else {
		wlr_scene_output_commit(m->scene_output, NULL);
	}

skip:
	//Send frame completion notification
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(m->scene_output, &now);

	// If more frames are needed, make sure to schedule the next one
	if (need_more_frames && allow_frame_scheduling) {
		request_fresh_all_monitors();
	}
}

void requestdecorationmode(struct wl_listener *listener, void *data) {
	Client *c = wl_container_of(listener, c, set_decoration_mode);
	struct wlr_xdg_toplevel_decoration_v1 *deco = data;

	if (c->surface.xdg->initialized) {
		// Get the mode requested by the client
		enum wlr_xdg_toplevel_decoration_v1_mode requested_mode =
			deco->requested_mode;

		// If not specified by the client, use the default mode
		if (!c->allow_csd) {
			requested_mode = WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
		}

		wlr_xdg_toplevel_decoration_v1_set_mode(c->decoration, requested_mode);
	}
}

static void requestdrmlease(struct wl_listener *listener, void *data) {
	struct wlr_drm_lease_request_v1 *req = data;
	struct wlr_drm_lease_v1 *lease = wlr_drm_lease_request_v1_grant(req);

	if (!lease) {
		wlr_log(WLR_ERROR, "Failed to grant lease request");
		wlr_drm_lease_request_v1_reject(req);
	}
}

void requeststartdrag(struct wl_listener *listener, void *data) {
	struct wlr_seat_request_start_drag_event *event = data;

	if (wlr_seat_validate_pointer_grab_serial(seat, event->origin,
											  event->serial))
		wlr_seat_start_pointer_drag(seat, event->drag, event->serial);
	else
		wlr_data_source_destroy(event->drag->source);
}

void setborder_color(Client *c) {
	if (!c || !c->mon)
		return;

	float *border_color = get_border_color(c);
	memcpy(c->opacity_animation.target_border_color, border_color,
		   sizeof(c->opacity_animation.target_border_color));
	client_set_border_color(c, border_color);
}

void exchange_two_client(Client *c1, Client *c2) {
	Monitor *tmp_mon = NULL;
	uint32_t tmp_tags;
	double master_inner_per = 0.0f;
	double master_mfact_per = 0.0f;
	double stack_inner_per = 0.0f;
	struct ScrollerStackNode *n1 = NULL;
	struct ScrollerStackNode *n2 = NULL;
	struct TagScrollerState *st1 = NULL;
	struct TagScrollerState *st2 = NULL;

	if (c1 == NULL || c2 == NULL ||
		(!config.exchange_cross_monitor && c1->mon != c2->mon)) {
		return;
	}

	master_inner_per = c1->master_inner_per;
	master_mfact_per = c1->master_mfact_per;
	stack_inner_per = c1->stack_inner_per;
	c1->master_inner_per = c2->master_inner_per;
	c1->master_mfact_per = c2->master_mfact_per;
	c1->stack_inner_per = c2->stack_inner_per;
	c2->master_inner_per = master_inner_per;
	c2->master_mfact_per = master_mfact_per;
	c2->stack_inner_per = stack_inner_per;

	bool c1_scroller = c1->mon && is_scroller_layout(c1->mon);
	bool c2_scroller = c2->mon && is_scroller_layout(c2->mon);
	Monitor *m1 = c1->mon;
	Monitor *m2 = c2->mon;
	uint32_t tag1 = m1->pertag->curtag;
	uint32_t tag2 = m2->pertag->curtag;

	if (c1_scroller) {
		st1 = ensure_scroller_state(m1, tag1);
		n1 = find_scroller_node(st1, c1);
	}

	if (c2_scroller) {
		st2 = ensure_scroller_state(m2, tag2);
		n2 = find_scroller_node(st2, c2);
	}

	if (!n1 || !n2)
		goto exchange_common;

	if (n1 && n2) {

		/* Swapping is not allowed when there is a stacking relationship between monitors and either side */
		if (m1 != m2 && (n1->prev_in_stack || n2->prev_in_stack ||
						 n1->next_in_stack || n2->next_in_stack))
			return;

		/* Get the respective stack head node */
		struct ScrollerStackNode *head1 = n1;
		while (head1->prev_in_stack)
			head1 = head1->prev_in_stack;
		struct ScrollerStackNode *head2 = n2;
		while (head2->prev_in_stack)
			head2 = head2->prev_in_stack;

		/* Exchange within the same stack */
		if (head1 == head2) {
			float tmp_scroller = n1->scroller_proportion;
			float tmp_stack = n1->stack_proportion;
			n1->scroller_proportion = n2->scroller_proportion;
			n1->stack_proportion = n2->stack_proportion;
			n2->scroller_proportion = tmp_scroller;
			n2->stack_proportion = tmp_stack;

			/* Swap stacked linked list pointers */
			struct ScrollerStackNode *p1 = n1->prev_in_stack;
			struct ScrollerStackNode *next1 = n1->next_in_stack;
			struct ScrollerStackNode *p2 = n2->prev_in_stack;
			struct ScrollerStackNode *next2 = n2->next_in_stack;

			if (n1->next_in_stack == n2) {
				n1->next_in_stack = next2;
				n2->prev_in_stack = p1;
				n1->prev_in_stack = n2;
				n2->next_in_stack = n1;
				if (p1)
					p1->next_in_stack = n2;
				if (next2)
					next2->prev_in_stack = n1;
			} else if (n2->next_in_stack == n1) {
				n2->next_in_stack = next1;
				n1->prev_in_stack = p2;
				n2->prev_in_stack = n1;
				n1->next_in_stack = n2;
				if (p2)
					p2->next_in_stack = n1;
				if (next1)
					next1->prev_in_stack = n2;
			} else {
				if (p1)
					p1->next_in_stack = n2;
				if (next1)
					next1->prev_in_stack = n2;
				if (p2)
					p2->next_in_stack = n1;
				if (next2)
					next2->prev_in_stack = n1;
				n1->prev_in_stack = p2;
				n1->next_in_stack = next2;
				n2->prev_in_stack = p1;
				n2->next_in_stack = next1;
			}

			sync_scroller_state_to_clients(m1, tag1);
			arrange(m1, false, false);
		} else {
			/* Different stacks: swap the overall positions of the two stacks */
			if (n1 != head1 || n2 != head2) {
				/* Currently not the head, recursively exchange the head */
				exchange_two_client(head1->client, head2->client);
				return;
			}
		}
	}

exchange_common:

	/* Swapping is not allowed when there is a stacking relationship between monitors and either side */
	if (m1 != m2 && ((n1 && n1->prev_in_stack) || (n2 && n2->prev_in_stack) ||
					 (n1 && n1->next_in_stack) || (n2 && n2->next_in_stack)))
		return;

	struct wl_list *tmp1_prev = c1->link.prev;
	struct wl_list *tmp2_prev = c2->link.prev;
	struct wl_list *tmp1_next = c1->link.next;
	struct wl_list *tmp2_next = c2->link.next;

	if (c1->link.next == &c2->link) {
		c1->link.next = c2->link.next;
		c1->link.prev = &c2->link;
		c2->link.next = &c1->link;
		c2->link.prev = tmp1_prev;
		tmp1_prev->next = &c2->link;
		tmp2_next->prev = &c1->link;
	} else if (c2->link.next == &c1->link) {
		c2->link.next = c1->link.next;
		c2->link.prev = &c1->link;
		c1->link.next = &c2->link;
		c1->link.prev = tmp2_prev;
		tmp2_prev->next = &c1->link;
		tmp1_next->prev = &c2->link;
	} else {
		c2->link.next = tmp1_next;
		c2->link.prev = tmp1_prev;
		c1->link.next = tmp2_next;
		c1->link.prev = tmp2_prev;
		tmp1_prev->next = &c2->link;
		tmp1_next->prev = &c2->link;
		tmp2_prev->next = &c1->link;
		tmp2_next->prev = &c1->link;
	}

	if (config.exchange_cross_monitor && c1->mon != c2->mon) {
		DwindleNode **c1_root = &m1->pertag->dwindle_root[m1->pertag->curtag];
		DwindleNode *c1node = dwindle_find_leaf(*c1_root, c1);

		DwindleNode **c2_root = &m2->pertag->dwindle_root[m2->pertag->curtag];
		DwindleNode *c2node = dwindle_find_leaf(*c2_root, c2);

		if (c1node)
			c1node->client = c2;

		if (c2node)
			c2node->client = c1;

		tmp_mon = c2->mon;
		tmp_tags = c2->tags;
		c2->mon = c1->mon;
		c1->mon = tmp_mon;
		c2->tags = c1->tags;
		c1->tags = tmp_tags;

		arrange(c1->mon, false, false);
		arrange(c2->mon, false, false);
	} else {
		if (c1->mon &&
			c1->mon->pertag->ltidxs[c1->mon->pertag->curtag]->id == DWINDLE) {
			dwindle_swap_clients(
				&c1->mon->pertag->dwindle_root[c1->mon->pertag->curtag], c1,
				c2);
		}
		arrange(c1->mon, false, false);
	}

	// In order to facilitate repeated exchanges for get_focused_stack_client
	// set c2 focus order behind c1
	wl_list_remove(&c2->flink);
	wl_list_insert(&c1->flink, &c2->flink);
}

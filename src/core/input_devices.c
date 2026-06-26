
void cleanuplisteners(void) {
	wl_list_remove(&print_status_listener.link);
	wl_list_remove(&cursor_axis.link);
	wl_list_remove(&cursor_button.link);
	wl_list_remove(&cursor_frame.link);
	wl_list_remove(&cursor_motion.link);
	wl_list_remove(&cursor_motion_absolute.link);
	wl_list_remove(&gpu_reset.link);
	wl_list_remove(&new_idle_inhibitor.link);
	wl_list_remove(&layout_change.link);
	wl_list_remove(&new_input_device.link);
	wl_list_remove(&new_virtual_keyboard.link);
	wl_list_remove(&new_virtual_pointer.link);
	wl_list_remove(&new_pointer_constraint.link);
	wl_list_remove(&new_output.link);
	wl_list_remove(&new_xdg_toplevel.link);
	wl_list_remove(&new_xdg_decoration.link);
	wl_list_remove(&new_xdg_popup.link);
	wl_list_remove(&new_layer_surface.link);
	wl_list_remove(&output_mgr_apply.link);
	wl_list_remove(&output_mgr_test.link);
	wl_list_remove(&output_power_mgr_set_mode.link);
	wl_list_remove(&request_activate.link);
	wl_list_remove(&request_cursor.link);
	wl_list_remove(&request_set_psel.link);
	wl_list_remove(&request_set_sel.link);
	wl_list_remove(&request_set_cursor_shape.link);
	wl_list_remove(&request_start_drag.link);
	wl_list_remove(&start_drag.link);
	wl_list_remove(&new_session_lock.link);
	wl_list_remove(&tearing_new_object.link);
	wl_list_remove(&keyboard_shortcuts_inhibit_new_inhibitor.link);
	if (drm_lease_manager) {
		wl_list_remove(&drm_lease_request.link);
	}
#ifdef XWAYLAND
	wl_list_remove(&new_xwayland_surface.link);
	wl_list_remove(&xwayland_ready.link);
#endif
}

void cleanup(void) {
	cleanuplisteners();
#ifdef XWAYLAND
	wlr_xwayland_destroy(xwayland);
	xwayland = NULL;
#endif

	wl_display_destroy_clients(dpy);
	if (child_pid > 0) {
		kill(-child_pid, SIGTERM);
		waitpid(child_pid, NULL, 0);
	}
	wlr_xcursor_manager_destroy(cursor_mgr);

	destroykeyboardgroup(&kb_group->destroy, NULL);

	aether_im_relay_finish(aether_input_method_relay);

	/* If it's not destroyed manually it will cause a use-after-free of
	 * wlr_seat. Destroy it until it's fixed in the wlroots side */
	wlr_backend_destroy(backend);

	wl_display_destroy(dpy);
	/* Destroy after the wayland display (when the monitors are already
	   destroyed) to avoid destroying them with an invalid scene output. */
	wlr_scene_node_destroy(&scene->tree.node);
}

void cleanupmon(struct wl_listener *listener, void *data) {
	Monitor *m = wl_container_of(listener, m, destroy);
	LayerSurface *l = NULL, *tmp = NULL;
	uint32_t i;

	m->iscleanuping = true;

	/* m->layers[i] are intentionally not unlinked */
	for (i = 0; i < LENGTH(m->layers); i++) {
		wl_list_for_each_safe(l, tmp, &m->layers[i], link)
			wlr_layer_surface_v1_destroy(l->layer_surface);
	}

	// clean ext-workspaces grouplab
	wlr_ext_workspace_group_handle_v1_output_leave(m->ext_group, m->wlr_output);
	wlr_ext_workspace_group_handle_v1_destroy(m->ext_group);
	cleanup_workspaces_by_monitor(m);

	wl_list_remove(&m->destroy.link);
	wl_list_remove(&m->frame.link);
	wl_list_remove(&m->link);
	wl_list_remove(&m->request_state.link);
	if (m->lock_surface)
		destroylocksurface(&m->destroy_lock_surface, NULL);
	m->wlr_output->data = NULL;
	wlr_output_layout_remove(output_layout, m->wlr_output);
	wlr_scene_output_destroy(m->scene_output);

	closemon(m);
	if (m->blur) {
		wlr_scene_node_destroy(&m->blur->node);
		m->blur = NULL;
	}
	if (m->skip_frame_timeout) {
		monitor_stop_skip_frame_timer(m);
		wl_event_source_remove(m->skip_frame_timeout);
		m->skip_frame_timeout = NULL;
	}
	m->wlr_output->data = NULL;

	for (uint32_t t = 0; t < LENGTH(tags) + 1; t++)
		dwindle_free_tree(m->pertag->dwindle_root[t]);

	cleanup_monitor_scroller(m);

	free(m->pertag);
	free(m);
}

void closemon(Monitor *m) {
	/* update selmon if needed and
	 * move closed monitor's clients to the focused one */
	Client *c = NULL;
	int32_t i = 0, nmons = wl_list_length(&mons);
	if (!nmons) {
		selmon = NULL;
	} else if (m == selmon) {
		do /* don't switch to disabled mons */
			selmon = wl_container_of(mons.next, selmon, link);
		while (!selmon->wlr_output->enabled && i++ < nmons);

		if (!selmon->wlr_output->enabled)
			selmon = NULL;
	}

	wl_list_for_each(c, &clients, link) {
		if (c->mon == m) {

			if (selmon == NULL) {
				if (c->foreign_toplevel) {
					wlr_foreign_toplevel_handle_v1_output_leave(
						c->foreign_toplevel, c->mon->wlr_output);
					wlr_foreign_toplevel_handle_v1_destroy(c->foreign_toplevel);
					c->foreign_toplevel = NULL;
				}

				c->mon = NULL;
			} else {
				client_change_mon(c, selmon);
			}
			// record the oldmonname which is used to restore
			if (c->oldmonname[0] == '\0') {
				client_update_oldmonname_record(c, m);
			}
		}
	}
	if (selmon) {
		focusclient(focustop(selmon), 1);
		printstatus();
	}
}

static void iter_layer_scene_buffers(struct wlr_scene_buffer *buffer,
									 int32_t sx, int32_t sy, void *user_data) {
	struct wlr_scene_surface *scene_surface =
		wlr_scene_surface_try_from_buffer(buffer);
	if (!scene_surface) {
		return;
	}

	wlr_scene_buffer_set_backdrop_blur(buffer, true);
	wlr_scene_buffer_set_backdrop_blur_ignore_transparent(buffer, true);
	if (config.blur_optimized) {
		wlr_scene_buffer_set_backdrop_blur_optimized(buffer, true);
	} else {
		wlr_scene_buffer_set_backdrop_blur_optimized(buffer, false);
	}
}

void layer_flush_blur_background(LayerSurface *l) {
	if (!config.blur)
		return;

	// If the background layer changes, the marker optimized blur background cache needs to be updated
	if (l->layer_surface->current.layer ==
		ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
		if (l->mon) {
			wlr_scene_optimized_blur_mark_dirty(l->mon->blur);
		}
	}
}

void maplayersurfacenotify(struct wl_listener *listener, void *data) {
	LayerSurface *l = wl_container_of(listener, l, map);
	struct wlr_layer_surface_v1 *layer_surface = l->layer_surface;
	int32_t ji;
	ConfigLayerRule *r;

	l->mapped = 1;

	if (!l->mon)
		return;
	strncpy(l->mon->last_surface_ws_name, layer_surface->namespace,
			sizeof(l->mon->last_surface_ws_name) - 1); //Copy up to 255 characters
	l->mon->last_surface_ws_name[sizeof(l->mon->last_surface_ws_name) - 1] =
		'\0'; // Make sure the string is null terminated

	//Initialize geometric position
	get_layer_target_geometry(l, &l->geom);

	l->noanim = 0;
	l->dirty = false;
	l->noblur = 0;
	l->shadow = NULL;
	l->need_output_flush = true;

	//Apply layer rules
	for (ji = 0; ji < config.layer_rules_count; ji++) {
		if (config.layer_rules_count < 1)
			break;
		if (regex_match(config.layer_rules[ji].layer_name,
						l->layer_surface->namespace)) {

			r = &config.layer_rules[ji];
			APPLY_INT_PROP(l, r, noblur);
			APPLY_INT_PROP(l, r, noanim);
			APPLY_INT_PROP(l, r, noshadow);
			APPLY_STRING_PROP(l, r, animation_type_open);
			APPLY_STRING_PROP(l, r, animation_type_close);
		}
	}

	//Initialize shadow
	if (layer_surface->current.exclusive_zone == 0 &&
		layer_surface->current.layer != ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM &&
		layer_surface->current.layer != ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
		l->shadow =
			wlr_scene_shadow_create(l->scene, 0, 0, config.border_radius,
									config.shadows_blur, config.shadowscolor);
		wlr_scene_node_lower_to_bottom(&l->shadow->node);
		wlr_scene_node_set_enabled(&l->shadow->node, true);
	}

	//Initialize animation
	if (config.animations && config.layer_animations && !l->noanim) {
		l->animation.duration = config.animation_duration_open;
		l->animation.action = OPEN;
		layer_set_pending_state(l);
	}
	// Refresh the layout so that the window can sense changes in exclude_zone and set the exclusive surface
	arrangelayers(l->mon);
	reset_exclusive_layers_focus(l->mon);
}

void commitlayersurfacenotify(struct wl_listener *listener, void *data) {
	LayerSurface *l = wl_container_of(listener, l, surface_commit);
	struct wlr_layer_surface_v1 *layer_surface = l->layer_surface;
	struct wlr_scene_tree *scene_layer =
		layers[layermap[layer_surface->current.layer]];
	struct wlr_layer_surface_v1_state old_state;
	struct wlr_box box;

	if (l->layer_surface->initial_commit) {
		client_set_scale(layer_surface->surface, l->mon->wlr_output->scale);

		/* Temporarily set the layer's current state to pending
		 * so that we can easily arrange it */
		old_state = l->layer_surface->current;
		l->layer_surface->current = l->layer_surface->pending;
		arrangelayers(l->mon);
		l->layer_surface->current = old_state;
		//On-demand interactive layer only sets focus before map
		if (!exclusive_focus &&
			l->layer_surface->current.keyboard_interactive ==
				ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND) {
			focuslayer(l);
		}
		return;
	}

	// Check if the surface has a buffer
	// Empty buffer, just hidden, does not change the mapped state
	if (l->mapped && !layer_surface->surface->buffer) {
		wlr_scene_node_set_enabled(&l->scene->node, false);
		return;
	} else {
		wlr_scene_node_set_enabled(&l->scene->node, true);
	}

	get_layer_target_geometry(l, &box);

	if (config.animations && config.layer_animations && !l->noanim &&
		l->mapped &&
		layer_surface->current.layer != ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM &&
		layer_surface->current.layer != ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND &&
		!wlr_box_equal(&box, &l->geom)) {

		l->geom.x = box.x;
		l->geom.y = box.y;
		l->geom.width = box.width;
		l->geom.height = box.height;
		l->animation.action = MOVE;
		l->animation.duration = config.animation_duration_move;
		l->need_output_flush = true;
		layer_set_pending_state(l);
	}

	if (config.blur && config.blur_layer) {

		if (!l->noblur &&
			layer_surface->current.layer != ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM &&
			layer_surface->current.layer !=
				ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {

			wlr_scene_node_for_each_buffer(&l->scene->node,
										   iter_layer_scene_buffers, l);
		}
	}

	layer_flush_blur_background(l);

	if (layer_surface->current.committed == 0 &&
		l->mapped == layer_surface->surface->mapped)
		return;
	l->mapped = layer_surface->surface->mapped;

	if (layer_surface->current.committed & WLR_LAYER_SURFACE_V1_STATE_LAYER) {
		if (scene_layer != l->scene->node.parent) {
			wlr_scene_node_reparent(&l->scene->node, scene_layer);
			wl_list_remove(&l->link);
			wl_list_insert(&l->mon->layers[layer_surface->current.layer],
						   &l->link);
			wlr_scene_node_reparent(
				&l->popups->node,
				(layer_surface->current.layer < ZWLR_LAYER_SHELL_V1_LAYER_TOP
					 ? layers[LyrTop]
					 : scene_layer));
		}
	}

	arrangelayers(l->mon);

	if (layer_surface->current.committed &
		WLR_LAYER_SURFACE_V1_STATE_KEYBOARD_INTERACTIVITY) {
		reset_exclusive_layers_focus(l->mon);
	}
}

void commitnotify(struct wl_listener *listener, void *data) {
	Client *c = wl_container_of(listener, c, commit);
	struct wlr_box *new_geo;

	if (c->surface.xdg->initial_commit) {
		// xdg client will first enter this before mapnotify
		init_client_properties(c);
		applyrules(c);
		if (c->mon) {
			client_set_scale(client_surface(c), c->mon->wlr_output->scale);
		}
		setmon(c, NULL, 0,
			   true); /* Make sure to reapply rules in mapnotify() */

		uint32_t serial = wlr_xdg_surface_schedule_configure(c->surface.xdg);
		if (serial > 0) {
			c->configure_serial = serial;
		}

		uint32_t wm_caps = WLR_XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN;

		if (!c->ignore_minimize)
			wm_caps |= WLR_XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE;

		if (!c->ignore_maximize)
			wm_caps |= WLR_XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE;

		wlr_xdg_toplevel_set_wm_capabilities(c->surface.xdg->toplevel, wm_caps);

		if (c->mon) {
			wlr_xdg_toplevel_set_bounds(c->surface.xdg->toplevel,
										c->mon->w.width - 2 * c->bw,
										c->mon->w.height - 2 * c->bw);
		}

		if (c->decoration)
			requestdecorationmode(&c->set_decoration_mode, c->decoration);
		return;
	}

	if (!c || c->iskilling || c->animation.tagouting || c->animation.tagouted ||
		c->animation.tagining)
		return;

	if (c->configure_serial &&
		c->configure_serial <= c->surface.xdg->current.configure_serial)
		c->configure_serial = 0;

	if (!c->dirty) {
		new_geo = &c->surface.xdg->geometry;
		c->dirty = new_geo->width != c->geom.width - 2 * c->bw ||
				   new_geo->height != c->geom.height - 2 * c->bw ||
				   new_geo->x != 0 || new_geo->y != 0;
	}

	if (c == grabc || !c->dirty)
		return;

	resize(c, c->geom, 0);

	new_geo = &c->surface.xdg->geometry;
	c->dirty = new_geo->width != c->geom.width - 2 * c->bw ||
			   new_geo->height != c->geom.height - 2 * c->bw ||
			   new_geo->x != 0 || new_geo->y != 0;
}

void destroydecoration(struct wl_listener *listener, void *data) {
	Client *c = wl_container_of(listener, c, destroy_decoration);

	wl_list_remove(&c->destroy_decoration.link);
	wl_list_remove(&c->set_decoration_mode.link);
}

static bool popup_unconstrain(Popup *popup) {
	struct wlr_xdg_popup *wlr_popup = popup->wlr_popup;
	Client *c = NULL;
	LayerSurface *l = NULL;
	int32_t type = -1;

	if (!wlr_popup || !wlr_popup->parent) {
		return false;
	}

	struct wlr_scene_node *parent_node = wlr_popup->parent->data;
	if (!parent_node) {
		wlr_log(WLR_ERROR, "Popup parent has no scene node");
		return false;
	}

	type = toplevel_from_wlr_surface(wlr_popup->base->surface, &c, &l);
	if ((l && !l->mon) || (c && !c->mon)) {
		return true;
	}

	struct wlr_box usable = type == LayerShell ? l->mon->m : c->mon->w;

	int lx, ly;
	struct wlr_box constraint_box;

	if (type == LayerShell) {
		wlr_scene_node_coords(&l->scene_layer->tree->node, &lx, &ly);
		constraint_box.x = usable.x - lx;
		constraint_box.y = usable.y - ly;
		constraint_box.width = usable.width;
		constraint_box.height = usable.height;
	} else {
		constraint_box.x =
			usable.x - (c->geom.x + c->bw - c->surface.xdg->current.geometry.x);
		constraint_box.y =
			usable.y - (c->geom.y + c->bw - c->surface.xdg->current.geometry.y);
		constraint_box.width = usable.width;
		constraint_box.height = usable.height;
	}

	wlr_xdg_popup_unconstrain_from_box(wlr_popup, &constraint_box);
	return false;
}

static void destroypopup(struct wl_listener *listener, void *data) {
	Popup *popup = wl_container_of(listener, popup, destroy);
	wl_list_remove(&popup->destroy.link);
	wl_list_remove(&popup->reposition.link);
	free(popup);
}

static void commitpopup(struct wl_listener *listener, void *data) {
	Popup *popup = wl_container_of(listener, popup, commit);

	struct wlr_surface *surface = data;
	bool should_destroy = false;
	struct wlr_xdg_popup *wlr_popup =
		wlr_xdg_popup_try_from_wlr_surface(surface);

	if (!wlr_popup->base->initial_commit)
		return;

	if (!wlr_popup->parent || !wlr_popup->parent->data) {
		should_destroy = true;
		goto cleanup_popup_commit;
	}

	wlr_scene_node_raise_to_top(wlr_popup->parent->data);

	wlr_popup->base->surface->data =
		wlr_scene_xdg_surface_create(wlr_popup->parent->data, wlr_popup->base);

	popup->wlr_popup = wlr_popup;

	should_destroy = popup_unconstrain(popup);

cleanup_popup_commit:

	wl_list_remove(&popup->commit.link);
	popup->commit.notify = NULL;

	if (should_destroy) {
		wlr_xdg_popup_destroy(wlr_popup);
	}
}

static void repositionpopup(struct wl_listener *listener, void *data) {
	Popup *popup = wl_container_of(listener, popup, reposition);
	(void)popup_unconstrain(popup);
}

static void createpopup(struct wl_listener *listener, void *data) {
	struct wlr_xdg_popup *wlr_popup = data;

	Popup *popup = calloc(1, sizeof(Popup));
	if (!popup)
		return;

	popup->destroy.notify = destroypopup;
	wl_signal_add(&wlr_popup->events.destroy, &popup->destroy);

	popup->commit.notify = commitpopup;
	wl_signal_add(&wlr_popup->base->surface->events.commit, &popup->commit);

	popup->reposition.notify = repositionpopup;
	wl_signal_add(&wlr_popup->events.reposition, &popup->reposition);
}

void createdecoration(struct wl_listener *listener, void *data) {
	struct wlr_xdg_toplevel_decoration_v1 *deco = data;
	Client *c = deco->toplevel->base->data;
	c->decoration = deco;

	LISTEN(&deco->events.request_mode, &c->set_decoration_mode,
		   requestdecorationmode);
	LISTEN(&deco->events.destroy, &c->destroy_decoration, destroydecoration);

	requestdecorationmode(&c->set_decoration_mode, deco);
}

void createidleinhibitor(struct wl_listener *listener, void *data) {
	struct wlr_idle_inhibitor_v1 *idle_inhibitor = data;
	LISTEN_STATIC(&idle_inhibitor->events.destroy, destroyidleinhibitor);

	checkidleinhibitor(NULL);
}

void createkeyboard(struct wlr_keyboard *keyboard) {

	struct libinput_device *device = NULL;

	if (wlr_input_device_is_libinput(&keyboard->base) &&
		(device = wlr_libinput_get_device_handle(&keyboard->base))) {

		InputDevice *input_dev = calloc(1, sizeof(InputDevice));
		input_dev->wlr_device = &keyboard->base;
		input_dev->libinput_device = device;
		input_dev->device_data = keyboard;

		input_dev->destroy_listener.notify = destroyinputdevice;
		wl_signal_add(&keyboard->base.events.destroy,
					  &input_dev->destroy_listener);

		wl_list_insert(&inputdevices, &input_dev->link);
	}

	/* Set the keymap to match the group keymap */
	wlr_keyboard_set_keymap(keyboard, kb_group->wlr_group->keyboard.keymap);

	wlr_keyboard_notify_modifiers(keyboard, 0, 0, locked_mods, 0);

	/* Add the new keyboard to the group */
	wlr_keyboard_group_add_keyboard(kb_group->wlr_group, keyboard);
}

KeyboardGroup *createkeyboardgroup(void) {
	KeyboardGroup *group = ecalloc(1, sizeof(*group));
	struct xkb_context *context;
	struct xkb_keymap *keymap;

	group->wlr_group = wlr_keyboard_group_create();
	group->wlr_group->data = group;

	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!(keymap = xkb_keymap_new_from_names(context, &config.xkb_rules,
											 XKB_KEYMAP_COMPILE_NO_FLAGS)))
		die("failed to compile keymap");

	wlr_keyboard_set_keymap(&group->wlr_group->keyboard, keymap);

	if (config.numlockon) {
		xkb_mod_index_t mod_index =
			xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_NUM);
		if (mod_index != XKB_MOD_INVALID)
			locked_mods |= (uint32_t)1 << mod_index;
	}

	if (config.capslock) {
		xkb_mod_index_t mod_index =
			xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
		if (mod_index != XKB_MOD_INVALID)
			locked_mods |= (uint32_t)1 << mod_index;
	}

	if (locked_mods)
		wlr_keyboard_notify_modifiers(&group->wlr_group->keyboard, 0, 0,
									  locked_mods, 0);

	xkb_keymap_unref(keymap);
	xkb_context_unref(context);

	wlr_keyboard_set_repeat_info(&group->wlr_group->keyboard,
								 config.repeat_rate, config.repeat_delay);

	/* Set up listeners for keyboard events */
	LISTEN(&group->wlr_group->keyboard.events.key, &group->key, keypress);
	LISTEN(&group->wlr_group->keyboard.events.modifiers, &group->modifiers,
		   keypressmod);

	group->key_repeat_source =
		wl_event_loop_add_timer(event_loop, keyrepeat, group);

	/* A seat can only have one keyboard, but this is a limitation of the
	 * Wayland protocol - not wlroots. We assign all connected keyboards to the
	 * same wlr_keyboard_group, which provides a single wlr_keyboard interface
	 * for all of them. Set this combined wlr_keyboard as the seat keyboard.
	 */
	wlr_seat_set_keyboard(seat, &group->wlr_group->keyboard);
	return group;
}

void createlayersurface(struct wl_listener *listener, void *data) {
	struct wlr_layer_surface_v1 *layer_surface = data;
	LayerSurface *l = NULL;
	struct wlr_surface *surface = layer_surface->surface;
	struct wlr_scene_tree *scene_layer =
		layers[layermap[layer_surface->pending.layer]];

	if (!layer_surface->output &&
		!(layer_surface->output = selmon ? selmon->wlr_output : NULL)) {
		wlr_layer_surface_v1_destroy(layer_surface);
		return;
	}

	l = layer_surface->data = ecalloc(1, sizeof(*l));
	l->type = LayerShell;
	LISTEN(&surface->events.map, &l->map, maplayersurfacenotify);
	LISTEN(&surface->events.commit, &l->surface_commit,
		   commitlayersurfacenotify);
	LISTEN(&surface->events.unmap, &l->unmap, unmaplayersurfacenotify);

	l->layer_surface = layer_surface;
	l->mon = layer_surface->output->data;
	l->scene_layer =
		wlr_scene_layer_surface_v1_create(scene_layer, layer_surface);
	l->scene = l->scene_layer->tree;
	l->popups = surface->data = wlr_scene_tree_create(
		layer_surface->current.layer < ZWLR_LAYER_SHELL_V1_LAYER_TOP
			? layers[LyrTop]
			: scene_layer);
	l->scene->node.data = l->popups->node.data = l;

	LISTEN(&l->scene->node.events.destroy, &l->destroy, destroylayernodenotify);

	wl_list_insert(&l->mon->layers[layer_surface->pending.layer], &l->link);
	wlr_surface_send_enter(surface, layer_surface->output);
}

void createlocksurface(struct wl_listener *listener, void *data) {
	SessionLock *lock = wl_container_of(listener, lock, new_surface);
	struct wlr_session_lock_surface_v1 *lock_surface = data;
	Monitor *m = lock_surface->output->data;
	struct wlr_scene_tree *scene_tree = lock_surface->surface->data =
		wlr_scene_subsurface_tree_create(lock->scene, lock_surface->surface);
	m->lock_surface = lock_surface;

	wlr_scene_node_set_position(&scene_tree->node, m->m.x, m->m.y);
	wlr_session_lock_surface_v1_configure(lock_surface, m->m.width,
										  m->m.height);

	LISTEN(&lock_surface->events.destroy, &m->destroy_lock_surface,
		   destroylocksurface);

	if (m == selmon)
		client_notify_enter(lock_surface->surface, wlr_seat_get_keyboard(seat));
}

struct wlr_output_mode *get_nearest_output_mode(struct wlr_output *output,
												int32_t width, int32_t height,
												float refresh) {
	struct wlr_output_mode *mode, *nearest_mode = NULL;
	float min_diff = 99999.0f;

	wl_list_for_each(mode, &output->modes, link) {
		if (mode->width == width && mode->height == height) {
			float mode_refresh = mode->refresh / 1000.0f;
			float diff = fabsf(mode_refresh - refresh);

			if (diff < min_diff) {
				min_diff = diff;
				nearest_mode = mode;
			}
		}
	}

	return nearest_mode;
}

void enable_adaptive_sync(Monitor *m, struct wlr_output_state *state) {
	wlr_output_state_set_adaptive_sync_enabled(state, true);
	if (!wlr_output_test_state(m->wlr_output, state)) {
		wlr_output_state_set_adaptive_sync_enabled(state, false);
		wlr_log(WLR_DEBUG, "failed to enable adaptive sync for output %s",
				m->wlr_output->name);
	} else {
		wlr_log(WLR_INFO, "adaptive sync enabled for output %s",
				m->wlr_output->name);
	}
}

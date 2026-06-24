
bool monitor_matches_rule(Monitor *m, const ConfigMonitorRule *rule) {
	if (rule->name != NULL && !regex_match(rule->name, m->wlr_output->name))
		return false;
	if (rule->make != NULL && (m->wlr_output->make == NULL ||
							   strcmp(rule->make, m->wlr_output->make) != 0))
		return false;
	if (rule->model != NULL && (m->wlr_output->model == NULL ||
								strcmp(rule->model, m->wlr_output->model) != 0))
		return false;
	if (rule->serial != NULL &&
		(m->wlr_output->serial == NULL ||
		 strcmp(rule->serial, m->wlr_output->serial) != 0))
		return false;
	return true;
}

/* 将规则中的显示参数应用到 wlr_output_state 中，返回是否设置了自定义模式 */
bool apply_rule_to_state(Monitor *m, const ConfigMonitorRule *rule,
						 struct wlr_output_state *state, int vrr, int custom) {
	bool mode_set = false;
	if (rule->width > 0 && rule->height > 0 && rule->refresh > 0) {
		struct wlr_output_mode *internal_mode = get_nearest_output_mode(
			m->wlr_output, rule->width, rule->height, rule->refresh);
		if (internal_mode) {
			wlr_output_state_set_mode(state, internal_mode);
			mode_set = true;
		} else if (custom || wlr_output_is_headless(m->wlr_output)) {
			wlr_output_state_set_custom_mode(
				state, rule->width, rule->height,
				(int32_t)roundf(rule->refresh * 1000));
			mode_set = true;
		}
	}
	if (vrr) {
		enable_adaptive_sync(m, state);
	} else {
		wlr_output_state_set_adaptive_sync_enabled(state, false);
	}
	wlr_output_state_set_scale(state, rule->scale);
	wlr_output_state_set_transform(state, rule->rr);
	return mode_set;
}

void createmon(struct wl_listener *listener, void *data) {
	/* This event is raised by the backend when a new output (aka a display or
	 * monitor) becomes available. */
	struct wlr_output *wlr_output = data;
	const ConfigMonitorRule *r;
	uint32_t i;
	int32_t ji, vrr, custom;
	struct wlr_output_state state;
	Monitor *m = NULL;
	bool custom_monitor_mode = false;

	if (!wlr_output_init_render(wlr_output, alloc, drw))
		return;

	if (wlr_output->non_desktop) {
		if (drm_lease_manager) {
			wlr_drm_lease_v1_manager_offer_output(drm_lease_manager,
												  wlr_output);
		}
		return;
	}

	struct wl_event_loop *loop = wl_display_get_event_loop(dpy);
	m = wlr_output->data = ecalloc(1, sizeof(*m));

	m->iscleanuping = false;
	m->skip_frame_timeout =
		wl_event_loop_add_timer(loop, monitor_skip_frame_timeout_callback, m);
	m->skiping_frame = false;
	m->resizing_count_pending = 0;
	m->resizing_count_current = 0;

	m->wlr_output = wlr_output;
	m->wlr_output->data = m;

	wl_list_init(&m->aether_ipc_outputs);

	for (i = 0; i < LENGTH(m->layers); i++)
		wl_list_init(&m->layers[i]);

	m->gappih           = config.gappih;
	m->gappiv           = config.gappiv;
	m->gap_outer_left   = config.gap_outer_left;
	m->gap_outer_right  = config.gap_outer_right;
	m->gap_outer_top    = config.gap_outer_top;
	m->gap_outer_bottom = config.gap_outer_bottom;
	m->isoverview = 0;
	m->sel = NULL;
	m->is_in_hotarea = 0;
	m->m.x = INT32_MAX;
	m->m.y = INT32_MAX;
	float scale = 1;
	enum wl_output_transform rr = WL_OUTPUT_TRANSFORM_NORMAL;

	wlr_output_state_init(&state);
	wlr_output_state_set_scale(&state, scale);
	wlr_output_state_set_transform(&state, rr);

	for (ji = 0; ji < config.monitor_rules_count; ji++) {
		if (config.monitor_rules_count < 1)
			break;

		r = &config.monitor_rules[ji];

		if (monitor_matches_rule(m, r)) {
			m->m.x = r->x == INT32_MAX ? INT32_MAX : r->x;
			m->m.y = r->y == INT32_MAX ? INT32_MAX : r->y;
			vrr = r->vrr >= 0 ? r->vrr : 0;
			custom = r->custom >= 0 ? r->custom : 0;
			scale = r->scale;
			rr = r->rr;

			if (apply_rule_to_state(m, r, &state, vrr, custom)) {
				custom_monitor_mode = true;
			}
			break; // 只应用第一个匹配规则
		}
	}

	if (!custom_monitor_mode)
		wlr_output_state_set_mode(&state,
								  wlr_output_preferred_mode(wlr_output));

	/* Set up event listeners */
	LISTEN(&wlr_output->events.frame, &m->frame, rendermon);
	LISTEN(&wlr_output->events.destroy, &m->destroy, cleanupmon);
	LISTEN(&wlr_output->events.request_state, &m->request_state,
		   requestmonstate);

	wlr_output_state_set_enabled(&state, 1);
	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);

	wl_list_insert(&mons, &m->link);

	m->pertag = calloc(1, sizeof(Pertag));
	for (int i = 0; i < LENGTH(tags) + 1; i++)
		m->pertag->scroller_state[i] = NULL;

	if (chvt_backup_tag &&
		regex_match(chvt_backup_selmon, m->wlr_output->name)) {
		m->tagset[0] = m->tagset[1] = (1 << (chvt_backup_tag - 1)) & TAGMASK;
		m->pertag->curtag = m->pertag->prevtag = chvt_backup_tag;
		chvt_backup_tag = 0;
		memset(chvt_backup_selmon, 0, sizeof(chvt_backup_selmon));
	} else {
		m->tagset[0] = m->tagset[1] = 1;
		m->pertag->curtag = m->pertag->prevtag = 1;
	}

	for (i = 0; i <= LENGTH(tags); i++) {
		m->pertag->nmasters[i] = config.default_nmaster;
		m->pertag->mfacts[i] = config.default_mfact;
		m->pertag->ltidxs[i] = &layouts[0];
	}

	// apply tag rule
	parse_tagrule(m);

	/* The xdg-protocol specifies:
	 *
	 * If the fullscreened surface is not opaque, the compositor must make
	 * sure that other screen content not part of the same surface tree (made
	 * up of subsurfaces, popups or similarly coupled surfaces) are not
	 * visible below the fullscreened surface.
	 *
	 */

	/* Adds this to the output layout in the order it was configured.
	 *
	 * The output layout utility automatically adds a wl_output global to the
	 * display, which Wayland clients can see to find out information about the
	 * output (such as DPI, scale factor, manufacturer, etc).
	 */
	m->scene_output = wlr_scene_output_create(scene, wlr_output);
	if (m->m.x == INT32_MAX || m->m.y == INT32_MAX)
		wlr_output_layout_add_auto(output_layout, wlr_output);
	else
		wlr_output_layout_add(output_layout, wlr_output, m->m.x, m->m.y);

	if (config.blur) {
		m->blur = wlr_scene_optimized_blur_create(&scene->tree, 0, 0);
		wlr_scene_node_set_position(&m->blur->node, m->m.x, m->m.y);
		wlr_scene_node_reparent(&m->blur->node, layers[LyrBlur]);
		wlr_scene_optimized_blur_set_size(m->blur, m->m.width, m->m.height);
	}
	m->ext_group = wlr_ext_workspace_group_handle_v1_create(
		ext_manager, EXT_WORKSPACE_ENABLE_CAPS);
	wlr_ext_workspace_group_handle_v1_output_enter(m->ext_group, m->wlr_output);

	for (i = 1; i <= LENGTH(tags); i++) {
		add_workspace_by_tag(i, m);
	}

	printstatus();
}

void // fix for 0.5
createnotify(struct wl_listener *listener, void *data) {
	/* This event is raised when wlr_xdg_shell receives a new xdg surface from a
	 * client, either a toplevel (application window) or popup,
	 * or when wlr_layer_shell receives a new popup from a layer.
	 * If you want to do something tricky with popups you should check if
	 * its parent is wlr_xdg_shell or wlr_layer_shell */
	struct wlr_xdg_toplevel *toplevel = data;
	Client *c = NULL;

	/* Allocate a Client for this surface */
	c = toplevel->base->data = ecalloc(1, sizeof(*c));
	c->surface.xdg = toplevel->base;
	c->bw = config.borderpx;

	LISTEN(&toplevel->base->surface->events.commit, &c->commit, commitnotify);
	LISTEN(&toplevel->base->surface->events.map, &c->map, mapnotify);
	LISTEN(&toplevel->base->surface->events.unmap, &c->unmap, unmapnotify);
	LISTEN(&toplevel->events.destroy, &c->destroy, destroynotify);
	LISTEN(&toplevel->events.request_fullscreen, &c->fullscreen,
		   fullscreennotify);
	LISTEN(&toplevel->events.request_maximize, &c->maximize, maximizenotify);
	LISTEN(&toplevel->events.request_minimize, &c->minimize, minimizenotify);
	LISTEN(&toplevel->events.request_move, &c->request_move, requestmovenotify);
	LISTEN(&toplevel->events.request_resize, &c->request_resize, requestresizenotify);
	LISTEN(&toplevel->events.set_title, &c->set_title, updatetitle);
}

void destroyinputdevice(struct wl_listener *listener, void *data) {
	InputDevice *input_dev =
		wl_container_of(listener, input_dev, destroy_listener);

	// 清理设备特定数据
	if (input_dev->device_data) {
		// 根据设备类型进行特定清理
		switch (input_dev->wlr_device->type) {
		case WLR_INPUT_DEVICE_SWITCH: {
			Switch *sw = (Switch *)input_dev->device_data;
			// 移除 toggle 监听器
			wl_list_remove(&sw->toggle.link);
			// 释放 Switch 内存
			free(sw);
			break;
		}
		// 可以添加其他设备类型的清理代码
		default:
			break;
		}
		input_dev->device_data = NULL;
	}

	// 从设备列表中移除
	wl_list_remove(&input_dev->link);
	// 移除 destroy 监听器
	wl_list_remove(&input_dev->destroy_listener.link);
	// 释放内存
	free(input_dev);
}

void pointer_set_accel(struct libinput_device *device, bool natural_scrolling,
					   uint32_t mouse_accel_profile, double mouse_accel_speed) {
	libinput_device_config_scroll_set_natural_scroll_enabled(device,
															 natural_scrolling);
	if (mouse_accel_profile &&
		libinput_device_config_accel_is_available(device)) {
		libinput_device_config_accel_set_profile(device, mouse_accel_profile);
		libinput_device_config_accel_set_speed(device, mouse_accel_speed);
	} else {
		// profile cannot be directly applied to 0, need to set to 1 first
		libinput_device_config_accel_set_profile(device, 1);
		libinput_device_config_accel_set_profile(device, 0);
		libinput_device_config_accel_set_speed(device, 0);
	}
}

void configure_pointer(struct libinput_device *device) {
	if (libinput_device_config_tap_get_finger_count(device)) {
		libinput_device_config_tap_set_enabled(device, config.tap_to_click);
		libinput_device_config_tap_set_drag_enabled(device,
													config.tap_and_drag);
		libinput_device_config_tap_set_drag_lock_enabled(device,
														 config.drag_lock);
		libinput_device_config_tap_set_button_map(device, config.button_map);
		pointer_set_accel(device, config.trackpad_natural_scrolling,
						  config.trackpad_accel_profile,
						  config.trackpad_accel_speed);
	} else {
		pointer_set_accel(device, config.mouse_natural_scrolling,
						  config.mouse_accel_profile, config.mouse_accel_speed);
	}

	if (libinput_device_config_dwt_is_available(device))
		libinput_device_config_dwt_set_enabled(device,
											   config.disable_while_typing);

	if (libinput_device_config_left_handed_is_available(device))
		libinput_device_config_left_handed_set(device, config.left_handed);

	if (libinput_device_config_middle_emulation_is_available(device))
		libinput_device_config_middle_emulation_set_enabled(
			device, config.middle_button_emulation);

	if (libinput_device_config_scroll_get_methods(device) !=
		LIBINPUT_CONFIG_SCROLL_NO_SCROLL)
		libinput_device_config_scroll_set_method(device, config.scroll_method);
	if (libinput_device_config_scroll_get_methods(device) ==
		LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN)
		libinput_device_config_scroll_set_button(device, config.scroll_button);

	if (libinput_device_config_click_get_methods(device) !=
		LIBINPUT_CONFIG_CLICK_METHOD_NONE)
		libinput_device_config_click_set_method(device, config.click_method);

	if (libinput_device_config_send_events_get_modes(device))
		libinput_device_config_send_events_set_mode(device,
													config.send_events_mode);
}

void createpointer(struct wlr_pointer *pointer) {

	struct libinput_device *device = NULL;

	if (wlr_input_device_is_libinput(&pointer->base) &&
		(device = wlr_libinput_get_device_handle(&pointer->base))) {

		configure_pointer(device);

		InputDevice *input_dev = calloc(1, sizeof(InputDevice));
		input_dev->wlr_device = &pointer->base;
		input_dev->libinput_device = device;

		input_dev->destroy_listener.notify = destroyinputdevice;
		wl_signal_add(&pointer->base.events.destroy,
					  &input_dev->destroy_listener);

		wl_list_insert(&inputdevices, &input_dev->link);
	}
	wlr_cursor_attach_input_device(cursor, &pointer->base);
}

void switch_toggle(struct wl_listener *listener, void *data) {
	// 获取包含监听器的结构体
	Switch *sw = wl_container_of(listener, sw, toggle);

	// 处理切换事件
	struct wlr_switch_toggle_event *event = data;
	SwitchBinding *s;
	int32_t ji;

	for (ji = 0; ji < config.switch_bindings_count; ji++) {
		if (config.switch_bindings_count < 1)
			break;
		s = &config.switch_bindings[ji];
		if (event->switch_state == s->fold && s->func) {
			s->func(&s->arg);
			return;
		}
	}
}

void createswitch(struct wlr_switch *switch_device) {

	struct libinput_device *device = NULL;

	if (wlr_input_device_is_libinput(&switch_device->base) &&
		(device = wlr_libinput_get_device_handle(&switch_device->base))) {

		InputDevice *input_dev = calloc(1, sizeof(InputDevice));
		input_dev->wlr_device = &switch_device->base;
		input_dev->libinput_device = device;
		input_dev->device_data = NULL; // 初始化为 NULL

		input_dev->destroy_listener.notify = destroyinputdevice;
		wl_signal_add(&switch_device->base.events.destroy,
					  &input_dev->destroy_listener);

		// 创建 Switch 特定数据
		Switch *sw = calloc(1, sizeof(Switch));
		sw->wlr_switch = switch_device;
		sw->toggle.notify = switch_toggle;
		sw->input_dev = input_dev;

		// 将 Switch 指针保存到 input_device 中
		input_dev->device_data = sw;

		// 添加 toggle 监听器
		wl_signal_add(&switch_device->events.toggle, &sw->toggle);

		// 添加到全局列表
		wl_list_insert(&inputdevices, &input_dev->link);
	}
}

void createpointerconstraint(struct wl_listener *listener, void *data) {
	PointerConstraint *pointer_constraint =
		ecalloc(1, sizeof(*pointer_constraint));
	pointer_constraint->constraint = data;
	LISTEN(&pointer_constraint->constraint->events.destroy,
		   &pointer_constraint->destroy, destroypointerconstraint);

	if (!selmon || !selmon->sel)
		return;

	struct wlr_surface *focused_surface = client_surface(selmon->sel);
	if (focused_surface &&
		focused_surface == pointer_constraint->constraint->surface) {
		cursorconstrain(pointer_constraint->constraint);
	}
}

void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint) {
	if (active_constraint == constraint)
		return;

	if (active_constraint) {
		if (constraint == NULL) {
			cursorwarptohint();
		}
		wlr_pointer_constraint_v1_send_deactivated(active_constraint);
	}

	active_constraint = constraint;

	if (constraint) {
		wlr_pointer_constraint_v1_send_activated(constraint);
	}
}

void cursorframe(struct wl_listener *listener, void *data) {
	/* This event is forwarded by the cursor when a pointer emits an frame
	 * event. Frame events are sent after regular pointer events to group
	 * multiple events together. For instance, two axis events may happen at
	 * the same time, in which case a frame event won't be sent in between.
	 */
	/* Notify the client with pointer focus of the frame event. */
	wlr_seat_pointer_notify_frame(seat);
}

void cursorwarptohint(void) {
	Client *c = NULL;
	double sx = active_constraint->current.cursor_hint.x;
	double sy = active_constraint->current.cursor_hint.y;

	toplevel_from_wlr_surface(active_constraint->surface, &c, NULL);
	if (c && active_constraint->current.cursor_hint.enabled) {
		wlr_cursor_warp(cursor, NULL, sx + c->geom.x + c->bw,
						sy + c->geom.y + c->bw);
		wlr_seat_pointer_warp(active_constraint->seat, sx, sy);
	}
}

void destroydragicon(struct wl_listener *listener, void *data) {
	/* Focus enter isn't sent during drag, so refocus the focused node. */
	focusclient(focustop(selmon), 1);
	motionnotify(0, NULL, 0, 0, 0, 0);
	wl_list_remove(&listener->link);
	free(listener);
}

void destroyidleinhibitor(struct wl_listener *listener, void *data) {
	/* `data` is the wlr_surface of the idle inhibitor being destroyed,
	 * at this point the idle inhibitor is still in the list of the manager
	 */
	checkidleinhibitor(wlr_surface_get_root_surface(data));
	wl_list_remove(&listener->link);
	free(listener);
}

void destroylayernodenotify(struct wl_listener *listener, void *data) {
	LayerSurface *l = wl_container_of(listener, l, destroy);

	wl_list_remove(&l->link);
	wl_list_remove(&l->destroy.link);
	wl_list_remove(&l->map.link);
	wl_list_remove(&l->unmap.link);
	wl_list_remove(&l->surface_commit.link);
	wlr_scene_node_destroy(&l->popups->node);
	free(l);
}

void destroylock(SessionLock *lock, int32_t unlock) {
	wlr_seat_keyboard_notify_clear_focus(seat);
	if ((locked = !unlock))
		goto destroy;

	if (locked_bg->node.enabled) {
		wlr_scene_node_set_enabled(&locked_bg->node, false);
	}

	focusclient(focustop(selmon), 0);
	motionnotify(0, NULL, 0, 0, 0, 0);

destroy:
	wl_list_remove(&lock->new_surface.link);
	wl_list_remove(&lock->unlock.link);
	wl_list_remove(&lock->destroy.link);

	wlr_scene_node_destroy(&lock->scene->node);
	cur_lock = NULL;
	free(lock);
}

void destroylocksurface(struct wl_listener *listener, void *data) {
	Monitor *m = wl_container_of(listener, m, destroy_lock_surface);
	struct wlr_session_lock_surface_v1 *surface,
		*lock_surface = m->lock_surface;

	m->lock_surface = NULL;
	wl_list_remove(&m->destroy_lock_surface.link);

	if (lock_surface->surface != seat->keyboard_state.focused_surface) {
		if (exclusive_focus && !locked) {
			reset_exclusive_layers_focus(m);
		}
		return;
	}

	if (locked && cur_lock && !wl_list_empty(&cur_lock->surfaces)) {
		surface = wl_container_of(cur_lock->surfaces.next, surface, link);
		client_notify_enter(surface->surface, wlr_seat_get_keyboard(seat));
	} else if (!locked) {
		reset_exclusive_layers_focus(selmon);
	} else {
		wlr_seat_keyboard_clear_focus(seat);
	}
}

void // 0.7 custom
destroynotify(struct wl_listener *listener, void *data) {
	/* Called when the xdg_toplevel is destroyed. */
	Client *c = wl_container_of(listener, c, destroy);
	wl_list_remove(&c->destroy.link);
	wl_list_remove(&c->set_title.link);
	wl_list_remove(&c->fullscreen.link);
	wl_list_remove(&c->maximize.link);
	wl_list_remove(&c->minimize.link);
#ifdef XWAYLAND
	if (c->type != XDGShell) {
		wl_list_remove(&c->activate.link);
		wl_list_remove(&c->associate.link);
		wl_list_remove(&c->configure.link);
		wl_list_remove(&c->dissociate.link);
		wl_list_remove(&c->set_hints.link);
	} else
#endif
	{
		wl_list_remove(&c->request_move.link);
		wl_list_remove(&c->request_resize.link);
		wl_list_remove(&c->commit.link);
		wl_list_remove(&c->map.link);
		wl_list_remove(&c->unmap.link);
	}
	free(c);
}

void destroypointerconstraint(struct wl_listener *listener, void *data) {
	PointerConstraint *pointer_constraint =
		wl_container_of(listener, pointer_constraint, destroy);

	if (active_constraint == pointer_constraint->constraint) {
		cursorwarptohint();
		active_constraint = NULL;
	}

	wl_list_remove(&pointer_constraint->destroy.link);
	free(pointer_constraint);
}

void destroysessionlock(struct wl_listener *listener, void *data) {
	SessionLock *lock = wl_container_of(listener, lock, destroy);
	destroylock(lock, 0);
}

void destroykeyboardgroup(struct wl_listener *listener, void *data) {
	KeyboardGroup *group = wl_container_of(listener, group, destroy);
	wl_event_source_remove(group->key_repeat_source);
	wl_list_remove(&group->key.link);
	wl_list_remove(&group->modifiers.link);
	wl_list_remove(&group->destroy.link);
	wlr_keyboard_group_destroy(group->wlr_group);
	free(group);
}

void focusclient(Client *c, int32_t lift) {

	Client *last_focus_client = NULL;
	Monitor *um = NULL;

	struct wlr_surface *old_keyboard_focus_surface =
		seat->keyboard_state.focused_surface;

	if (locked)
		return;

	if (c && c->iskilling)
		return;

	if (c && !client_surface(c)->mapped)
		return;

	if (c && client_should_ignore_focus(c) && client_is_x11_popup(c))
		return;

	if (c && c->nofocus)
		return;

	/* Raise client in stacking order if requested */
	if (c && lift)
		wlr_scene_node_raise_to_top(&c->scene->node); // 将视图提升到顶层

	/* In floating layout mode, always raise focused floating window */
	if (c && c->mon && c->isfloating &&
	    c->mon->pertag->ltidxs[c->mon->pertag->curtag]->id == FLOATING)
		wlr_scene_node_raise_to_top(&c->scene->node);

	if (c && client_surface(c) == old_keyboard_focus_surface && selmon &&
		selmon->sel)
		return;

	if (selmon && selmon->sel && selmon->sel != c &&
		selmon->sel->foreign_toplevel) {
		wlr_foreign_toplevel_handle_v1_set_activated(
			selmon->sel->foreign_toplevel, false);
	}

	if (c && !c->iskilling && !client_is_unmanaged(c) && c->mon) {

		last_focus_client = selmon->sel;
		selmon = c->mon;
		selmon->prevsel = selmon->sel;
		selmon->sel = c;
		c->isfocusing = true;

		check_keep_idle_inhibit(c);

		if (last_focus_client && !last_focus_client->iskilling &&
			last_focus_client != c) {
			last_focus_client->isfocusing = false;
			client_set_unfocused_opacity_animation(last_focus_client);
		}

		client_set_focused_opacity_animation(c);

		// decide whether need to re-arrange

		if (c && selmon->prevsel &&
			(selmon->prevsel->tags & selmon->tagset[selmon->seltags]) &&
			(c->tags & selmon->tagset[selmon->seltags]) && !c->isfloating &&
			is_scroller_layout(selmon)) {
			arrange(selmon, false, false);
		}

		// change focus link position
		wl_list_remove(&c->flink);
		wl_list_insert(&fstack, &c->flink);

		// change border color
		c->isurgent = 0;
	}

	// update other monitor focus disappear
	wl_list_for_each(um, &mons, link) {
		if (um->wlr_output->enabled && um != selmon && um->sel &&
			!um->sel->iskilling && um->sel->isfocusing) {

			um->sel->isfocusing = false;
			client_set_unfocused_opacity_animation(um->sel);

			if (um->sel->foreign_toplevel) {
				wlr_foreign_toplevel_handle_v1_set_activated(
					um->sel->foreign_toplevel, false);
			}
		}
	}

	if (c && !c->iskilling && c->foreign_toplevel)
		wlr_foreign_toplevel_handle_v1_set_activated(c->foreign_toplevel, true);

	/* Deactivate old client if focus is changing */
	if (old_keyboard_focus_surface &&
		(!c || client_surface(c) != old_keyboard_focus_surface)) {
		/* If an exclusive_focus layer is focused, don't focus or activate
		 * the client, but only update its position in fstack to render its
		 * border with focuscolor and focus it after the exclusive_focus
		 * layer is closed. */
		Client *w = NULL;
		LayerSurface *l = NULL;
		int32_t type =
			toplevel_from_wlr_surface(old_keyboard_focus_surface, &w, &l);
		if (type == LayerShell && l->scene->node.enabled &&
			l->layer_surface->current.layer >= ZWLR_LAYER_SHELL_V1_LAYER_TOP &&
			l == exclusive_focus) {
			return;
		} else if (w && w == exclusive_focus && client_wants_focus(w)) {
			return;
			/* Don't deactivate old_keyboard_focus_surface client if the new
			 * one wants focus, as this causes issues with winecfg and
			 * probably other clients */
		} else if (w && !client_is_unmanaged(w) &&
				   (!c || !client_wants_focus(c))) {
			client_activate_surface(old_keyboard_focus_surface, 0);
		}
	}
	printstatus();

	if (!c) {

		if (selmon && selmon->sel &&
			(!VISIBLEON(selmon->sel, selmon) || selmon->sel->iskilling ||
			 !client_surface(selmon->sel)->mapped))
			selmon->sel = NULL;

		// clear text input focus state
		aether_im_relay_set_focus(aether_input_method_relay, NULL);
		wlr_seat_keyboard_notify_clear_focus(seat);
		if (active_constraint) {
			cursorconstrain(NULL);
		}
		return;
	}

	/* Change cursor surface */
	motionnotify(0, NULL, 0, 0, 0, 0);

	// set text input focus
	// must before client_notify_enter,
	// otherwise the position of text_input will be wrong.
	aether_im_relay_set_focus(aether_input_method_relay, client_surface(c));

	/* Have a client, so focus its top-level wlr_surface */
	client_notify_enter(client_surface(c), wlr_seat_get_keyboard(seat));

	/* Activate the new client */
	client_activate_surface(client_surface(c), 1);

	if (active_constraint && active_constraint->surface != client_surface(c)) {
		cursorconstrain(NULL);
	}

	struct wlr_pointer_constraint_v1 *constraint;
	wl_list_for_each(constraint, &pointer_constraints->constraints, link) {
		if (constraint->surface == client_surface(c)) {
			cursorconstrain(constraint);
			break;
		}
	}
}

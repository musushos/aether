
void set_activation_env() {
	if (!getenv("DBUS_SESSION_BUS_ADDRESS")) {
		wlr_log(WLR_INFO, "Not updating dbus execution environment: "
						  "DBUS_SESSION_BUS_ADDRESS not set");
		return;
	}

	wlr_log(WLR_INFO, "Updating dbus execution environment");

	char *env_keys = join_strings(env_vars, " ");

	// first command: dbus-update-activation-environment
	const char *arg1 = env_keys;
	char *cmd1 = string_printf("dbus-update-activation-environment %s", arg1);
	if (!cmd1) {
		wlr_log(WLR_ERROR, "Failed to allocate command string");
		goto cleanup;
	}
	spawn(&(Arg){.v = cmd1});
	free(cmd1);

	// second command: systemctl --user
	const char *action = "import-environment";
	char *cmd2 = string_printf("systemctl --user %s %s", action, env_keys);
	if (!cmd2) {
		wlr_log(WLR_ERROR, "Failed to allocate command string");
		goto cleanup;
	}
	spawn(&(Arg){.v = cmd2});
	free(cmd2);

cleanup:
	free(env_keys);
}

void // 17
run(char *startup_cmd) {

	set_env();

	/* Add a Unix socket to the Wayland display. */
	const char *socket = wl_display_add_socket_auto(dpy);
	if (!socket)
		die("startup: display_add_socket_auto");
	setenv("WAYLAND_DISPLAY", socket, 1);

	/* Start the backend. This will enumerate outputs and inputs, become the
	 * DRM master, etc */
	if (!wlr_backend_start(backend))
		die("startup: backend_start");

	/* Now that the socket exists and the backend is started, run the
	 * startup command */

	if (startup_cmd) {
		int32_t piperw[2];
		if (pipe(piperw) < 0)
			die("startup: pipe:");
		if ((child_pid = fork()) < 0)
			die("startup: fork:");
		if (child_pid == 0) {
			setsid();
			dup2(piperw[0], STDIN_FILENO);
			close(piperw[0]);
			close(piperw[1]);
			execl("/bin/sh", "/bin/sh", "-c", startup_cmd, NULL);
			die("startup: execl:");
		}
		dup2(piperw[1], STDOUT_FILENO);
		close(piperw[1]);
		close(piperw[0]);
	}

	/* Mark stdout as non-blocking to avoid people who does not close stdin
	 * nor consumes it in their startup script getting aether frozen */
	if (fd_set_nonblock(STDOUT_FILENO) < 0)
		close(STDOUT_FILENO);

	printstatus();

	/* At this point the outputs are initialized, choose initial selmon
	 * based on cursor position, and set default cursor image */
	selmon = xytomon(cursor->x, cursor->y);

	/* TODO hack to get cursor to display in its initial location (100, 100)
	 * instead of (0, 0) and then jumping. still may not be fully
	 * initialized, as the image/coordinates are not transformed for the
	 * monitor when displayed here */
	wlr_cursor_warp_closest(cursor, NULL, cursor->x, cursor->y);
	wlr_cursor_set_xcursor(cursor, cursor_mgr, "left_ptr");
	handlecursoractivity();

	set_activation_env();

	run_exec();
	run_exec_once();

	/* Run the Wayland event loop. This does not return until you exit the
	 * compositor. Starting the backend rigged up all of the necessary event
	 * loop configuration to listen to libinput events, DRM events, generate
	 * frame events at the refresh rate, and so on. */

	wl_display_run(dpy);
}

void setcursor(struct wl_listener *listener, void *data) {
	/* This event is raised by the seat when a client provides a cursor
	 * image */
	struct wlr_seat_pointer_request_set_cursor_event *event = data;
	/* If we're "grabbing" the cursor, don't use the client's image, we will
	 * restore it after "grabbing" sending a leave event, followed by a
	 * enter event, which will result in the client requesting set the
	 * cursor surface
	 */
	if (cursor_mode != CurNormal && cursor_mode != CurPressed)
		return;
	/* This can be sent by any client, so we check to make sure this one is
	 * actually has pointer focus first. If so, we can tell the cursor to
	 * use the provided surface as the cursor image. It will set the
	 * hardware cursor on the output that it's currently on and continue to
	 * do so as the cursor moves between outputs. */
	if (event->seat_client == seat->pointer_state.focused_client) {
		/* Clear previous surface destroy listener if any */
		if (last_cursor.surface &&
			last_cursor_surface_destroy_listener.link.prev != NULL)
			wl_list_remove(&last_cursor_surface_destroy_listener.link);

		last_cursor.shape = 0;
		last_cursor.surface = event->surface;
		last_cursor.hotspot_x = event->hotspot_x;
		last_cursor.hotspot_y = event->hotspot_y;

		/* Track surface destruction to avoid dangling pointer */
		if (event->surface)
			wl_signal_add(&event->surface->events.destroy,
						  &last_cursor_surface_destroy_listener);

		if (!cursor_hidden)
			wlr_cursor_set_surface(cursor, event->surface, event->hotspot_x,
								   event->hotspot_y);
	}
}

void // 0.5
setfloating(Client *c, int32_t floating) {

	Client *fc = NULL;
	struct wlr_box target_box;
	int32_t old_floating_state = c->isfloating;
	c->isfloating = floating;
	bool window_size_outofrange = false;

	if (!c || !c->mon || !client_surface(c)->mapped || c->iskilling)
		return;

	target_box = c->geom;

	if (floating == 1 && c != grabc) {

		if (c->isfullscreen) {
			client_pending_fullscreen_state(c, 0);
			client_set_fullscreen(c, 0);
		}

		client_pending_maximized_state(c, 0);
		exit_scroller_stack(c);

		// 重新计算居中的坐标
		if (!client_is_x11(c) && !c->iscustompos)
			target_box =
				setclient_coordinate_center(c, c->mon, target_box, 0, 0);
		else
			target_box = c->geom;

		// restore to the memeroy geom
		if (c->float_geom.width > 0 && c->float_geom.height > 0) {
			if (c->mon &&
				c->float_geom.width >= c->mon->w.width - config.gappoh) {
				c->float_geom.width = c->mon->w.width * 0.9;
				window_size_outofrange = true;
			}
			if (c->mon &&
				c->float_geom.height >= c->mon->w.height - config.gappov) {
				c->float_geom.height = c->mon->w.height * 0.9;
				window_size_outofrange = true;
			}
			if (window_size_outofrange) {
				c->float_geom =
					setclient_coordinate_center(c, c->mon, c->float_geom, 0, 0);
			}
			resize(c, c->float_geom, 0);
		} else {
			resize(c, target_box, 0);
		}

		c->need_float_size_reduce = 0;
	} else if (c->isfloating && c == grabc) {
		c->need_float_size_reduce = 0;
	} else {
		c->need_float_size_reduce = 1;
		c->is_scratchpad_show = 0;
		c->is_in_scratchpad = 0;
		c->isnamedscratchpad = 0;
		// 让当前tag中的全屏窗口退出全屏参与平铺
		wl_list_for_each(fc, &clients,
						 link) if (fc && fc != c && VISIBLEON(fc, c->mon) &&
								   c->tags & fc->tags && ISFULLSCREEN(fc) &&
								   old_floating_state) {
			clear_fullscreen_flag(fc);
		}
	}

	if (c->isoverlay) {
		wlr_scene_node_reparent(&c->scene->node, layers[LyrOverlay]);
	} else if (client_should_overtop(c) && c->isfloating) {
		wlr_scene_node_reparent(&c->scene->node, layers[LyrTop]);
	} else {
		wlr_scene_node_reparent(&c->scene->node,
								layers[c->isfloating ? LyrFloat : LyrTile]);
	}

	if (!c->isfloating && old_floating_state &&
		(c->old_stack_inner_per > 0.0f || c->old_master_inner_per > 0.0f)) {
		restore_size_per(c->mon, c);
	}

	if (c->isfloating && !old_floating_state) {
		save_old_size_per(c->mon);
	}

	if (!c->force_fakemaximize)
		client_set_maximized(c, false);

	if (!c->isfloating || c->force_tiled_state) {
		client_set_tiled(c, WLR_EDGE_TOP | WLR_EDGE_BOTTOM | WLR_EDGE_LEFT |
								WLR_EDGE_RIGHT);
	} else {
		client_set_tiled(c, WLR_EDGE_NONE);
	}

	arrange(c->mon, false, false);

	if (!c->isfloating) {
		c->old_master_inner_per = c->master_inner_per;
		c->old_stack_inner_per = c->stack_inner_per;
	}

	setborder_color(c);
	printstatus();
}

void reset_maximizescreen_size(Client *c) {
	c->geom.x = c->mon->w.x + config.gappoh;
	c->geom.y = c->mon->w.y + config.gappov;
	c->geom.width = c->mon->w.width - 2 * config.gappoh;
	c->geom.height = c->mon->w.height - 2 * config.gappov;
	resize(c, c->geom, 0);
}

void exit_scroller_stack(Client *c) {
	if (!c || !c->mon)
		return;

	uint32_t tag = c->mon->pertag->curtag;
	struct TagScrollerState *st = c->mon->pertag->scroller_state[tag];
	if (st) {
		struct ScrollerStackNode *n = find_scroller_node(st, c);
		if (n) {
			scroller_node_remove(st, n);
			return; /* 节点已移除，客户端指针已在函数内清空 */
		}
	}
}

void setmaximizescreen(Client *c, int32_t maximizescreen) {
	struct wlr_box maximizescreen_box;
	if (!c || !c->mon || !client_surface(c)->mapped || c->iskilling)
		return;

	if (c->mon->isoverview)
		return;

	int32_t old_maximizescreen_state = c->ismaximizescreen;
	client_pending_maximized_state(c, maximizescreen);

	if (maximizescreen) {

		if (c->isfullscreen) {
			client_pending_fullscreen_state(c, 0);
			client_set_fullscreen(c, 0);
		}

		exit_scroller_stack(c);

		maximizescreen_box.x = c->mon->w.x + config.gappoh;
		maximizescreen_box.y = c->mon->w.y + config.gappov;
		maximizescreen_box.width = c->mon->w.width - 2 * config.gappoh;
		maximizescreen_box.height = c->mon->w.height - 2 * config.gappov;
		wlr_scene_node_raise_to_top(&c->scene->node);
		if (!is_scroller_layout(c->mon) || c->isfloating)
			resize(c, maximizescreen_box, 0);
	} else {
		c->bw = c->isnoborder ? 0 : config.borderpx;
		if (c->isfloating)
			setfloating(c, 1);
	}

	wlr_scene_node_reparent(&c->scene->node,
							layers[c->isfloating ? LyrFloat : LyrTile]);
	if (!c->ismaximizescreen && old_maximizescreen_state) {
		restore_size_per(c->mon, c);
	}

	if (c->ismaximizescreen && !old_maximizescreen_state) {
		save_old_size_per(c->mon);
	}

	if (!c->force_fakemaximize && !c->ismaximizescreen) {
		client_set_maximized(c, false);
	} else if (!c->force_fakemaximize && c->ismaximizescreen) {
		client_set_maximized(c, true);
	}

	arrange(c->mon, false, false);
}

void setfakefullscreen(Client *c, int32_t fakefullscreen) {
	c->isfakefullscreen = fakefullscreen;
	if (!c->mon)
		return;

	if (c->isfullscreen)
		setfullscreen(c, 0);

	client_set_fullscreen(c, fakefullscreen);
}

void setfullscreen(Client *c, int32_t fullscreen) // 用自定义全屏代理自带全屏
{

	if (!c || !c->mon || !client_surface(c)->mapped || c->iskilling)
		return;

	if (c->mon->isoverview)
		return;

	int32_t old_fullscreen_state = c->isfullscreen;
	c->isfullscreen = fullscreen;

	client_set_fullscreen(c, fullscreen);
	client_pending_fullscreen_state(c, fullscreen);

	if (fullscreen) {

		if (c->ismaximizescreen && !c->force_fakemaximize) {
			client_set_maximized(c, false);
		}

		client_pending_maximized_state(c, 0);

		exit_scroller_stack(c);
		c->isfakefullscreen = 0;

		c->bw = 0;
		wlr_scene_node_raise_to_top(&c->scene->node); // 将视图提升到顶层
		if (!is_scroller_layout(c->mon) || c->isfloating)
			resize(c, c->mon->m, 1);
	} else {
		c->bw = c->isnoborder ? 0 : config.borderpx;
		if (c->isfloating)
			setfloating(c, 1);
	}

	if (c->isoverlay) {
		wlr_scene_node_reparent(&c->scene->node, layers[LyrOverlay]);
	} else if (client_should_overtop(c) && c->isfloating) {
		wlr_scene_node_reparent(&c->scene->node, layers[LyrTop]);
	} else {
		wlr_scene_node_reparent(
			&c->scene->node,
			layers[fullscreen ? LyrTop : (c->isfloating ? LyrFloat : LyrTile)]);
	}

	if (!c->isfullscreen && old_fullscreen_state) {
		restore_size_per(c->mon, c);
	}

	if (c->isfullscreen && !old_fullscreen_state) {
		save_old_size_per(c->mon);
	}

	arrange(c->mon, false, false);
}

void setgaps(int32_t oh, int32_t ov, int32_t ih, int32_t iv) {
	selmon->gappoh = MAX(oh, 0);
	selmon->gappov = MAX(ov, 0);
	selmon->gappih = MAX(ih, 0);
	selmon->gappiv = MAX(iv, 0);
	arrange(selmon, false, false);
}

void reset_keyboard_layout(void) {
	if (!kb_group || !kb_group->wlr_group || !seat) {
		wlr_log(WLR_ERROR, "Invalid keyboard group or seat");
		return;
	}

	struct wlr_keyboard *keyboard = &kb_group->wlr_group->keyboard;
	if (!keyboard || !keyboard->keymap) {
		wlr_log(WLR_ERROR, "Invalid keyboard or keymap");
		return;
	}

	// Get current layout
	xkb_layout_index_t current = xkb_state_serialize_layout(
		keyboard->xkb_state, XKB_STATE_LAYOUT_EFFECTIVE);
	const int32_t num_layouts = xkb_keymap_num_layouts(keyboard->keymap);
	if (num_layouts < 1) {
		wlr_log(WLR_INFO, "No layouts available");
		return;
	}

	// Create context
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!context) {
		wlr_log(WLR_ERROR, "Failed to create XKB context");
		return;
	}

	struct xkb_keymap *new_keymap = xkb_keymap_new_from_names(
		context, &config.xkb_rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!new_keymap) {
		// 理论上这里不应该失败，因为前面已经验证过了
		wlr_log(WLR_ERROR,
				"Unexpected failure to create keymap after validation");
		goto cleanup_context;
	}

	// 验证新keymap是否有布局
	const int32_t new_num_layouts = xkb_keymap_num_layouts(new_keymap);
	if (new_num_layouts < 1) {
		wlr_log(WLR_ERROR, "New keymap has no layouts");
		xkb_keymap_unref(new_keymap);
		goto cleanup_context;
	}

	// 确保当前布局索引在新keymap中有效
	if (current >= new_num_layouts) {
		wlr_log(WLR_INFO,
				"Current layout index %u out of range for new keymap, "
				"resetting to 0",
				current);
		current = 0;
	}

	// Apply the new keymap
	uint32_t depressed = keyboard->modifiers.depressed;
	uint32_t latched = keyboard->modifiers.latched;
	uint32_t locked = keyboard->modifiers.locked;

	wlr_keyboard_set_keymap(keyboard, new_keymap);

	wlr_keyboard_notify_modifiers(keyboard, depressed, latched, locked, 0);
	keyboard->modifiers.group = current; // Keep the same layout index

	// Update seat
	wlr_seat_set_keyboard(seat, keyboard);
	wlr_seat_keyboard_notify_modifiers(seat, &keyboard->modifiers);

	InputDevice *id;
	wl_list_for_each(id, &inputdevices, link) {
		if (id->wlr_device->type != WLR_INPUT_DEVICE_KEYBOARD) {
			continue;
		}

		struct wlr_keyboard *tkb = (struct wlr_keyboard *)id->device_data;

		wlr_keyboard_set_keymap(tkb, keyboard->keymap);
		wlr_keyboard_notify_modifiers(tkb, depressed, latched, locked, 0);
		tkb->modifiers.group = 0;

		// 7. 更新 seat
		wlr_seat_set_keyboard(seat, tkb);
		wlr_seat_keyboard_notify_modifiers(seat, &tkb->modifiers);
	}

	// Cleanup
	xkb_keymap_unref(new_keymap);

cleanup_context:
	xkb_context_unref(context);
}

void setmon(Client *c, Monitor *m, uint32_t newtags, bool focus) {
	Monitor *oldmon = c->mon;

	if (oldmon == m)
		return;

	if (oldmon && oldmon->sel == c) {
		oldmon->sel = NULL;
	}

	if (oldmon && oldmon->prevsel == c) {
		oldmon->prevsel = NULL;
	}

	c->mon = m;

	/* Scene graph sends surface leave/enter events on move and resize */
	if (oldmon)
		arrange(oldmon, false, false);
	if (m) {
		/* Make sure window actually overlaps with the monitor */
		reset_foreign_tolevel(c, oldmon, m);
		resize(c, c->geom, 0);
		client_reset_mon_tags(c, m, newtags);
		check_match_tag_floating_rule(c, m);
		/* In floating compositor mode, force new windows to float */
		if (!c->isfloating && !client_is_unmanaged(c) &&
		    m->pertag->isFloatingCompositorMode[
		        get_tags_first_tag_num(c->tags ? c->tags : m->tagset[m->seltags])])
			c->isfloating = 1;
		setfloating(c, c->isfloating);
		setfullscreen(c, c->isfullscreen); /* This will call arrange(c->mon) */
	}

	if (focus && !client_is_x11_popup(c)) {
		focusclient(focustop(selmon), 1);
	}
}

void setpsel(struct wl_listener *listener, void *data) {
	/* This event is raised by the seat when a client wants to set the
	 * selection, usually when the user copies something. wlroots allows
	 * compositors to ignore such requests if they so choose, but in aether we
	 * always honor
	 */
	struct wlr_seat_request_set_primary_selection_event *event = data;
	wlr_seat_set_primary_selection(seat, event->source, event->serial);
}

void setsel(struct wl_listener *listener, void *data) {
	/* This event is raised by the seat when a client wants to set the
	 * selection, usually when the user copies something. wlroots allows
	 * compositors to ignore such requests if they so choose, but in aether we
	 * always honor
	 */
	struct wlr_seat_request_set_selection_event *event = data;
	wlr_seat_set_selection(seat, event->source, event->serial);
}

void show_hide_client(Client *c) {
	uint32_t target = 1;

	set_size_per(c->mon, c);
	target = get_tags_first_tag(c->oldtags);

	if (!c->is_in_scratchpad) {
		tag_client(&(Arg){.ui = target}, c);
	} else {
		c->tags = c->oldtags;
		arrange(c->mon, false, false);
	}
	client_pending_minimized_state(c, 0);
	focusclient(c, 1);

	if (c->foreign_toplevel)
		wlr_foreign_toplevel_handle_v1_set_activated(c->foreign_toplevel, true);
}

void create_output(struct wlr_backend *backend, void *data) {
	bool *done = data;
	if (*done) {
		return;
	}

	if (wlr_backend_is_wl(backend)) {
		wlr_wl_output_create(backend);
		*done = true;
	} else if (wlr_backend_is_headless(backend)) {
		wlr_headless_add_output(backend, 1920, 1080);
		*done = true;
	}
#if WLR_HAS_X11_BACKEND
	else if (wlr_backend_is_x11(backend)) {
		wlr_x11_output_create(backend);
		*done = true;
	}
#endif
}

// 修改信号处理函数，接收掩码参数
void handle_print_status(struct wl_listener *listener, void *data) {

	Monitor *m = NULL;
	wl_list_for_each(m, &mons, link) {
		if (!m->wlr_output->enabled) {
			continue;
		}
		aether_ext_workspace_printstatus(m);

		aether_ipc_output_printstatus(m);
	}
}

void setup(void) {

	setenv("XDG_CURRENT_DESKTOP", "aether", 1);
	setenv("_JAVA_AWT_WM_NONREPARENTING", "1", 1);

	parse_config();
	if (cli_debug_log) {
		config.log_level = WLR_DEBUG;
	}
	init_baked_points();

	int32_t drm_fd, i, sig[] = {SIGCHLD, SIGINT, SIGTERM, SIGPIPE};
	struct sigaction sa = {.sa_flags = SA_RESTART, .sa_handler = handlesig};
	sigemptyset(&sa.sa_mask);

	for (i = 0; i < LENGTH(sig); i++)
		sigaction(sig[i], &sa, NULL);

	wlr_log_init(config.log_level, NULL);

	/* The Wayland display is managed by libwayland. It handles accepting
	 * clients from the Unix socket, manging Wayland globals, and so on. */
	dpy = wl_display_create();
	event_loop = wl_display_get_event_loop(dpy);
	/* The backend is a wlroots feature which abstracts the underlying input
	 * and output hardware. The autocreate option will choose the most
	 * suitable backend based on the current environment, such as opening an
	 * X11 window if an X11 server is running. The NULL argument here
	 * optionally allows you to pass in a custom renderer if wlr_renderer
	 * doesn't meet your needs. The backend uses the renderer, for example,
	 * to fall back to software cursors if the backend does not support
	 * hardware cursors (some older GPUs don't). */
	if (!(backend = wlr_backend_autocreate(event_loop, &session)))
		die("couldn't create backend");

	headless_backend = wlr_headless_backend_create(event_loop);
	if (!headless_backend) {
		wlr_log(WLR_ERROR, "Failed to create secondary headless backend");
	} else {
		wlr_multi_backend_add(backend, headless_backend);
	}

	/* Initialize the scene graph used to lay out windows */
	scene = wlr_scene_create();
	root_bg = wlr_scene_rect_create(&scene->tree, 0, 0, config.rootcolor);
	for (i = 0; i < NUM_LAYERS; i++)
		layers[i] = wlr_scene_tree_create(&scene->tree);
	drag_icon = wlr_scene_tree_create(&scene->tree);
	wlr_scene_node_place_below(&drag_icon->node, &layers[LyrBlock]->node);

	/* Create a renderer with the default implementation */
	if (!(drw = fx_renderer_create(backend)))
		die("couldn't create renderer");

	wl_signal_add(&drw->events.lost, &gpu_reset);

	/* Create shm, drm and linux_dmabuf interfaces by ourselves.
	 * The simplest way is call:
	 *      wlr_renderer_init_wl_display(drw);
	 * but we need to create manually the linux_dmabuf interface to
	 * integrate it with wlr_scene. */
	wlr_renderer_init_wl_shm(drw, dpy);

	if (wlr_renderer_get_texture_formats(drw, WLR_BUFFER_CAP_DMABUF)) {
		wlr_drm_create(dpy, drw);
		wlr_scene_set_linux_dmabuf_v1(
			scene, wlr_linux_dmabuf_v1_create_with_renderer(dpy, 4, drw));
	}

	if (config.syncobj_enable && (drm_fd = wlr_renderer_get_drm_fd(drw)) >= 0 &&
		drw->features.timeline && backend->features.timeline)
		wlr_linux_drm_syncobj_manager_v1_create(dpy, 1, drm_fd);

	/* Create a default allocator */
	if (!(alloc = wlr_allocator_autocreate(backend, drw)))
		die("couldn't create allocator");

	/* This creates some hands-off wlroots interfaces. The compositor is
	 * necessary for clients to allocate surfaces and the data device
	 * manager handles the clipboard. Each of these wlroots interfaces has
	 * room for you to dig your fingers in and play with their behavior if
	 * you want. Note that the clients cannot set the selection directly
	 * without compositor approval, see the setsel() function. */
	compositor = wlr_compositor_create(dpy, 6, drw);
	wlr_export_dmabuf_manager_v1_create(dpy);
	wlr_screencopy_manager_v1_create(dpy);
	wlr_ext_image_copy_capture_manager_v1_create(dpy, 1);
	wlr_ext_output_image_capture_source_manager_v1_create(dpy, 1);
	wlr_data_control_manager_v1_create(dpy);
	wlr_data_device_manager_create(dpy);
	wlr_primary_selection_v1_device_manager_create(dpy);
	wlr_viewporter_create(dpy);
	wlr_single_pixel_buffer_manager_v1_create(dpy);
	wlr_fractional_scale_manager_v1_create(dpy, 1);
	wlr_presentation_create(dpy, backend, 2);
	wlr_subcompositor_create(dpy);
	wlr_alpha_modifier_v1_create(dpy);
	wlr_ext_data_control_manager_v1_create(dpy, 1);

	// 在 setup 函数中
	wl_signal_init(&aether_print_status);
	wl_signal_add(&aether_print_status, &print_status_listener);

	/* Initializes the interface used to implement urgency hints */
	activation = wlr_xdg_activation_v1_create(dpy);
	wl_signal_add(&activation->events.request_activate, &request_activate);

	wlr_scene_set_gamma_control_manager_v1(
		scene, wlr_gamma_control_manager_v1_create(dpy));

	power_mgr = wlr_output_power_manager_v1_create(dpy);
	wl_signal_add(&power_mgr->events.set_mode, &output_power_mgr_set_mode);

	tearing_control = wlr_tearing_control_manager_v1_create(dpy, 1);
	tearing_new_object.notify = handle_tearing_new_object;
	wl_signal_add(&tearing_control->events.new_object, &tearing_new_object);

	/* Creates an output layout, which a wlroots utility for working with an
	 * arrangement of screens in a physical layout. */
	output_layout = wlr_output_layout_create(dpy);
	wl_signal_add(&output_layout->events.change, &layout_change);
	wlr_xdg_output_manager_v1_create(dpy, output_layout);

	/* Configure a listener to be notified when new outputs are available on
	 * the backend. */
	wl_list_init(&mons);
	wl_signal_add(&backend->events.new_output, &new_output);

	/* Set up our client lists and the xdg-shell. The xdg-shell is a
	 * Wayland protocol which is used for application windows. For more
	 * detail on shells, refer to the article:
	 *
	 * https://drewdevault.com/2018/07/29/Wayland-shells.html
	 */
	wl_list_init(&clients);
	wl_list_init(&fstack);
	wl_list_init(&fadeout_clients);
	wl_list_init(&fadeout_layers);

	idle_notifier = wlr_idle_notifier_v1_create(dpy);

	idle_inhibit_mgr = wlr_idle_inhibit_v1_create(dpy);
	wl_signal_add(&idle_inhibit_mgr->events.new_inhibitor, &new_idle_inhibitor);

	keep_idle_inhibit_source = wl_event_loop_add_timer(
		wl_display_get_event_loop(dpy), keep_idle_inhibit, NULL);

	layer_shell = wlr_layer_shell_v1_create(dpy, 4);
	wl_signal_add(&layer_shell->events.new_surface, &new_layer_surface);

	xdg_shell = wlr_xdg_shell_create(dpy, 6);
	wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel);
	wl_signal_add(&xdg_shell->events.new_popup, &new_xdg_popup);

	session_lock_mgr = wlr_session_lock_manager_v1_create(dpy);
	wl_signal_add(&session_lock_mgr->events.new_lock, &new_session_lock);

	locked_bg =
		wlr_scene_rect_create(layers[LyrBlock], sgeom.width, sgeom.height,
							  (float[4]){0.1, 0.1, 0.1, 1.0});
	wlr_scene_node_set_enabled(&locked_bg->node, false);

	/* Use decoration protocols to negotiate server-side decorations */
	wlr_server_decoration_manager_set_default_mode(
		wlr_server_decoration_manager_create(dpy),
		WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);
	xdg_decoration_mgr = wlr_xdg_decoration_manager_v1_create(dpy);
	wl_signal_add(&xdg_decoration_mgr->events.new_toplevel_decoration,
				  &new_xdg_decoration);

	pointer_constraints = wlr_pointer_constraints_v1_create(dpy);
	wl_signal_add(&pointer_constraints->events.new_constraint,
				  &new_pointer_constraint);

	relative_pointer_mgr = wlr_relative_pointer_manager_v1_create(dpy);

	/*
	 * Creates a cursor, which is a wlroots utility for tracking the cursor
	 * image shown on screen.
	 */
	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, output_layout);

	/* Creates an xcursor manager, another wlroots utility which loads up
	 * Xcursor themes to source cursor images from and makes sure that
	 * cursor images are available at all scale factors on the screen
	 * (necessary for HiDPI support). Scaled cursors will be loaded with
	 * each output. */

	set_xcursor_env();

	cursor_mgr =
		wlr_xcursor_manager_create(config.cursor_theme, config.cursor_size);
	/*
	 * wlr_cursor *only* displays an image on screen. It does not move
	 * around when the pointer moves. However, we can attach input devices
	 * to it, and it will generate aggregate events for all of them. In
	 * these events, we can choose how we want to process them, forwarding
	 * them to clients and moving the cursor around. More detail on this
	 * process is described in my input handling blog post:
	 *
	 * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html
	 *
	 * And more comments are sprinkled throughout the notify functions
	 * above.
	 */
	wl_signal_add(&cursor->events.motion, &cursor_motion);
	wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);
	wl_signal_add(&cursor->events.button, &cursor_button);
	wl_signal_add(&cursor->events.axis, &cursor_axis);
	wl_signal_add(&cursor->events.frame, &cursor_frame);

	// 这两句代码会造成obs窗口里的鼠标光标消失,不知道注释有什么影响
	cursor_shape_mgr = wlr_cursor_shape_manager_v1_create(dpy, 1);
	wl_signal_add(&cursor_shape_mgr->events.request_set_shape,
				  &request_set_cursor_shape);
	hide_cursor_source = wl_event_loop_add_timer(wl_display_get_event_loop(dpy),
												 hidecursor, cursor);
	/*
	 * Configures a seat, which is a single "seat" at which a user sits and
	 * operates the computer. This conceptually includes up to one keyboard,
	 * pointer, touch, and drawing tablet device. We also rig up a listener
	 * to let us know when new input devices are available on the backend.
	 */
	wl_list_init(&inputdevices);
	wl_list_init(&keyboard_shortcut_inhibitors);
	wl_signal_add(&backend->events.new_input, &new_input_device);
	virtual_keyboard_mgr = wlr_virtual_keyboard_manager_v1_create(dpy);
	wl_signal_add(&virtual_keyboard_mgr->events.new_virtual_keyboard,
				  &new_virtual_keyboard);
	virtual_pointer_mgr = wlr_virtual_pointer_manager_v1_create(dpy);
	wl_signal_add(&virtual_pointer_mgr->events.new_virtual_pointer,
				  &new_virtual_pointer);

	pointer_gestures = wlr_pointer_gestures_v1_create(dpy);
	LISTEN_STATIC(&cursor->events.swipe_begin, swipe_begin);
	LISTEN_STATIC(&cursor->events.swipe_update, swipe_update);
	LISTEN_STATIC(&cursor->events.swipe_end, swipe_end);
	LISTEN_STATIC(&cursor->events.pinch_begin, pinch_begin);
	LISTEN_STATIC(&cursor->events.pinch_update, pinch_update);
	LISTEN_STATIC(&cursor->events.pinch_end, pinch_end);
	LISTEN_STATIC(&cursor->events.hold_begin, hold_begin);
	LISTEN_STATIC(&cursor->events.hold_end, hold_end);

	seat = wlr_seat_create(dpy, "seat0");

	wl_list_init(&last_cursor_surface_destroy_listener.link);
	wl_signal_add(&seat->events.request_set_cursor, &request_cursor);
	wl_signal_add(&seat->events.request_set_selection, &request_set_sel);
	wl_signal_add(&seat->events.request_set_primary_selection,
				  &request_set_psel);
	wl_signal_add(&seat->events.request_start_drag, &request_start_drag);
	wl_signal_add(&seat->events.start_drag, &start_drag);

	kb_group = createkeyboardgroup();
	wl_list_init(&kb_group->destroy.link);

	keyboard_shortcuts_inhibit = wlr_keyboard_shortcuts_inhibit_v1_create(dpy);
	wl_signal_add(&keyboard_shortcuts_inhibit->events.new_inhibitor,
				  &keyboard_shortcuts_inhibit_new_inhibitor);

	output_mgr = wlr_output_manager_v1_create(dpy);
	wl_signal_add(&output_mgr->events.apply, &output_mgr_apply);
	wl_signal_add(&output_mgr->events.test, &output_mgr_test);

	wlr_scene_set_blur_data(
		scene, config.blur_params.num_passes, config.blur_params.radius,
		config.blur_params.noise, config.blur_params.brightness,
		config.blur_params.contrast, config.blur_params.saturation);

	/* create text_input-, and input_method-protocol relevant globals */
	input_method_manager = wlr_input_method_manager_v2_create(dpy);
	text_input_manager = wlr_text_input_manager_v3_create(dpy);

	aether_input_method_relay = aether_im_relay_create();

	drm_lease_manager = wlr_drm_lease_v1_manager_create(dpy, backend);
	if (drm_lease_manager) {
		wl_signal_add(&drm_lease_manager->events.request, &drm_lease_request);
	} else {
		wlr_log(WLR_DEBUG, "Failed to create wlr_drm_lease_device_v1.");
		wlr_log(WLR_INFO, "VR will not be available.");
	}

	wl_global_create(dpy, &zaether_ipc_manager_v2_interface, 2, NULL,
					 aether_ipc_manager_bind);

	// 创建顶层管理句柄
	foreign_toplevel_manager = wlr_foreign_toplevel_manager_v1_create(dpy);
	struct wlr_xdg_foreign_registry *foreign_registry =
		wlr_xdg_foreign_registry_create(dpy);
	wlr_xdg_foreign_v1_create(dpy, foreign_registry);
	wlr_xdg_foreign_v2_create(dpy, foreign_registry);

	// ext-workspace协议
	workspaces_init();
#ifdef XWAYLAND
	/*
	 * Initialise the XWayland X server.
	 * It will be started when the first X client is started.
	 */
	xwayland =
		wlr_xwayland_create(dpy, compositor, !config.xwayland_persistence);
	if (xwayland) {
		wl_signal_add(&xwayland->events.ready, &xwayland_ready);
		wl_signal_add(&xwayland->events.new_surface, &new_xwayland_surface);

		setenv("DISPLAY", xwayland->display_name, 1);
	} else {
		fprintf(stderr,
				"failed to setup XWayland X server, continuing without it\n");
	}
	sync_keymap = wl_event_loop_add_timer(wl_display_get_event_loop(dpy),
										  synckeymap, NULL);
#endif
}

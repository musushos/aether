void client_change_mon(Client *c, Monitor *m) {
	setmon(c, m, c->tags, true);
	if (c->isfloating) {
		c->float_geom = c->geom =
			setclient_coordinate_center(c, c->mon, c->geom, 0, 0);
	}
}

void applybounds(Client *c, struct wlr_box *bbox) {
	/* set minimum possible */
	c->geom.width = MAX(1 + 2 * (int32_t)c->bw, c->geom.width);
	c->geom.height = MAX(1 + 2 * (int32_t)c->bw, c->geom.height);

	if (c->geom.x >= bbox->x + bbox->width)
		c->geom.x = bbox->x + bbox->width - c->geom.width;
	if (c->geom.y >= bbox->y + bbox->height)
		c->geom.y = bbox->y + bbox->height - c->geom.height;
	if (c->geom.x + c->geom.width <= bbox->x)
		c->geom.x = bbox->x;
	if (c->geom.y + c->geom.height <= bbox->y)
		c->geom.y = bbox->y;
}

void clear_fullscreen_and_maximized_state(Monitor *m) {
	Client *fc = NULL;
	wl_list_for_each(fc, &clients, link) {
		if (fc && VISIBLEON(fc, m) && ISFULLSCREEN(fc)) {
			clear_fullscreen_flag(fc);
		}
	}
}

/*清除全屏标志,还原全屏时清0的border*/
void clear_fullscreen_flag(Client *c) {

	if ((c->mon->pertag->ltidxs[get_tags_first_tag_num(c->tags)]->id ==
			 SCROLLER ||
		 c->mon->pertag->ltidxs[get_tags_first_tag_num(c->tags)]->id ==
			 VERTICAL_SCROLLER) &&
		!c->isfloating) {
		return;
	}

	if (c->isfullscreen) {
		setfullscreen(c, false);
	}

	if (c->ismaximizescreen) {
		setmaximizescreen(c, 0);
	}
}

void client_pending_fullscreen_state(Client *c, int32_t isfullscreen) {
	c->isfullscreen = isfullscreen;

	if (c->foreign_toplevel && !c->iskilling)
		wlr_foreign_toplevel_handle_v1_set_fullscreen(c->foreign_toplevel,
													  isfullscreen);
}

void client_pending_maximized_state(Client *c, int32_t ismaximized) {
	c->ismaximizescreen = ismaximized;
	if (c->foreign_toplevel && !c->iskilling)
		wlr_foreign_toplevel_handle_v1_set_maximized(c->foreign_toplevel,
													 ismaximized);
}

void client_pending_minimized_state(Client *c, int32_t isminimized) {
	c->isminimized = isminimized;
	if (c->foreign_toplevel && !c->iskilling)
		wlr_foreign_toplevel_handle_v1_set_minimized(c->foreign_toplevel,
													 isminimized);
}

void show_scratchpad(Client *c) {
	c->is_scratchpad_show = 1;
	if (c->isfullscreen || c->ismaximizescreen) {
		client_pending_fullscreen_state(c, 0);
		client_pending_maximized_state(c, 0);
		c->bw = c->isnoborder ? 0 : config.borderpx;
	}

	/* return if fullscreen */
	if (!c->isfloating) {
		setfloating(c, 1);
		c->geom.width = c->iscustomsize
							? c->float_geom.width
							: c->mon->w.width * config.scratchpad_width_ratio;
		c->geom.height =
			c->iscustomsize ? c->float_geom.height
							: c->mon->w.height * config.scratchpad_height_ratio;
		// 重新计算居中的坐标
		c->float_geom = c->geom = c->animainit_geom = c->animation.current =
			setclient_coordinate_center(c, c->mon, c->geom, 0, 0);
		c->iscustomsize = 1;
		resize(c, c->geom, 0);
	}

	c->oldtags = c->mon->tagset[c->mon->seltags];
	wl_list_remove(&c->link);					  // 从原来位置移除
	wl_list_insert(clients.prev->next, &c->link); // 插入开头
	show_hide_client(c);
	setborder_color(c);
}

void client_update_oldmonname_record(Client *c, Monitor *m) {
	if (!c || c->iskilling || !client_surface(c)->mapped)
		return;
	memset(c->oldmonname, 0, sizeof(c->oldmonname));
	strncpy(c->oldmonname, m->wlr_output->name, sizeof(c->oldmonname) - 1);
	c->oldmonname[sizeof(c->oldmonname) - 1] = '\0';
}

void swallow(Client *c, Client *w) {
	c->bw = w->bw;
	c->isfloating = w->isfloating;
	c->isurgent = w->isurgent;
	c->is_in_scratchpad = w->is_in_scratchpad;
	c->is_scratchpad_show = w->is_scratchpad_show;
	c->tags = w->tags;
	c->geom = w->geom;
	c->float_geom = w->float_geom;
	c->stack_inner_per = w->stack_inner_per;
	c->master_inner_per = w->master_inner_per;
	c->master_mfact_per = w->master_mfact_per;
	c->scroller_proportion = w->scroller_proportion;
	c->isglobal = w->isglobal;

	/* 调整 w 的邻居指针，让它们指向 c */
	c->stack_proportion = w->stack_proportion;

	/* 全局链表替换 */
	wl_list_insert(&w->link, &c->link);
	wl_list_insert(&w->flink, &c->flink);

	if (w->foreign_toplevel) {
		wlr_foreign_toplevel_handle_v1_output_leave(w->foreign_toplevel,
													w->mon->wlr_output);
		wlr_foreign_toplevel_handle_v1_destroy(w->foreign_toplevel);
		w->foreign_toplevel = NULL;
	}

	wlr_scene_node_set_enabled(&w->scene->node, false);
	wlr_scene_node_set_enabled(&c->scene->node, true);
	wlr_scene_node_set_enabled(&c->scene_surface->node, true);

	if (!c->foreign_toplevel && c->mon)
		add_foreign_toplevel(c);
	else if (c->foreign_toplevel && c->mon) {
		wlr_foreign_toplevel_handle_v1_output_enter(c->foreign_toplevel,
													c->mon->wlr_output);
	}

	client_pending_fullscreen_state(c, w->isfullscreen);
	client_pending_maximized_state(c, w->ismaximizescreen);
	client_pending_minimized_state(c, w->isminimized);

	/* ---------- 跨 tag 同步：dwindle 与 scroller ---------- */
	Monitor *m;
	wl_list_for_each(m, &mons, link) {
		for (uint32_t t = 0; t < LENGTH(tags) + 1; t++) {
			/* dwindle */
			DwindleNode **root = &m->pertag->dwindle_root[t];
			dwindle_remove(root, c);
			DwindleNode *dnode = dwindle_find_leaf(*root, w);
			if (dnode)
				dnode->client = c;

			/* scroller */
			struct TagScrollerState *st = m->pertag->scroller_state[t];
			if (!st)
				continue;

			/* 先移除 c 在任意 tag 中的旧节点 */
			struct ScrollerStackNode *cn = find_scroller_node(st, c);
			if (cn)
				scroller_node_remove(st, cn);

			/* 将 w 的节点（如果存在）转给 c */
			struct ScrollerStackNode *wn = find_scroller_node(st, w);
			if (wn)
				wn->client = c;
		}
	}

	/* 同步当前活动 tag 的全局客户端字段 */
	if (c->mon) {
		uint32_t curtag = c->mon->pertag->curtag;
		sync_scroller_state_to_clients(c->mon, curtag);
	}
}

bool switch_scratchpad_client_state(Client *c) {

	if (config.scratchpad_cross_monitor && selmon && c->mon != selmon &&
		c->is_in_scratchpad) {
		// 保存原始monitor用于尺寸计算
		Monitor *oldmon = c->mon;
		c->scratchpad_switching_mon = true;
		c->mon = selmon;
		reset_foreign_tolevel(c, oldmon, c->mon);
		client_update_oldmonname_record(c, selmon);

		// 根据新monitor调整窗口尺寸
		c->float_geom.width =
			(int32_t)(c->float_geom.width * c->mon->w.width / oldmon->w.width);
		c->float_geom.height = (int32_t)(c->float_geom.height *
										 c->mon->w.height / oldmon->w.height);

		c->float_geom =
			setclient_coordinate_center(c, c->mon, c->float_geom, 0, 0);

		// 只有显示状态的scratchpad才需要聚焦和返回true
		if (c->is_scratchpad_show) {
			c->tags = get_tags_first_tag(selmon->tagset[selmon->seltags]);
			resize(c, c->float_geom, 0);
			arrange(selmon, false, false);
			focusclient(c, true);
			c->scratchpad_switching_mon = false;
			return true;
		} else {
			resize(c, c->float_geom, 0);
			c->scratchpad_switching_mon = false;
		}
	}

	if (c->is_in_scratchpad && c->is_scratchpad_show &&
		(c->mon->tagset[c->mon->seltags] & c->tags) == 0) {
		c->tags = c->mon->tagset[c->mon->seltags];
		arrange(c->mon, false, false);
		focusclient(c, true);
		return true;
	} else if (c->is_in_scratchpad && c->is_scratchpad_show &&
			   (c->mon->tagset[c->mon->seltags] & c->tags) != 0) {
		set_minimized(c);
		return true;
	} else if (c && c->is_in_scratchpad && !c->is_scratchpad_show) {
		show_scratchpad(c);
		return true;
	}

	return false;
}

void apply_named_scratchpad(Client *target_client) {
	Client *c = NULL;
	wl_list_for_each(c, &clients, link) {

		if (!config.scratchpad_cross_monitor && c->mon != selmon) {
			continue;
		}

		if (config.single_scratchpad && c->is_in_scratchpad &&
			c->is_scratchpad_show && c != target_client) {
			set_minimized(c);
		}
	}

	if (!target_client->is_in_scratchpad) {
		set_minimized(target_client);
		switch_scratchpad_client_state(target_client);
	} else
		switch_scratchpad_client_state(target_client);
}

void gpureset(struct wl_listener *listener, void *data) {
	struct wlr_renderer *old_drw = drw;
	struct wlr_allocator *old_alloc = alloc;
	struct Monitor *m = NULL;

	wlr_log(WLR_DEBUG, "gpu reset");

	if (!(drw = fx_renderer_create(backend)))
		die("couldn't recreate renderer");

	if (!(alloc = wlr_allocator_autocreate(backend, drw)))
		die("couldn't recreate allocator");

	wl_list_remove(&gpu_reset.link);
	wl_signal_add(&drw->events.lost, &gpu_reset);

	wlr_compositor_set_renderer(compositor, drw);

	wl_list_for_each(m, &mons, link) {
		wlr_output_init_render(m->wlr_output, alloc, drw);
	}

	wlr_allocator_destroy(old_alloc);
	wlr_renderer_destroy(old_drw);
}

void handlesig(int32_t signo) {
	if (signo == SIGCHLD)
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	else if (signo == SIGINT || signo == SIGTERM)
		quit(NULL);
}

void toggle_hotarea(int32_t x_root, int32_t y_root) {
	// 左下角热区坐标计算,兼容多显示屏
	Arg arg = {0};

	// 在刚启动的时候,selmon为NULL,但鼠标可能已经处于热区,
	// 必须判断避免奔溃
	if (!selmon)
		return;

	if (grabc)
		return;

	// 根据热角位置计算不同的热区坐标
	unsigned hx, hy;

	switch (config.hotarea_corner) {
	case BOTTOM_RIGHT: // 右下角
		hx = selmon->m.x + selmon->m.width - config.hotarea_size;
		hy = selmon->m.y + selmon->m.height - config.hotarea_size;
		break;
	case TOP_LEFT: // 左上角
		hx = selmon->m.x + config.hotarea_size;
		hy = selmon->m.y + config.hotarea_size;
		break;
	case TOP_RIGHT: // 右上角
		hx = selmon->m.x + selmon->m.width - config.hotarea_size;
		hy = selmon->m.y + config.hotarea_size;
		break;
	case BOTTOM_LEFT: // 左下角（默认）
	default:
		hx = selmon->m.x + config.hotarea_size;
		hy = selmon->m.y + selmon->m.height - config.hotarea_size;
		break;
	}

	// 判断鼠标是否在热区内
	int in_hotarea = 0;

	switch (config.hotarea_corner) {
	case BOTTOM_RIGHT: // 右下角
		in_hotarea = (y_root > hy && x_root > hx &&
					  x_root <= (selmon->m.x + selmon->m.width) &&
					  y_root <= (selmon->m.y + selmon->m.height));
		break;
	case TOP_LEFT: // 左上角
		in_hotarea = (y_root < hy && x_root < hx && x_root >= selmon->m.x &&
					  y_root >= selmon->m.y);
		break;
	case TOP_RIGHT: // 右上角
		in_hotarea = (y_root < hy && x_root > hx &&
					  x_root <= (selmon->m.x + selmon->m.width) &&
					  y_root >= selmon->m.y);
		break;
	case BOTTOM_LEFT: // 左下角（默认）
	default:
		in_hotarea = (y_root > hy && x_root < hx && x_root >= selmon->m.x &&
					  y_root <= (selmon->m.y + selmon->m.height));
		break;
	}

	if (config.enable_hotarea == 1 && selmon->is_in_hotarea == 0 &&
		in_hotarea) {
		toggleoverview(&arg);
		selmon->is_in_hotarea = 1;
	} else if (config.enable_hotarea == 1 && selmon->is_in_hotarea == 1 &&
			   !in_hotarea) {
		selmon->is_in_hotarea = 0;
	}
}

static void apply_rule_properties(Client *c, const ConfigWinRule *r) {
	APPLY_INT_PROP(c, r, isterm);
	APPLY_INT_PROP(c, r, allow_csd);
	APPLY_INT_PROP(c, r, force_fakemaximize);
	APPLY_INT_PROP(c, r, force_tiled_state);
	APPLY_INT_PROP(c, r, force_tearing);
	APPLY_INT_PROP(c, r, noswallow);
	APPLY_INT_PROP(c, r, nofocus);
	APPLY_INT_PROP(c, r, nofadein);
	APPLY_INT_PROP(c, r, nofadeout);
	APPLY_INT_PROP(c, r, no_force_center);
	APPLY_INT_PROP(c, r, isfloating);
	APPLY_INT_PROP(c, r, isfullscreen);
	APPLY_INT_PROP(c, r, isfakefullscreen);
	APPLY_INT_PROP(c, r, isnoborder);
	APPLY_INT_PROP(c, r, isnoshadow);
	APPLY_INT_PROP(c, r, isnoradius);
	APPLY_INT_PROP(c, r, isnoanimation);
	APPLY_INT_PROP(c, r, isopensilent);
	APPLY_INT_PROP(c, r, istagsilent);
	APPLY_INT_PROP(c, r, isnamedscratchpad);
	APPLY_INT_PROP(c, r, isglobal);
	APPLY_INT_PROP(c, r, isoverlay);
	APPLY_INT_PROP(c, r, ignore_maximize);
	APPLY_INT_PROP(c, r, ignore_minimize);
	APPLY_INT_PROP(c, r, isnosizehint);
	APPLY_INT_PROP(c, r, indleinhibit_when_focus);
	APPLY_INT_PROP(c, r, isunglobal);
	APPLY_INT_PROP(c, r, noblur);
	APPLY_INT_PROP(c, r, allow_shortcuts_inhibit);

	APPLY_FLOAT_PROP(c, r, scroller_proportion);
	APPLY_FLOAT_PROP(c, r, scroller_proportion_single);
	APPLY_FLOAT_PROP(c, r, focused_opacity);
	APPLY_FLOAT_PROP(c, r, unfocused_opacity);

	APPLY_STRING_PROP(c, r, animation_type_open);
	APPLY_STRING_PROP(c, r, animation_type_close);
}

void set_float_malposition(Client *tc) {
	Client *c = NULL;
	int32_t x, y, offset, xreverse, yreverse;
	x = tc->geom.x;
	y = tc->geom.y;
	xreverse = 1;
	yreverse = 1;
	offset = MIN(tc->mon->w.width / 20, tc->mon->w.height / 20);

	wl_list_for_each(c, &clients, link) {
		if (c->isfloating && c != tc && VISIBLEON(c, tc->mon) &&
			abs(x - c->geom.x) < offset && abs(y - c->geom.y) < offset) {

			x = c->geom.x + offset * xreverse;
			y = c->geom.y + offset * yreverse;
			if (x < tc->mon->w.x) {
				x = x + offset;
				xreverse = 1;
			}

			if (y < tc->mon->w.y) {
				y = y + offset;
				yreverse = 1;
			}

			if (x + tc->geom.width > tc->mon->w.x + tc->mon->w.width) {
				x = x - offset;
				xreverse = -1;
			}

			if (y + tc->geom.height > tc->mon->w.y + tc->mon->w.height) {
				y = y - offset;
				yreverse = -1;
			}
		}
	}

	tc->float_geom.x = tc->geom.x = x;
	tc->float_geom.y = tc->geom.y = y;
}

void client_reset_mon_tags(Client *c, Monitor *mon, uint32_t newtags) {
	if (!newtags && mon && !mon->isoverview) {
		c->tags = mon->tagset[mon->seltags];
	} else if (!newtags && mon && mon->isoverview) {
		c->tags = mon->ovbk_current_tagset;
	} else if (newtags) {
		c->tags = newtags;
	} else {
		c->tags = mon->tagset[mon->seltags];
	}
}

void check_match_tag_floating_rule(Client *c, Monitor *mon) {
	if (c->tags && !c->isfloating && mon && !c->swallowedby &&
		mon->pertag->open_as_floating[get_tags_first_tag_num(c->tags)]) {
		c->isfloating = 1;
	}
}

void applyrules(Client *c) {
	/* rule matching */
	const char *appid, *title;
	uint32_t i, newtags = 0;
	const ConfigWinRule *r;
	Monitor *m = NULL;
	Client *fc = NULL;
	Client *parent = NULL;

	if (!c)
		return;

	parent = client_get_parent(c);

	Monitor *mon = parent && parent->mon ? parent->mon : selmon;

	c->isfloating = client_is_float_type(c) || parent;

#ifdef XWAYLAND
	if (c->isfloating && client_is_x11(c)) {
		fix_xwayland_unmanaged_coordinate(c);
		c->float_geom = c->geom;
	}
#endif

	if (!(appid = client_get_appid(c)))
		appid = broken;
	if (!(title = client_get_title(c)))
		title = broken;

	for (i = 0; i < config.window_rules_count; i++) {

		r = &config.window_rules[i];

		// rule matching
		if (!is_window_rule_matches(r, appid, title))
			continue;

		// set general properties
		apply_rule_properties(c, r);

		// // set tags
		if (r->tags > 0) {
			newtags |= r->tags;
		} else if (parent) {
			newtags = parent->tags;
		}

		// set monitor of client
		wl_list_for_each(m, &mons, link) {
			if (match_monitor_spec(r->monitor, m)) {
				mon = m;
			}
		}

		if (c->isnamedscratchpad) {
			c->isfloating = 1;
		}

		// set geometry of floating client

		if (r->width > 1)
			c->float_geom.width = r->width;
		else if (r->width > 0 && r->width <= 1)
			c->float_geom.width = round(mon->m.width * r->width);
		if (r->height > 1)
			c->float_geom.height = r->height;
		else if (r->height > 0 && r->height <= 1)
			c->float_geom.height = round(mon->m.height * r->height);

		if (r->width > 0 || r->height > 0) {
			c->iscustomsize = 1;
		}

		if (r->offsetx || r->offsety) {
			c->iscustompos = 1;
			c->float_geom = c->geom = setclient_coordinate_center(
				c, mon, c->float_geom, r->offsetx, r->offsety);
		}
		if (c->isfloating) {
			c->geom = c->float_geom.width > 0 && c->float_geom.height > 0
						  ? c->float_geom
						  : c->geom;
			if (!c->isnosizehint)
				client_set_size_bound(c);
		}
	}

	if (mon)
		set_size_per(mon, c);

	// if no geom rule hit and is normal winodw, use the center pos and record
	// the hit size
	if (!c->iscustompos &&
		(!client_is_x11(c) || (c->geom.x == 0 && c->geom.y == 0))) {
		c->float_geom = c->geom =
			setclient_coordinate_center(c, mon, c->geom, 0, 0);
	} else {
		c->float_geom = c->geom;
	}

	/*-----------------------apply rule action-------------------------*/

	// rule action only apply after map not apply in the init commit
	struct wlr_surface *surface = client_surface(c);
	if (!surface || !surface->mapped)
		return;

	// apply swallow rule
	c->pid = client_get_pid(c);
	if (!c->noswallow && !c->isfloating && !client_is_float_type(c) &&
		!c->surface.xdg->initial_commit) {
		Client *p = termforwin(c);
		if (p && !p->isminimized) {
			c->swallowedby = p;
			p->swallowing = c;
			wl_list_remove(&c->link);
			wl_list_remove(&c->flink);
			swallow(c, p);
			wl_list_remove(&p->link);
			wl_list_remove(&p->flink);
			mon = p->mon;
			newtags = p->tags;
		}
	}

	int32_t fullscreen_state_backup =
		c->isfullscreen || client_wants_fullscreen(c);

	bool should_init_get_focus =
		!c->isopensilent &&
		!(client_is_x11_popup(c) && client_should_ignore_focus(c)) && mon &&
		(!c->istagsilent || !newtags || newtags & mon->tagset[mon->seltags]);

	if (!should_init_get_focus) {
		wl_list_remove(&c->flink);
		wl_list_insert(fstack.prev, &c->flink);
	}

	setmon(c, mon, newtags, should_init_get_focus);

	if (!c->isfloating) {
		c->old_stack_inner_per = c->stack_inner_per;
		c->old_master_inner_per = c->master_inner_per;
	}

	if (c->mon &&
		!(c->mon == selmon && c->tags & c->mon->tagset[c->mon->seltags]) &&
		!c->isopensilent && !c->istagsilent) {
		c->animation.tag_from_rule = true;
		view_in_mon(&(Arg){.ui = c->tags}, true, c->mon, true);
	}

	setfullscreen(c, fullscreen_state_backup);

	if (c->isfakefullscreen) {
		setfakefullscreen(c, 1);
	}

	/*
	if there is a new non-floating window in the current tag, the fullscreen
	window in the current tag will exit fullscreen and participate in tiling
	*/
	wl_list_for_each(fc, &clients,
					 link) if (fc && fc != c && c->tags & fc->tags && c->mon &&
							   VISIBLEON(fc, c->mon) && ISFULLSCREEN(fc) &&
							   !c->isfloating) {
		clear_fullscreen_flag(fc);
		arrange(c->mon, false, false);
	}

	if (c->isfloating && !c->iscustompos && !c->isnamedscratchpad) {
		wl_list_remove(&c->link);
		wl_list_insert(clients.prev, &c->link);
		set_float_malposition(c);
	}

	// apply named scratchpad rule
	if (c->isnamedscratchpad) {
		apply_named_scratchpad(c);
	}

	// apply overlay rule
	if (c->isoverlay && c->scene) {
		wlr_scene_node_reparent(&c->scene->node, layers[LyrOverlay]);
		wlr_scene_node_raise_to_top(&c->scene->node);
	}
}

void arrangelayer(Monitor *m, struct wl_list *list, struct wlr_box *usable_area,
				  int32_t exclusive) {
	LayerSurface *l = NULL;
	struct wlr_box full_area = m->m;

	wl_list_for_each(l, list, link) {
		struct wlr_layer_surface_v1 *layer_surface = l->layer_surface;

		if (exclusive != (layer_surface->current.exclusive_zone > 0) ||
			!layer_surface->initialized)
			continue;

		if (l->being_unmapped)
			continue;

		wlr_scene_layer_surface_v1_configure(l->scene_layer, &full_area,
											 usable_area);
		wlr_scene_node_set_position(&l->popups->node, l->scene->node.x,
									l->scene->node.y);
	}
}

void apply_window_snap(Client *c) {
	int32_t snap_up = 99999, snap_down = 99999, snap_left = 99999,
			snap_right = 99999;
	int32_t snap_up_temp = 0, snap_down_temp = 0, snap_left_temp = 0,
			snap_right_temp = 0;
	int32_t snap_up_screen = 0, snap_down_screen = 0, snap_left_screen = 0,
			snap_right_screen = 0;
	int32_t snap_up_mon = 0, snap_down_mon = 0, snap_left_mon = 0,
			snap_right_mon = 0;

	uint32_t cbw = !render_border || c->fake_no_border ? config.borderpx : 0;
	uint32_t tcbw;
	uint32_t cx, cy, cw, ch, tcx, tcy, tcw, tch;
	cx = c->geom.x + cbw;
	cy = c->geom.y + cbw;
	cw = c->geom.width - 2 * cbw;
	ch = c->geom.height - 2 * cbw;

	Client *tc = NULL;
	if (!c || !c->mon || !client_surface(c)->mapped || c->iskilling)
		return;

	if (!c->isfloating || !config.enable_floating_snap)
		return;

	wl_list_for_each(tc, &clients, link) {
		if (tc && tc->isfloating && !tc->iskilling &&
			client_surface(tc)->mapped && VISIBLEON(tc, c->mon)) {

			tcbw = !render_border || tc->fake_no_border ? config.borderpx : 0;
			tcx = tc->geom.x + tcbw;
			tcy = tc->geom.y + tcbw;
			tcw = tc->geom.width - 2 * tcbw;
			tch = tc->geom.height - 2 * tcbw;

			snap_left_temp = cx - tcx - tcw;
			snap_right_temp = tcx - cx - cw;
			snap_up_temp = cy - tcy - tch;
			snap_down_temp = tcy - cy - ch;

			if (snap_left_temp < snap_left && snap_left_temp >= 0) {
				snap_left = snap_left_temp;
			}
			if (snap_right_temp < snap_right && snap_right_temp >= 0) {
				snap_right = snap_right_temp;
			}
			if (snap_up_temp < snap_up && snap_up_temp >= 0) {
				snap_up = snap_up_temp;
			}
			if (snap_down_temp < snap_down && snap_down_temp >= 0) {
				snap_down = snap_down_temp;
			}
		}
	}

	snap_left_mon = cx - c->mon->m.x;
	snap_right_mon = c->mon->m.x + c->mon->m.width - cx - cw;
	snap_up_mon = cy - c->mon->m.y;
	snap_down_mon = c->mon->m.y + c->mon->m.height - cy - ch;

	if (snap_up_mon >= 0 && snap_up_mon < snap_up)
		snap_up = snap_up_mon;
	if (snap_down_mon >= 0 && snap_down_mon < snap_down)
		snap_down = snap_down_mon;
	if (snap_left_mon >= 0 && snap_left_mon < snap_left)
		snap_left = snap_left_mon;
	if (snap_right_mon >= 0 && snap_right_mon < snap_right)
		snap_right = snap_right_mon;

	snap_left_screen = cx - c->mon->w.x;
	snap_right_screen = c->mon->w.x + c->mon->w.width - cx - cw;
	snap_up_screen = cy - c->mon->w.y;
	snap_down_screen = c->mon->w.y + c->mon->w.height - cy - ch;

	if (snap_up_screen >= 0 && snap_up_screen < snap_up)
		snap_up = snap_up_screen;
	if (snap_down_screen >= 0 && snap_down_screen < snap_down)
		snap_down = snap_down_screen;
	if (snap_left_screen >= 0 && snap_left_screen < snap_left)
		snap_left = snap_left_screen;
	if (snap_right_screen >= 0 && snap_right_screen < snap_right)
		snap_right = snap_right_screen;

	if (snap_left < snap_right && snap_left < config.snap_distance) {
		c->geom.x = c->geom.x - snap_left;
	}

	if (snap_right <= snap_left && snap_right < config.snap_distance) {
		c->geom.x = c->geom.x + snap_right;
	}

	if (snap_up < snap_down && snap_up < config.snap_distance) {
		c->geom.y = c->geom.y - snap_up;
	}

	if (snap_down <= snap_up && snap_down < config.snap_distance) {
		c->geom.y = c->geom.y + snap_down;
	}

	c->float_geom = c->geom;
	resize(c, c->geom, 0);
}

/* Get or create the scroller status of a tag in the specified monitor */
static struct TagScrollerState *ensure_scroller_state(Monitor *m,
													  uint32_t tag) {
	if (!m->pertag->scroller_state[tag]) {
		struct TagScrollerState *st =
			calloc(1, sizeof(struct TagScrollerState));
		m->pertag->scroller_state[tag] = st;
	}
	return m->pertag->scroller_state[tag];
}

/* Find the node corresponding to the client in the tag status (return NULL if none) */
static struct ScrollerStackNode *find_scroller_node(struct TagScrollerState *st,
													Client *c) {
	if (!st)
		return NULL;
	for (struct ScrollerStackNode *n = st->all_first; n; n = n->all_next)
		if (n->client == c)
			return n;
	return NULL;
}

/* Create a new node and insert it into the all linked list of tag status */
static struct ScrollerStackNode *
scroller_node_create(struct TagScrollerState *st, Client *c) {
	struct ScrollerStackNode *n = calloc(1, sizeof(*n));
	n->client = c;
	n->scroller_proportion = c->scroller_proportion;
	n->stack_proportion = c->stack_proportion;
	n->scroller_proportion_single = c->scroller_proportion_single;
	n->next_in_stack = NULL;
	n->prev_in_stack = NULL;
	n->all_next = st->all_first;
	st->all_first = n;
	st->count++;
	return n;
}

/* Remove a node from the tag state and release it */
static void scroller_node_remove(struct TagScrollerState *st,
								 struct ScrollerStackNode *target) {
	if (!st || !target)
		return;

	/* Save neighbors */
	struct ScrollerStackNode *prev = target->prev_in_stack;
	struct ScrollerStackNode *next = target->next_in_stack;

	/* Remove from stacked linked list */
	if (prev)
		prev->next_in_stack = next;
	if (next)
		next->prev_in_stack = prev;

	/* Remove from all linked list */
	struct ScrollerStackNode **indirect = &st->all_first;
	while (*indirect && *indirect != target)
		indirect = &(*indirect)->all_next;
	if (*indirect == target) {
		*indirect = target->all_next;
		st->count--;
	}
	free(target);
}

/* Clear all scroller status of a tag */
static void clear_scroller_state(struct TagScrollerState *st) {
	if (!st)
		return;
	struct ScrollerStackNode *n = st->all_first;
	while (n) {
		struct ScrollerStackNode *next = n->all_next;
		free(n);
		n = next;
	}
	free(st);
}

/* Clear the scroller status of all tags when the Monitor is destroyed */
static void cleanup_monitor_scroller(Monitor *m) {
	for (int t = 0; t < LENGTH(tags) + 1; t++) {
		if (m->pertag->scroller_state[t]) {
			clear_scroller_state(m->pertag->scroller_state[t]);
			m->pertag->scroller_state[t] = NULL;
		}
	}
}

/* Synchronize the status of a certain tag back to the global fields of all clients */
static void sync_scroller_state_to_clients(Monitor *m, uint32_t tag) {
	struct TagScrollerState *st = m->pertag->scroller_state[tag];
	if (!st)
		return;
	for (struct ScrollerStackNode *n = st->all_first; n; n = n->all_next) {
		Client *c = n->client;
		c->scroller_proportion = n->scroller_proportion;
		c->stack_proportion = n->stack_proportion;
		c->scroller_proportion_single = n->scroller_proportion_single;
	}
}

void vertical_scroll_adjust_fullandmax(Client *c, struct wlr_box *target_geom) {
	Monitor *m = c->mon;
	int32_t cur_gappiv = enablegaps ? m->gappiv : 0;
	int32_t cur_gap_outer_top    = enablegaps ? m->gap_outer_top    : 0;
	int32_t cur_gap_outer_bottom = enablegaps ? m->gap_outer_bottom : 0;
	int32_t cur_gap_outer_left  = enablegaps ? m->gap_outer_left  : 0;
	int32_t cur_gap_outer_right = enablegaps ? m->gap_outer_right : 0;

	cur_gappiv = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gappiv;
	cur_gap_outer_top = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_top;
	cur_gap_outer_bottom = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_bottom;
	cur_gap_outer_left = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_left;
	cur_gap_outer_right = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_right;

	if (c->isfullscreen) {
		target_geom->width  = m->m.width;
		target_geom->height = m->m.height;
		target_geom->x      = m->m.x;
		return;
	}

	if (c->ismaximizescreen) {
		target_geom->width  = m->w.width  - cur_gap_outer_left - cur_gap_outer_right;
		target_geom->height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
		target_geom->x = m->w.x + cur_gap_outer_left;
		return;
	}

	target_geom->width = m->w.width - cur_gap_outer_left - cur_gap_outer_right;
	target_geom->x = m->w.x + (m->w.width - target_geom->width) / 2;
}

void vertical_check_scroller_root_inside_mon(Client *c,
											 struct wlr_box *geometry) {
	if (!GEOMINSIDEMON(geometry, c->mon)) {
		geometry->y = c->mon->w.y + (c->mon->w.height - geometry->height) / 2;
	}
}

void horizontal_scroll_adjust_fullandmax(Client *c,
										 struct wlr_box *target_geom) {
	Monitor *m = c->mon;
	int32_t cur_gappih = enablegaps ? m->gappih : 0;
	int32_t cur_gap_outer_left  = enablegaps ? m->gap_outer_left  : 0;
	int32_t cur_gap_outer_right = enablegaps ? m->gap_outer_right : 0;
	int32_t cur_gap_outer_top    = enablegaps ? m->gap_outer_top    : 0;
	int32_t cur_gap_outer_bottom = enablegaps ? m->gap_outer_bottom : 0;

	cur_gappih = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gappih;
	cur_gap_outer_left = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_left;
	cur_gap_outer_right = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_right;
	cur_gap_outer_top = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_top;
	cur_gap_outer_bottom = config.smartgaps && m->visible_scroll_tiling_clients == 1
					 ? 0
					 : cur_gap_outer_bottom;

	if (c->isfullscreen) {
		target_geom->height = m->m.height;
		target_geom->width  = m->m.width;
		target_geom->y      = m->m.y;
		return;
	}

	if (c->ismaximizescreen) {
		target_geom->height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
		target_geom->width  = m->w.width  - cur_gap_outer_left - cur_gap_outer_right;
		target_geom->y = m->w.y + cur_gap_outer_top;
		return;
	}

	target_geom->height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
	target_geom->y = m->w.y + (m->w.height - target_geom->height) / 2;
}

void horizontal_check_scroller_root_inside_mon(Client *c,
											   struct wlr_box *geometry) {
	if (!GEOMINSIDEMON(geometry, c->mon)) {
		geometry->x = c->mon->w.x + (c->mon->w.width - geometry->width) / 2;
	}
}

void arrange_stack_node(struct ScrollerStackNode *head, struct wlr_box geometry,
						int32_t gappiv) {
	int32_t stack_size = 0;
	struct ScrollerStackNode *iter = head;
	while (iter) {
		stack_size++;
		iter = iter->next_in_stack;
	}
	if (stack_size == 0)
		return;

	/* Normalized ratio */
	float total_proportion = 0.0f;
	iter = head;
	while (iter) {
		if (iter->stack_proportion <= 0.0f || iter->stack_proportion >= 1.0f)
			iter->stack_proportion =
				stack_size == 1 ? 1.0f : 1.0f / (stack_size - 1);
		total_proportion += iter->stack_proportion;
		iter = iter->next_in_stack;
	}
	iter = head;
	while (iter) {
		iter->stack_proportion /= total_proportion;
		iter = iter->next_in_stack;
	}

	/* Vertical arrangement (horizontal stacking) */
	int32_t client_height;
	int32_t current_y = geometry.y;
	int32_t remain_client_height = geometry.height - (stack_size - 1) * gappiv;
	float remain_proportion = 1.0f;

	iter = head;
	while (iter) {
		client_height =
			remain_client_height * (iter->stack_proportion / remain_proportion);
		struct wlr_box client_geom = {.x = geometry.x,
									  .y = current_y,
									  .width = geometry.width,
									  .height = client_height};
		resize(iter->client, client_geom, 0);
		remain_proportion -= iter->stack_proportion;
		remain_client_height -= client_height;
		current_y += client_height + gappiv;
		iter = iter->next_in_stack;
	}
}

void arrange_stack_vertical_node(struct ScrollerStackNode *head,
								 struct wlr_box geometry, int32_t gappih) {
	int32_t stack_size = 0;
	struct ScrollerStackNode *iter = head;
	while (iter) {
		stack_size++;
		iter = iter->next_in_stack;
	}
	if (stack_size == 0)
		return;

	/* Normalized ratio */
	float total_proportion = 0.0f;
	iter = head;
	while (iter) {
		if (iter->stack_proportion <= 0.0f || iter->stack_proportion >= 1.0f)
			iter->stack_proportion =
				stack_size == 1 ? 1.0f : 1.0f / (stack_size - 1);
		total_proportion += iter->stack_proportion;
		iter = iter->next_in_stack;
	}
	iter = head;
	while (iter) {
		iter->stack_proportion /= total_proportion;
		iter = iter->next_in_stack;
	}

	/* Horizontal arrangement (vertical stacking) */
	int32_t client_width;
	int32_t current_x = geometry.x;
	int32_t remain_client_width = geometry.width - (stack_size - 1) * gappih;
	float remain_proportion = 1.0f;

	iter = head;
	while (iter) {
		client_width =
			remain_client_width * (iter->stack_proportion / remain_proportion);
		struct wlr_box client_geom = {.y = geometry.y,
									  .x = current_x,
									  .height = geometry.height,
									  .width = client_width};
		resize(iter->client, client_geom, 0);
		remain_proportion -= iter->stack_proportion;
		remain_client_width -= client_width;
		current_x += client_width + gappih;
		iter = iter->next_in_stack;
	}
}

void scroller(Monitor *m) {
	uint32_t tag = m->pertag->curtag;
	struct TagScrollerState *st = ensure_scroller_state(m, tag);
	Client *c = NULL;

	/* Collect all stacking headers in the order of the global client linked list to ensure the correct visual order */
	struct ScrollerStackNode *heads[64];
	int32_t n_heads = 0;
	wl_list_for_each(c, &clients, link) {
		if (VISIBLEON(c, m) && ISSCROLLTILED(c)) {
			struct ScrollerStackNode *node = find_scroller_node(st, c);
			if (node && !node->prev_in_stack) {
				bool already = false;
				for (int k = 0; k < n_heads; k++) {
					if (heads[k] == node) {
						already = true;
						break;
					}
				}
				if (!already)
					heads[n_heads++] = node;
			}
		}
	}

	if (n_heads == 0) {
		sync_scroller_state_to_clients(m, tag);
		return;
	}

	m->visible_scroll_tiling_clients = n_heads;

	int32_t cur_gappih = enablegaps ? m->gappih : 0;
	int32_t cur_gappiv = enablegaps ? m->gappiv : 0;
	int32_t cur_gap_outer_left  = enablegaps ? m->gap_outer_left  : 0;
	int32_t cur_gap_outer_right = enablegaps ? m->gap_outer_right : 0;
	int32_t cur_gap_outer_top    = enablegaps ? m->gap_outer_top    : 0;
	int32_t cur_gap_outer_bottom = enablegaps ? m->gap_outer_bottom : 0;
	if (config.smartgaps && n_heads == 1) {
		cur_gappih = cur_gappiv = cur_gap_outer_left = cur_gap_outer_right = cur_gap_outer_top = cur_gap_outer_bottom = 0;
	}
	int32_t max_client_width =
		m->w.width - 2 * config.scroller_structs - cur_gappih;

	/* Single client special case */
	if (n_heads == 1 && !config.scroller_ignore_proportion_single &&
		!heads[0]->client->isfullscreen &&
		!heads[0]->client->ismaximizescreen) {
		struct ScrollerStackNode *head = heads[0];
		float single_proportion =
			head->scroller_proportion_single > 0.0f
				? head->scroller_proportion_single
				: config.scroller_default_proportion_single;
		struct wlr_box target_geom;
		target_geom.height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
		target_geom.width = (m->w.width - cur_gap_outer_left - cur_gap_outer_right) * single_proportion;
		target_geom.x = m->w.x + (m->w.width - target_geom.width) / 2;
		target_geom.y = m->w.y + (m->w.height - target_geom.height) / 2;
		horizontal_check_scroller_root_inside_mon(head->client, &target_geom);
		arrange_stack_node(head, target_geom, cur_gappiv);
		sync_scroller_state_to_clients(m, tag);
		return;
	}

	struct ScrollerStackNode *root_node = NULL;
	if (m->sel && ISSCROLLTILED(m->sel)) {
		root_node = find_scroller_node(st, m->sel);
		if (root_node) {
			while (root_node->prev_in_stack)
				root_node = root_node->prev_in_stack;
		}
	}
	if (!root_node && m->prevsel && ISSCROLLTILED(m->prevsel)) {
		root_node = find_scroller_node(st, m->prevsel);
		if (root_node) {
			while (root_node->prev_in_stack)
				root_node = root_node->prev_in_stack;
		}
	}
	if (!root_node)
		root_node = heads[n_heads / 2]; /* Simple rollback */

	int32_t focus_index = -1;
	for (int i = 0; i < n_heads; i++) {
		if (heads[i] == root_node) {
			focus_index = i;
			break;
		}
	}
	if (focus_index < 0)
		focus_index = n_heads / 2;

	/* Determine whether scrolling, overspread, and center are required */
	bool need_scroller = false;
	bool over_overspread_to_left = false;
	Client *root_client = root_node->client;

	if (root_client->geom.x >= m->w.x + config.scroller_structs &&
		root_client->geom.x + root_client->geom.width <=
			m->w.x + m->w.width - config.scroller_structs) {
		need_scroller = false;
	} else {
		need_scroller = true;
	}

	bool need_apply_overspread =
		config.scroller_prefer_overspread && n_heads > 1 &&
		(focus_index == 0 || focus_index == n_heads - 1) &&
		heads[focus_index]->scroller_proportion < 1.0f;

	if (need_apply_overspread) {
		if (focus_index == 0) {
			over_overspread_to_left = true;
		} else {
			over_overspread_to_left = false;
		}
		if (over_overspread_to_left &&
			(!INSIDEMON(heads[1]->client) ||
			 (heads[1]->scroller_proportion + heads[0]->scroller_proportion >=
			  1.0f))) {
			need_scroller = true;
		} else if (!over_overspread_to_left &&
				   (!INSIDEMON(heads[focus_index - 1]->client) ||
					(heads[focus_index - 1]->scroller_proportion +
						 heads[focus_index]->scroller_proportion >=
					 1.0f))) {
			need_scroller = true;
		} else {
			need_apply_overspread = false;
		}
	}

	bool need_apply_center =
		config.scroller_focus_center || n_heads == 1 ||
		(config.scroller_prefer_center && !need_apply_overspread &&
		 (!m->prevsel ||
		  (ISSCROLLTILED(m->prevsel) &&
		   (m->prevsel->scroller_proportion * max_client_width) +
				   (heads[focus_index]->scroller_proportion *
					max_client_width) >
			   m->w.width - 2 * config.scroller_structs - cur_gappih)));

	if (n_heads == 1 && config.scroller_ignore_proportion_single) {
		need_scroller = true;
	}
	if (start_drag_window)
		need_scroller = false;

	struct wlr_box target_geom;
	target_geom.height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
	target_geom.width =
		max_client_width * heads[focus_index]->scroller_proportion;
	target_geom.y = m->w.y + (m->w.height - target_geom.height) / 2;
	horizontal_scroll_adjust_fullandmax(heads[focus_index]->client,
										&target_geom);

	if (heads[focus_index]->client->isfullscreen) {
		target_geom.x = m->m.x;
		horizontal_check_scroller_root_inside_mon(heads[focus_index]->client,
												  &target_geom);
		arrange_stack_node(heads[focus_index], target_geom, cur_gappiv);
	} else if (heads[focus_index]->client->ismaximizescreen) {
		target_geom.x = m->w.x + cur_gap_outer_left;
		horizontal_check_scroller_root_inside_mon(heads[focus_index]->client,
												  &target_geom);
		arrange_stack_node(heads[focus_index], target_geom, cur_gappiv);
	} else if (need_scroller) {
		if (need_apply_center) {
			target_geom.x = m->w.x + (m->w.width - target_geom.width) / 2;
		} else if (need_apply_overspread) {
			if (over_overspread_to_left) {
				target_geom.x = m->w.x + config.scroller_structs;
			} else {
				target_geom.x =
					m->w.x + (m->w.width -
							  heads[focus_index]->scroller_proportion *
								  max_client_width -
							  config.scroller_structs);
			}
		} else {
			target_geom.x =
				root_client->geom.x > m->w.x + (m->w.width) / 2
					? m->w.x + (m->w.width -
								heads[focus_index]->scroller_proportion *
									max_client_width -
								config.scroller_structs)
					: m->w.x + config.scroller_structs;
		}
		horizontal_check_scroller_root_inside_mon(heads[focus_index]->client,
												  &target_geom);
		arrange_stack_node(heads[focus_index], target_geom, cur_gappiv);
	} else {
		target_geom.x = root_client->geom.x;
		horizontal_check_scroller_root_inside_mon(heads[focus_index]->client,
												  &target_geom);
		arrange_stack_node(heads[focus_index], target_geom, cur_gappiv);
	}

	/* Arrange the stack on the left */
	for (int i = 1; i <= focus_index; i++) {
		struct ScrollerStackNode *cur = heads[focus_index - i];
		struct wlr_box left_geom;
		left_geom.height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
		left_geom.width = max_client_width * cur->scroller_proportion;
		horizontal_scroll_adjust_fullandmax(cur->client, &left_geom);
		left_geom.x = heads[focus_index - i + 1]->client->geom.x - cur_gappih -
					  left_geom.width;
		arrange_stack_node(cur, left_geom, cur_gappiv);
	}

	/* Arrange the stack on the right */
	for (int i = 1; i < n_heads - focus_index; i++) {
		struct ScrollerStackNode *cur = heads[focus_index + i];
		struct wlr_box right_geom;
		right_geom.height = m->w.height - cur_gap_outer_top - cur_gap_outer_bottom;
		right_geom.width = max_client_width * cur->scroller_proportion;
		horizontal_scroll_adjust_fullandmax(cur->client, &right_geom);
		right_geom.x = heads[focus_index + i - 1]->client->geom.x + cur_gappih +
					   heads[focus_index + i - 1]->client->geom.width;
		arrange_stack_node(cur, right_geom, cur_gappiv);
	}

	sync_scroller_state_to_clients(m, tag);
}

void vertical_scroller(Monitor *m) {
	uint32_t tag = m->pertag->curtag;
	struct TagScrollerState *st = ensure_scroller_state(m, tag);
	Client *c = NULL;

	/* Collect stack headers in global order */
	struct ScrollerStackNode *heads[64];
	int32_t n_heads = 0;
	wl_list_for_each(c, &clients, link) {
		if (VISIBLEON(c, m) && ISSCROLLTILED(c)) {
			struct ScrollerStackNode *node = find_scroller_node(st, c);
			if (node && !node->prev_in_stack) {
				bool already = false;
				for (int k = 0; k < n_heads; k++)
					if (heads[k] == node)
						already = true;
				if (!already)
					heads[n_heads++] = node;
			}
		}
	}

	if (n_heads == 0) {
		sync_scroller_state_to_clients(m, tag);
		return;
	}

	m->visible_scroll_tiling_clients = n_heads;

	int32_t cur_gappiv = enablegaps ? m->gappiv : 0;
	int32_t cur_gap_outer_top    = enablegaps ? m->gap_outer_top    : 0;
	int32_t cur_gap_outer_bottom = enablegaps ? m->gap_outer_bottom : 0;
	int32_t cur_gap_outer_left  = enablegaps ? m->gap_outer_left  : 0;
	int32_t cur_gap_outer_right = enablegaps ? m->gap_outer_right : 0;
	int32_t cur_gappih = enablegaps ? m->gappih : 0;
	if (config.smartgaps && n_heads == 1) {
		cur_gappiv = cur_gap_outer_top = cur_gap_outer_bottom = cur_gap_outer_left = cur_gap_outer_right = 0;
	}
	int32_t max_client_height =
		m->w.height - 2 * config.scroller_structs - cur_gappiv;

	if (n_heads == 1 && !config.scroller_ignore_proportion_single &&
		!heads[0]->client->isfullscreen &&
		!heads[0]->client->ismaximizescreen) {
		struct ScrollerStackNode *head = heads[0];
		float single_proportion =
			head->scroller_proportion_single > 0.0f
				? head->scroller_proportion_single
				: config.scroller_default_proportion_single;
		struct wlr_box target_geom;
		target_geom.width = m->w.width - cur_gap_outer_left - cur_gap_outer_right;
		target_geom.height = (m->w.height - cur_gap_outer_top - cur_gap_outer_bottom) * single_proportion;
		target_geom.y = m->w.y + (m->w.height - target_geom.height) / 2;
		target_geom.x = m->w.x + (m->w.width - target_geom.width) / 2;
		vertical_check_scroller_root_inside_mon(head->client, &target_geom);
		arrange_stack_vertical_node(head, target_geom, cur_gappih);
		sync_scroller_state_to_clients(m, tag);
		return;
	}

	struct ScrollerStackNode *root_node = NULL;
	if (m->sel && ISSCROLLTILED(m->sel)) {
		root_node = find_scroller_node(st, m->sel);
		if (root_node) {
			while (root_node->prev_in_stack)
				root_node = root_node->prev_in_stack;
		}
	}
	if (!root_node && m->prevsel && ISSCROLLTILED(m->prevsel)) {
		root_node = find_scroller_node(st, m->prevsel);
		if (root_node) {
			while (root_node->prev_in_stack)
				root_node = root_node->prev_in_stack;
		}
	}
	if (!root_node)
		root_node = heads[n_heads / 2];

	int32_t focus_index = -1;
	for (int i = 0; i < n_heads; i++) {
		if (heads[i] == root_node) {
			focus_index = i;
			break;
		}
	}
	if (focus_index < 0)
		focus_index = n_heads / 2;

	bool need_scroller = false;
	bool over_overspread_to_up = false;
	Client *root_client = root_node->client;

	if (root_client->geom.y >= m->w.y + config.scroller_structs &&
		root_client->geom.y + root_client->geom.height <=
			m->w.y + m->w.height - config.scroller_structs) {
		need_scroller = false;
	} else {
		need_scroller = true;
	}

	bool need_apply_overspread =
		config.scroller_prefer_overspread && n_heads > 1 &&
		(focus_index == 0 || focus_index == n_heads - 1) &&
		heads[focus_index]->scroller_proportion < 1.0f;

	if (need_apply_overspread) {
		if (focus_index == 0) {
			over_overspread_to_up = true;
		} else {
			over_overspread_to_up = false;
		}
		if (over_overspread_to_up &&
			(!INSIDEMON(heads[1]->client) ||
			 (heads[1]->scroller_proportion + heads[0]->scroller_proportion >=
			  1.0f))) {
			need_scroller = true;
		} else if (!over_overspread_to_up &&
				   (!INSIDEMON(heads[focus_index - 1]->client) ||
					(heads[focus_index - 1]->scroller_proportion +
						 heads[focus_index]->scroller_proportion >=
					 1.0f))) {
			need_scroller = true;
		} else {
			need_apply_overspread = false;
		}
	}

	bool need_apply_center =
		config.scroller_focus_center || n_heads == 1 ||
		(config.scroller_prefer_center && !need_apply_overspread &&
		 (!m->prevsel ||
		  (ISSCROLLTILED(m->prevsel) &&
		   (m->prevsel->scroller_proportion * max_client_height) +
				   (heads[focus_index]->scroller_proportion *
					max_client_height) >
			   m->w.height - 2 * config.scroller_structs - cur_gappiv)));

	if (n_heads == 1 && config.scroller_ignore_proportion_single) {
		need_scroller = true;
	}
	if (start_drag_window)
		need_scroller = false;

	struct wlr_box target_geom;
	target_geom.width = m->w.width - cur_gap_outer_left - cur_gap_outer_right;
	target_geom.height =
		max_client_height * heads[focus_index]->scroller_proportion;
	target_geom.x = m->w.x + (m->w.width - target_geom.width) / 2;
	vertical_scroll_adjust_fullandmax(heads[focus_index]->client, &target_geom);

	if (heads[focus_index]->client->isfullscreen) {
		target_geom.y = m->m.y;
		vertical_check_scroller_root_inside_mon(heads[focus_index]->client,
												&target_geom);
		arrange_stack_vertical_node(heads[focus_index], target_geom,
									cur_gappih);
	} else if (heads[focus_index]->client->ismaximizescreen) {
		target_geom.y = m->w.y + cur_gap_outer_top;
		vertical_check_scroller_root_inside_mon(heads[focus_index]->client,
												&target_geom);
		arrange_stack_vertical_node(heads[focus_index], target_geom,
									cur_gappih);
	} else if (need_scroller) {
		if (need_apply_center) {
			target_geom.y = m->w.y + (m->w.height - target_geom.height) / 2;
		} else if (need_apply_overspread) {
			if (over_overspread_to_up) {
				target_geom.y = m->w.y + config.scroller_structs;
			} else {
				target_geom.y =
					m->w.y + (m->w.height -
							  heads[focus_index]->scroller_proportion *
								  max_client_height -
							  config.scroller_structs);
			}
		} else {
			target_geom.y =
				root_client->geom.y > m->w.y + (m->w.height) / 2
					? m->w.y + (m->w.height -
								heads[focus_index]->scroller_proportion *
									max_client_height -
								config.scroller_structs)
					: m->w.y + config.scroller_structs;
		}
		vertical_check_scroller_root_inside_mon(heads[focus_index]->client,
												&target_geom);
		arrange_stack_vertical_node(heads[focus_index], target_geom,
									cur_gappih);
	} else {
		target_geom.y = root_client->geom.y;
		vertical_check_scroller_root_inside_mon(heads[focus_index]->client,
												&target_geom);
		arrange_stack_vertical_node(heads[focus_index], target_geom,
									cur_gappih);
	}

	for (int i = 1; i <= focus_index; i++) {
		struct ScrollerStackNode *cur = heads[focus_index - i];
		struct wlr_box up_geom;
		up_geom.width = m->w.width - cur_gap_outer_left - cur_gap_outer_right;
		up_geom.height = max_client_height * cur->scroller_proportion;
		vertical_scroll_adjust_fullandmax(cur->client, &up_geom);
		up_geom.y = heads[focus_index - i + 1]->client->geom.y - cur_gappiv -
					up_geom.height;
		arrange_stack_vertical_node(cur, up_geom, cur_gappih);
	}

	for (int i = 1; i < n_heads - focus_index; i++) {
		struct ScrollerStackNode *cur = heads[focus_index + i];
		struct wlr_box down_geom;
		down_geom.width = m->w.width - cur_gap_outer_left - cur_gap_outer_right;
		down_geom.height = max_client_height * cur->scroller_proportion;
		vertical_scroll_adjust_fullandmax(cur->client, &down_geom);
		down_geom.y = heads[focus_index + i - 1]->client->geom.y + cur_gappiv +
					  heads[focus_index + i - 1]->client->geom.height;
		arrange_stack_vertical_node(cur, down_geom, cur_gappih);
	}

	sync_scroller_state_to_clients(m, tag);
}

void scroller_remove_client(Client *c) {
	Monitor *m;
	wl_list_for_each(m, &mons, link) {
		for (uint32_t t = 0; t < LENGTH(tags) + 1; t++) {
			struct TagScrollerState *st = m->pertag->scroller_state[t];
			if (!st)
				continue;
			struct ScrollerStackNode *node = find_scroller_node(st, c);
			if (node) {
				scroller_node_remove(st, node);
			}
		}
	}
}

void scroller_insert_stack(Client *c, Client *target_client,
						   bool insert_before) {
	if (!target_client || target_client->mon != c->mon)
		return;

	if (c->isfullscreen)
		setfullscreen(c, 0);
	if (c->ismaximizescreen)
		setmaximizescreen(c, 0);

	Monitor *m = c->mon;
	uint32_t tag = m->pertag->curtag;
	struct TagScrollerState *st = ensure_scroller_state(m, tag);

	struct ScrollerStackNode *cnode = find_scroller_node(st, c);
	if (cnode)
		scroller_node_remove(st, cnode);

	struct ScrollerStackNode *tnode = find_scroller_node(st, target_client);
	if (!tnode)
		tnode = scroller_node_create(st, target_client);

	struct ScrollerStackNode *newnode = scroller_node_create(st, c);
	/* Insert the new node before or after tnode */
	if (insert_before) {
		newnode->next_in_stack = tnode;
		newnode->prev_in_stack = tnode->prev_in_stack;
		if (tnode->prev_in_stack)
			tnode->prev_in_stack->next_in_stack = newnode;
		tnode->prev_in_stack = newnode;
		wl_list_remove(&c->link);
		wl_list_insert(tnode->client->link.prev, &c->link);
	} else {
		newnode->prev_in_stack = tnode;
		newnode->next_in_stack = tnode->next_in_stack;
		if (tnode->next_in_stack)
			tnode->next_in_stack->prev_in_stack = newnode;
		tnode->next_in_stack = newnode;
		wl_list_remove(&c->link);
		wl_list_insert(&tnode->client->link, &c->link);
	}

	/* Handle the full screen/maximized state of the stack header*/
	struct ScrollerStackNode *head = tnode;
	while (head->prev_in_stack)
		head = head->prev_in_stack;
	Client *stack_head = head->client;
	if (stack_head->ismaximizescreen)
		setmaximizescreen(stack_head, 0);
	if (stack_head->isfullscreen)
		setfullscreen(stack_head, 0);

	/* Synchronize to Client field */
	sync_scroller_state_to_clients(m, tag);

	arrange(m, false, false);
}

void scroller_drop_tile(Client *c, Client *closest, int vertical) {

	// It must be updated first, otherwise the node inside will still contain cnode information.
	// This will cause the client pointed to by stach_head/stack_tail to be incorrect.
	update_scroller_state(c->mon);

	Client *stack_head = scroll_get_stack_head_client(closest);
	Client *stack_tail = scroll_get_stack_tail_client(closest);

	if (vertical) {
		if (closest->drop_direction == LEFT) {
			setfloating(c, 0);
			scroller_insert_stack(c, closest, true);
			return;
		} else if (closest->drop_direction == RIGHT) {
			setfloating(c, 0);
			scroller_insert_stack(c, closest, false);
			return;
		} else if (closest->drop_direction == UP) {
			if (c != stack_head) {
				wl_list_remove(&c->link);
				wl_list_insert(stack_head->link.prev, &c->link);
			}
		} else if (closest->drop_direction == DOWN) {
			if (c != stack_tail) {
				wl_list_remove(&c->link);
				wl_list_insert(&stack_tail->link, &c->link);
			}
		}
	} else {
		if (closest->drop_direction == UP) {
			setfloating(c, 0);
			scroller_insert_stack(c, closest, true);
			return;
		} else if (closest->drop_direction == DOWN) {
			setfloating(c, 0);
			scroller_insert_stack(c, closest, false);
			return;
		} else if (closest->drop_direction == LEFT) {
			if (c != stack_head) {
				wl_list_remove(&c->link);
				wl_list_insert(stack_head->link.prev, &c->link);
			}
		} else if (closest->drop_direction == RIGHT) {
			if (c != stack_tail) {
				wl_list_remove(&c->link);
				wl_list_insert(&stack_tail->link, &c->link);
			}
		}
	}

	setfloating(c, 0);
}

Client *scroll_get_stack_head_client(Client *c) {
	if (!c || !c->mon)
		return c;
	uint32_t tag = c->mon->pertag->curtag;
	struct TagScrollerState *st = c->mon->pertag->scroller_state[tag];
	if (st) {
		struct ScrollerStackNode *n = find_scroller_node(st, c);
		if (n) {
			while (n->prev_in_stack)
				n = n->prev_in_stack;
			return n->client;
		}
	}
	return c;
}

Client *scroll_get_stack_tail_client(Client *c) {
	if (!c || !c->mon)
		return c;
	uint32_t tag = c->mon->pertag->curtag;
	struct TagScrollerState *st = c->mon->pertag->scroller_state[tag];
	if (st) {
		struct ScrollerStackNode *n = find_scroller_node(st, c);
		if (n) {
			while (n->next_in_stack)
				n = n->next_in_stack;
			return n->client;
		}
	}
	return c;
}

static void update_scroller_state(Monitor *m) {
	uint32_t tag = m->pertag->curtag;
	struct TagScrollerState *st = ensure_scroller_state(m, tag);

	/* Collect all currently visible scroller tile windows */
	Client *vis[512];
	int32_t count = 0;
	Client *c;
	wl_list_for_each(c, &clients, link) {
		if (VISIBLEON(c, m) && ISSCROLLTILED(c))
			vis[count++] = c;
		if (count == 512)
			break;
	}

	/* Remove nodes that are no longer visible */
	struct ScrollerStackNode *n = st->all_first;
	while (n) {
		bool found = false;
		for (int i = 0; i < count; i++) {
			if (vis[i] == n->client) {
				found = true;
				break;
			}
		}
		struct ScrollerStackNode *next = n->all_next;
		if (!found)
			scroller_node_remove(st, n);
		n = next;
	}

	/* Create node for new visible window */
	for (int i = 0; i < count; i++) {
		if (!find_scroller_node(st, vis[i])) {
			scroller_node_create(st, vis[i]);
		}
	}
}
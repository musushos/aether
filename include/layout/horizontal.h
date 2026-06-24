void overview(Monitor *m) {
	int32_t i, n;
	int32_t cx, cy, cw, ch;
	int32_t dx;
	int32_t cols, rows, overcols;
	Client *c = NULL;
	n = 0;
	int32_t target_gappo =
		enablegaps ? m->isoverview ? config.overviewgappo
		           : (int32_t)((config.gap_outer_left + config.gap_outer_right) / 2) : 0;
	int32_t target_gappi =
		enablegaps ? m->isoverview ? config.overviewgappi : config.gappih : 0;
	float single_width_ratio = m->isoverview ? 0.7 : 0.9;
	float single_height_ratio = m->isoverview ? 0.8 : 0.9;

	n = m->isoverview ? m->visible_clients : m->visible_tiling_clients;

	if (n == 0) {
		return;
	}

	if (n == 1) {
		wl_list_for_each(c, &clients, link) {
			if (c->mon != m)
				continue;

			if (VISIBLEON(c, m) && !c->isunglobal &&
				((m->isoverview && !client_is_x11_popup(c)) || ISTILED(c))) {
				cw = (m->w.width - 2 * target_gappo) * single_width_ratio;
				ch = (m->w.height - 2 * target_gappo) * single_height_ratio;
				c->geom.x = m->w.x + (m->w.width - cw) / 2;
				c->geom.y = m->w.y + (m->w.height - ch) / 2;
				c->geom.width = cw;
				c->geom.height = ch;
				resize(c, c->geom, 0);
				return;
			}
		}
	}

	if (n == 2) {
		cw = (m->w.width - 2 * target_gappo - target_gappi) / 2;
		ch = (m->w.height - 2 * target_gappo) * 0.65;
		i = 0;
		wl_list_for_each(c, &clients, link) {
			if (c->mon != m)
				continue;

			if (VISIBLEON(c, m) && !c->isunglobal &&
				((m->isoverview && !client_is_x11_popup(c)) || ISTILED(c))) {
				if (i == 0) {
					c->geom.x = m->w.x + target_gappo;
					c->geom.y = m->w.y + (m->w.height - ch) / 2 + target_gappo;
					c->geom.width = cw;
					c->geom.height = ch;
					resize(c, c->geom, 0);
				} else if (i == 1) {
					c->geom.x = m->w.x + cw + target_gappo + target_gappi;
					c->geom.y = m->w.y + (m->w.height - ch) / 2 + target_gappo;
					c->geom.width = cw;
					c->geom.height = ch;
					resize(c, c->geom, 0);
				}
				i++;
			}
		}
		return;
	}

	for (cols = 0; cols <= n / 2; cols++) {
		if (cols * cols >= n) {
			break;
		}
	}
	rows = (cols && (cols - 1) * cols >= n) ? cols - 1 : cols;

	ch = (m->w.height - 2 * target_gappo - (rows - 1) * target_gappi) / rows;
	cw = (m->w.width - 2 * target_gappo - (cols - 1) * target_gappi) / cols;

	overcols = n % cols;
	if (overcols) {
		dx = (m->w.width - overcols * cw - (overcols - 1) * target_gappi) / 2 -
			 target_gappo;
	}

	i = 0;
	wl_list_for_each(c, &clients, link) {
		if (c->mon != m)
			continue;

		if (VISIBLEON(c, m) && !c->isunglobal &&
			((m->isoverview && !client_is_x11_popup(c)) || ISTILED(c))) {
			cx = m->w.x + (i % cols) * (cw + target_gappi);
			cy = m->w.y + (i / cols) * (ch + target_gappi);
			if (overcols && i >= n - overcols) {
				cx += dx;
			}
			c->geom.x = cx + target_gappo;
			c->geom.y = cy + target_gappo;
			c->geom.width = cw;
			c->geom.height = ch;
			resize(c, c->geom, 0);
			i++;
		}
	}
}

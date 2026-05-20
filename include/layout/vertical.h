void vertical_overview(Monitor *m) {
	int32_t i, n;
	int32_t cx, cy, cw, ch;
	int32_t dy;
	int32_t rows, cols, overrows;
	Client *c = NULL;
	int32_t target_gappo =
		enablegaps ? m->isoverview ? config.overviewgappo : config.gappov : 0;
	int32_t target_gappi =
		enablegaps ? m->isoverview ? config.overviewgappi : config.gappiv : 0;
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
				ch = (m->w.height - 2 * target_gappo) * single_height_ratio;
				cw = (m->w.width - 2 * target_gappo) * single_width_ratio;
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
		ch = (m->w.height - 2 * target_gappo - target_gappi) / 2;
		cw = (m->w.width - 2 * target_gappo) * 0.65;
		i = 0;
		wl_list_for_each(c, &clients, link) {
			if (c->mon != m)
				continue;

			if (VISIBLEON(c, m) && !c->isunglobal &&
				((m->isoverview && !client_is_x11_popup(c)) || ISTILED(c))) {
				if (i == 0) {
					c->geom.x = m->w.x + (m->w.width - cw) / 2 + target_gappo;
					c->geom.y = m->w.y + target_gappo;
					c->geom.width = cw;
					c->geom.height = ch;
					resize(c, c->geom, 0);
				} else if (i == 1) {
					c->geom.x = m->w.x + (m->w.width - cw) / 2 + target_gappo;
					c->geom.y = m->w.y + ch + target_gappo + target_gappi;
					c->geom.width = cw;
					c->geom.height = ch;
					resize(c, c->geom, 0);
				}
				i++;
			}
		}
		return;
	}

	for (rows = 0; rows <= n / 2; rows++) {
		if (rows * rows >= n) {
			break;
		}
	}
	cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

	cw = (m->w.width - 2 * target_gappo - (cols - 1) * target_gappi) / cols;
	ch = (m->w.height - 2 * target_gappo - (rows - 1) * target_gappi) / rows;

	overrows = n % rows;
	if (overrows) {
		dy = (m->w.height - overrows * ch - (overrows - 1) * target_gappi) / 2 -
			 target_gappo;
	}

	i = 0;
	wl_list_for_each(c, &clients, link) {
		if (c->mon != m)
			continue;

		if (VISIBLEON(c, m) && !c->isunglobal &&
			((m->isoverview && !client_is_x11_popup(c)) || ISTILED(c))) {
			cx = m->w.x + (i / rows) * (cw + target_gappi);
			cy = m->w.y + (i % rows) * (ch + target_gappi);
			if (overrows && i >= n - overrows) {
				cy += dy;
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

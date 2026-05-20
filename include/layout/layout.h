static void overview(Monitor *m);
static void scroller(Monitor *m);
static void vertical_overview(Monitor *m);
static void vertical_scroller(Monitor *m);
static void dwindle(Monitor *m);

/* layout(s) */
Layout overviewlayout = {"󰃇", overview, "overview"};

enum {
	SCROLLER,
	VERTICAL_SCROLLER,
	DWINDLE,
};

Layout layouts[] = {
	// 最少两个,不能删除少于两个
	/* symbol     arrange function   name */
	{"DW", dwindle, "dwindle", DWINDLE},
	{"S", scroller, "scroller", SCROLLER},			 // 滚动布局
	{"VS", vertical_scroller, "vertical_scroller",
	 VERTICAL_SCROLLER},							   // 垂直滚动布局
};
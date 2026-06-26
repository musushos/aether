static void overview(Monitor *m);
static void scroller(Monitor *m);
static void vertical_overview(Monitor *m);
static void vertical_scroller(Monitor *m);
static void dwindle(Monitor *m);
static void floating_layout(Monitor *m);

/* layout(s) */
Layout overviewlayout = {"󰃇", overview, "overview"};

enum {
	FLOATING,
	SCROLLER,
	VERTICAL_SCROLLER,
	DWINDLE,
};

Layout layouts[] = {
	{"󰋰", floating_layout, "floating", FLOATING},   // وضع عائم
	{"DW", dwindle, "dwindle", DWINDLE},
	{"S", scroller, "scroller", SCROLLER},			 // scroll layout
	{"VS", vertical_scroller, "vertical_scroller",
	 VERTICAL_SCROLLER},							   //Vertical scroll layout
};
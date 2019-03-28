/* modifier 0 means no modifier */
static int surfuseragent    = 1;  /* Append Surf version to default WebKit user agent */
static char *fulluseragent  = ""; /* Or override the whole user agent string */
static char *scriptfile     = "~/.surf/script.js";
static char *styledir       = "~/.surf/styles/";
static char *certdir        = "~/.surf/certificates/";
static char *cachedir       = "~/.surf/cache/";
static char *cookiefile     = "~/.surf/cookies.txt";

/* Webkit default features */
/* Highest priority value will be used.
 * Default parameters are priority 0
 * Per-uri parameters are priority 1
 * Command parameters are priority 2
 */
static Parameter defconfig[ParameterLast] = {
	/* parameter                    Arg value       priority */
	[AcceleratedCanvas]   =       { { .i = 1 },     },
	[AccessMicrophone]    =       { { .i = 0 },     },
	[AccessWebcam]        =       { { .i = 0 },     },
	[Certificate]         =       { { .i = 0 },     },
	[CaretBrowsing]       =       { { .i = 0 },     },
	[CookiePolicies]      =       { { .v = "@Aa" }, },
	[DefaultCharset]      =       { { .v = "UTF-8" }, },
	[DiskCache]           =       { { .i = 1 },     },
	[DNSPrefetch]         =       { { .i = 0 },     },
	[FileURLsCrossAccess] =       { { .i = 0 },     },
	[FontSize]            =       { { .i = 12 },    },
	[FrameFlattening]     =       { { .i = 0 },     },
	[Geolocation]         =       { { .i = 0 },     },
	[HideBackground]      =       { { .i = 0 },     },
	[Inspector]           =       { { .i = 0 },     },
	[Java]                =       { { .i = 1 },     },
	[JavaScript]          =       { { .i = 1 },     },
	[KioskMode]           =       { { .i = 0 },     },
	[LoadImages]          =       { { .i = 1 },     },
	[MediaManualPlay]     =       { { .i = 1 },     },
	[Plugins]             =       { { .i = 1 },     },
	[PreferredLanguages]  =       { { .v = (char *[]){ NULL } }, },
	[RunInFullscreen]     =       { { .i = 0 },     },
	[ScrollBars]          =       { { .i = 1 },     },
	[ShowIndicators]      =       { { .i = 1 },     },
	[SiteQuirks]          =       { { .i = 1 },     },
	[SmoothScrolling]     =       { { .i = 0 },     },
	[SpellChecking]       =       { { .i = 0 },     },
	[SpellLanguages]      =       { { .v = ((char *[]){ "en_US", NULL }) }, },
	[StrictTLS]           =       { { .i = 1 },     },
	[Style]               =       { { .i = 1 },     },
	[WebGL]               =       { { .i = 0 },     },
	[ZoomLevel]           =       { { .f = 1.5 },   },
};

static UriParameters uriparams[] = {
	{ "(://|\\.)suckless\\.org(/|$)", {
	  [JavaScript] = { { .i = 0 }, 1 },
	  [Plugins]    = { { .i = 0 }, 1 },
	}, },
};

/* default window size: width, height */
static int winsize[] = { 800, 600 };

static WebKitFindOptions findopts = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                                    WEBKIT_FIND_OPTIONS_WRAP_AROUND;

#define PROMPT_GO   "Go:"
#define PROMPT_FIND "Find:"

/* SETPROP(readprop, setprop, prompt)*/
#define SETPROP(r, s, p) { \
        .v = (const char *[]){ "/bin/sh", "-c", \
             "prop=\"$(printf '%b' \"$(xprop -id $1 $2 " \
             "| sed \"s/^$2(STRING) = //;s/^\\\"\\(.*\\)\\\"$/\\1/\")\" " \
             "| rofi -dmenu -location 7 -width 100% -lines 1 -p \"$4\" " \
             "-m \"wid:$1\" -font 'Iosevka Term ss08 14' " \
             "-theme-str 'window { children: [listview, inputbar]; } listview { scrollbar: false; }')\" " \
             "&& xprop -id $1 -f $3 8s -set $3 \"$prop\"", \
             "surf-setprop", winid, r, s, p, NULL \
        } \
}

/* DOWNLOAD(URI, referer) */
#define DOWNLOAD(u, r) { \
        .v = (const char *[]){ "st", "-e", "/bin/sh", "-c",\
             "curl -g -L -J -O -A \"$1\" -b \"$2\" -c \"$2\"" \
             " -e \"$3\" \"$4\"; read", \
             "surf-download", useragent, cookiefile, r, u, NULL \
        } \
}

/* PLUMB(URI) */
/* This called when some URI which does not begin with "about:",
 * "http://" or "https://" should be opened.
 */
#define PLUMB(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "xdg-open \"$0\"", u, NULL \
        } \
}

/* VIDEOPLAY(URI) */
#define VIDEOPLAY(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "mpv --really-quiet \"$0\"", u, NULL \
        } \
}

/* styles */
/*
 * The iteration will stop at the first match, beginning at the beginning of
 * the list.
 */
static SiteSpecific styles[] = {
	/* regexp               file in $styledir */
	{ ".*",                 "default.css" },
};

/* certificates */
/*
 * Provide custom certificate for urls
 */
static SiteSpecific certs[] = {
	/* regexp               file in $certdir */
	{ "://suckless\\.org/", "suckless.org.crt" },
};

#define MODKEY GDK_CONTROL_MASK

/* hotkeys */
/*
 * If you use anything else but MODKEY and GDK_SHIFT_MASK, don't forget to
 * edit the CLEANMASK() macro.
 */
static Key keys[] = {
	/* mode         modifier                  keyval             function    arg */
	{ ModeNormal,   0,                        GDK_KEY_o,         spawn,      SETPROP("_SURF_URI", "_SURF_GO", PROMPT_GO) },
	{ ModeAll,      MODKEY,                   GDK_KEY_f,         spawn,      SETPROP("_SURF_FIND", "_SURF_FIND", PROMPT_FIND) },
	{ ModeNormal,   0,                        GDK_KEY_slash,     spawn,      SETPROP("_SURF_FIND", "_SURF_FIND", PROMPT_FIND) },

	{ ModeNormal,   0,                        GDK_KEY_i,         setmode,    { .i = ModeInsert } },
	{ ModeAll,      0,                        GDK_KEY_Escape,    setmode,    { .i = ModeNormal } },

	{ ModeAll,      GDK_SHIFT_MASK,           GDK_KEY_Escape,    stop,       { 0 } },
	{ ModeAll,      MODKEY,                   GDK_KEY_c,         stop,       { 0 } },

	{ ModeNormal,   0,                        GDK_KEY_r,         reload,     { .i = 0 } },
	{ ModeNormal,   GDK_SHIFT_MASK,           GDK_KEY_r,         reload,     { .i = 1 } },

	{ ModeNormal,   GDK_SHIFT_MASK,           GDK_KEY_l,         navigate,   { .i = +1 } },
	{ ModeNormal,   GDK_SHIFT_MASK,           GDK_KEY_h,         navigate,   { .i = -1 } },

	/* vertical and horizontal scrolling, in viewport percentage */
	{ ModeNormal,   0,                        GDK_KEY_j,         scrollv,    { .i = +10 } },
	{ ModeNormal,   0,                        GDK_KEY_k,         scrollv,    { .i = -10 } },
	{ ModeNormal,   0,                        GDK_KEY_space,     scrollv,    { .i = +50 } },
	{ ModeNormal,   0,                        GDK_KEY_b,         scrollv,    { .i = -50 } },
	{ ModeNormal,   0,                        GDK_KEY_g,         scrollv,    { .i = -1000000 } },
	{ ModeNormal,   GDK_SHIFT_MASK,           GDK_KEY_g,         scrollv,    { .i = +1000000 } },
	{ ModeNormal,   0,                        GDK_KEY_i,         scrollh,    { .i = +10 } },
	{ ModeNormal,   0,                        GDK_KEY_u,         scrollh,    { .i = -10 } },

	{ ModeAll,      MODKEY,                   GDK_KEY_minus,     zoom,       { .i = -1 } },
	{ ModeAll,      MODKEY,                   GDK_KEY_equal,     zoom,       { .i = +1 } },
	{ ModeAll,      MODKEY,                   GDK_KEY_0,         zoom,       { .i = 0 } },

	{ ModeNormal,   0,                        GDK_KEY_p,         clipboard,  { .i = 1 } },
	{ ModeNormal,   0,                        GDK_KEY_y,         clipboard,  { .i = 0 } },

	{ ModeNormal,   0,                        GDK_KEY_n,         find,       { .i = +1 } },
	{ ModeNormal,   GDK_SHIFT_MASK,           GDK_KEY_n,         find,       { .i = -1 } },
	{ ModeAll,      MODKEY,                   GDK_KEY_n,         find,       { .i = +1 } },
	{ ModeAll,      MODKEY|GDK_SHIFT_MASK,    GDK_KEY_n,         find,       { .i = -1 } },

	{ ModeAll,      MODKEY|GDK_SHIFT_MASK,    GDK_KEY_p,         print,      { 0 } },
	{ ModeAll,      MODKEY,                   GDK_KEY_t,         showcert,   { 0 } },

	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_a,         togglecookiepolicy, { 0 } },
	{ ModeNormal,   0,                        GDK_KEY_F11,       togglefullscreen, { 0 } },
	{ ModeAll,      0,                        GDK_KEY_F12,       toggleinspector, { 0 } },

	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_c,         toggle,     { .i = CaretBrowsing } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_f,         toggle,     { .i = FrameFlattening } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_g,         toggle,     { .i = Geolocation } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_s,         toggle,     { .i = JavaScript } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_i,         toggle,     { .i = LoadImages } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_v,         toggle,     { .i = Plugins } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_b,         toggle,     { .i = ScrollBars } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_t,         toggle,     { .i = StrictTLS } },
	{ ModeNormal,   MODKEY|GDK_SHIFT_MASK,    GDK_KEY_m,         toggle,     { .i = Style } },
};

/* button definitions */
/* target can be OnDoc, OnLink, OnImg, OnMedia, OnEdit, OnBar, OnSel, OnAny */
static Button buttons[] = {
	/* target       event mask      button  function        argument        stop event */
	{ OnLink,       0,              2,      clicknewwindow, { .i = 0 },     1 },
	{ OnLink,       MODKEY,         2,      clicknewwindow, { .i = 1 },     1 },
	{ OnLink,       MODKEY,         1,      clicknewwindow, { .i = 1 },     1 },
	{ OnAny,        0,              8,      clicknavigate,  { .i = -1 },    1 },
	{ OnAny,        0,              9,      clicknavigate,  { .i = +1 },    1 },
	{ OnMedia,      MODKEY,         1,      clickexternplayer, { 0 },       1 },
};

static SearchEngine searchengines[] = {
    { "d", "https://duckduckgo.com/?q=%s&kp=-1&kl=us-en" },
    { "w", "https://en.wikipedia.org/wiki/%s" },
};

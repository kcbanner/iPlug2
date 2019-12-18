/*
 * Copyright (C) Alexey Zhelezov, 2019 www.azslow.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
/*
 * XCB based toolkit.
 */
#ifndef __XCBT_H
#define __XCBT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <xcb/xcb.h>

// it should match defines and common_atom_names in the code
#define XCBT_COMMON_ATOMS_COUNT 3


/**
 * Rectangle
 */
typedef struct {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} xcbt_rect;

/**
 * Connection is opaque structure. Do not use directly.
 */
typedef struct xcbt_ {
  xcb_connection_t *conn;       // xcb connection
  int               def_screen; // default screen
  xcb_atom_t        catoms[XCBT_COMMON_ATOMS_COUNT]; // common atoms
  
  void             *xlib_dpy;
} *xcbt;

/**
 * Window is opaque structure. Do not use directly.
 */
typedef struct xcbt_window_ {
  xcbt x;
  xcb_window_t wnd;
  int screen;  
  xcb_window_t prt;
  xcbt_rect pos; // position inside parent (always, top window will get spos set)
  int mapped; // bool
} *xcbt_window;

/**
 * Event handler. If chained, user code is responsible for saving previous handler and user data
 * in the chain and call them when required.
 * 
 * Parameters:
 *   evt - event to process, 
 *         When NULL, the window is about to be destroyed (X window is already destroyed).
 *         In this special case, previous handler must be called.
 *   udata - the data set with xcbt_window_set_handler call
 */
typedef void (*xcbt_window_handler)(xcbt_window xw, xcb_generic_event_t *evt, void *udata);

/**
 * Some atoms, require XCBT_INIT_ATOMS during connection
 * 
 * WARNING: no checks for x validity and can return 0 when is(could) not initialize(d)
 */
#define XCBT_WM_PROTOCOLS(x) ((x)->catoms[0])
#define XCBT_WM_DELETE_WINDOW(x) ((x)->catoms[1])
#define XCBT_XEMBED_INFO(x) ((x->catoms[2]))

/**
 * Flags for xcbt_connect
 */
typedef enum {
  XCBT_USE_GL = 1, // allow GL rendering
  XCBT_INIT_ATOMS = 2, // ascure common atoms during connect
} XCBT_CONNECT_FLAGS;

/**
 * Return XCB connection
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_conn(x) ((x)?(x)->conn:(xcb_connection_t *)NULL)

/**
 * Returns default screen
 * 
 * Return:
 *   screen number or -1
 */
#define xcbt_default_screen(x) ((x)?(x)->def_screen:-1)


/**
 * Can be used to check the connect is XLib (and so GL) compatible
 * 
 * Return:
 *   XLib display (as void *), if connection is initiated by XLib
 */
#define xcbt_display(x) ((x)?(x)->xlib_dpy:NULL) 

/**
 * xcb_flush wrapper
 * 
 */
#define xcbt_flush(x) xcb_flush((xcb_connection_t *)x);

/**
 * Initialize xcbt
 * 
 * Parameters:
 *   flags   - bit set of XCBT_CONNECT_FLAGS
 * 
 * Return:
 *   Initialized xcbt or NULL in case of errors.
 */
xcbt xcbt_connect(uint32_t flags);

/**
 * Finalize xcbt
 * 
 * Parameters:
 *   x - xcbt to disconnect (can be NULL)
 */
void xcbt_disconnect(xcbt x);

/**
 * Sync with X server by requesting current focus.
 * Note: does flush internally.
 */
int xcbt_sync(xcbt x);


/**
 * Return connection for window
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_window_conn(xw) ((xw)?(xw)->x->conn:NULL)

/**
 * Return XID of window
 *
 * Return:
 *   X window on 0
 */
#define xcbt_window_xwnd(xw) ((xw)?(xw)->wnd:(xcb_window_t)0)

/**
 * Return connection for window
 *
 * Return:
 *   X window on 0
 */
#define xcbt_window_x(xw) ((xw)?(xw)->x:NULL)


/**
 * Return screen for window
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_window_screen(xw) ((xw)?(xw)->screen:-1)

/**
 * Return XCB parent window
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_window_xprt(xw) ((xw)?(xw)->prt:(xcb_window_t)0)

/**
 * xcb_change_window_attributes wrapper
 *
 */
#define xcbt_window_change_attributes(xw, vm, vl) xcb_change_window_attributes(xcbt_window_conn(xw), xcbt_window_xwnd(xw), vm, vl)

/**
 * Set _XEMBED_INFO property, reflecting known mapping state
 */
void xcbt_window_set_xembed_info(xcbt_window pxw);

/**
 * Finalize window.
 * That always should be done explictly, the structure is not automatically destoyed with X window,
 * but X window is also destroyed during this call.
 * 
 *   X Server is asked to destroy the window, all created resources are also freed.
 * 
 * Parameters:
 *   xw - window to destroy
 */
void xcbt_window_destroy(xcbt_window xw);

/**
 * When we destroy X parent, X child windows will be destroyed in the background.
 * The process is async, so children are not yet informed about the fact from X server side
 * right after we send this request. Attempts to use not existing XID will fail.
 */
void xcbt_window_parent_destroyed(xcbt_window xw);

/**
 * Create GL window
 * 
 * Parameters:
 *   prt - parent, should not be root since only child GL windows are supported
 *   pos - position within parent
 *   gl_major, gl_minor - GL version to support (2.1, 3.0, etc.)
 *   debug - set to non zero to enable debugging
 * 
 * Return:
 *   created window or NULL
 */
xcbt_window xcbt_window_gl_create(xcbt x, xcb_window_t prt, const xcbt_rect *pos, int gl_major, int gl_minor, int debug);

/**
 * Create top level window
 * 
 * Parameters:
 *   screen - screen for this window
 *   title - title for the window (ASCII) or NULL
 *   pos - position within parent
 * 
 * Return:
 *   created window or NULL
 */
xcbt_window xcbt_window_top_create(xcbt x, int screen, const char *title, const xcbt_rect *pos);

/**
 * Map (show) the window)
 * 
 * Parameters:
 *   xw - window to show
 */
void xcbt_window_map(xcbt_window xw);

/**
 * Unmap (hide) the window)
 * 
 * Parameters:
 *   xw - window to show
 */
void xcbt_window_unmap(xcbt_window xw);

/**
 * Wait till the window is mapped.
 * Note: that is waiting call, potentially dangerous at the moment. 
 *   To use just after mapping for embeded windows, before reporting XID to embedder.
 */
int xcbt_window_wait_map(xcbt_window xw);

/**
 * For GL window set context
 * 
 * Parameter:
 *   xw - window
 * 
 * Return:
 *   activated GL context or NULL
 */
void *xcbt_window_draw_begin(xcbt_window xw);

/**
 * Finish GL draw and show the drawing.
 * 
 * Parameter:
 *   xw - window
 * 
 * Return:
 *   non zero if draw was really stopped (was not called inside another paint)
 * 
 */
int xcbt_window_draw_end(xcbt_window xw);

/**
 * Finish GL draw, but discard the drawing.
 * 
 * Parameter:
 *   xw - window
 * 
 * Return:
 *   non zero if draw was really stopped (was not called inside another paint)
 */
int xcbt_window_draw_stop(xcbt_window xw);

/**
 * Get actual window client size
 * 
 * PArameters:
 *  pr - client size (x and y will be 0)
 */
void xcbt_window_get_client_size(xcbt_window xw, xcbt_rect *pr);

/**
 * Get screen information for specified screen
 * 
 * Return:
 *   screen information (connection live time, should not be freed) or NULL
 */
xcb_screen_t *xcbt_screen_info(xcbt px, int screen);

/**
 * Set event handler and user data, old hander and user data are returned
 * (should be preserved and called when needed).
 * 
 * Parameters:
 *   new_handler, new_data - new hander and data (data can be NULL, handler does not)
 *   old_handler, old_data - place to store current hander and data, should not be NULL
 * 
 * Return:
 *   non zero on success
 */
int xcbt_window_set_handler(xcbt_window xw, xcbt_window_handler new_handler, void *new_data, xcbt_window_handler *old_handler, void **old_data);

/**
 * Create new GC for the window
 * 
 * Parameters:
 *   as in xcb_create_gc
 * 
 * 
 * Returns:
 *   new context or 0
 */
xcb_gcontext_t xcbt_window_create_gc(xcbt_window xw, uint32_t value_mask, const uint32_t *value_list);

/**
 * Process all pending events and timers, without waiting for new events
 * 
 * Return the nearest timer or -1 (so any returned value except 0 is not a error)
 */
int xcbt_process(xcbt px);

/**
 * Event loop.
 * 
 * Parameters:
 *   exit_cond - a pointer to variable, when this variable is not zero the loop exit. When NULL, only currently pending events are processed.
 *
 */
void xcbt_event_loop(xcbt px, int *exit_cond);

/**
 *
 * User defined timer callback.
 * 
 * Parameters:
 *   id - timer id
 *   udata - given by xcbt_timer_set
 */

typedef void (*xcbt_timer_cb)(xcbt px, int timer_id, void *udata);

/**
 * Set/unset timer. Timer is removed when triggered.
 * 
 * Parameters:
 *   id  - id (positive) for the timer, -1 means remove all currently defined timers for specified window (msec is ignored)
 *   msec - time in milliseconds from now when the timer should be triggerer, when <0 remove specified by id timer
 */
void xcbt_timer_set(xcbt px, int timer_id, int msec, xcbt_timer_cb cb, void *udata);

/**
 * 
 * "Main loop" is processing events and timers. There is no general main loop on Linux, so xcbt support 3 modes of operations:
 *  * own main loop
 *      Use (blocking) xcbt_event_loop for the application live time.
 *  * integrating into external main loop using polling. 
 *      Call (non blocking) xcbt_process periodically, more then 50ms can produce visible "lag" for the user. Also clearly limit
 *      any animation speed.
 *  * integrating using (up to) 2 monitoring file descriptors and 1 (one shot) timer.
 *      xcbt_embed_set should be called, xcbt_process still can be used when desired. 
 *
 * 
 *  set_timer : should schedule timer after around specified milliseconds (no precision expected) or remove it
 *  watch     : monitor the input of specified fd, user code should support at least 2 different fds. Remove all monitoring if fd is negaive.
 *  
 *    in both cases (timer or input available) user code should call xcbt_process.
 */
typedef struct xcbt_embed_ {
  void (*dtor)(struct xcbt_embed_ *e);
  int  (*set_x)(struct xcbt_embed_ *e, xcbt x);
  int  (*set_timer)(struct xcbt_embed_ *e, int msec); // milliseconds
  int  (*watch)(struct xcbt_embed_ *e, int fd);
} xcbt_embed;


/**
 * call dtor for embed
 */
#define xcbt_embed_dtor(e) ({if(e) (e)->dtor(e);})

/**
 * Set or unset embed main loop processing. 
 * 
 * Parameters:
 *  e - embed interface to use, it should stay alive till xcbt is destroyed or its embed is replaces
 * 
 * Return zero in case of immediate errors (so the user knows the app can not work as desired) 
 */
int xcbt_embed_set(xcbt px, xcbt_embed *e);


/**
 * GLib (GDK/GTK) embedding factory
 */
xcbt_embed *xcbt_embed_glib();


#ifdef __cplusplus
};
#endif
#endif  /* __XCBT_H */

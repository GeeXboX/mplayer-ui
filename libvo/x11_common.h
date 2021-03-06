
#ifndef X11_COMMON_H
#define X11_COMMON_H

#ifdef X11_FULLSCREEN

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define vo_wm_LAYER 1
#define vo_wm_FULLSCREEN 2
#define vo_wm_STAYS_ON_TOP 4
#define vo_wm_ABOVE 8
#define vo_wm_BELOW 16
#define vo_wm_NETWM (vo_wm_FULLSCREEN | vo_wm_STAYS_ON_TOP | vo_wm_ABOVE | vo_wm_BELOW)

/* EWMH state actions, see
	 http://freedesktop.org/Standards/wm-spec/index.html#id2768769 */
#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

extern int metacity_hack;
extern int vo_fsmode;

extern int vo_depthonscreen;
extern int vo_screenwidth;
extern int vo_screenheight;
extern int vo_dwidth;
extern int vo_dheight;
extern int vo_fs;
extern int vo_fs_layer;
extern int vo_wm_type;
extern int vo_fs_type;
extern char** vo_fstype_list;
extern int vo_ontop;
extern int vo_rootwin;

extern char *mDisplayName;
extern Display *mDisplay;
extern Window mRootWin;
extern int mScreen;
extern int mLocalDisplay;
extern int WinID;

extern int vo_mouse_timer_const;
extern int vo_mouse_autohide;

extern int vo_init( void );
extern void vo_uninit( void );
extern void vo_hidecursor ( Display* , Window );
extern void vo_showcursor( Display *disp, Window win );
extern void vo_x11_decoration( Display * vo_Display,Window w,int d );
extern void vo_x11_classhint( Display * display,Window window,char *name );
extern void vo_x11_nofs_sizepos(int x, int y, int width, int height);
extern void vo_x11_sizehint( int x, int y, int width, int height, int max );
extern int vo_x11_check_events(Display *mydisplay);
extern void vo_x11_selectinput_witherr(Display *display, Window w, long event_mask);
extern void vo_x11_fullscreen( void );
extern void vo_x11_setlayer( Display * mDisplay,Window vo_window,int layer );
extern void vo_x11_uninit();
extern Colormap vo_x11_create_colormap(XVisualInfo *vinfo);
extern uint32_t vo_x11_set_equalizer(char *name, int value);
extern uint32_t vo_x11_get_equalizer(char *name, int *value);
extern void fstype_help(void);
extern Window vo_x11_create_smooth_window( Display *mDisplay, Window mRoot,
	Visual *vis, int x, int y, unsigned int width, unsigned int height,
	int depth, Colormap col_map);
extern void vo_x11_clearwindow_part(Display *mDisplay, Window vo_window,
	int img_width, int img_height, int use_fs);
extern void vo_x11_clearwindow( Display *mDisplay, Window vo_window );
extern void vo_x11_ontop();
extern void vo_x11_ewmh_fullscreen( int action );

#endif

extern Window     vo_window;
extern GC         vo_gc;
extern XSizeHints vo_hint;

#ifdef HAVE_XV
extern int vo_xv_set_eq(uint32_t xv_port, char * name, int value);
extern int vo_xv_get_eq(uint32_t xv_port, char * name, int *value);
#endif

#ifdef HAVE_NEW_GUI
 extern void vo_setwindow( Window w,GC g );
 extern void vo_x11_putkey(int key);
#endif

void saver_off( Display * );
void saver_on( Display * );

#ifdef HAVE_XINERAMA
void vo_x11_xinerama_move(Display *dsp, Window w);
#endif

#ifdef HAVE_XF86VM
void vo_vm_switch(uint32_t, uint32_t, int*, int*);
void vo_vm_close(Display*);
#endif

int vo_find_depth_from_visuals(Display *dpy, int screen, Visual **visual_return);

#endif


#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include "defs.h"
#include "type.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_XLIB_IMPLEMENTATION
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_xlib.h"

#define DTIME           20
#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   600

#define UNUSED(a) (void)a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])


//#include "style.c"

#include "err_msg.h"
#include "instr.h"
#include "import.h"
#include "lang.h"
#include "vm.h"

static m_uint active = 0;
/*static struct nk_context* CTX = NULL;*/
typedef struct XWindow XWindow;
struct XWindow {
    Display *dpy;
    Window root;
    Visual *vis;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    int screen;
    XFont *font;
    unsigned int width;
    unsigned int height;
};

typedef struct GWindow
{
/*  m_str* name;*/
  struct XWindow*    win;
  struct nk_context* ctx;
  Vector widget;
/*  Vector event;*/
  long started, dt, running;
  pthread_t thread;
} GWindow;

static GWindow* last_window = NULL;
static M_Object last_widget = NULL;

static void // TODO use gw_err instead
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static void*
xcalloc(size_t siz, size_t n)
{
    void *ptr = calloc(siz, n);
    if (!ptr) die("Out of memory\n");
    return ptr;
}

static long
timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}

static void
sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the include
 * and the corresponding function. */
/*#include "../style.c"*/
/*#include "../calculator.c"*/
/*#include "../overview.c"*/
/*#include "../node_editor.c"*/

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
static m_int o_nk_data;
static m_int o_nk_name;
static m_int o_nk_exec;
static m_int o_nk_gwin;
static m_int o_nk_list;
static m_int o_nk_ival;
static m_int o_nk_parent;
  static m_int o_nk_imin;
  static m_int o_nk_imax;
  static m_int o_nk_istp;
  static m_int o_nk_iinc;
static m_int o_nk_fval;
  static m_int o_nk_fmin;
  static m_int o_nk_fmax;
  static m_int o_nk_fstp;
  static m_int o_nk_finc;
/*static m_int o_nk_sval;*/


/*m_int o_event_shred = SZ_INT;*/
#define LIST(o) *(Vector*)(o->data + o_nk_list)
#define NAME(o) STRING(*(M_Object*)(o->data + o_nk_name))
typedef void (*f_nk)(M_Object o, struct nk_context* ctx);


static m_int o_nk_r, o_nk_g, o_nk_b, o_nk_a;
#define R(o) *(o->data + o_nk_r)
#define G(o) *(o->data + o_nk_g)
#define B(o) *(o->data + o_nk_b)
#define A(o) *(o->data + o_nk_a)
#define COLOR(o) (struct nk_color){ R(o), G(o), B(o), A(o) }

static void* _loop(void* data)
{
  m_uint i;
  GWindow* gw = (GWindow*)data;
  XWindow* xw = gw->win;
  struct nk_context* ctx = gw->ctx;
  xw->font = nk_xfont_create(xw->dpy, "fixed");
  ctx = nk_xlib_init(xw->font, xw->dpy, xw->screen, xw->win, xw->width, xw->height);
  while(1)
  {
//    XLockDisplay(xw->dpy);
    XEvent evt;
    nk_input_begin(ctx);
    while (XCheckWindowEvent(xw->dpy, xw->win, xw->swa.event_mask, &evt)){
        if (XFilterEvent(&evt, xw->win)) continue;
        nk_xlib_handle_event(xw->dpy, xw->screen, xw->win, &evt);
    }
    nk_input_end(ctx);

        /* GUI */
/*        if (nk_window_is_closed(ctx, "Demo")) break;*/
          for(i = 0; i < vector_size(gw->widget); i++)
          {
            M_Object o = (M_Object)vector_at(gw->widget, i);
            f_nk exec = *(f_nk*)(o->data + o_nk_exec);
            exec(o, ctx);
          }
        /* Draw */
        XClearWindow(xw->dpy, xw->win);
        nk_xlib_render(xw->win, nk_rgb(30,30,30));
        XFlush(xw->dpy);
        /* Timing */
/*        gw->dt = timestamp() - gw->started;*/
/*        if (gw->dt < DTIME)*/
/*            sleep_for(DTIME - gw->dt);*/
/*        XUnlockDisplay(xw->dpy);*/
//    XUnlockDisplay(xw->dpy);
            usleep(10000);

//    XUnlockDisplay(xw->dpy);
    } 
}

 static void gw_nk_init(GWindow* gw, VM_Shred shred)
{
  XWindow* xw = gw->win;
  if(!active)
    XInitThreads();
  gw->ctx = malloc(sizeof(struct nk_context));
  if(!active) {
    xw->dpy = XOpenDisplay(NULL);
    xw->root = DefaultRootWindow(xw->dpy);
    xw->screen = XDefaultScreen(xw->dpy);
    xw->vis = XDefaultVisual(xw->dpy, xw->screen);
    xw->cmap = XCreateColormap(xw->dpy,xw->root,xw->vis,AllocNone);
    xw->swa.colormap = xw->cmap;
    xw->swa.event_mask =
    ExposureMask | KeyPressMask | KeyReleaseMask |
    ButtonPress | ButtonReleaseMask| ButtonMotionMask |
    Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
    PointerMotionMask | KeymapStateMask;
    xw->win = XCreateWindow(xw->dpy, xw->root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
      XDefaultDepth(xw->dpy, xw->screen), InputOutput,
      xw->vis, CWEventMask | CWColormap, &xw->swa);
  }
  else {
    xw->dpy    = last_window->win->dpy;
    xw->root   = last_window->win->root;
    xw->screen = last_window->win->screen;
    xw->vis = XDefaultVisual(xw->dpy, xw->screen);
    xw->cmap     = last_window->win->cmap;
    xw->cmap = XCreateColormap(xw->dpy,xw->root,xw->vis,AllocNone);
    xw->swa.colormap = xw->cmap;
    xw->swa.event_mask =
      ExposureMask | KeyPressMask | KeyReleaseMask |
      ButtonPress | ButtonReleaseMask| ButtonMotionMask |
      Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
      PointerMotionMask | KeymapStateMask;
    xw->win = XCreateWindow(xw->dpy, xw->root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
      XDefaultDepth(xw->dpy, xw->screen), InputOutput,
      xw->vis, CWEventMask | CWColormap, &xw->swa);
  }
  if(!xw->dpy) die("Could not open a display; perhaps $DISPLAY is not set?");
  char name[256];
  sprintf(name, "GWion: %p", gw);
  XStoreName(xw->dpy, xw->win, name);
  XMapWindow(xw->dpy, xw->win);
  XGetWindowAttributes(xw->dpy, xw->win, &xw->attr);
  xw->width = (unsigned int)xw->attr.width;
  xw->height = (unsigned int)xw->attr.height;
  ++active;
}

static void nk_ctor(M_Object o, VM_Shred shred) {
  GWindow* gw = malloc(sizeof(GWindow));
  gw->win = malloc(sizeof(XWindow));
  gw->widget = new_vector();
  gw_nk_init(gw, shred);
  last_window = gw;
  pthread_create(&gw->thread, NULL, &_loop, gw);
  *(GWindow**)(o->data + o_nk_data) = gw;
}

static void gw_nk_shutdown(struct XWindow* xw) {
//  XUnmapWindow(xw->dpy, xw->win);
//  if(!active)
//    XFreeColormap(xw->dpy, xw->cmap);
//  XDestroyWindow(xw->dpy, xw->win);
//  if(!active)
  if(!--active) {
//    nk_xfont_del(xw->dpy, xw->font);
//    XCloseDisplay(xw->dpy);
//    nk_xlib_shutdown();
  }
}
static void nk_dtor(M_Object o, VM_Shred shred)
{
  GWindow* gw = *(GWindow**)(o->data + o_nk_data);
  pthread_cancel(gw->thread);
  pthread_join(gw->thread, NULL);
//  XUnmapWindow(gw->win->dpy, gw->win->win);
//  XDestroyWindow(gw->win->dpy, gw->win->win);
  gw_nk_shutdown(gw->win);
  free(gw->ctx);
  for(m_uint i = vector_size(gw->widget) + 1; --i;)
    release((M_Object)vector_at(gw->widget, i), shred);
  vector_release(gw->widget);
  free(gw->win);
  free(gw);
  *(GWindow**)(o->data + o_nk_data) = NULL;
}


static void widget_ctor(M_Object o, VM_Shred shred)
{
  char name[256];
  sprintf(name, "Widget:%p", o);
  (*(M_Object*)(o->data + o_nk_name)) = new_String(NULL, name);
  (*(GWindow**)(o->data + o_nk_gwin)) = last_window;
  (*(M_Object*)(o->data + o_nk_parent)) = last_widget;
  if(last_widget && strcmp(o->type_ref->name, "NkLayout")
    && strcmp(o->type_ref->name, "NkMenu"))
    vector_add(LIST(last_widget), (vtype)o);
}

static m_uint o_nk_align;
static void sval_ctor(M_Object o, VM_Shred shred)
{
/*  *(f_nk*)(o->data + o_nk_exec) = label_execute;*/
  *(m_uint*)(o->data + o_nk_align) = NK_TEXT_LEFT;
}
static m_uint o_nk_wrap, o_nk_labelcolor;
static void label_execute(M_Object o, struct nk_context* ctx)
{
  if(*(M_Object*)(o->data + o_nk_labelcolor))
  {
    M_Object color = *(M_Object*)(o->data + o_nk_labelcolor);
    if(*(m_uint*)(o->data + o_nk_wrap))
      nk_label_colored_wrap(ctx, NAME(o), COLOR(color));           
    else
      nk_label_colored(ctx, NAME(o), *(m_uint*)(o->data + o_nk_align), COLOR(color));
  }
  else
  {
    if(*(m_uint*)(o->data + o_nk_wrap))
      nk_label_wrap(ctx, NAME(o));           
    else
      nk_label(ctx, NAME(o), *(m_uint*)(o->data + o_nk_align));
  }
}
static void label_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = label_execute;
}

static void text_execute(M_Object o, struct nk_context* ctx)
{
  if(*(m_uint*)(o->data + o_nk_wrap))
    nk_text_wrap(ctx, NAME(o), strlen(NAME(o)));           
  else
    nk_text(ctx, NAME(o), strlen(NAME(o)), *(m_uint*)(o->data + o_nk_align));           
}
static void text_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = text_execute;
}

static m_uint o_nk_select;
static void slabel_execute(M_Object o, struct nk_context* ctx)
{
  if(nk_selectable_label(ctx, NAME(o), *(m_uint*)(o->data + o_nk_align), (int*)&*(m_uint*)(o->data + o_nk_select)))
    broadcast(o);
}
static void slabel_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = label_execute;
}

static void stext_execute(M_Object o, struct nk_context* ctx)
{
  if(nk_selectable_text(ctx, NAME(o), strlen(NAME(o)), *(m_uint*)(o->data + o_nk_align), (int*)&*(m_uint*)(o->data + o_nk_select)))
    broadcast(o);
}
static void stext_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = text_execute;
}

static m_int o_nk_prog, o_nk_progmax, o_nk_progmod;
static void prog_execute(M_Object o, struct nk_context* ctx)
{
  if(nk_progress(ctx, (m_uint*)(o->data + o_nk_prog), *(m_uint*)(o->data + o_nk_progmax), *(m_uint*)(o->data + o_nk_progmod)))
   broadcast(o);
}
static void prog_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = prog_execute;
  *(m_uint*)(o->data + o_nk_prog) = 0;
  *(m_uint*)(o->data + o_nk_progmax) = 100;
  *(m_uint*)(o->data + o_nk_progmod) = 1;
}


static m_int o_nk_behavior, o_nk_button_color;
static void button_execute(M_Object o, struct nk_context* ctx)
{
  if(*(m_uint*)(o->data + o_nk_button_color))
  {
printf("%p %i %p\n", EV_SHREDS(o), vector_size(EV_SHREDS(o)), *(M_Object*)(o->data + o_nk_button_color));
  M_Object color = *(M_Object*)(o->data + o_nk_button_color);
printf("%i %i %i %i\n", R(color), G(color), B(color), A(color));
struct nk_color c = COLOR(color);
if(nk_button_color(ctx, COLOR(color)))
/*      exit(2);*/
      broadcast(o);

}
  else if(NAME(o))
  {
      if(nk_button_label(ctx, NAME(o)))
        broadcast(o);
  }
}
static void button_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = button_execute;
}
static void group_exec(M_Object o, struct nk_context* ctx)
{
  m_uint i;
  Vector v = LIST(o);
  for(i = 0; i < vector_size(v); i++)
  {
    M_Object obj = (M_Object)vector_at(v, i);
    f_nk exec = *(f_nk*)(obj->data + o_nk_exec);
    exec(obj, ctx);
  }
}
static void group_ctor(M_Object o, VM_Shred shred)
{
  LIST(o)= new_vector();
  last_widget = o;
}
static void group_dtor(M_Object o, VM_Shred shred)
{
  free_vector(LIST(o));
  last_widget = o;
}
static MFUN(group_end)
{
  last_widget = (*(M_Object*)(o->data + o_nk_parent));
}

static MFUN(group_begin)
{
  last_widget = o;
}

static m_int o_nk_rowcol;
static m_int o_nk_rowh;
static m_int o_nk_roww;
static m_int o_nk_static;
static void row(M_Object o, struct nk_context* ctx)
{
  if(*(m_uint*)(o->data + o_nk_static))
    nk_layout_row_static(ctx, *(m_uint*)(o->data + o_nk_rowh), 
      *(m_uint*)(o->data + o_nk_roww), *(m_uint*)(o->data + o_nk_rowcol));
  else
    nk_layout_row_dynamic(ctx, *(m_uint*)(o->data + o_nk_rowh), 
      *(m_uint*)(o->data + o_nk_rowcol));

}
static void rowd_execute(M_Object o, struct nk_context* ctx)
{
  row(o, ctx);
  group_exec(o, ctx);
}

static void rowd_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec)) = rowd_execute;
  *(m_uint*)(o->data + o_nk_rowcol) = 1;
  *(m_uint*)(o->data + o_nk_rowh) = 30;
  *(m_uint*)(o->data + o_nk_roww) = 100;
}

static m_int o_nk_x;
static m_int o_nk_y;
static m_int o_nk_w;
static m_int o_nk_h;
static m_int o_nk_flags;
static void layout_execute(M_Object o, struct nk_context* ctx)
{
  m_uint i;
  Vector v = LIST(o);
  char name[256];
  sprintf(name, "%p", o);
  if (nk_begin_titled(ctx, name,  NAME(o),
      nk_rect(*(m_uint*)(o->data + o_nk_x),
        *(m_uint*)(o->data + o_nk_y),
        *(m_uint*)(o->data + o_nk_w),
        *(m_uint*)(o->data + o_nk_h)),
    *(m_uint*)(o->data + o_nk_flags)))
  {
    row(o, ctx);
    group_exec(o, ctx);
  }
  nk_end(ctx);
}

static void layout_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec))    = layout_execute;
  (*(m_uint*)(o->data + o_nk_x))     = 50;
  (*(m_uint*)(o->data + o_nk_y))     = 50;
  (*(m_uint*)(o->data + o_nk_w))     = 400;
  (*(m_uint*)(o->data + o_nk_h))     = 400;
  (*(m_uint*)(o->data + o_nk_flags)) = 
    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
    NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE;
  
  if(last_window)
  {
    M_Object obj = (M_Object)vector_at(last_window->widget, vector_size(last_window->widget) - 1);
    if(obj)
    {
      if(vector_find(LIST(obj), (vtype)o) > -1)
      vector_rem(LIST(obj), vector_find(LIST(obj), (vtype)o));
    }
    vector_add(last_window->widget, (vtype)o);
  }
}

static m_int o_nk_state;
static void tree_execute(M_Object o, struct nk_context* ctx)
{
  int i;
  if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
    broadcast(o);
  if(nk_tree_push_id(ctx, *(m_uint*)(o->data + o_nk_state), NAME(o), i, (m_uint)o))
  {
    row(o, ctx);
    group_exec(o, ctx);
    nk_tree_pop(ctx);
  }
}
static void tree_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec)) = tree_execute;
  (*(m_uint*)(o->data + o_nk_state)) = NK_TREE_TAB;
}

static m_int o_nk_comboval;
static void combo_execute(M_Object o, struct nk_context* ctx)
{
  m_uint i;
  struct nk_vec2 vec = {200.0, 200.0};
  if(nk_combo_begin_label(ctx, NAME(o), vec))
  {
    row(o, ctx);
    for(i = 0; i < vector_size(LIST(o)); i++)
    {
      if(nk_combo_item_label(ctx, STRING(vector_at(LIST(o), i)), NK_TEXT_LEFT))
      {
        *(m_uint*)(o->data + o_nk_comboval) = i;
        broadcast(o);
        break;
      }
    }
    nk_combo_end(ctx);
  }
}
static void combo_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = combo_execute;
}
static MFUN(combo_add)
{
  vector_add(LIST(o), (vtype)*(M_Object*)(shred->mem + SZ_INT));
  *(m_uint*)RETURN = (m_uint)*(M_Object*)(shred->mem + SZ_INT);
}

static void menubar_execute(M_Object o, struct nk_context* ctx)
{
  m_uint i;
  struct nk_panel menu;
  nk_menubar_begin(ctx);
  group_exec(o, ctx);
  nk_menubar_end(ctx);
}
static void menubar_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = menubar_execute;
}
static MFUN(menubar_add)
{
  vector_add(LIST(o), (vtype)*(M_Object*)(shred->mem + SZ_INT));
  *(m_uint*)RETURN = (m_uint)*(M_Object*)(shred->mem + SZ_INT);
}

static m_int  o_nk_menuval;
static void menu_execute(M_Object o, struct nk_context* ctx)
{
  m_uint i;
  struct nk_vec2 vec = {200.0, 200.0};
  if(nk_menu_begin_label(ctx, NAME(o), NK_TEXT_LEFT, vec))
  {
    for(i = 0; i < vector_size(LIST(o)); i++)
    {
      row(o, ctx);
      if(nk_menu_item_label(ctx, STRING(vector_at(LIST(o), i)), NK_TEXT_LEFT))
      {
        *(m_uint*)(o->data + o_nk_comboval) = i;
        broadcast(o);
        break;
      }
    }
    nk_menu_end(ctx);
  }
}

static void menu_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = menu_execute;
}
static MFUN(menu_add)
{
  vector_add(LIST(o), (vtype)*(M_Object*)(shred->mem + SZ_INT));
  *(m_uint*)RETURN = (m_uint)*(M_Object*)(shred->mem + SZ_INT);
}

static m_int o_nk_edit_type;
static void nkstring_execute(M_Object o, struct nk_context* ctx)
{
  char c[512];
  memset(c, 0, 512);
  strcat(c, NAME(o));
  nk_layout_row_static(ctx, 180, 278, 1);
  nk_edit_string_zero_terminated(ctx, *(m_uint*)(o->data + o_nk_edit_type), c, 512, nk_filter_default);           
  NAME(o) = strdup(c);
}

static void nkstring_ctor(M_Object o, VM_Shred shred)
{
  *(f_nk*)(o->data + o_nk_exec) = nkstring_execute;
  *(m_uint*)(o->data + o_nk_edit_type) = NK_EDIT_SIMPLE;
/*  *(m_uint*)(o->data + o_nk_edit_type) = NK_EDIT_BOX;*/
}

static void check_execute(M_Object o, struct nk_context* ctx)
{
  if(nk_checkbox_label(ctx, NAME(o), (int*)&*(m_int*)(o->data + o_nk_ival)))
    broadcast(o);
}
static void check_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec)) = check_execute;
}

static void propi_execute(M_Object o, struct nk_context* ctx)
{
  m_int property = *(m_int*)(o->data + o_nk_ival);
  nk_property_int(ctx, NAME(o), *(m_int*)(o->data + o_nk_imin), (int*)&*(m_int*)(o->data + o_nk_ival),
    *(m_int*)(o->data + o_nk_imax), *(m_int*)(o->data + o_nk_istp), *(m_float*)(o->data + o_nk_iinc));
  if(property != *(m_int*)(o->data + o_nk_ival))
    broadcast(o);
}
static void propi_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec)) = propi_execute;
  *(m_int*)(o->data + o_nk_imax) = 100;
  *(m_int*)(o->data + o_nk_istp) = 10;
  *(m_float*)(o->data + o_nk_iinc) = 1.0;
}

static void slideri_execute(M_Object o, struct nk_context* ctx)
{
  if(nk_slider_int(ctx, *(m_int*)(o->data + o_nk_imin), (int*)&*(m_int*)(o->data + o_nk_ival),
    *(m_int*)(o->data + o_nk_imax), *(m_int*)(o->data + o_nk_istp)))
    broadcast(o);
}
static void slideri_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec)) = slideri_execute;
  *(m_int*)(o->data + o_nk_imax) = 1.0;
  *(m_int*)(o->data + o_nk_ival) = 0.0;
  *((m_int*)((M_Object)o)->data + o_nk_istp) = 0.1; // ? BUG
}

static void propf_execute(M_Object o, struct nk_context* ctx)
{
  m_float property = *(m_float*)(o->data + o_nk_fval);
#ifdef USE_DOUBLE
  nk_property_double(ctx, NAME(o), *(m_float*)(o->data + o_nk_fmin), &*(m_float*)(o->data + o_nk_fval),
#else
  nk_property_float(ctx, NAME(o), *(m_float*)(o->data + o_nk_fmin), &*(m_float*)(o->data + o_nk_fval),
#endif
    *(m_float*)(o->data + o_nk_fmax), 0.1, *(m_float*)(o->data + o_nk_finc));
  if(property != *(m_float*)(o->data + o_nk_fval))
    broadcast(o);
}
static void propf_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)(o->data + o_nk_exec)) = propf_execute;
  *(m_float*)(o->data + o_nk_fmax) = 1.0;
  *(m_float*)(o->data + o_nk_fval) = 0.0;
  *(m_float*)(o->data + o_nk_fstp) = 1;
  *(m_float*)(o->data + o_nk_finc) = .5;
}

static void slider_execute(M_Object o, struct nk_context* ctx)
{
  if(nk_slider_float(ctx, *(m_float*)(o->data + o_nk_fmin), (float*)&*(m_float*)(o->data + o_nk_fval),
    *(m_float*)(o->data + o_nk_fmax), .01))
    /*if(nk_slider_float(ctx, *(m_float*)(o->data + o_nk_fmin), (float*)&*(m_float*)(o->data + o_nk_fval),*/
    /**(m_float*)(o->data + o_nk_fmax), *((m_float*)((M_Object)o)->d.data + o_nk_fstp)))*/
    broadcast(o);
}
static void slider_ctor(M_Object o, VM_Shred shred)
{
  (*(f_nk*)   (o->data + o_nk_exec)) = slider_execute;
  *(m_float*)(o->data + o_nk_fmax)  = 1.0;
  *(m_float*)(o->data + o_nk_fval)  = 0.0;
  *(m_float*)(o->data + o_nk_fstp)  = 0.1; // ? BUG
  *(m_float*)(o->data + o_nk_finc)  = .11;
}

IMPORT {
  Type t_color, t_panel, t_widget, t_sval, t_label, t_text,
    t_slabel, t_stext, t_prog, t_button, t_group, t_rowd,
    t_layout, t_tree, t_combo, t_menubar, t_menu,
    t_nkstring, t_ival, t_fval, t_check, t_propi,
    t_slideri, t_propf, t_slider;
  CHECK_OB((t_color  = gwi_mk_type(gwi, "NkColor", SZ_INT, t_object)))
  CHECK_OB((t_widget  = gwi_mk_type(gwi, "NkWidget", SZ_INT, t_event)))
  CHECK_OB((t_sval  = gwi_mk_type(gwi, "NkSval", SZ_INT, t_widget)))
  CHECK_OB((t_panel  = gwi_mk_type(gwi, "NkPanel", SZ_INT, t_object)))
  CHECK_OB((t_label  = gwi_mk_type(gwi, "NkLabel", SZ_INT, t_sval)))
  CHECK_OB((t_text  = gwi_mk_type(gwi, "NkText", SZ_INT, t_label)))
  CHECK_OB((t_slabel  = gwi_mk_type(gwi, "NkSLabel", SZ_INT, t_sval)))
  CHECK_OB((t_stext  = gwi_mk_type(gwi, "NkStext", SZ_INT, t_slabel)))
  CHECK_OB((t_prog = gwi_mk_type(gwi, "NkProg", SZ_INT, t_widget)))
  CHECK_OB((t_button  = gwi_mk_type(gwi, "NkButton", SZ_INT, t_widget)))
  CHECK_OB((t_group   = gwi_mk_type(gwi, "NkGroup", SZ_INT, t_widget)))
  CHECK_OB((t_rowd = gwi_mk_type(gwi, "NkRow", SZ_INT, t_group)))
  CHECK_OB((t_layout  = gwi_mk_type(gwi, "NkLayout", SZ_INT, t_rowd)))
  CHECK_OB((t_tree  = gwi_mk_type(gwi, "NkTree", SZ_INT, t_rowd)))
  CHECK_OB((t_combo  = gwi_mk_type(gwi, "NkCombo", SZ_INT, t_rowd)))
  CHECK_OB((t_menubar  = gwi_mk_type(gwi, "NkMenuBar", SZ_INT, t_group)))
  CHECK_OB((t_menu = gwi_mk_type(gwi, "NkMenu", SZ_INT, t_rowd)))
  CHECK_OB((t_nkstring  = gwi_mk_type(gwi, "NkString", SZ_INT, t_rowd)))
  CHECK_OB((t_ival  = gwi_mk_type(gwi, "NkIval", SZ_INT, t_widget)))
  CHECK_OB((t_fval  = gwi_mk_type(gwi, "NkFval", SZ_INT, t_widget)))
  CHECK_OB((t_check  = gwi_mk_type(gwi, "NkCheck", SZ_INT, t_ival)))
  CHECK_OB((t_propi  = gwi_mk_type(gwi, "NkPropI", SZ_INT, t_ival)))
  CHECK_OB((t_slideri  = gwi_mk_type(gwi, "NkSliderI", SZ_INT, t_ival)))
  CHECK_OB((t_propf  = gwi_mk_type(gwi, "NkPropF", SZ_INT, t_fval)))
  CHECK_OB((t_slider = gwi_mk_type(gwi, "NkSlider", SZ_INT, t_fval)))

  CHECK_BB(gwi_class_ini(gwi, t_color, NULL, NULL))
	gwi_item_ini(gwi,"int", "r");
  o_nk_r = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_r)
	gwi_item_ini(gwi,"int", "g");
  o_nk_g = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_g)
	gwi_item_ini(gwi,"int", "b");
  o_nk_b = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_b)
	gwi_item_ini(gwi,"int", "a");
  o_nk_a = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_a)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_panel, nk_ctor, nk_dtor))
	gwi_item_ini(gwi,"int", "@win");
  o_nk_data = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_data)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_widget, widget_ctor, NULL))
	gwi_item_ini(gwi,"string", "name");
  o_nk_name = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_name);
	gwi_item_ini(gwi,"int", "@parent");
  o_nk_parent = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_parent);
	gwi_item_ini(gwi,"int", "@exe");
  o_nk_exec = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_exec)
	gwi_item_ini(gwi,"int", "@win");
  o_nk_gwin = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_gwin)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_sval, sval_ctor, NULL))
	gwi_item_ini(gwi, "int",  "align");
  o_nk_align = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_align)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_label, label_ctor, NULL))
	gwi_item_ini(gwi, "int",  "wrap");
  o_nk_wrap = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_wrap)
	gwi_item_ini(gwi, "NkColor",  "color");
  o_nk_labelcolor= gwi_item_end(gwi, ae_flag_ref, NULL);
  CHECK_BB(o_nk_labelcolor)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_text, text_ctor, NULL))
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_slabel, slabel_ctor, NULL))
	gwi_item_ini(gwi, "int",  "selectable");
  o_nk_select = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_select)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_prog, prog_ctor, NULL))
	gwi_item_ini(gwi,"int", "val");
  o_nk_prog = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_prog)
	gwi_item_ini(gwi,"int", "max");
  o_nk_progmax = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_progmax)
	gwi_item_ini(gwi,"int", "mod");
  o_nk_progmod = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_progmod)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_button, button_ctor, NULL))
	gwi_item_ini(gwi, "int",  "behavior");
  o_nk_behavior = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_behavior)
	gwi_item_ini(gwi, "NkColor",  "color");
  o_nk_button_color= gwi_item_end(gwi, ae_flag_ref, NULL);
  CHECK_BB(o_nk_button_color)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_group, group_ctor, group_dtor))
	gwi_item_ini(gwi,"int", "@widget");
  o_nk_list = gwi_item_end(gwi, ae_flag_member, NULL);
  gwi_func_ini(gwi, "void", "begin", group_begin);
  CHECK_BB(gwi_func_end(gwi, 0))
  gwi_func_ini(gwi, "void", "end", group_end);
  CHECK_BB(gwi_func_end(gwi, 0))
  CHECK_BB(o_nk_list)
  CHECK_BB(gwi_class_end(gwi))



  CHECK_BB(gwi_class_ini(gwi, t_rowd, rowd_ctor, NULL))
	gwi_item_ini(gwi, "int",  "height");
  o_nk_rowh = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_rowh)
	gwi_item_ini(gwi, "int",  "width");
  o_nk_roww = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_roww)
	gwi_item_ini(gwi, "int",  "col");
  o_nk_rowcol = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_rowcol)
	gwi_item_ini(gwi, "int",  "static");
  o_nk_static = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_static)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_layout, layout_ctor, NULL))
	gwi_item_ini(gwi, "int",  "x");
  o_nk_x = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_x)
	gwi_item_ini(gwi, "int",  "y");
  o_nk_y = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_y)
	gwi_item_ini(gwi, "int",  "w");
  o_nk_w = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_w)
	gwi_item_ini(gwi, "int",  "h");
  o_nk_h = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_h)
	gwi_item_ini(gwi, "int",  "flag");
  o_nk_flags = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_flags)
  m_uint * border  = malloc(SZ_INT);
  *border  = NK_WINDOW_BORDER;
	gwi_item_ini(gwi,"int", "BORDER");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, border);
  m_uint * movable = malloc(SZ_INT);
  *movable = NK_WINDOW_MOVABLE;
	gwi_item_ini(gwi,"int", "MOVABLE");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, movable);
  m_uint *scalable = malloc(SZ_INT);
  *scalable = NK_WINDOW_SCALABLE;
	gwi_item_ini(gwi,"int", "SCALABLE");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, scalable);
  m_uint * closable = malloc(SZ_INT);
  *closable = NK_WINDOW_CLOSABLE;
	gwi_item_ini(gwi,"int", "CLOSABLE");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, closable);
  m_uint * minimizable = malloc(SZ_INT);
  *minimizable = NK_WINDOW_MINIMIZABLE;
	gwi_item_ini(gwi,"int", "MINIMIZABLE");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, minimizable);
  m_uint * title = malloc(SZ_INT);
  *title = NK_WINDOW_TITLE;
	gwi_item_ini(gwi,"int", "TITLE");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, title);
  m_uint * menu = malloc(SZ_INT);;
  *menu = NK_PANEL_MENU;
	gwi_item_ini(gwi,"int", "MENU");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, menu);
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_combo, combo_ctor, NULL))
	gwi_item_ini(gwi, "int",  "val");
  o_nk_comboval = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_comboval)
  gwi_func_ini(gwi, "string", "add", combo_add);
  gwi_func_arg(gwi, "string", "s");
  CHECK_BB(gwi_func_end(gwi, 0))
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_menu, menu_ctor, NULL))
	gwi_item_ini(gwi, "int",  "val");
  o_nk_menuval = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_menuval)
  gwi_func_ini(gwi, "string", "add", menu_add);
  gwi_func_arg(gwi, "string", "s");
  CHECK_BB(gwi_func_end(gwi, 0))
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_menubar, menubar_ctor, NULL))
  gwi_func_ini(gwi, "void", "add", menubar_add);
  gwi_func_arg(gwi, "NkMenu", "s");
  CHECK_BB(gwi_func_end(gwi, 0))
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_tree, tree_ctor, NULL))
	gwi_item_ini(gwi, "int",  "state");
  o_nk_state = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_state)
  CHECK_BB(gwi_class_end(gwi))


  m_uint * simple = malloc(SZ_INT);
  m_uint * field  = malloc(SZ_INT);
  m_uint * box    = malloc(SZ_INT);
  m_uint * editor = malloc(SZ_INT);
  * simple = NK_EDIT_SIMPLE;
  * field  = NK_EDIT_FIELD;
  * box    = NK_EDIT_BOX;
  * editor = NK_EDIT_EDITOR;
  CHECK_BB(gwi_class_ini(gwi, t_nkstring, nkstring_ctor, NULL))
	gwi_item_ini(gwi,"int", "type");
  o_nk_edit_type= gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_edit_type)
	gwi_item_ini(gwi,"int", "SIMPLE");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, simple);
	gwi_item_ini(gwi,"int", "FIELD");
  gwi_item_end(gwi,  ae_flag_static | ae_flag_const, field);
	gwi_item_ini(gwi,"int", "BOX");
  gwi_item_end(gwi,    ae_flag_static | ae_flag_const, box);
	gwi_item_ini(gwi,"int", "EDITOR");
  gwi_item_end(gwi, ae_flag_static | ae_flag_const, editor);
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_ival, NULL, NULL))
	gwi_item_ini(gwi, "int",  "val");
  o_nk_ival = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_ival)
	gwi_item_ini(gwi, "int",  "min");
  o_nk_imin = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_imin)
	gwi_item_ini(gwi, "int",  "max");
  o_nk_imax = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_imax)
	gwi_item_ini(gwi,"int", "step");
  o_nk_istp = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_istp)
	gwi_item_ini(gwi,"float", "inc");
  o_nk_iinc = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_iinc)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_propi, propi_ctor, NULL))
  CHECK_BB(gwi_class_end(gwi))
  CHECK_BB(gwi_class_ini(gwi, t_slideri, slideri_ctor, NULL))
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_check, check_ctor, NULL))
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_fval, NULL, NULL))
	gwi_item_ini(gwi, "float",  "val");
  o_nk_fval = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_fval)
	gwi_item_ini(gwi, "float",  "min");
  o_nk_fmin = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_fmin)
	gwi_item_ini(gwi, "float",  "max");
  o_nk_fmax = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_fmax)

	gwi_item_ini(gwi,"float", "step");
  o_nk_fstp = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_fstp)
	gwi_item_ini(gwi,"float", "inc");
  o_nk_finc = gwi_item_end(gwi, ae_flag_member, NULL);
  CHECK_BB(o_nk_finc)
  CHECK_BB(gwi_class_end(gwi))

  CHECK_BB(gwi_class_ini(gwi, t_propf, propf_ctor, NULL))
  CHECK_BB(gwi_class_end(gwi))
  CHECK_BB(gwi_class_ini(gwi, t_slider, slider_ctor, NULL))
  CHECK_BB(gwi_class_end(gwi))
  return 1;

}


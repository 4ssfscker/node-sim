/*
 * simwin.cc
 *
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* Subfenster fuer Sim
 * keine Klasse, da die funktionen von C-Code aus aufgerufen werden koennen
 *
 * Die Funktionen implementieren ein 'Object' Windowmanager
 * Es gibt nur diesen einen Windowmanager
 *
 * 17.11.97, Hj. Malthaner
 *
 * Fenster jetzt typisiert
 * 21.06.98, Hj. Malthaner
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "simwin.h"
#include "simworld.h"
#include "simtime.h"
#include "dings/zeiger.h"

#include "simcolor.h"

#include "ifc/gui_fenster.h"
#include "gui/help_frame.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "simgraph.h"
#include "simdisplay.h"

#include "simskin.h"
#include "besch/skin_besch.h"


#include "tpl/minivec_tpl.h"

#define dragger_size 12


static minivec_tpl <int> kill_list (200);


// (Mathew Hounsell)
// I added a button to the map window to fix it's size to the best one.
// This struct is the flow back to the object of the refactoring.
struct simwin_gadget_flags {
   simwin_gadget_flags( void ) : close( false ) , help( false ) , prev( false ), size( false ), next( false ) { }

   bool close;
   bool help;
   bool prev;
   bool size;
   bool next;
};

struct simwin {
     koord pos;         // Fensterposition
     int dauer;        // Wie lange soll das Fenster angezeigt werden ?
     int xoff, yoff;   // Offsets zur Maus beim verschieben
     enum wintype wt;
     int magic_number;
     gui_fenster_t *gui;
     bool closing;

     simwin_gadget_flags flags; // (Mathew Hounsell) See Above.
};

static const int MAX_WIN = 64;          // 64 Fenster sollten reichen

static struct simwin wins[MAX_WIN+1];
static int ins_win = 0;		        // zeiger auf naechsten freien eintrag

static karte_t *wl=NULL;		// Zeiger auf aktuelle Welt, wird in win_get_event gesetzt

static bool focus_granted = false;      // default: keyboard input into game engine


// Hajo: tooltip data
static int tooltip_xpos = 0;
static int tooltip_ypos = 0;
static const char * tooltip_text = 0;


// Hajo: if we are inside the event handler, windows may not be
// destroyed immediately
static bool inside_event_handling = false;

static void destroy_framed_win(int win);

//=========================================================================
// Helper Functions

// (Mathew Hounsell) A "Gadget Box" is a windows control button.
enum simwin_gadget_et { GADGET_CLOSE, GADGET_HELP, GADGET_SIZE, GADGET_PREV, GADGET_NEXT, COUNT_GADGET };


/**
 * Display a window gadget
 * @author Hj. Malthaner
 */
static int display_gadget_box(simwin_gadget_et const  code,
			      int const x, int const y,
			      int const color,
			      bool const pushed)
{
    display_vline_wh(x,    y,   16, color+1, false);
    display_vline_wh(x+15, y+1, 14, SCHWARZ, false);
    display_vline_wh(x+16, y+1, 14, color+1, false);

    if(pushed) {
	display_fillbox_wh(x+1, y+1, 14, 14, color+1, false);
    }

    // "x", "?", "=", "�", "�"
    const int img = skinverwaltung_t::window_skin->gib_bild(code+1)->gib_nummer();

    display_img(img, x, y, false, false);

    // Hajo: return width of gadget
    return 16;
}


//-------------------------------------------------------------------------
// (Mathew Hounsell) Created
static int display_gadget_boxes(
               simwin_gadget_flags const * const flags,
               int const x, int const y,
               int const color,
               bool const pushed
) {
    int width = 0;

    // Only the close gadget can be pushed.
    if( flags->close ) {
        width += display_gadget_box( GADGET_CLOSE, x + width, y, color, pushed );
    }
    // %TODO: Move this before help after debugged clicking.
    if( flags->size ) {
        width += display_gadget_box( GADGET_SIZE, x + width, y, color, false );
    }
    if( flags->prev) {
        width += display_gadget_box( GADGET_PREV, x + width, y, color, false );
    }
    if( flags->next) {
        width += display_gadget_box( GADGET_NEXT, x + width, y, color, false );
    }
    if( flags->help ) {
        width += display_gadget_box( GADGET_HELP, x + width, y, color, false );
    }
    return ( width );
};

static simwin_gadget_et decode_gadget_boxes(
               simwin_gadget_flags const * const flags,
               int const x,
               int const px
) {
    simwin_gadget_et code = COUNT_GADGET ;
    int offset = x;

    // Only the close gadget can be pushed.
    if( flags->close ) {
        if( px >= offset ) code = GADGET_CLOSE ;
        offset += 16;
    }
    // %TODO: Move this before help after debugged clicking.
    if( flags->size ) {
        if( px >= offset ) code = GADGET_SIZE ;
        offset += 16;
    }
    if( flags->prev) {
        if( px >= offset ) code = GADGET_PREV;
        offset += 16;
    }
    if( flags->prev) {
        if( px >= offset ) code = GADGET_NEXT;
        offset += 16;
    }
    if( flags->help ) {
        if( px >= offset ) code = GADGET_HELP ;
        offset += 16;
    }

    if( px >= offset ) code = COUNT_GADGET ;

    return ( code );
};

//-------------------------------------------------------------------------
// (Mathew Hounsell) Refactored
static void win_draw_window_title(const koord pos, const koord gr,
                                  const int titel_farbe,
				  const char * const text,
				  const bool closing,
				  const simwin_gadget_flags * const flags )
{
	PUSH_CLIP(pos.x, pos.y, gr.x-2, gr.y);
    display_fillbox_wh(pos.x, pos.y, gr.x, 1, titel_farbe+1, true);
    display_fillbox_wh(pos.x, pos.y+1, gr.x, 14, titel_farbe, true);
    display_fillbox_wh(pos.x, pos.y+15, gr.x, 1, SCHWARZ, true);
    display_vline_wh(pos.x+gr.x-1, pos.y,   15, SCHWARZ, true);

    // Draw the gadgets and then move left and draw text.
    int width = display_gadget_boxes( flags, pos.x, pos.y, titel_farbe, closing );
    display_proportional_clip( pos.x + width + 8, pos.y+(16-large_font_height)/2, text, ALIGN_LEFT, WEISS, true );
    POP_CLIP();
}


//-------------------------------------------------------------------------

/**
 * Draw dragger widget
 * @author Hj. Malthaner
 */
static void win_draw_window_dragger(koord pos, koord gr)
{
  pos += gr;

  for(int x=0; x<dragger_size; x++) {
    display_fillbox_wh(pos.x-x,
		       pos.y-dragger_size+x,
		       x, 1, (x & 1) ? SCHWARZ : MN_GREY4, true);
  }
}


//=========================================================================

/**
 * redirect keyboard input into UI windows
 *
 * @return true if focus granted
 * @author Hj. Malthaner
 */
bool request_focus()
{
    if(focus_granted) {
	// someone has already requested the focus

	dbg->warning("bool request_focus()",
		     "Focus was already granted");

	return false;
    } else {
	focus_granted = true;

	return true;
    }
}


/**
 * redirect keyboard input into game engine
 *
 * @author Hj. Malthaner
 */
void release_focus()
{
    if(focus_granted) {
	focus_granted = false;
    } else {
	dbg->warning("void release_focus()",
		     "Focus was already released");
    }
}


/**
 * Checks if a window is a top level window
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_fenster_t *ig)
{
  const int i = ins_win - 1;
  return wins[i].gui == ig;
}


// window functions

int
create_win(gui_fenster_t *gui, enum wintype wt, int magic)
{
    return create_win(-1, -1, -1, gui, wt, magic);
}

int
create_win(int x, int y, gui_fenster_t *gui, enum wintype wt)
{
    return create_win(x, y, -1, gui, wt);
}

int
create_win(int x, int y, int dauer, gui_fenster_t *gui,
           enum wintype wt, int magic)
{
    assert(ins_win >= 0);

    if(ins_win < MAX_WIN) {

	// (Mathew Hounsell) Make Sure Closes Aren't Forgotten.
        // Must Reset as the entries and thus flags are reused
        wins[ins_win].flags.close = true;
        wins[ins_win].flags.help = ( gui->gib_hilfe_datei() != NULL );
	wins[ins_win].flags.prev = gui->has_prev();
	wins[ins_win].flags.next = gui->has_next();
	wins[ins_win].flags.size = gui->has_min_sizer();

	if(magic != -1) {
	    // es kann nur ein fenster fuer jede pos. magic number geben

	    for(int i=0; i<ins_win; i++) {
		if(wins[i].magic_number == magic) {
		    // gibts schon, wir machen kein neues
		    // aber wir machen es sichtbar, falls verdeckt
		    top_win(i);
		    // if 'special' magic number, delete 'new'-ed object
                    if (magic >= magic_sprachengui_t) { delete gui; }
		    return -1;
		}
	    }
	}

	// Hajo: Notify window to be shown
	if(gui) {
	  event_t ev;

	  ev.ev_class = INFOWIN;
	  ev.ev_code = WIN_OPEN;
	  ev.mx = 0;
	  ev.my = 0;
	  ev.cx = 0;
	  ev.cy = 0;
	  ev.button_state = 0;

	  gui->infowin_event(&ev);
	}


        // let's see if Hajo's tip really works.... it does!
        int i;
        for (i=0; i<ins_win; i++) {
	  if (wins[i].gui == gui) {
	    top_win(i);
	    return -1;
	  }
	}

	wins[ins_win].gui = gui;
	wins[ins_win].wt = wt;
	wins[ins_win].dauer = dauer;
	wins[ins_win].magic_number = magic;
	wins[ins_win].closing = false;

	koord gr;

	if(gui != NULL) {
	    gr = gui->gib_fenstergroesse();
	} else {
	    gr = koord(192, 92);
	}

	if(x == -1) {
	    x = CLIP(gib_maus_x() - gr.x/2, 0, display_get_width()-gr.x);
	    y = CLIP(gib_maus_y() - gr.y-32, 0, display_get_height()-gr.y);
	}

	wins[ins_win].pos.x = x;
	wins[ins_win].pos.y = MAX(32, y);

	return ins_win ++;
    } else {
	return -1;
    }
}

/**
 * Destroy a framed window
 * @author Hj. Malthaner
 */
static void destroy_framed_win(int win)
{
    assert(win >= 0);
    assert(win < MAX_WIN);

    // printf("destroy_framed_win(): win=%d of %d, gui=%p\n", win, ins_win, wins[win].gui);

    // Hajo: do not destroy frameless windows
    if(wins[win].wt == w_frameless) {
      return;
    }

    gui_fenster_t *tmp = NULL;

    if(wins[win].gui) {
	event_t ev;

	ev.ev_class = INFOWIN;
	ev.ev_code = WIN_CLOSE;
	ev.mx = 0;
	ev.my = 0;
	ev.cx = 0;
	ev.cy = 0;
  	ev.button_state = 0;
	wins[win].gui->infowin_event(&ev);
    }

    if(wins[win].wt == w_autodelete) {
	tmp = wins[win].gui;
    }

    // reset fields to 'safe' values
    wins[win].pos = koord(0,0);
    wins[win].dauer = -1;
    wins[win].xoff = 0;
    wins[win].yoff = 0;
    wins[win].wt = w_info;
    wins[win].magic_number = magic_none;
    wins[win].gui = NULL;
    wins[win].closing = false;

    // if there was an autodelete object, delete it
    delete tmp;

    // printf("destroy_framed_win(): destroyed\n");

    // if we removed not the last window, we need
    // to compact the list
    if(win < ins_win-1) {
	memmove(&wins[win], &wins[win+1], sizeof(struct simwin) * (ins_win - win - 1));
    }

    ins_win--;
}

void
destroy_win(const gui_fenster_t *gui)
{
  int i;
  for(i=ins_win-1; i>=0; i--) {
    if(wins[i].gui == gui) {
      if(inside_event_handling) {
	kill_list.append(i);
      } else {
	destroy_framed_win(i);
      }
      break;
    }
  }
}

void destroy_all_win()
{
  for(int i=ins_win-1; i >=0; i--) {
    if(inside_event_handling) {
      kill_list.append(i);
    } else {
      destroy_framed_win(i);
    }
  }
}


int top_win(int win)
{
    if (win == ins_win-1) {
	return win;
    } // already topped

    simwin tmp = wins[win];
    memmove(&wins[win], &wins[win+1], sizeof(struct simwin) * (ins_win - win - 1));
    wins[ins_win-1] = tmp;
    return ins_win-1;
}


void display_win(int win)
{
    gui_fenster_t *komp = wins[win].gui;
    koord gr = komp->gib_fenstergroesse();
    int titel_farbe = komp->gib_fensterfarben().titel;

    // %HACK (Mathew Hounsell) So draw will know if gadget is needed.
    wins[win].flags.help = ( komp->gib_hilfe_datei() != NULL );

    // titelleiste zeichnen wenn noetig
    if(wins[win].wt != w_frameless) {
      win_draw_window_title(wins[win].pos,
			    gr,
			    titel_farbe,
			    translator::translate(komp->gib_name()),
			    wins[win].closing,
			    ( & wins[win].flags ) );
    }

    komp->zeichnen(wins[win].pos, gr);

    // dragger zeichnen
    if(komp->get_resizemode() != gui_fenster_t::no_resize) {
      win_draw_window_dragger(wins[win].pos, gr);
    }
}


void
display_all_win()
{
    int i;

    for(i=0; i<ins_win; i++) {
	display_win(i);
    }
}


static void remove_old_win()
{
  // alte fenster entfernen, falls dauer abgelaufen

  for(int i=ins_win-1; i>=0; i--) {
    if(wins[i].dauer > 0) {
      wins[i].dauer --;
      if(wins[i].dauer == 0) {
	destroy_framed_win(i);
      }
    }
  }
}

void
move_win(int win, event_t *ev)
{
  koord gr = wins[win].gui->gib_fenstergroesse();
  int xfrom = ev->cx;
  int yfrom = ev->cy;
  int xto = ev->mx;
  int yto = ev->my;
  int x,y, xdelta, ydelta;

  // CLIP(wert,min,max)
  x = CLIP(wins[win].pos.x + (xto-xfrom),
	   8-gr.x, display_get_width()-16);
  y = CLIP(wins[win].pos.y + (yto-yfrom),
	   32, display_get_height()-24);

  // delta is actual window movement.
  xdelta = x - wins[win].pos.x;
  ydelta = y - wins[win].pos.y;

  wins[win].pos.x += xdelta;
  wins[win].pos.y += ydelta;
  change_drag_start(xdelta, ydelta);

}


int win_get_posx(gui_fenster_t *gui)
{
    int i;
    for(i=ins_win-1; i>=0; i--) {
	if(wins[i].gui == gui) {
	    return wins[i].pos.x;
	}
    }
    return -1;
}


int win_get_posy(gui_fenster_t *gui)
{
    int i;
    for(i=ins_win-1; i>=0; i--) {
	if(wins[i].gui == gui) {
	    return wins[i].pos.y;
	}
    }
    return -1;
}


void win_set_pos(gui_fenster_t *gui, int x, int y)
{
    int i;
    for(i=ins_win-1; i>=0; i--) {
	if(wins[i].gui == gui) {
	    wins[i].pos.x = x;
	    wins[i].pos.y = y;
	    return;
	}
    }
}


static void process_kill_list()
{
  for(int i=0; i<kill_list.count(); i++) {
    destroy_framed_win(kill_list.get(i));
  }
  kill_list.clear();
}


bool
check_pos_win(event_t *ev)
{
  static bool is_resizing = false;

  int i;
  bool swallowed = false;

  const int x = ev->mx;
  const int y = ev->my;


  inside_event_handling = true;

  // for the moment, no none events
  if (ev->ev_class == EVENT_NONE) {
    process_kill_list();
    inside_event_handling = false;
    return swallowed;
  }

  // we stop resizing once the user releases the button
  if(is_resizing && IS_LEFTRELEASE(ev)) {
    is_resizing = false;
    // printf("Leave resizing mode\n");
  }


  for(i=ins_win-1; i>=0; i--) {

    // printf("checking Fenster %d bei %d %d\n",i, x,y);

    koord gr = wins[i].gui->gib_fenstergroesse();

    // check click inside window
    if((ev->ev_class != EVENT_NONE &&
	ev->cx > wins[i].pos.x && ev->cx < wins[i].pos.x+gr.x &&
	ev->cy > wins[i].pos.y && ev->cy < wins[i].pos.y+gr.y)
       ||
       (is_resizing && i==ins_win-1)) {

      // Top first
      if (IS_LEFTCLICK(ev)) {
	i = top_win(i);
      }

      // all events in window are swallowed
      swallowed = true;

      // Hajo: if within title bar && window needs decoration
      if( ev->cy < wins[i].pos.y+16 &&
	  wins[i].wt != w_frameless) {

	// %HACK (Mathew Hounsell) So decode will know if gadget is needed.
	wins[i].flags.help = ( wins[i].gui->gib_hilfe_datei() != NULL );

	// Where Was It ?
	simwin_gadget_et code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x, ev->cx );

	switch( code ) {
	case GADGET_CLOSE : {
	  if (IS_LEFTCLICK(ev)) {
	    wins[i].closing = true;
	  } else if  (IS_LEFTRELEASE(ev)) {
	    if (x < wins[i].pos.x+16 && y < wins[i].pos.y+16) {
	      destroy_win(wins[i].gui);
	    } else {
	      wins[i].closing = false;
	    }
	  }
	  break; }
	case GADGET_HELP : {
	  if (IS_LEFTCLICK(ev)) {
	    create_win(new help_frame_t(wins[i].gui->gib_hilfe_datei()), w_autodelete, magic_none);
	    process_kill_list();
	    inside_event_handling = false;
	    return swallowed;
	  }
	  break; }
	case GADGET_PREV: {
	  if (IS_LEFTCLICK(ev)) {
	    ev->ev_class = WINDOW_CHOOSE_NEXT;
	    ev->ev_code = PREV_WINDOW;  // backward
	    wins[i].gui->infowin_event( ev );
	  }
	  break; }
	case GADGET_NEXT: {
	  if (IS_LEFTCLICK(ev)) {
	    ev->ev_class = WINDOW_CHOOSE_NEXT;
	    ev->ev_code = NEXT_WINDOW;  // forward
	    wins[i].gui->infowin_event( ev );
	  }
	  break; }
	case GADGET_SIZE : { // (Mathew Hounsell)
	  if (IS_LEFTCLICK(ev)) {
	    ev->ev_class = WINDOW_MAKE_MIN_SIZE;
	    ev->ev_code = 0;
	    wins[i].gui->infowin_event( ev );
	  }
	  break; }
	default : { // Title
	  if (IS_LEFTDRAG(ev)) {
	    i = top_win(i);
	    move_win(i, ev);
	  }
	}
	}

	// It has been handled so stop checking.
	break;

	// click in Window / Resize
      } else {

	gui_fenster_t *gui = wins[i].gui;
	if(gui != NULL) {

	  //11-May-02   markus weber added

	  // resizer hit ?
	  const bool canresize =
	    is_resizing ||
	    (ev->mx > wins[i].pos.x + gr.x - dragger_size &&
	     ev->my > wins[i].pos.y + gr.y - dragger_size);

	  if((IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) &&
	     canresize &&
	     gui->get_resizemode() != gui_fenster_t::no_resize) {
	    // Hajo: go into resize mode
	    is_resizing = true;
	    // printf("Enter resizing mode\n");


	    ev->ev_class = WINDOW_RESIZE;
	    ev->ev_code = 0;
	    event_t wev = *ev;
	    translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);

	    gui->infowin_event( &wev );
	  } else {
	    // click in Window

	    event_t wev = *ev;
	    translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);

	    wins[i].gui->infowin_event( &wev );
	  }
	} //if(gui != NULL)
	break;
      }
      continue;
    }

    // check moved window
    if (ev->ev_class != EVENT_NONE &&
	ev->mx > wins[i].pos.x && ev->mx < wins[i].pos.x+gr.x &&
	ev->my > wins[i].pos.y+16 && ev->my < wins[i].pos.y+gr.y) {

      gui_fenster_t *gui = wins[i].gui;

      if(gui != NULL) {
	event_t wev = *ev;
	translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);

	gui->infowin_event( &wev );
      }
      break;
    }
  }

  // if no focused, we do not deliver keyboard input
  if(!focus_granted && ev->ev_class == EVENT_KEYBOARD) {
    swallowed = false;
  }


  process_kill_list();
  inside_event_handling = false;

  return swallowed;
}


/*
 * warning! this is a real HACK, this following routine.
 * problem:  when user presses button which opens new window,
 *           release event cannot reach window, and button will not
 *           unpress.
 * solution: callback function, which sets variable to 'false'
 *           and swallows release event.
 */
static bool *snre_pointer;
void swallow_next_release_event(bool *set_to_false)
{
  snre_pointer = set_to_false;
}
void
win_get_event(struct event_t *ev)
{
  display_get_event(ev);
  if (snre_pointer && IS_LEFTRELEASE(ev)) {
    *snre_pointer = false; snre_pointer = 0;
    ev->ev_class = EVENT_NONE;
  }
}

void
win_poll_event(struct event_t *ev)
{
  display_poll_event(ev);
  if (snre_pointer && IS_LEFTRELEASE(ev)) {
    *snre_pointer = false; snre_pointer = 0;
    ev->ev_class = EVENT_NONE;
  }
}

static const char*  tooltips[21] =
{
    "Einstellungsfenster",
    "Reliefkarte",
    "Abfrage",
    "Anheben",
    "Absenken",
    "SLOPETOOLS",
    "Schienenbau",
    "Strassenbau",
    "Wasserbau",
    "Trams",
    "Posthaus",
    "SPECIALTOOLS",
    "Abriss",
    "",
    "Line Management",
    "LISTTOOLS",
    "Mailbox",
    "Finanzen",
    "",
    "Screenshot",
    "Pause",
};

static void win_display_tooltips()
{
    if(gib_maus_y() <= 32 && gib_maus_x() < 704) {
	if(*tooltips[gib_maus_x() / 32] != 0) {

	    const char *text = translator::translate(tooltips[gib_maus_x() / 32]);

            const int len = proportional_string_width(text) + 7;
	    int x = ((gib_maus_x() & 0xFFE0)+16)-(len/2);

	    x = CLIP(x, 4, display_get_width()-len);
	    display_ddd_proportional(x, 32+8, len, 0, 2, SCHWARZ, text, true);
	}
    }
}

static const int hours2night[] =
{
    4,4,4,4,4,4,4,4,

    4,4,4,4,3,2,1,0,

    0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,1,

    2,3,4,4,4,4,4,4
};

static const char * seasons[] =
{
    "q1", "q2", "q3", "q4"
};


void
win_display_flush(int ticks, int color, double konto)
{
    display_all_win();
    remove_old_win();

    display_icon_leiste(color, skinverwaltung_t::hauptmenu->gib_bild(0)->bild_nr);

    win_display_tooltips();

    koord3d pos;

    if(wl) {
	const ding_t *dt = wl->gib_zeiger();
	pos = dt->gib_pos();
    }

    const int tage = ticks >> karte_t::ticks_bits_per_tag;
    const int stunden4 = ((ticks - (tage << karte_t::ticks_bits_per_tag)) * 96) >> karte_t::ticks_bits_per_tag;

    if(umgebung_t::night_shift) {
	display_day_night_shift(hours2night[stunden4>>1]);
    } else {
	display_day_night_shift(0);
    }


    // Hajo: check if there is a tooltip to display

    if(tooltip_text) {
      PUSH_CLIP(0, 0, display_get_width(), display_get_height());

      const int width = proportional_string_width(tooltip_text)+7;

      display_ddd_proportional(tooltip_xpos,
			       tooltip_ypos,
			       width,
			       0,
			       2,
			       SCHWARZ,
			       tooltip_text,
			       true);

      // Hajo: clear tooltip to avoid sticky tooltips
      tooltip_text = 0;

      POP_CLIP();
    }



    char time [128];
    char info [128];
    // @author hsiegeln - updated to show month
    if (umgebung_t::show_month)
    {
    	sprintf(time, "%s, %s %d",
		translator::translate(month_names[tage % 12]),
		translator::translate(seasons[(tage/3)&3]),
		umgebung_t::starting_year+tage/12);
    } else {
    	sprintf(time, "%s %d",
		translator::translate(seasons[(tage/3)&3]),
		umgebung_t::starting_year+tage/12);
    }

    sprintf(info,"(%d,%d,%d)  (T=%1.2f)", pos.x, pos.y, pos.z / 16, get_time_multi()/16.0);

    display_flush(stunden4,
                  color,
                  konto,
                  time,
		  info);
}

void win_setze_welt(karte_t *welt)
{
    wl = welt;
}

void win_set_zoom_factor(int rw)
{
    if(rw != get_zoom_factor()) {
	set_zoom_factor(rw);

	event_t ev;

	ev.ev_class = WINDOW_REZOOM;
	ev.ev_code = rw;
	ev.mx = 0;
	ev.my = 0;
	ev.cx = 0;
	ev.cy = 0;
  	ev.button_state = 0;

	for(int i=0; i<ins_win; i++) {
	    wins[i].gui->infowin_event(&ev);
	}
    }
}


/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_tooltip(int xpos, int ypos, const char *text)
{
  tooltip_xpos = xpos;
  tooltip_ypos = ypos;
  tooltip_text = text;
}

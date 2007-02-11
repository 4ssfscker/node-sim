/*
 * simdisplay.c
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simdisplay.c
 *
 * Benutzt simgraph.c um an Simutrans angepasste Zeichenfunktionen
 * anzubieten.
 *
 * Hj. Malthaner, Jan. 2001
 */

#include <stdio.h>

#include "simtypes.h"
#include "simgraph.h"
#include "simimg.h"
#include "simdisplay.h"
#include "simcolor.h"
#include "utils/simstring.h"


static const char *progress_text=NULL;

void
display_set_progress_text(const char *t)
{
	progress_text = t;
}



/**
 * Zeichnet eine Fortschrittsanzeige
 * @author Hj. Malthaner
 */
void
display_progress(int part, int total)
{
	const int disp_width=display_get_width();
	const int disp_height=display_get_height();

	const int width=disp_width/2;
	part = (part*width)/total;

	// outline
	display_ddd_box(width/2-2, disp_height/2-9, width+4, 20, COL_GREY6, COL_GREY4);
	display_ddd_box(width/2-1, disp_height/2-8, width+2, 18, COL_GREY4, COL_GREY6);

	// inner
	display_fillbox_wh(width/2, disp_height/2-7, width, 16, COL_GREY5, TRUE);

	// progress
	display_fillbox_wh(width/2, disp_height/2-5, part, 12, COL_BLUE, TRUE);

	if(progress_text) {
		display_proportional(width,display_get_height()/2-4,progress_text,ALIGN_MIDDLE,COL_WHITE,0);
	}
}


/**
 * Zeichnet die Iconleiste
 * @author Hj. Malthaner
 */
void
display_icon_leiste(const int redraw, int basis_bild)
{
    int dirty=redraw;

#ifdef USE_SOFTPOINTER
	extern int old_my;
	if(old_my <= 32) {
		dirty = TRUE;
	}
#endif

    display_fillbox_wh(0,0, display_get_width(), 32, MN_GREY1, dirty);

    display_color_img(basis_bild++,0,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,64,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,128,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,192,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,256,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,320,0, 0, FALSE, dirty);

    //display_color_img(157,320,0, color, FALSE, dirty);
    display_color_img(basis_bild++,384,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,448,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,512,0, 0, FALSE, dirty);
    display_color_img(basis_bild++,576,0, 0, FALSE, dirty);
	// added for extended menus
    display_color_img(basis_bild++,640,0, 0, FALSE, dirty);
}



/**
 * Kopiert Puffer ins Fenster
 * @author Hj. Malthaner
 */
void display_flush(int season_img, int stunden4, double konto, const char* day_str, const char* info, const char* player_name, COLOR_VAL player_color)
{
	char buffer[256];
	const int disp_width=display_get_width();
	const int disp_height=display_get_height();

	display_setze_clip_wh( 0, 0, disp_width, disp_height );
	display_fillbox_wh(0, disp_height-16, disp_width, 1, MN_GREY4, FALSE);
	display_fillbox_wh(0, disp_height-15, disp_width, 15, MN_GREY1, FALSE);

	if(season_img!=IMG_LEER) {
		display_color_img( season_img, 2, disp_height-15, 0, false, true );
	}

	sprintf(buffer,"%s %2d:%02dh", day_str, stunden4 >> 2, (stunden4 & 3)*15);
	display_proportional(20, disp_height-12, buffer, ALIGN_LEFT, COL_BLACK, TRUE);

	if(player_name!=NULL) {
		display_proportional(256, disp_height-12, player_name, ALIGN_MIDDLE, PLAYER_FLAG|(player_color+4), TRUE);
	}

	money_to_string(buffer, konto);
	display_proportional(disp_width/2, disp_height-12, buffer, ALIGN_MIDDLE, konto >= 0.0?MONEY_PLUS:MONEY_MINUS, TRUE);

	display_proportional(disp_width-30, disp_height-12, info, ALIGN_RIGHT, COL_BLACK, TRUE);

	display_flush_buffer();
}

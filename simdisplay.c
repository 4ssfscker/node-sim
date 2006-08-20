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

#include "simgraph.h"
#include "simimg.h"
#include "simdisplay.h"
#include "simcolor.h"
#include "utils/simstring.h"


/**
 * Zeichnet eine Fortschrittsanzeige
 * @author Hj. Malthaner
 */
void
display_progress(int part, int total)
{
    extern int disp_width;
    extern int disp_height;

    // umri�
    display_ddd_box((disp_width-total-4)/2, disp_height/2-9, total+3, 18, COL_GREY6, COL_GREY4);
    display_ddd_box((disp_width-total-2)/2, disp_height/2-8, total+1, 16, COL_GREY4, COL_GREY6);

    // flaeche
    display_fillbox_wh((disp_width-total)/2, disp_height/2-7, total, 14, COL_GREY5, TRUE);

    // progress
    display_fillbox_wh((disp_width-total)/2, disp_height/2-5, part, 10, COL_BLUE, TRUE);
}


/**
 * Zeichnet die Iconleiste
 * @author Hj. Malthaner
 */
void
display_icon_leiste(const int color, int basis_bild)
{
    static int old_color = 0;
    int dirty;
    extern int disp_width, old_my;
//    extern int disp_height;

	if(color!=old_color  ||  color==-1
#ifdef USE_SOFTPOINTER
		|| old_my <= 32
#endif
	) {
		dirty = TRUE;
		if(color!=-1) {
			old_color = color;
		}
	} else {
		dirty = FALSE;
		return;
	}

    display_fillbox_wh(0,0, disp_width, 32, MN_GREY1, dirty);

    display_color_img(basis_bild++,0,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,64,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,128,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,192,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,256,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,320,0, old_color, FALSE, dirty);

    //display_color_img(157,320,0, color, FALSE, dirty);
    display_color_img(basis_bild++,384,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,448,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,512,0, old_color, FALSE, dirty);
    display_color_img(basis_bild++,576,0, old_color, FALSE, dirty);
	// added for extended menus
    display_color_img(basis_bild++,640,0, old_color, FALSE, dirty);
}



/**
 * Kopiert Puffer ins Fenster
 * @author Hj. Malthaner
 */
void
display_flush(const int season_img,int stunden4, int color, double konto, const char *day_str, const char *info, const char *player_name, const int player_color)
{
	char buffer[256];
	extern int disp_width;
	extern int disp_height;

	display_setze_clip_wh( 0, 0, disp_width, disp_height );
	display_fillbox_wh(0, disp_height-16, disp_width, 1, MN_GREY4, FALSE);
	display_fillbox_wh(0, disp_height-15, disp_width, 15, MN_GREY1, FALSE);

	if(season_img!=IMG_LEER) {
		display_color_img( season_img, 2, disp_height-15, player_color, false, true );
	}

	sprintf(buffer,"%s %2d:%02dh", day_str, stunden4 >> 2, (stunden4 & 3)*15);
	display_proportional(20, disp_height-12, buffer, ALIGN_LEFT, COL_BLACK, TRUE);

	if(player_name!=NULL) {
		display_proportional(256, disp_height-12, player_name, ALIGN_MIDDLE, player_color, TRUE);
	}

	money_to_string(buffer, konto);
	display_proportional(disp_width/2, disp_height-12, buffer, ALIGN_MIDDLE, konto >= 0.0?MONEY_PLUS:MONEY_MINUS, TRUE);

	display_proportional(disp_width-30, disp_height-12, info, ALIGN_RIGHT, COL_BLACK, TRUE);

	display_flush_buffer();
}

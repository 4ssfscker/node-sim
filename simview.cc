/*
 * Copyright (c) 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

#include <stdio.h>

#include "simworld.h"
#include "simview.h"
#include "simgraph.h"

#include "simticker.h"
#include "simdebug.h"
#include "simdings.h"
#include "simconst.h"
#include "simplan.h"
#include "simplay.h"
#include "besch/grund_besch.h"
#include "dataobj/umgebung.h"
#include "dings/zeiger.h"


karte_ansicht_t::karte_ansicht_t(karte_t *welt)
{
    this->welt = welt;
}

static const sint8 hours2night[] =
{
    4,4,4,4,4,4,4,4,
    4,4,4,4,3,2,1,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,1,
    2,3,4,4,4,4,4,4
};



void
karte_ansicht_t::display(bool force_dirty)
{
	const sint16 disp_width = display_get_width();
	const sint16 disp_real_height = display_get_height();

	const sint16 disp_height = display_get_height() - 32 - 16 - (!ticker::empty() ? 16 : 0);
	display_setze_clip_wh( 0, 32, disp_width, disp_height );

	// zuerst den boden zeichnen
	// denn der Boden kann kein Objekt verdecken
	force_dirty = force_dirty || welt->ist_dirty();
	welt->setze_dirty_zurueck();

	sint16 IMG_SIZE = get_tile_raster_width();

	const int dpy_width = disp_width/IMG_SIZE + 2;
	const int dpy_height = (disp_real_height*4)/IMG_SIZE;

	const int i_off = welt->gib_ij_off().x - disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE;
	const int j_off = welt->gib_ij_off().y + disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE;
	const int const_x_off = welt->gib_x_off();
	const int const_y_off = welt->gib_y_off();

	// these are the values needed to go directly from a tile to the display
	welt->setze_ansicht_ij_offset(
		koord( - disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE,
					disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE	)
	);

	// change to night mode?
	// images will be recalculated only, when there has been a change, so we set always
	if(grund_t::underground_mode  ||  !umgebung_t::night_shift) {
		display_day_night_shift(0);
	}
	else {
		uint32 stunden4;
		const uint32 ticks_this_month = welt->gib_zeit_ms() & ((1<<welt->ticks_bits_per_tag)-1);
		if(umgebung_t::show_month>1) {
			static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
			stunden4 = ((ticks_this_month*tage_per_month[welt->get_last_month()]*96l) >> welt->ticks_bits_per_tag)%96l;
		} else {
			stunden4 = (ticks_this_month * 96l) >> welt->ticks_bits_per_tag;
		}
		display_day_night_shift(hours2night[stunden4>>1]);
	}

	// not very elegant, but works:
	// fill everything with black for Underground mode ...
	if(grund_t::underground_mode) {
		display_fillbox_wh(0, 32, disp_width, disp_height, COL_BLACK, TRUE);
	}

	// first display ground
	int	y;
	for(y=-dpy_height; y<dpy_height+dpy_width; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-dpy_width-(y & 1); x<=dpy_width+dpy_height; x+=2) {

			const sint16 i = ((y+x) >> 1) + i_off;
			const sint16 j = ((y-x) >> 1) + j_off;
			const sint16 xpos = x*(IMG_SIZE/2) + const_x_off;

			if(xpos+IMG_SIZE>0  &&  xpos<disp_width) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->gib_kartenboden()) {
					sint16 yypos = ypos - tile_raster_scale_y( plan->gib_kartenboden()->gib_hoehe()*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE<disp_height  &&  yypos+IMG_SIZE>32) {
						plan->display_boden(xpos, yypos, IMG_SIZE, true);
					}
				}
				else {
					// ouside ...
					display_img(grund_besch_t::ausserhalb->gib_bild(hang_t::flach), xpos,ypos - tile_raster_scale_y( welt->gib_grundwasser()*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE ), force_dirty);
				}
			}
		}
	}

	// and then things (and other ground)
	for(y=-dpy_height; y<dpy_height+dpy_width; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-dpy_width-(y & 1); x<=dpy_width+dpy_height; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*(IMG_SIZE/2) + const_x_off;

			if(xpos+IMG_SIZE>0  &&  xpos<disp_width) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->gib_kartenboden()) {
					sint16 yypos = ypos - tile_raster_scale_y( plan->gib_kartenboden()->gib_hoehe()*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE*2-32<disp_height  &&  yypos+IMG_SIZE>32) {
						plan->display_dinge(xpos, yypos, IMG_SIZE, true);
					}
				}
			}
		}
	}

	// and finally overlays (station coverage and signs)
	for(y=-dpy_height; y<dpy_height+dpy_width; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-dpy_width-(y & 1); x<=dpy_width+dpy_height; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*(IMG_SIZE/2) + const_x_off;

			if(xpos+IMG_SIZE>0  &&  xpos<disp_width) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->gib_kartenboden()) {
					sint16 yypos = ypos - tile_raster_scale_y( plan->gib_kartenboden()->gib_hoehe()*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE*2-32<disp_height  &&  yypos+IMG_SIZE>32) {
						plan->display_overlay(xpos, yypos, IMG_SIZE, true);
					}
				}
			}
		}
	}
	// finally display the maus pointer
	ding_t *zeiger = welt->gib_zeiger();
	if(zeiger) {
		// better not try to twist your brain to follow the retransformation ...
		const sint16 rasterweite=get_tile_raster_width();
		const koord diff = zeiger->gib_pos().gib_2d()-welt->gib_ij_off()-welt->gib_ansicht_ij_offset();
		const sint16 x = (diff.x-diff.y)*(rasterweite/2) + tile_raster_scale_x(zeiger->gib_xoff(), rasterweite);
		const sint16 y = (diff.x+diff.y)*(rasterweite/4) + tile_raster_scale_y( zeiger->gib_yoff()-zeiger->gib_pos().z*TILE_HEIGHT_STEP/Z_TILE_STEP, rasterweite) + ((display_get_width()/rasterweite)&1)*(rasterweite/4);
		zeiger->display( x+welt->gib_x_off(), y+welt->gib_y_off(), true);
		zeiger->clear_flag(ding_t::dirty);
	}

	if(welt) {
		// finally update the ticker
		for(int x=0; x<MAX_PLAYER_COUNT; x++) {
			welt->gib_spieler(x)->display_messages();
		}
	}

	if(force_dirty) {
		mark_rect_dirty_wc( 0, 0, display_get_width(), display_get_height() );
	}
}

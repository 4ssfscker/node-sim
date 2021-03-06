/* fundament.cc
 *
 * Untergrund f�r Geb�ude in Simutrans.
 * �berarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "../simhalt.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simimg.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../dataobj/freelist.h"
#include "../dataobj/loadsave.h"

#include "grund.h"
#include "fundament.h"


fundament_t::fundament_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
	setze_bild( IMG_LEER );
	rdwr(file);
	slope = (uint8)hang_t::flach;
}

fundament_t::fundament_t(karte_t *welt, koord3d pos, hang_t::typ hang ) : grund_t(welt, pos)
{
	setze_bild( IMG_LEER );
	if(hang) {
		pos = gib_pos();
		pos.z += Z_TILE_STEP;
		setze_pos( pos );
	}
	slope = (uint8)hang_t::flach;
}


/**
 * Auffforderung, ein Infofenster zu �ffnen.
 * @author Hj. Malthaner
 */
bool fundament_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	return false;
}



void
fundament_t::calc_bild()
{
	slope = 0;
	if(ist_im_tunnel()) {
		clear_back_bild();
		setze_bild(IMG_LEER);
	}
	else {
		grund_t::calc_bild();
		setze_bild( grund_besch_t::gib_ground_tile(0,gib_pos().z) );
		grund_t::calc_back_bild(gib_pos().z/Z_TILE_STEP,0);
	}
}



void * fundament_t::operator new(size_t /*s*/)
{
//	assert(s==sizeof(fundament_t));
	return static_cast<fundament_t *>(freelist_t::gimme_node(sizeof(fundament_t)));
}


void fundament_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(fundament_t),p);
}

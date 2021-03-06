/* bruecke.cc
 *
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../boden/grund.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simmem.h"
#include "../bauer/brueckenbauer.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "bruecke.h"



bruecke_t::bruecke_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	rdwr(file);
}



bruecke_t::bruecke_t(karte_t *welt, koord3d pos, spieler_t *sp,
		     const bruecke_besch_t *besch, bruecke_besch_t::img_t img) :
 ding_t(welt, pos)
{
	this->besch = besch;
	this->img = img;
	setze_besitzer( sp );
	if(gib_besitzer()) {
		gib_besitzer()->buche(-besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}



void
bruecke_t::calc_bild()
{
	grund_t *gr=welt->lookup(gib_pos());
	if(gr) {
		// if we are on the bridge, put the image into the ground, so we can have two ways ...
		if(gr->gib_weg_nr(0)) {
			if(img>=bruecke_besch_t::N_Start  &&  img<=bruecke_besch_t::W_Start) {
				// must take the upper value for the start of the bridge
				gr->gib_weg_nr(0)->setze_bild(besch->gib_hintergrund(img, gib_pos().z+Z_TILE_STEP >= welt->get_snowline()));
			}
			else {
				gr->gib_weg_nr(0)->setze_bild(besch->gib_hintergrund(img, gib_pos().z >= welt->get_snowline()));
			}
		}
		setze_yoff( -gr->gib_weg_yoff() );
	}
}



void bruecke_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	const char *s = NULL;

	if(file->is_saving()) {
		s = besch->gib_name();
	}
	file->rdwr_str(s, "");
	file->rdwr_enum(img, "");

	if(file->is_loading()) {
		besch = brueckenbauer_t::gib_besch(s);
		if(besch==NULL) {
			besch = brueckenbauer_t::gib_besch(translator::compatibility_name(s));
		}
		if(besch==NULL) {
			if(strstr(s,"onorail") ) {
				besch = brueckenbauer_t::find_bridge(monorail_wt,50,0);
			}
			else if(strstr(s,"ail") ) {
				besch = brueckenbauer_t::find_bridge(track_wt,50,0);
			}
			else {
				besch = brueckenbauer_t::find_bridge(road_wt,50,0);
			}
			if(besch==NULL) {
				dbg->fatal("bruecke_t::rdwr()","Unknown bridge %s",s);
			}
		}
		guarded_free(const_cast<char *>(s));
	}
}



// correct speed and maitainace
void bruecke_t::laden_abschliessen()
{
	const grund_t *gr = welt->lookup(gib_pos());
	assert(gr->obj_bei(0)!=this);
	spieler_t *sp=gib_besitzer();
	if(sp) {
		// change maitainance
		weg_t *weg = gr->gib_weg(besch->gib_waytype());
		weg->setze_max_speed(besch->gib_topspeed());
		weg->setze_besitzer(sp);
		sp->add_maintenance(-weg->gib_besch()->gib_wartung());
		sp->add_maintenance( besch->gib_wartung() );
	}
}



// correct speed and maitainace
void bruecke_t::entferne( spieler_t *sp2 )
{
	if(sp2==NULL) {
		// only set during destroying of the map
		return;
	}
	spieler_t *sp = gib_besitzer();
	if(sp) {
		// on bridge => do nothing but change maitainance
		const grund_t *gr = welt->lookup(gib_pos());
		if(gr) {
			weg_t *weg = gr->gib_weg( besch->gib_waytype() );
			assert(weg);
			weg->setze_max_speed( weg->gib_besch()->gib_topspeed() );
			sp->add_maintenance( weg->gib_besch()->gib_wartung());
			sp->add_maintenance( -besch->gib_wartung() );
		}
	}
	if(sp2) {
		sp2->buche( -besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION );
	}
}

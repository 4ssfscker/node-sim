/* schiene.cc
 *
 * Schienen f�r Simutrans
 *
 * �berarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "../../simworld.h"
#include "../../simconvoi.h"
#include "../../simvehikel.h"
#include "../../simplay.h"
#include "../../simcolor.h"
#include "../../simgraph.h"

#include "../grund.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"

#include "../../dings/signal.h"

#include "../../utils/cbuffer_t.h"

#include "../../besch/weg_besch.h"
#include "../../bauer/wegbauer.h"

#include "schiene.h"

const weg_besch_t *schiene_t::default_schiene=NULL;
bool schiene_t::show_reservations = false;

schiene_t::schiene_t(karte_t *welt) : weg_t(welt)
{
	reserved = convoihandle_t();
	setze_besch(schiene_t::default_schiene);
}



schiene_t::schiene_t(karte_t *welt, loadsave_t *file) : weg_t(welt)
{
	reserved = convoihandle_t();
	rdwr(file);
}


void
schiene_t::entferne(spieler_t *)
{
	// removes reservation
	if(reserved.is_bound()) {
		setze_ribi(ribi_t::keine);
		reserved->recalc_route();
	}
}



void schiene_t::info(cbuffer_t & buf) const
{
	weg_t::info(buf);
	if(reserved.is_bound()) {
		buf.append(translator::translate("\nis reserved by:"));
		buf.append(reserved->gib_name());
#ifdef DEBUG_PBS
		reserved->zeige_info();
#endif
	}
}



/**
 * true, if this rail can be reserved
 * @author prissi
 */
bool
schiene_t::reserve(convoihandle_t c) {
	if(can_reserve(c)) {
		reserved = c;
		if(schiene_t::show_reservations) {
			mark_image_dirty(gib_bild(),0);
		}
		return true;
	}
	// reserve anyway ...
	return false;
}



/**
* releases previous reservation
* only true, if there was something to release
* @author prissi
*/
bool
schiene_t::unreserve(convoihandle_t c)
{
	// is this tile reserved by us?
	if(reserved.is_bound()  &&  reserved==c) {
		reserved = convoihandle_t();
		if(schiene_t::show_reservations) {
			mark_image_dirty(gib_bild(),0);
		}
		return true;
	}
	return false;
}




/**
* releases previous reservation
* @author prissi
*/
bool
schiene_t::unreserve(vehikel_t *v)
{
	// is this tile empty?
	if(!reserved.is_bound()) {
		return true;
	}
	if(!welt->lookup(gib_pos())->suche_obj(v->gib_typ())) {
		reserved = convoihandle_t();
		if(schiene_t::show_reservations) {
			mark_image_dirty(gib_bild(),0);
		}
		return true;
	}
	return false;
}




void
schiene_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->get_version()<99008) {
		sint32 blocknr=-1;
		file->rdwr_long(blocknr, "\n");
	}

	if(file->get_version()<89000) {
		uint8 dummy;
		file->rdwr_byte(dummy, "\n");
		set_electrify(dummy);
	}

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "\n");
	}
	else {
		char bname[128];
		file->rd_str_into(bname, "\n");

		int old_max_speed=gib_max_speed();
		const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
		if(besch==NULL) {
			besch = default_schiene;
			dbg->warning("schiene_t::rwdr()", "Unknown rail %s replaced by a rail %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,gib_pos().x,gib_pos().y,old_max_speed);
	}
}

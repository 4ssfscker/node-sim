/*
 * simfab.cc
 *
 * Fabrikfunktionen und Fabrikbau
 *
 * Hansj�rg Malthaner
 *
 *
 * 25.03.00 Anpassung der Lagerkapazit�ten: min. 5 normale Lieferungen
 *          sollten an Lager gehalten werden.
 */

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdebug.h"
#include "simio.h"
#include "simimg.h"
#include "simmem.h"
#include "simcolor.h"
#include "boden/grund.h"
#include "boden/fundament.h"
#include "simfab.h"
#include "simcity.h"
#include "simhalt.h"
#include "simskin.h"
#include "simtools.h"
#include "simware.h"
#include "simworld.h"
#include "besch/haus_besch.h"
#include "besch/ware_besch.h"
#include "simplay.h"


#include "simintr.h"

#include "dings/wolke.h"
#include "dings/gebaeude.h"
#include "dings/field.h"
#include "dings/leitung2.h"

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"

#include "besch/skin_besch.h"
#include "besch/fabrik_besch.h"

#include "bauer/warenbauer.h"
#include "bauer/fabrikbauer.h"

#include "utils/cbuffer_t.h"

#include "simgraph.h"
#include "simdisplay.h"

// Fabrik_t


static const int FAB_MAX_INPUT = 15000;




fabrik_t * fabrik_t::gib_fab(const karte_t *welt, const koord pos)
{
	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr  &&  gr->first_obj()!=NULL  &&  gr->first_obj()->get_fabrik()!=NULL) {
		return gr->first_obj()->get_fabrik();
	}
	// not found
	return 0;
}



void fabrik_t::link_halt(halthandle_t halt)
{
	welt->access(pos.gib_2d())->add_to_haltlist(halt);
}



void fabrik_t::unlink_halt(halthandle_t halt)
{
	planquadrat_t *plan=welt->access(pos.gib_2d());
	if(plan) {
		plan->remove_from_haltlist(welt,halt);
	}
}


void
fabrik_t::add_lieferziel(koord ziel)
{
	lieferziele.append_unique(ziel,4);
	fabrik_t * fab = fabrik_t::gib_fab(welt, ziel);
	if (fab) {
		fab->add_supplier(gib_pos().gib_2d());
	}
}


void
fabrik_t::rem_lieferziel(koord ziel)
{
	lieferziele.remove(ziel);
}


fabrik_t::fabrik_t(karte_t *wl, loadsave_t *file) : lieferziele(0), suppliers(0), fields(0), eingang(0), ausgang(0)
{
	welt = wl;

	besch = NULL;

	besitzer_p = NULL;

	rdwr(file);

	delta_sum = 0;
	last_lieferziel_start = 0;
	total_input = total_output = 0;
	status = nothing;
}


fabrik_t::fabrik_t(karte_t *wl, koord3d pos, spieler_t *spieler, const fabrik_besch_t *fabesch) : lieferziele(0), suppliers(0), fields(0), eingang(0), ausgang(0)
{
	this->pos = pos;
	this->pos.z = wl->max_hgt(pos.gib_2d());
	besch = fabesch;

	welt = wl;

	besitzer_p = spieler;
	prodfaktor = 16;
	prodbase = besch->gib_produktivitaet() + simrand(besch->gib_bereich());

	delta_sum = 0;
	last_lieferziel_start = 0;
	total_input = total_output = 0;
	status = nothing;

	// create producer information
	for(int i=0; i < fabesch->gib_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = fabesch->gib_lieferant(i);
		ware_production_t ware;
		ware.setze_typ( lieferant->gib_ware() );
		ware.max = lieferant->gib_kapazitaet() << fabrik_t::precision_bits;
		ware.menge = 0;
		eingang.append(ware,1);
	}

	// create consumer information
	for(int i=0; i < fabesch->gib_produkte(); i++) {
		const fabrik_produkt_besch_t *produkt = fabesch->gib_produkt(i);
		ware_production_t ware;
		ware.setze_typ( produkt->gib_ware() );
		ware.max = produkt->gib_kapazitaet() << fabrik_t::precision_bits;
		// if source then start with full storage (thus AI will built immeadiately lines)
		ware.menge = (fabesch->gib_lieferanten()==0) ? ware.max-(16<<fabrik_t::precision_bits) : 0;
		ausgang.append(ware,1);
	}
}



fabrik_t::~fabrik_t()
{
	while(!fields.empty()) {
		grund_t *gr = welt->lookup_kartenboden( fields.back() );
		if(gr) {
			field_t *f=(field_t*)(gr->suche_obj(ding_t::field));
			delete f;
		}
		else {
			fields.remove_at( fields.get_count()-1 );
		}
	}
}


// must be extended for non-square factories!
bool
fabrik_t::is_fabrik( koord check )
{
	return (check.x>=pos.x   &&  check.x<pos.x+besch->gib_haus()->gib_groesse(rotate).x  &&  check.y>=pos.y  &&  check.y<pos.y+besch->gib_haus()->gib_groesse(rotate).y);
}

bool
fabrik_t::is_fabrik( koord lu, koord rd )
{
	return (rd.x>=pos.x   &&  lu.x<pos.x+besch->gib_haus()->gib_groesse(rotate).x  &&  rd.y>=pos.y  &&  lu.y<pos.y+besch->gib_haus()->gib_groesse(rotate).y);
}


void
fabrik_t::add_arbeiterziel(stadt_t *stadt)
{
	// do not add a city twice!
	if(!arbeiterziele.contains(stadt)) {
		arbeiterziele.insert(stadt);
	}
}


void
fabrik_t::rem_arbeiterziel(stadt_t *stadt)
{
	arbeiterziele.remove(stadt);
}


/**
 * Baut Geb�ude f�r die Fabrik
 *
 * @author Hj. Malthaner
 */
void
fabrik_t::baue(int rotate, bool clear)
{
	if(besch) {
		this->rotate = rotate;
		pos = welt->lookup_kartenboden(pos.gib_2d())->gib_pos();
		hausbauer_t::baue(welt, besitzer_p, pos, rotate, besch->gib_haus(), clear, this);
		pos = welt->lookup_kartenboden(pos.gib_2d())->gib_pos();
		if(besch->gib_field()) {
			// if there are fields
			if(!fields.empty()) {
				for( uint16 i=0;  i<fields.get_count();  i++  ) {
					const koord k = fields[i];
					grund_t *gr=welt->lookup_kartenboden(k);
					// first make foundation below
					grund_t *gr2 = new fundament_t(welt, gr->gib_pos(), gr->gib_grund_hang());
					welt->access(k)->boden_ersetzen(gr, gr2);
					gr2->obj_add( new field_t( welt, gr2->gib_pos(), besitzer_p, besch->gib_field(), this ) );
				}
			}
			else {
				// we will start with a certain minimum number
				while(fields.get_count()<besch->gib_field()->gib_min_fields()  &&  add_random_field(0))
					;
			}
		}
	}
	else {
		dbg->error("fabrik_t::baue()", "Good pak not available!");
	}
}



/* field generation code
 * spawns a field for sure if probability==0 and zero for 1024
 * @author Kieron Green
 */
bool
fabrik_t::add_random_field(uint16 probability)
{
	// has fields, and not yet too many?
	const field_besch_t *fb = besch->gib_field();
	if(fb==NULL  ||  fb->gib_max_fields() <= fields.get_count()) {
		return false;
	}
	// we are lucky and are allowed to generate a field
	if(simrand(10000)<probability) {
		return false;
	}

	// we start closest to the factory, and check for valid tiles as we move out
	uint8 radius = 1;

	// pick a coordinate to use - create a list of valid locations and choose a random one
	slist_tpl<grund_t *> build_locations;
	climate_bits place_climate = besch->gib_haus()->get_allowed_climate_bits();
	do {
		for(sint32 xoff = -radius; xoff < radius + gib_besch()->gib_haus()->gib_groesse().x ; xoff++) {
			for(sint32 yoff =-radius ; yoff < radius + gib_besch()->gib_haus()->gib_groesse().y; yoff++) {
				// if we can build on this tile then add it to the list
				grund_t *gr = welt->lookup_kartenboden(pos.gib_2d()+koord(xoff,yoff));
				if(gr  &&  gr->gib_typ()==grund_t::boden  &&  gr->gib_hoehe()==pos.z  &&  gr->gib_grund_hang()==hang_t::flach  &&  gr->ist_natur()  &&  (gr->suche_obj(ding_t::leitung) || gr->kann_alle_obj_entfernen(NULL)==NULL)) {
					// only on same height => climate will match!
					build_locations.append(gr);
					assert(gr->suche_obj(ding_t::field)==NULL);
				}
				// skip inside of rectange (already checked earlier)
				if(radius > 1 && yoff == -radius && (xoff > -radius && xoff < radius + gib_besch()->gib_haus()->gib_groesse().x - 1)) {
					yoff = radius + gib_besch()->gib_haus()->gib_groesse().y - 2;
				}
			}
		}
		if(build_locations.count() == 0) {
			radius++;
		}
	} while (radius < 10 && build_locations.count() == 0);
	// built on one of the positions
	if(build_locations.count() > 0) {
		grund_t *gr = build_locations.at(simrand(build_locations.count()));
		leitung_t *lt = (leitung_t *)(gr->suche_obj( ding_t::leitung ));
		if(lt) {
			gr->obj_remove(lt);
		}
		gr->obj_loesche_alle(NULL);
		// first make foundation below
		const koord k = gr->gib_pos().gib_2d();
		assert(!fields.is_contained(k));
		fields.append(k,10);
		grund_t *gr2 = new fundament_t(welt, gr->gib_pos(), gr->gib_grund_hang());
		welt->access(k)->boden_ersetzen(gr, gr2);
		gr2->obj_add( new field_t( welt, gr2->gib_pos(), besitzer_p, fb, this ) );
		prodbase += fb->gib_field_production();
		if(lt) {
			gr2->obj_add( lt );
		}
		return true;
	}
	return false;
}



void
fabrik_t::remove_field_at(koord pos)
{
	assert(fields.is_contained(pos));
	prodbase -= besch->gib_field()->gib_field_production();
	fields.remove(pos);
}




bool
fabrik_t::ist_bauplatz(karte_t *welt, koord pos, koord groesse,bool wasser,climate_bits cl)
{
    if(pos.x > 0 && pos.y > 0 &&
       pos.x+groesse.x < welt->gib_groesse_x() && pos.y+groesse.y < welt->gib_groesse_y() &&
       ( wasser  ||  welt->ist_platz_frei(pos, groesse.x, groesse.y, NULL, cl) )&&
       !ist_da_eine(welt,pos-koord(5,5),pos+groesse+koord(3,3))) {

		// check for water (no shore in sight!)
		if(wasser) {
			for(int y=0;y<groesse.y;y++) {
				for(int x=0;x<groesse.x;x++) {
					const grund_t *gr=welt->lookup_kartenboden(pos+koord(x,y));
					if(!gr->ist_wasser()  ||  gr->gib_grund_hang()!=hang_t::flach) {
						return false;
					}
				}
			}
		}

		return true;
	}
	return false;
}



vector_tpl<fabrik_t *> &
fabrik_t::sind_da_welche(karte_t *welt, koord min_pos, koord max_pos)
{
	static vector_tpl <fabrik_t*> fablist(16);
	fablist.clear();

	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			const grund_t *gr = welt->lookup_kartenboden(koord(x,y));
			if(gr  &&  gr->first_obj()!=NULL  &&  gr->first_obj()->get_fabrik()!=NULL) {
				if( fablist.append_unique( gr->first_obj()->get_fabrik(), 4 )  ) {
//DBG_MESSAGE("fabrik_t::sind_da_welche()","appended factory %s at (%i,%i)",gr->first_obj()->get_fabrik()->gib_besch()->gib_name(),x,y);
				}
			}
		}
	}
	return fablist;
}



bool
fabrik_t::ist_da_eine(karte_t *welt, koord min_pos, koord max_pos )
{
	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			const grund_t *gr = welt->lookup_kartenboden(koord(x,y));
			if(gr  &&  gr->first_obj()!=NULL  &&  gr->first_obj()->get_fabrik()!=NULL) {
				return true;
			}
		}
	}
	return false;
}



void
fabrik_t::rdwr(loadsave_t *file)
{
	int i;
	int spieler_n;
	int eingang_count;
	int ausgang_count;
	int anz_lieferziele;
	const char *s = NULL;

	if(file->is_saving()) {
		eingang_count = eingang.get_count();
		ausgang_count = ausgang.get_count();
		anz_lieferziele = lieferziele.get_count();
		s = besch->gib_name();
	}
	file->rdwr_str(s, "-");
	if(file->is_loading()) {
DBG_DEBUG("fabrik_t::rdwr()","loading factory '%s'",s);
		besch = fabrikbauer_t::gib_fabesch(s);
		if(!besch) {
			besch = fabrikbauer_t::gib_fabesch(translator::compatibility_name(s));
		}
		guarded_free(const_cast<char *>(s));
		// set ware arrays ...
		if(besch) {
			eingang.clear();
			eingang.resize(besch->gib_lieferanten());
			ausgang.clear();
			ausgang.resize(besch->gib_produkte());
		}
		else {
			// save defaults for loading only, factory will be ignored!
			eingang.clear();
			eingang.resize(16);
			ausgang.clear();
			ausgang.resize(10);
		}
	}
	pos.rdwr(file);

	file->rdwr_delim("Bau: ");
	file->rdwr_byte(rotate, "\n");

	// now rebuilt information for recieved goods
	file->rdwr_long(eingang_count, "\n");
	for(i=0; i<eingang_count; i++) {
		ware_production_t dummy;
		const char *typ = NULL;

		if(file->is_saving()) {
			typ = eingang[i].gib_typ()->gib_name();
			dummy.menge = eingang[i].menge << (old_precision_bits - precision_bits);
			dummy.max   = eingang[i].max   << (old_precision_bits - precision_bits);
		}

		file->rdwr_delim("Ein: ");
		file->rdwr_str(typ, " ");
		file->rdwr_long(dummy.menge, " ");
		file->rdwr_long(dummy.max, "\n");
		if(file->is_loading()) {
			dummy.setze_typ( warenbauer_t::gib_info(typ) );
			guarded_free(const_cast<char *>(typ));

			// Hajo: repair files that have 'insane' values
			dummy.menge >>= (old_precision_bits-precision_bits);
			dummy.max >>= (old_precision_bits-precision_bits);
			if(dummy.menge < 0) {
				dummy.menge = 0;
			}
			if(dummy.menge > (FAB_MAX_INPUT << precision_bits)) {
				dummy.menge = (FAB_MAX_INPUT << precision_bits);
			}
			eingang.append(dummy, 1);
		}
	}

	// now rebuilt information for produced goods
	file->rdwr_long(ausgang_count, "\n");
	for(i=0; i<ausgang_count; i++) {
		ware_production_t dummy;
		const char *typ = NULL;

		if(file->is_saving()) {
			typ = ausgang[i].gib_typ()->gib_name();
			dummy.menge = ausgang[i].menge << (old_precision_bits - precision_bits);
			dummy.max   = ausgang[i].max   << (old_precision_bits - precision_bits);
			dummy.abgabe_sum   = ausgang[i].abgabe_sum;
			dummy.abgabe_letzt = ausgang[i].abgabe_letzt;
		}
		file->rdwr_delim("Aus: ");
		file->rdwr_str(typ, " ");
		file->rdwr_long(dummy.menge, " ");
		file->rdwr_long(dummy.max, "\n");
		file->rdwr_long(dummy.abgabe_sum, " ");
		file->rdwr_long(dummy.abgabe_letzt, "\n");

		if(file->is_loading()) {
			dummy.setze_typ( warenbauer_t::gib_info(typ));
			guarded_free(const_cast<char *>(typ));
			dummy.menge >>= (old_precision_bits-precision_bits);
			dummy.max >>= (old_precision_bits-precision_bits);
			ausgang.append(dummy, 4);
		}
	}
	// restore other information
	spieler_n = welt->sp2num(besitzer_p);
	file->rdwr_delim("Bes: ");
	file->rdwr_long(spieler_n, "\n");
	file->rdwr_delim("Prf: ");
	file->rdwr_long(prodbase, "\n");
	file->rdwr_delim("Prb: ");
	file->rdwr_long(prodfaktor, "\n");
	// owner stuff
	if(file->is_loading()) {
		// take care of old files
		if(prodfaktor==1  ||  prodfaktor>16) {
			prodfaktor = 16;
		}
		if(file->get_version() < 86001) {
			if(besch) {
				koord k=besch->gib_haus()->gib_groesse();
DBG_DEBUG("fabrik_t::rdwr()","correction of production by %i",k.x*k.y);
				// since we step from 86.01 per factory, not per tile!
				prodbase *= k.x*k.y*2;
			}
		}
		// Hajo: restore factory owner
		// Due to a omission in Volkers changes, there might be savegames
		// in which factories were saved without an owner. In this case
		// set the owner to the default of player 1
		if(spieler_n == -1) {
			// Use default
			besitzer_p = welt->gib_spieler(1);
		}
		else {
			// Restore owner pointer
			besitzer_p = welt->gib_spieler(spieler_n);
		}
	}

	file->rdwr_long(anz_lieferziele, "\n");

	// connect/save consumer
	if(file->is_loading()) {
		koord k;
		for(int i=0; i<anz_lieferziele; i++) {
			k.rdwr(file);
			add_lieferziel(k);
		}
	}
	else {
		for(int i=0; i<anz_lieferziele; i++) {
			koord k = lieferziele[i];
			k.rdwr(file);
		}
	}

	// information on fields ...
	if(file->get_version()>99009) {
		if(file->is_saving()) {
			uint16 nr=fields.get_count();
			file->rdwr_short(nr,"f");
			for(int i=0; i<nr; i++) {
				koord k = fields[i];
				k.rdwr(file);
			}
		}
		else {
			uint16 nr=0;
			koord k;
			file->rdwr_short(nr,"f");
			fields.resize(nr);
			for(int i=0; i<nr; i++) {
				k.rdwr(file);
				fields.append(k);
			}
		}
	}

	if(file->is_loading()  &&  besch) {
		// clear everything, even though it might be something important ...
		baue(rotate, true);
	}
}



/*
 * calculates the produktion per delta_t; this is now PRODUCTION_DELTA_T
 * @author Hj. Malthaner
 */
uint32 fabrik_t::produktion(const uint32 produkt) const
{
	// default prodfaktor = 16 => shift 4, default time = 1024 => shift 10, rest precion
	const uint32 max = prodbase * prodfaktor;
	uint32 menge = max >> (18-10+4-fabrik_t::precision_bits);

	if (ausgang.get_count() > produkt) {
		// wenn das lager voller wird, produziert eine Fabrik weniger pro step

		const uint32 maxi = ausgang[produkt].max;
		const uint32 actu = ausgang[produkt].menge;

		if(actu<maxi) {
			if(menge>(0x7FFFFFFFu/maxi)) {
				// avoid overflow
				menge = (((maxi-actu)>>5)*(menge>>5))/(maxi>>10);
			}
			else {
				// and that is the simple formula
				menge = (menge*(maxi-actu)) / maxi;
			}
		}
		else {
			// overfull?
			menge = maxi-1;
		}
	}

	return menge;
}



int
fabrik_t::vorrat_an(const ware_besch_t *typ)
{
	int menge = -1;

	for (uint32 index = 0; index < ausgang.get_count(); index++) {
		if (typ == ausgang[index].gib_typ()) {
			menge = ausgang[index].menge >> precision_bits;
			break;
		}
	}

	return menge;
}

int
fabrik_t::hole_ab(const ware_besch_t *typ, int menge)
{
	for (uint32 index = 0; index < ausgang.get_count(); index++) {
		if (ausgang[index].gib_typ() == typ) {
			if (ausgang[index].menge >> precision_bits >= menge) {
				ausgang[index].menge -= menge << precision_bits;
			} else {
				menge = ausgang[index].menge >> precision_bits;
				ausgang[index].menge = 0;
			}

			ausgang[index].abgabe_sum += menge;

			return menge;
		}
	}

	// ware "typ" wird hier nicht produziert
	return -1;
}



int
fabrik_t::liefere_an(const ware_besch_t *typ, int menge)
{
	for (uint32 index = 0; index < eingang.get_count(); index++) {
		if (eingang[index].gib_typ() == typ) {
			// Hajo: avoid overflow
			if (eingang[index].menge < (FAB_MAX_INPUT - menge) << precision_bits) {
				eingang[index].menge += menge << precision_bits;
			}

			// sollte maximale lagerkapazitaet pruefen
			return menge;
		}
	}

	// ware "typ" wird hier nicht verbraucht
	return -1;
}

int
fabrik_t::verbraucht(const ware_besch_t *typ)
{
	for(uint32 index = 0; index < eingang.get_count(); index ++) {
		if (eingang[index].gib_typ() == typ) {
			// sollte maximale lagerkapazitaet pruefen
			return eingang[index].menge > eingang[index].max;
		}
	}
	return -1;  // wird hier nicht verbraucht
}


void
fabrik_t::step(long delta_t)
{
	delta_sum += delta_t;

	if(delta_sum > PRODUCTION_DELTA_T) {

		// produce nothing/consumes nothing ...
		if (eingang.empty() && ausgang.empty()) {
			delta_sum += PRODUCTION_DELTA_T;
			return;
		}

		INT_CHECK("simfab 558");
//DBG_DEBUG("fabrik_t::step()","%s",besch->gib_name());

		// this we need to find out, if the was any production/consumption going on
		// if not no fields will grow and no smokestacks will smoke
		uint32 total_amount = 0;
		for (uint32 produkt = 0; produkt < eingang.get_count(); produkt++) {
			total_amount += eingang[produkt].menge;
		}
		for (uint32 produkt = 0; produkt < ausgang.get_count(); produkt++) {
			total_amount += ausgang[produkt].menge;
		}
		total_amount >>=  precision_bits;

		const uint32 ecount = eingang.get_count();
		uint32 index = 0;
		uint32 produkt=0;
		bool is_currently_producing = false;

		if (ausgang.empty()) {
			// consumer only ...
			uint32 menge = produktion(produkt) * (delta_sum/PRODUCTION_DELTA_T);

			// finally consume stock
			for(index = 0; index<ecount; index ++) {

				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 v = (menge*vb) >> 8;

				if ((uint32)eingang[index].menge > v) {
					eingang[index].menge -= v;
					is_currently_producing = true;
				}
				else {
					eingang[index].menge = 0;
				}
			}
		}
		else {

			// ok, calulate maximum allowed consumption
			uint32 max_menge = 0x7FFFFFFF;
			uint32 consumed_menge = 0;
			for(index = 0; index < ecount; index ++) {
				// verbrauch fuer eine Einheit des Produktes (in 1/256)
				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 n = eingang[index].menge * 256 / vb;

				if(n<max_menge) {
					max_menge = n;    // finde geringsten vorrat
				}
			}

			// produces something
			for (produkt = 0; produkt < ausgang.get_count(); produkt++) {
				uint32 menge;

				if(ecount>0) {

					// calculate production
					uint32 p_menge = 0;
					for( sint32 i=delta_sum/PRODUCTION_DELTA_T;  i>0;  i--  ) {
						p_menge += produktion(produkt);
					}
					menge = p_menge < max_menge ? p_menge : max_menge;  // production smaller than possible due to consumption
					if(menge>consumed_menge) {
						consumed_menge = menge;
					}

				}
				else {
					// source producer
					menge = 0;
					for( sint32 i=delta_sum/PRODUCTION_DELTA_T;  i>0;  i--  ) {
						menge += produktion(produkt);
					}
				}

				const uint32 pb = besch->gib_produkt(produkt)->gib_faktor();
				const uint32 p = (menge*pb) >> 8;

				// produce
				if (ausgang[produkt].menge < ausgang[produkt].max) {
					ausgang[produkt].menge += p;
					is_currently_producing = true;
				} else {
					ausgang[produkt].menge = ausgang[produkt].max - 1;
				}
			}

			// and finally consume stock
			for(index = 0; index<ecount; index ++) {

				const uint32 vb = besch->gib_lieferant(index)->gib_verbrauch();
				const uint32 v = (consumed_menge*vb) >> 8;

				if ((uint32)eingang[index].menge > v) {
					eingang[index].menge -= v;
					is_currently_producing = true;
				}
				else {
					eingang[index].menge = 0;
				}
			}
		}

		// Zeituhr zur�cksetzen
		while(  delta_sum>PRODUCTION_DELTA_T) {
			delta_sum -= PRODUCTION_DELTA_T;
		}

		// this we need to find out, if the was any production/consumption going on
		// if not no fields will grow and no smokestacks will smoke
		uint32 total_amount_end = 0;
		for (uint32 produkt = 0; produkt < eingang.get_count(); produkt++) {
			total_amount_end += eingang[produkt].menge;
		}
		for (uint32 produkt = 0; produkt < ausgang.get_count(); produkt++) {
			total_amount_end += ausgang[produkt].menge;
		}
		total_amount_end >>=  precision_bits;

		// distribute, if there are more than 10 waiting ...
		for (uint32 produkt = 0; produkt < ausgang.get_count(); produkt++) {
			if (ausgang[produkt].menge > 10 << precision_bits) {

				verteile_waren(produkt);
				INT_CHECK("simfab 636");
			}
		}
		recalc_factory_status();

		if(total_amount!=total_amount_end) {
			// let the chimney smoke
			const rauch_besch_t *rada = besch->gib_rauch();
			if(rada) {
				grund_t *gr=welt->lookup_kartenboden(pos.gib_2d()+rada->gib_pos_off());
				wolke_t *smoke =  new wolke_t(welt, pos+rada->gib_pos_off(), rada->gib_xy_off().x+simrand(7)-3, rada->gib_xy_off().y+simrand(7)-3, rada->gib_bilder()->gib_bild_nr(0), false );

				bool add_ok=gr->obj_add(smoke);
				assert(add_ok);
				welt->sync_add( smoke );
			}

			if(besch->gib_field()  &&  fields.get_count()<besch->gib_field()->gib_max_fields()) {
				// spawn new field with given probablitily
				add_random_field(besch->gib_field()->gib_probability());
			}
		}

	}

	// to distribute to all target equally, we use this counter, for the factory, to try first
	last_lieferziel_start ++;

}


/**
 * Die erzeugten waren auf die Haltestellen verteilen
 * @author Hj. Malthaner
 */
void fabrik_t::verteile_waren(const uint32 produkt)
{
	// wohin liefern ?
	if (lieferziele.empty()) {
		return;
	}

	// not connected?
	const planquadrat_t *plan = welt->lookup(pos.gib_2d());
	if(plan->get_haltlist_count()==0) {
		return;
	}

	slist_tpl<halthandle_t> halt_ok;
	slist_tpl<ware_t> ware_ok;
	bool still_overflow = true;

	// ok, first send everything away
	const halthandle_t *haltlist = plan->get_haltlist();
	for(  unsigned i=0;  i<plan->get_haltlist_count();  i++  ) {
		halthandle_t halt = haltlist[i];

		// �ber alle Ziele iterieren
		for(uint32 n=0; n<lieferziele.get_count(); n++) {

			// prissi: this way, the halt, that is tried first, will change. As a result, if all destinations are empty, it will be spread evenly
			const koord lieferziel = lieferziele[(n + last_lieferziel_start) % lieferziele.get_count()];

			fabrik_t * ziel_fab = gib_fab(welt, lieferziel);
			int vorrat;

			if (ziel_fab && (vorrat = ziel_fab->verbraucht(ausgang[produkt].gib_typ())) >= 0) {
				ware_t ware(ausgang[produkt].gib_typ());
				ware.menge = 1;
				ware.setze_zielpos( lieferziel );

				unsigned w;
				// find the index in the target factory
				for (w = 0; w < ziel_fab->gib_eingang().get_count() && ziel_fab->gib_eingang()[w].gib_typ() != ware.gib_typ(); w++) {
					// emtpy
				}

				// Station can only store up to 128 units of goods per square
				if(halt->gib_ware_summe(ware.gib_typ()) <halt->get_capacity()) {
					// ok, still enough space
					halt->suche_route(ware);

//DBG_MESSAGE("verteile_waren()","searched for route for %s with result %i,%i",translator::translate(ware.gib_name()),ware.gib_ziel().x,ware.gib_ziel().y);
					if(ware.gib_ziel().is_bound()) {
						// if only overflown factories found => deliver to first
						// else deliver to non-overflown factory
						bool overflown = (ziel_fab->gib_eingang()[w].menge >= ziel_fab->gib_eingang()[w].max);

						if(!welt->gib_einstellungen()->gib_just_in_time()) {

							// distribution also to overflowing factories
							if(still_overflow  &&  !overflown) {
								// not overflowing factory found
								still_overflow = false;
								halt_ok.clear();
								ware_ok.clear();
							}
							if(still_overflow  ||  !overflown) {
								halt_ok.insert(halt);
								ware_ok.insert(ware);
							}
						}
						else {

							// only distribute to no-overflowed factories
							if(!overflown) {
								halt_ok.insert(halt);
								ware_ok.insert(ware);
							}
						}
					}

				}
				else {
					// Station too full, notify player
					halt->bescheid_station_voll();
				}
			}
		}
	}

	// Auswertung der Ergebnisse
	if (!halt_ok.empty()) {

		// prissi: distribute goods to factory, that has not an overflowing input storage
		// if all have, then distribute evenly
		const int menge = ausgang[produkt].menge >> precision_bits;

//DBG_MESSAGE("verteile_waren()","halts %i",halt_ok.count());

		// Hajo: distribute goods to station that has the least amount stored
		if(menge > 1) {
			slist_iterator_tpl<halthandle_t> iter (halt_ok);
			slist_iterator_tpl<ware_t> ware_iter (ware_ok);

			ware_t best_ware       = ware_ok.front();
			halthandle_t best_halt = halt_ok.front();
			int best_amount        = 999999;

			while(iter.next() && ware_iter.next()) {
				halthandle_t halt = iter.get_current();
				const ware_t& ware = ware_iter.get_current();

				const int amount = halt->gib_ware_fuer_ziel(ware.gib_typ(),ware.gib_ziel());

				if(amount < best_amount) {
					best_ware = ware;
					best_ware.menge = menge;
					best_halt = halt;
					best_amount = amount;
				}
//DBG_MESSAGE("verteile_waren()","best_amount %i %s",best_amount,translator::translate(ware.gib_name()));
			}

			ausgang[produkt].menge -= menge << precision_bits;
			best_halt->starte_mit_route(best_ware);
		}

	}
}



void
fabrik_t::neuer_monat()
{
	for (uint32 index = 0; index < ausgang.get_count(); index++) {
		ausgang[index].abgabe_letzt = ausgang[index].abgabe_sum;
		ausgang[index].abgabe_sum = 0;
	}
}



static void info_add_ware_description(cbuffer_t & buf, const ware_production_t & ware)
{
  const ware_besch_t * type = ware.gib_typ();

  buf.append(" -");
  buf.append(translator::translate(type->gib_name()));
  buf.append(" ");
  buf.append(ware.menge >> fabrik_t::precision_bits);
  buf.append("/");
  buf.append(ware.max >> fabrik_t::precision_bits);
  buf.append(translator::translate(type->gib_mass()));

  if(type->gib_catg() != 0) {
    buf.append(", ");
    buf.append(translator::translate(type->gib_catg_name()));
  }
}


// static !
unsigned fabrik_t::status_to_color[5] = {COL_RED, COL_ORANGE, COL_GREEN, COL_YELLOW, COL_WHITE };

#define FL_WARE_NULL           1
#define FL_WARE_ALLENULL       2
#define FL_WARE_LIMIT          4
#define FL_WARE_ALLELIMIT      8
#define FL_WARE_UEBER75        16
#define FL_WARE_ALLEUEBER75    32
#define FL_WARE_HATWAS         64

/* returns the status of the current factory, as well as output */
void fabrik_t::recalc_factory_status()
{
	unsigned long warenlager;
	char status_ein;
	char status_aus;

	int haltcount=welt->lookup(pos.gib_2d())->get_haltlist_count();

	// set bits for input
	warenlager = 0;
	status_ein = FL_WARE_ALLELIMIT;
	for (uint j = 0; j < eingang.get_count(); j++) {
		if (eingang[j].menge >= eingang[j].max) {
			status_ein |= FL_WARE_LIMIT;
		}
		else {
			status_ein &= ~FL_WARE_ALLELIMIT;
		}
		warenlager += eingang[j].menge;
		status_ein |= FL_WARE_HATWAS;

	}
	warenlager >>= fabrik_t::precision_bits;
	if(warenlager==0) {
		status_ein |= FL_WARE_ALLENULL;
	}
	total_input = warenlager;

	// set bits for output
	warenlager = 0;
	status_aus = FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL;
	for (uint j = 0;j < ausgang.get_count(); j++) {
		if (ausgang[j].menge > 0) {

			status_aus &= ~FL_WARE_ALLENULL;
			if (ausgang[j].menge >= 0.75 * ausgang[j].max) {
				status_aus |= FL_WARE_UEBER75;
			}
			else {
				status_aus &= ~FL_WARE_ALLEUEBER75;
			}
			warenlager += ausgang[j].menge;
			status_aus &= ~FL_WARE_ALLENULL;
		}
		else {
			// menge = 0
			status_aus &= ~FL_WARE_ALLEUEBER75;
		}
	}
	warenlager >>= fabrik_t::precision_bits;
	total_output = warenlager;

	// now calculate status bar
	if(status_ein==(FL_WARE_ALLELIMIT|FL_WARE_ALLENULL)) {
		// does not consume anything, should just produce

		if(status_aus==(FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL)) {
			// does also not produce anything
			status = nothing;
		}
		else if(status_aus&FL_WARE_ALLEUEBER75  ||  status_aus&FL_WARE_UEBER75) {
			status = inactive;	// not connected?
			if(haltcount>0) {
				if(status_aus&FL_WARE_ALLEUEBER75) {
					status = bad;	// connect => needs better service
				}
				else {
					status = medium;	// connect => needs better service for at least one product
				}
			}
		}
		else {
			status = good;
		}
	}
	else if(status_aus==(FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL)) {
		// does not produce anything, just consumes

		if(status_ein&FL_WARE_ALLELIMIT) {
			// we assume not served
			status = bad;
		}
		else if(status_ein&FL_WARE_LIMIT) {
			// served, but still one at limit
			status = medium;
		}
		else if(status_ein&FL_WARE_ALLENULL) {
			status = inactive;	// assume not served
			if(haltcount>0) {
				// there is a halt => needs better service
				status = bad;
			}
		}
		else {
			status = good;
		}
	}
	else {
		// produces and consumes
		if((status_ein&FL_WARE_ALLELIMIT)!=0  &&  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = bad;
		}
		else if((status_ein&FL_WARE_ALLELIMIT)!=0  ||  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = medium;
		}
		else if((status_ein&FL_WARE_ALLENULL)!=0  &&  (status_aus&FL_WARE_ALLENULL)!=0) {
			// not producing
			status = inactive;
		}
		else if(haltcount>0  &&  ((status_ein&FL_WARE_ALLENULL)!=0  ||  (status_aus&FL_WARE_ALLENULL)!=0)) {
			// not producing but out of supply
			status = medium;
		}
		else {
			status = good;
		}
	}
}



void fabrik_t::info(cbuffer_t & buf)
{
	buf.append("\n");
	buf.append(translator::translate("Produktion"));
	buf.append(":\n");
	buf.append(translator::translate("Durchsatz"));
	buf.append(" ");
	buf.append( (prodbase * prodfaktor * 16l)>>(26l-(long)welt->ticks_bits_per_tag));
	buf.append(" ");
	buf.append(translator::translate("units/day"));
	buf.append("\n");

	if (!lieferziele.empty()) {
		buf.append("\n");
		buf.append(translator::translate("Abnehmer"));
		buf.append(":\n");

		for(uint32 i=0; i<lieferziele.get_count(); i++) {
			const koord lieferziel = lieferziele[i];

			ding_t * dt = welt->lookup_kartenboden(lieferziel)->first_obj();
			if(dt) {
				fabrik_t *fab = dt->get_fabrik();

				if(fab) {
					buf.append("     ");
					buf.append(translator::translate(fab->gib_name()));
					buf.append(" ");
					buf.append(lieferziel.x);
					buf.append(",");
					buf.append(lieferziel.y);
					buf.append("\n");
				}
			}
		}
	}

	if (!suppliers.empty()) {
		buf.append("\n");
		buf.append(translator::translate("Suppliers"));
		buf.append(":\n");

		for(uint32 i=0; i<suppliers.get_count(); i++) {
			const koord supplier = suppliers[i];

			ding_t * dt = welt->lookup_kartenboden(supplier)->first_obj();
			if(dt) {
				fabrik_t *fab = dt->get_fabrik();

				if(fab) {
					buf.append("     ");
					buf.append(translator::translate(fab->gib_name()));
					buf.append(" ");
					buf.append(supplier.x);
					buf.append(",");
					buf.append(supplier.y);
					buf.append("\n");
				}
			}
		}
	}

	if (!arbeiterziele.empty()) {
		slist_iterator_tpl<stadt_t *> iter (arbeiterziele);

		buf.append("\n");
		buf.append(translator::translate("Arbeiter aus:"));
		buf.append("\n");

		while(iter.next()) {
			stadt_t *stadt = iter.get_current();

			buf.append("     ");
			buf.append(stadt->gib_name());
			buf.append("\n");

		}
		// give a passenger level for orientation
		int passagier_rate = besch->gib_pax_level();
		buf.append("\n");
		buf.append(translator::translate("Passagierrate"));
		buf.append(": ");
		buf.append(passagier_rate);
		buf.append("\n");

		buf.append(translator::translate("Postrate"));
		buf.append(": ");
		buf.append(passagier_rate/3);
		buf.append("\n");
	}

	if (!ausgang.empty()) {

		buf.append("\n");
		buf.append(translator::translate("Produktion"));
		buf.append(":\n");

		for (uint32 index = 0; index < ausgang.get_count(); index++) {
			info_add_ware_description(buf, ausgang[index]);

			buf.append(", ");
			buf.append((int)(besch->gib_produkt(index)->gib_faktor()*100/256));
			buf.append("%\n");
		}
	}

	if (!eingang.empty()) {

		buf.append("\n");
		buf.append(translator::translate("Verbrauch"));
		buf.append(":\n");

		for (uint32 index = 0; index < eingang.get_count(); index++) {

			buf.append(" -");
			buf.append(translator::translate(eingang[index].gib_typ()->gib_name()));
			buf.append(" ");
			buf.append(eingang[index].menge >> precision_bits);
			buf.append("/");
			buf.append(eingang[index].max >> precision_bits);
			buf.append(translator::translate(eingang[index].gib_typ()->gib_mass()));
			buf.append(", ");
			buf.append((int)(besch->gib_lieferant(index)->gib_verbrauch()*100/256));
			buf.append("%\n");
		}
	}
}

void fabrik_t::laden_abschliessen()
{
	for(uint32 i=0; i<lieferziele.get_count(); i++) {
		fabrik_t * fab2 = fabrik_t::gib_fab(welt, lieferziele[i]);
		if (fab2) {
			fab2->add_supplier(pos.gib_2d());
		}
	}
}



void
fabrik_t::add_supplier(koord pos)
{
	suppliers.append_unique(pos,4);
}


void
fabrik_t::rem_supplier(koord pos)
{
	suppliers.remove(pos);
}


/** crossconnect everything possible
 */
void
fabrik_t::add_all_suppliers()
{

	for(int i=0; i < besch->gib_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = besch->gib_lieferant(i);
		const ware_besch_t *ware = lieferant->gib_ware();

		const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
		slist_iterator_tpl <fabrik_t *> iter (list);

		while( iter.next() ) {

			fabrik_t * fab = iter.get_current();

			// connect to an existing one, if this is an producer
			if(fab!=this  &&  fab->vorrat_an(ware) > -1) {
				// add us to this factory
				fab->add_lieferziel(pos.gib_2d());
			}
		}
	}
}

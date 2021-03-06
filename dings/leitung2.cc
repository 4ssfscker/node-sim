/*
 * leitung.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "leitung2.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simfab.h"
#include "../simskin.h"

#include "../simgraph.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/powernet.h"
#include "../dataobj/umgebung.h"

#include "../boden/grund.h"
#include "../bauer/wegbauer.h"


#define PROD 1000

/*
static const char * measures[] =
{
    "fail",
    "weak",
    "good",
    "strong",
};
*/


static bool
ist_leitung(karte_t *welt, koord pos)
{
  bool result = false;
  const planquadrat_t * plan = welt->lookup(pos);

  if(plan) {
    grund_t * gr = plan->gib_kartenboden();
    result = gr && (gr->suche_obj(ding_t::leitung)!=NULL  ||  gr->suche_obj(ding_t::pumpe)!=NULL  ||  gr->suche_obj(ding_t::senke)!=NULL);
  }

  return result;
}



static int
gimme_neighbours(karte_t *welt, koord base_pos, leitung_t **conn)
{
	int count = 0;

	for(int i=0; i<4; i++) {
		const koord pos = base_pos + koord::nsow[i];
		grund_t *gr = welt->lookup_kartenboden(pos);

		conn[i] = NULL;
		if(gr) {
			leitung_t *p = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::pumpe));
			if(p) {
				conn[i] = p;
				count ++;
				continue;
			}
			// now handle drain
			leitung_t *s = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::senke));
			if(s) {
				conn[i] = s;
				count ++;
				continue;
			}
			// and now handle line
			leitung_t *l = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
			if(l) {
				conn[i] = l;
				count ++;
			}
		}
	}
	return count;
}



fabrik_t *
leitung_t::suche_fab_4(const koord pos)
{
	for(int k=0; k<4; k++) {
		const grund_t *gr = welt->lookup(pos+koord::nsow[k])->gib_kartenboden();
		const int n = gr->gib_top();

		for(int i=0; i<n; i++) {
			const ding_t * dt=gr->obj_bei(i);
			if(dt!=NULL && dt->get_fabrik()!=NULL) {
				return dt->get_fabrik();
			}
		}
	}
	return NULL;
}


leitung_t::leitung_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
  set_net(NULL);
  rdwr(file);
}


leitung_t::leitung_t(karte_t *welt,
		     koord3d pos,
		     spieler_t *sp) : ding_t(welt, pos)
{
  set_net(NULL);
  setze_besitzer( sp );
  verbinde();
}


leitung_t::~leitung_t()
{
	grund_t *gr = welt->lookup(gib_pos());
	if(gr) {
		gr->obj_remove(this);

		leitung_t * conn[4];
		int neighbours = gimme_neighbours(welt, gib_pos().gib_2d(), conn);

		powernet_t *new_net = NULL;
		if(neighbours>1) {
			// only reconnect if two connections ...
			bool first = true;
			for(int i=0; i<4; i++) {
				if(conn[i]!=NULL) {
					if(!first) {
						// replace both nets
						koord pos = gr->gib_pos().gib_2d()+koord::nsow[i];
						new_net = new powernet_t();
						welt->sync_add(new_net);
						replace(pos, net, new_net);
						conn[i]->calc_neighbourhood();
					}
					first = false;
				}
			}
		}

		setze_pos(koord3d::invalid);

		if(neighbours==0) {
			// delete in last or crossing
			if(welt->sync_remove( net )) {
				// but there is still something wrong with the logic here ...
				// so we only delete, if still present in the world
				delete net;
			}
			else {
				dbg->warning("~leitung()","net %p already deleted at (%i,%i)!",net,gr->gib_pos().x,gr->gib_pos().y);
			}
		}
	}
	gib_besitzer()->add_maintenance(-wegbauer_t::leitung_besch->gib_wartung());
}



/* returns the net identifier at this position
 * @author prissi
 */
static bool get_net_at(const grund_t *gr, powernet_t **l_net)
{
	if(gr) {
		// only this way pumps are handled properly
		const pumpe_t *p = dynamic_cast<pumpe_t *> (gr->suche_obj(ding_t::pumpe));
		if(p) {
			*l_net = p->get_net();
			return true;
		}
		// now handle drain
		const senke_t *s = dynamic_cast<senke_t *> (gr->suche_obj(ding_t::senke));
		if(s) {
			*l_net = s->get_net();
			return true;
		}
		// and now handle line
		const leitung_t *l = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
		if(l) {
			*l_net = l->get_net();
//DBG_MESSAGE("get_net_at()","Found net %p",l->get_net());
			return true;
		}
	}
	*l_net = NULL;
	return false;
}



/* sets the net at a position
 * @author prissi
 */
static void set_net_at(const grund_t *gr, powernet_t *new_net)
{
	if(gr) {
		// only this way pumps are handled properly
		pumpe_t *p = dynamic_cast<pumpe_t *> (gr->suche_obj(ding_t::pumpe));
		if(p) {
			p->set_net(new_net);
			return;
		}
		// now handle drain
		senke_t *s = dynamic_cast<senke_t *> (gr->suche_obj(ding_t::senke));
		if(s) {
			s->set_net(new_net);
//DBG_MESSAGE("set_net_at()","Using new net %p",s->get_net());
			return;
		}
		// and now handle line
		leitung_t *l = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
		if(l) {
			l->set_net(new_net);
//DBG_MESSAGE("set_net_at()","Using new net %p",l->get_net());
			return;
		}
	}
}



/* replace networks connection
 * non-trivial to handle transformers correctly
 * @author prissi
 */
void leitung_t::replace(koord base_pos, powernet_t *old_net, powernet_t *new_net)
{
	powernet_t *current;
	if(get_net_at(welt->lookup_kartenboden(base_pos),&current)  &&  current!=new_net) {
		// convert myself ...
//DBG_MESSAGE("leitung_t::replace()","My net %p by %p at (%i,%i)",new_net,current,base_pos.x,base_pos.y);
		set_net_at(welt->lookup_kartenboden(base_pos),new_net);
		//get_net_at(welt->lookup(base_pos),&current);
	}

	for(int i=0; i<4; i++) {
		koord	pos=base_pos+koord::nsow[i];
		if(get_net_at(welt->lookup_kartenboden(pos),&current)  &&  current!=new_net) {
			if(current!=old_net) {
				replace(pos,current,new_net);
			}
			else {
				replace(pos,old_net,new_net);
			}
		}
	}
}



/**
 * Connect this piece of powerline to its neighbours
 * -> this can merge power networks
 * @author Hj. Malthaner
 */
void leitung_t::verbinde()
{
	const koord pos=gib_pos().gib_2d();
	powernet_t *new_net;

	// first get my own ...
	get_net_at(welt->lookup_kartenboden(pos),&new_net);

//DBG_MESSAGE("leitung_t::verbinde()","Searching net at (%i,%i)",pos.x,pos.y);
	for( int i=0;  i<4  &&  new_net==NULL;  i++ ) {
		get_net_at(welt->lookup_kartenboden(pos+koord::nsow[i]),&new_net);
	}
//DBG_MESSAGE("leitung_t::verbinde()","Found net %p",new_net);

	// we are alone?
	if(net==NULL) {
		if(new_net!=NULL) {
			replace(pos, NULL, new_net);
			net = new_net;
		}
		else {
			// then we start a new net
			net = new powernet_t();
			welt->sync_add(net);
//DBG_MESSAGE("leitung_t::verbinde()","Creating new net %p",new_net);
		}
	}
	else if(new_net  &&  new_net!=net) {
		powernet_t *my_net = net;
		replace(pos, net, new_net);
		if(my_net) {
			welt->sync_remove(my_net);
			delete my_net;
		}
	}
}



ribi_t::ribi leitung_t::gib_ribi()
{
	const koord pos = gib_pos().gib_2d();
	const ribi_t::ribi ribi =
			(ist_leitung(welt, pos + koord(0,-1)) ? ribi_t::nord : ribi_t::keine) |
			(ist_leitung(welt, pos + koord(0, 1)) ? ribi_t::sued : ribi_t::keine) |
			(ist_leitung(welt, pos + koord( 1,0)) ? ribi_t::ost  : ribi_t::keine) |
			(ist_leitung(welt, pos + koord(-1,0)) ? ribi_t::west : ribi_t::keine);
	return ribi;
}



/* extended by prissi */
void leitung_t::calc_bild()
{
	const koord pos = gib_pos().gib_2d();

	grund_t *gr = NULL;
	if(welt->lookup(pos)!=NULL) {
		gr = welt->lookup(pos)->gib_kartenboden();
	}
	if(gr==NULL) {
		// no valid gound; usually happens during building ...
		return;
	}

	const ribi_t::ribi ribi=gib_ribi();
	hang_t::typ hang = gr->gib_grund_hang();
	if(hang != hang_t::flach) {
		setze_bild( wegbauer_t::leitung_besch->gib_hang_bild_nr(hang, 0));
	}
	else {
		if(gr->hat_wege()  ||  !gr->ist_natur()) {
			// crossing with road or rail
			if(ribi_t::ist_gerade_ns(ribi)) {
				setze_bild( wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::nord|ribi_t::ost,0));
			}
			else {
				setze_bild( wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::sued|ribi_t::ost,0));
			}
		}
		else {
			if(ribi_t::ist_gerade(ribi)  &&  !ribi_t::ist_einfach(ribi)  &&  (pos.x+pos.y)&1) {
				// every second skip mast
				if(ribi_t::ist_gerade_ns(ribi)) {
					setze_bild( wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::nord|ribi_t::west,0));
				}
				else {
					setze_bild( wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::sued|ribi_t::west,0));
				}
			}
			else {
				setze_bild( wegbauer_t::leitung_besch->gib_bild_nr(ribi,0));
			}
		}
	}
}



/**
 * Recalculates the images of all neighbouring
 * powerlines and the powerline itself
 *
 * @author Hj. Malthaner
 */
void leitung_t::calc_neighbourhood()
{
	for(int i=0; i<4; i++) {
		const koord pos = gib_pos().gib_2d() + koord::nsow[i];
		const planquadrat_t * plan = welt->lookup(pos);
		grund_t * gr = plan ? plan->gib_kartenboden() : 0;

		if(gr) {
			leitung_t * line = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
			if(line) {
				line->calc_bild();
			}
		}
	}
	calc_bild();
}


/**
 * @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void leitung_t::info(cbuffer_t & buf) const
{
	buf.append(translator::translate("Power"));
	buf.append(": ");
	buf.append(get_net()->get_capacity());
	buf.append("\nNet: ");
	buf.append((unsigned long)get_net());
}


/**
 * Wird nach dem Laden der Welt aufgerufen - �blicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void leitung_t::laden_abschliessen()
{
	calc_neighbourhood();
	verbinde();
	gib_besitzer()->add_maintenance(wegbauer_t::leitung_besch->gib_wartung());
}



/**
 * Speichert den Zustand des Objekts.
 *
 * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
 * soll.
 * @author Hj. Malthaner
 */
void leitung_t::rdwr(loadsave_t *file)
{
	uint32 value;

	ding_t::rdwr(file);
	if(file->is_saving()) {
		value = (unsigned long)get_net();
		file->rdwr_long(value, "\n");
	}
	else {
		file->rdwr_long(value, "\n");
		//      net = powernet_t::load_net((powernet_t *) value);
		set_net(NULL);
	}
}



/* from here on pump (source) stuff */
pumpe_t::pumpe_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
	fab = NULL;
}



pumpe_t::pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
}



pumpe_t::~pumpe_t()
{
	if(fab) {
		fab->set_prodfaktor( max(16,fab->get_prodfaktor()/2) );
		welt->sync_remove(this);
		fab = NULL;
	}
	gib_besitzer()->add_maintenance(umgebung_t::cst_maintain_transformer);
}



bool
pumpe_t::sync_step(long delta_t)
{
	if(fab==NULL) {
		return false;
	}
	image_id new_bild;
	if (!fab->gib_eingang().empty() && fab->gib_eingang()[0].menge <= 0) {
		new_bild = skinverwaltung_t::pumpe->gib_bild_nr(0);
	} else {
		// no input needed or has something to consume
		get_net()->add_power(delta_t*fab->get_base_production()*5);
		new_bild = skinverwaltung_t::pumpe->gib_bild_nr(1);
	}
	if(bild!=new_bild) {
		set_flag(ding_t::dirty);
		setze_bild( new_bild );
	}
	return true;
}



void
pumpe_t::laden_abschliessen()
{
	leitung_t::laden_abschliessen();
	gib_besitzer()->add_maintenance(-umgebung_t::cst_maintain_transformer-wegbauer_t::leitung_besch->gib_wartung());

	if(fab==NULL  &&  get_net()) {
		fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
		if(fab) {
			fab->set_prodfaktor( fab->get_prodfaktor()*2 );
		}
	}
	welt->sync_add(this);
}



/************************************ From here on drain stuff ********************************************/

senke_t::senke_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
	fab = NULL;
	einkommen = 1;
	max_einkommen = 1;
}


senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	einkommen = 1;
	max_einkommen = 1;
}



senke_t::~senke_t()
{
	if(fab!=NULL) {
		fab->set_prodfaktor( 16 );
		welt->sync_remove(this);
		fab = NULL;
	}
	gib_besitzer()->add_maintenance(umgebung_t::cst_maintain_transformer);
}



bool
senke_t::sync_step(long time)
{
	if(fab==NULL) {
		return false;
	}

	int want_power = time*fab->get_base_production();
	int get_power = get_net()->withdraw_power(want_power);
	image_id new_bild;
	if(get_power>want_power/2) {
		new_bild = skinverwaltung_t::senke->gib_bild_nr(1);
	} else {
		new_bild = skinverwaltung_t::senke->gib_bild_nr(0);
	}
	if(bild!=new_bild) {
		set_flag(ding_t::dirty);
		setze_bild( new_bild );
	}
	max_einkommen += want_power;
	einkommen += get_power;

	int faktor = (16*einkommen+16)/max_einkommen;
	if(max_einkommen>(2000<<11)) {
		gib_besitzer()->buche(einkommen >> 11, gib_pos().gib_2d(), COST_POWERLINES);
		gib_besitzer()->buche(einkommen >> 11, gib_pos().gib_2d(), COST_INCOME);
		einkommen = 0;
		max_einkommen = 1;
	}

	fab->set_prodfaktor( 16+faktor );
	return true;
}



void
senke_t::laden_abschliessen()
{
	leitung_t::laden_abschliessen();
	gib_besitzer()->add_maintenance(-umgebung_t::cst_maintain_transformer-wegbauer_t::leitung_besch->gib_wartung());

	if(fab==NULL  &&  get_net()) {
		fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
	}
	welt->sync_add(this);
}



void
senke_t::info(cbuffer_t & buf) const
{
	/* info in a drain */
	buf.append(translator::translate("Power"));
	buf.append(": ");
	buf.append(get_net()->get_capacity());
	buf.append("\n");
	buf.append(translator::translate("Available"));
	buf.append(": ");
	buf.append((200*einkommen+1)/(2*max_einkommen));
	buf.append("\nNet: ");
	buf.append((unsigned long)get_net());
}

/*
 * oberleitung.h
 *
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef wayobj_t_h
#define wayobj_t_h

#include "../simtypes.h"
#include "../simimg.h"
#include "../simdings.h"
#include "../simworld.h"
#include "../dataobj/ribi.h"
#include "../besch/way_obj_besch.h"
#include "../tpl/stringhashtable_tpl.h"

class spieler_t;
class koord;
class werkzeug_parameter_waehler_t;

/**
 * Overhead powelines for elctrifed tracks.
 *
 * @author Hj. Malthaner
 */
class wayobj_t : public ding_t
{
private:
	const way_obj_besch_t *besch;

	uint8 diagonal:1;
	uint8 hang:7;

	// direction of this wayobj
	ribi_t::ribi dir;

	ribi_t::ribi find_next_ribi(const grund_t *start, const koord dir) const;


public:
	wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch);

	wayobj_t(karte_t *welt, loadsave_t *file);

	virtual ~wayobj_t();

	const way_obj_besch_t *gib_besch() const {return besch;}

	/**
	* the front image, drawn before vehicles
	* @author V. Meyer
	*/
	image_id gib_bild() const {
		return hang ? besch->get_back_slope_image_id(hang) :
			(diagonal ? besch->get_back_diagonal_image_id(dir) : besch->get_back_image_id(dir));
	}

	/**
	* the front image, drawn after everything else
	* @author V. Meyer
	*/
	image_id gib_after_bild() const {
		return hang ? besch->get_front_slope_image_id(hang) :
			diagonal ? besch->get_front_diagonal_image_id(dir) : besch->get_front_image_id(dir);
	}

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return Gibt den typ des Objekts zur�ck.
	* @author Hj. Malthaner
	*/
	enum ding_t::typ gib_typ() const {return wayobj;}

	/**
	* no infowin
	* @author Hj. Malthaner
	*/
	void zeige_info() {}

	void calc_bild();

	/**
	* Speichert den Zustand des Objekts.
	*
	* @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	* soll.
	* @author Hj. Malthaner
	*/
	void rdwr(loadsave_t *file);

	// substracts cost
	void entferne(spieler_t *sp);

	/**
	* calculate image after loading
	* @author prissi
	*/
	void laden_abschliessen();

	// specific for wayobj
	void set_dir(ribi_t::ribi dir) { this->dir = dir; calc_bild(); }
	ribi_t::ribi get_dir() const { return dir; }

	/* the static routines */
private:
	static slist_tpl<const way_obj_besch_t *> liste;
	static stringhashtable_tpl<const way_obj_besch_t *> table;

public:
	// use this constructor; it will extend a matching existing wayobj
	static void extend_wayobj_t(karte_t *welt, koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch);

	static bool register_besch(way_obj_besch_t *besch);
	static bool alles_geladen();

	// search an object (currently only used by AI for caternary)
	static const way_obj_besch_t* wayobj_search(waytype_t wt,waytype_t own,uint16 time);

	/**
	 * Fill menu with icons of given stops from the list
	 * @author Hj. Malthaner
	 */
	static void fill_menu(werkzeug_parameter_waehler_t *wzw,
		waytype_t wtyp,
		int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
		int sound_ok,
		int sound_ko,
		const karte_t *welt);
};

#endif

#ifndef simpeople_h
#define simpeople_h

#include "simverkehr.h"
template<class T> class slist_mit_gewichten_tpl;

class fussgaenger_besch_t;

/**
 * Fu�g�nger sind auch Verkehrsteilnehmer.
 *
 * @author Hj. Malthaner
 * @see verkehrsteilnehmer_t
 */
class fussgaenger_t : public verkehrsteilnehmer_t
{
private:
	static slist_mit_gewichten_tpl<const fussgaenger_besch_t *> liste;
	static stringhashtable_tpl<const fussgaenger_besch_t *> table;

public:
	static int count;

private:
	static int strecke[8];

	const fussgaenger_besch_t *besch;

protected:
	void rdwr(loadsave_t *file);

	void calc_bild();

public:
	fussgaenger_t(karte_t *welt, loadsave_t *file);
	fussgaenger_t(karte_t *welt, koord3d pos);

	const char *gib_name() const {return "Fussgaenger";}
	enum ding_t::typ gib_typ() const {return fussgaenger;}

	bool sync_step(long delta_t);

	// prissi: always free
	virtual bool ist_weg_frei() { return 1; }
	virtual bool hop_check() { return 1; }

	// class register functions
	static bool register_besch(const fussgaenger_besch_t *besch);
	static bool laden_erfolgreich();

	static int gib_anzahl_besch();

	static void erzeuge_fussgaenger_an(karte_t *welt, koord3d k, int &anzahl);
};

#endif

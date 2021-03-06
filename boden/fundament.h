#ifndef boden_fundament_h
#define boden_fundament_h

//#include "grund.h"
#include "boden.h"
#include "../simimg.h"


/**
 * Das Fundament dient als Untergrund fuer alle Bauwerke in Simutrans.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.9 $
 */
class fundament_t : public grund_t
{
public:
	fundament_t(karte_t *welt, loadsave_t *file);
	fundament_t(karte_t *welt, koord3d pos,hang_t::typ hang);

	/**
	* Das Fundament hat immer das gleiche Bild.
	* @author Hj. Malthaner
	*/
	void calc_bild();

	/**
	* Das Fundament heisst 'Fundament'.
	* @return gibt 'Fundament' zurueck.
	* @author Hj. Malthaner
	*/
	const char *gib_name() const {return "Fundament";}

	enum grund_t::typ gib_typ() const {return fundament;}

	/**
	* Auffforderung, ein Infofenster zu �ffnen.
	* @author Hj. Malthaner
	*/
	virtual bool zeige_info();

	bool set_slope(hang_t::typ) { slope = 0; return false; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif

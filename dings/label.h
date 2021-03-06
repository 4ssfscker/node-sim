#ifndef label_h
#define label_h

#include "../simdings.h"
#include "../simimg.h"

/**
 * prissi: a dummy typ for old things, which are now ignored
 */
class label_t : public ding_t
{
public:
	label_t(karte_t *welt, loadsave_t *file);
	label_t(karte_t *welt, koord3d pos, spieler_t *sp, const char *text);
	~label_t();

	enum ding_t::typ gib_typ() const { return ding_t::label; }

	image_id gib_bild() const;
};

#endif

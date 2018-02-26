/*
 *
 *  fussgaenger_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __fussgaenger_besch_h
#define __fussgaenger_besch_h

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"

/*
 *  class:
 *      fussgaenger_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *	Automatisch generierte Autos, die in der Stadt umherfahren.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste
 */
class fussgaenger_besch_t : public obj_besch_t {
    friend class pedestrian_writer_t;

    uint16 gewichtung;
public:
    const char *gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
    const char *gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }
    int gib_bild_nr(ribi_t::dir dir) const
    {
	const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(dir);

	return bild ? bild->bild_nr : -1;
    }
    int gib_gewichtung() const
    {
	return gewichtung;
    }
};

#endif // __FUSSGAENGER_BESCH_H

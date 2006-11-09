/*
 *
 *  bildliste_besch.h
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
#ifndef __BILDLISTE_BESCH_H
#define __BILDLISTE_BESCH_H

#include "bild_besch.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Beschreibung eines eindimensionalen Arrays von Bildern.
 *
 *  Kindknoten:
 *	0   1. Bild
 *	1   2. Bild
 *	... ...
 */
class bildliste_besch_t : public obj_besch_t {
    friend class imagelist_reader_t;
    friend class imagelist_writer_t;

    uint16  anzahl;

public:
    bildliste_besch_t() : anzahl(0) {}

    int gib_anzahl() const
    {
	return anzahl;
    }
    const bild_besch_t *gib_bild(int i) const
    {
	return i >= 0 && i < anzahl ?
	    static_cast<const bild_besch_t *>(gib_kind(i)) : 0;
    }
    image_id gib_bild_nr(int i) const
    {
	const bild_besch_t *bild = gib_bild(i);

	return bild ? bild->bild_nr : IMG_LEER;
    }
};

#endif

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  sound_reader_t.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: ground_reader.cpp    $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: ground_reader.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>

#include "../sound_besch.h"
#include "sound_reader.h"

#include "../obj_node_info.h"

#include "../../simdebug.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      ground_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void sound_reader_t::register_obj(obj_besch_t *&data)
{
	sound_besch_t *besch = static_cast<sound_besch_t *>(data);
	sound_besch_t::register_besch(besch);
	DBG_DEBUG("sound_reader_t::read_node()","sound %s registered at %i",besch->gib_name(),besch->sound_id);
}


/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * sound_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
	char *besch_buf = static_cast<char *>(alloca(node.size));
#else
	// Hajo: reading buffer is better allocated on stack
	char besch_buf [node.size];
#endif

	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];
	sound_besch_t *besch = new sound_besch_t();
	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

		// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version==1) {
		// Versioned node, version 2
		besch->nr = decode_uint16(p);
	}
	else {
		dbg->fatal("sound_reader_t::read_node()","version %i not supported. File corrupt?", version);
	}

	return besch;
}

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      ground_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool sound_reader_t::successfully_loaded() const
{
    return sound_besch_t::alles_geladen();
}
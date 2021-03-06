#include "../../utils/cstring_t.h"
#include "../../tpl/slist_tpl.h"
#include "../bildliste2d_besch.h"
#include "obj_node.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"


void imagelist2d_writer_t::write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<slist_tpl<cstring_t> >& keys)
{
	bildliste2d_besch_t besch;

	obj_node_t node(this, 4, &parent, false);

	slist_iterator_tpl<slist_tpl<cstring_t> > iter(keys);

	besch.anzahl = keys.count();

	while (iter.next()) {
		imagelist_writer_t::instance()->write_obj(fp, node, iter.get_current());
	}
	node.write_data_at(fp, &besch.anzahl, 0, sizeof(uint16));
	uint16 dummy16 = 0;
	node.write_data_at(fp, &dummy16, 2, sizeof(uint16));
	node.write(fp);
}

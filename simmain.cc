#include <stdio.h>
#include <new>
#include <time.h>

#ifdef _MSC_VER
#include <new.h> // for _set_new_handler
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "pathes.h"

#include "simworld.h"
#include "simware.h"
#include "simview.h"
#include "simwin.h"
#include "simhalt.h"
#include "simimg.h"
#include "simcolor.h"
#include "simdepot.h"
#include "simskin.h"
#include "simconst.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "simcity.h"
#include "simvehikel.h"
#include "simfab.h"
#include "simplay.h"
#include "simsound.h"
#include "simintr.h"
#include "simticker.h"
#include "simmesg.h"

#include "linehandle_t.h"

#include "simsys.h"
#include "simgraph.h"
#include "simevent.h"
#include "simdisplay.h"
#include "simtools.h"

#include "simversion.h"

#include "gui/banner.h"
#include "gui/welt.h"
#include "gui/sprachen.h"
#include "gui/climates.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/load_relief_frame.h"

#include "dings/baum.h"

#include "utils/simstring.h"
#include "utils/cstring_t.h"
#include "utils/searchfolder.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/einstellungen.h"
#include "dataobj/translator.h"

#include "besch/reader/obj_reader.h"
#include "besch/sound_besch.h"

#include "bauer/hausbauer.h"

#include "simverkehr.h"


/* diagnostic routine:
 * show the size of several internal structures
 */
static void show_sizes()
{
	DBG_MESSAGE("Debug", "size of structures");

	DBG_MESSAGE("sizes", "koord: %d", sizeof(koord));
	DBG_MESSAGE("sizes", "koord3d: %d", sizeof(koord3d));
	DBG_MESSAGE("sizes", "ribi_t::ribi: %d", sizeof(ribi_t::ribi));
	DBG_MESSAGE("sizes", "halthandle_t: %d\n", sizeof(halthandle_t));

	DBG_MESSAGE("sizes", "ding_t: %d", sizeof(ding_t));
	DBG_MESSAGE("sizes", "gebaeude_t: %d", sizeof(gebaeude_t));
	DBG_MESSAGE("sizes", "baum_t: %d", sizeof(baum_t));
	DBG_MESSAGE("sizes", "weg_t: %d", sizeof(weg_t));
	DBG_MESSAGE("sizes", "stadtauto_t: %d\n", sizeof(stadtauto_t));

	DBG_MESSAGE("sizes", "grund_t: %d", sizeof(grund_t));
	DBG_MESSAGE("sizes", "boden_t: %d", sizeof(boden_t));
	DBG_MESSAGE("sizes", "wasser_t: %d", sizeof(wasser_t));
	DBG_MESSAGE("sizes", "planquadrat_t: %d\n", sizeof(planquadrat_t));

	DBG_MESSAGE("sizes", "ware_t: %d", sizeof(ware_t));
	DBG_MESSAGE("sizes", "vehikel_t: %d", sizeof(vehikel_t));
	DBG_MESSAGE("sizes", "haltestelle_t: %d\n", sizeof(haltestelle_t));

	DBG_MESSAGE("sizes", "karte_t: %d", sizeof(karte_t));
	DBG_MESSAGE("sizes", "spieler_t: %d\n", sizeof(spieler_t));
}



// render tests ...
static void show_times(karte_t *welt, karte_ansicht_t *view)
{
	DBG_MESSAGE("test", "testing img ... ");
	int i;

	long ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_img(10, 50, 50, 1);
	DBG_MESSAGE("test", "display_img(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 120, 100, 0, 1, 1);
	DBG_MESSAGE("test", "display_color_img(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 160, 150, 16, 1, 1);
	DBG_MESSAGE("test", "display_color_img(): next AI: %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 220, 200, 20, 1, 1);
	DBG_MESSAGE("test", "display_color_img(), other AI: %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300;  i++)
		display_flush_buffer();
	DBG_MESSAGE("test", "display_flush_buffer(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_text_proportional_len_clip(100, 120, "Dies ist ein kurzer Textetxt ...", 0, 0, false, true, -1, false);
	DBG_MESSAGE("test", "display_text_proportional_len_clip(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_fillbox_wh(100, 120, 300, 50, 0, false);
	DBG_MESSAGE("test", "display_fillbox_wh(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0; i < 200; i++) {
		view->display(true);
	}
	DBG_MESSAGE("test", "view->display(true): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0; i < 200; i++) {
		view->display(true);
		win_display_flush(0.0);
	}
	DBG_MESSAGE("test", "view->display(true) and flush: %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	intr_set(welt, view);
	welt->set_fast_forward(true);
	intr_disable();
	for (i = 0; i < 200; i++) {
		welt->sync_step(200);
		welt->step();
	}
	DBG_MESSAGE("test", "welt->sync_step/step(200): %i iterations took %i ms", i, dr_time() - ms);
}



/**
 * Show Intro Screen
 */
static void zeige_banner(karte_t *welt)
{
	banner_t* b = new banner_t();
	event_t ev;

	destroy_all_win();	// since eventually the successful load message is still there ....

	create_win(0, -48, -1, b, w_autodelete);

	// hide titelbar with this trick
	win_set_pos( b, 0, -48 );

	do {
		win_poll_event(&ev);
		check_pos_win(&ev);
		INT_CHECK("simmain 189");
		dr_sleep(10);
		welt->step();
	} while(win_is_top(b));

	if (IS_LEFTCLICK(&ev)) {
		do {
			display_get_event(&ev);
		} while (!IS_LEFTRELEASE(&ev));
	}
}



// read the settings from this file
void
parse_simuconf( tabfile_t &simuconf, int &disp_width, int &disp_height, int &fullscreen, cstring_t &objfilename, bool &multiuser )
{
	tabfileobj_t contents;

	simuconf.read(contents);

	print("Initializing tombstones ...\n");

	umgebung_t::max_convoihandles = contents.get_int("convoys", umgebung_t::max_convoihandles );
	umgebung_t::max_linehandles = contents.get_int("lines", umgebung_t::max_linehandles );
	umgebung_t::max_halthandles = contents.get_int("stations", umgebung_t::max_halthandles );

	umgebung_t::max_route_steps = contents.get_int("max_route_steps", umgebung_t::max_route_steps);

	umgebung_t::fussgaenger = contents.get_int("random_pedestrians", umgebung_t::fussgaenger) != 0;
	umgebung_t::stadtauto_duration = contents.get_int("default_citycar_life", umgebung_t::stadtauto_duration);	// ten normal years
	umgebung_t::drive_on_left = contents.get_int("drive_left", umgebung_t::drive_on_left );

	umgebung_t::verkehrsteilnehmer_info = contents.get_int("pedes_and_car_info", umgebung_t::verkehrsteilnehmer_info) != 0;
	umgebung_t::tree_info = contents.get_int("tree_info", umgebung_t::tree_info) != 0;
	umgebung_t::ground_info = contents.get_int("ground_info", umgebung_t::ground_info) != 0;
	umgebung_t::townhall_info = contents.get_int("townhall_info", umgebung_t::townhall_info) != 0;
	umgebung_t::single_info = contents.get_int("only_single_info", umgebung_t::single_info);

	umgebung_t::window_buttons_right = contents.get_int("window_buttons_right", umgebung_t::window_buttons_right);

	umgebung_t::starting_money = contents.get_int("starting_money", umgebung_t::starting_money );
	umgebung_t::maint_building = contents.get_int("maintenance_building", 5000);

	umgebung_t::show_names = contents.get_int("show_names", umgebung_t::show_names);
	umgebung_t::numbered_stations = contents.get_int("numbered_stations", umgebung_t::numbered_stations) != 0;
	umgebung_t::station_coverage_size = contents.get_int("station_coverage", umgebung_t::station_coverage_size);
	// Max number of steps in goods pathfinding
	umgebung_t::set_max_hops = contents.get_int("max_hops", umgebung_t::set_max_hops );
	// Max number of transfers in goods pathfinding
	umgebung_t::max_transfers = contents.get_int("max_transfers", 7);
	umgebung_t::passenger_factor = contents.get_int("passenger_factor", umgebung_t::passenger_factor); /* this can manipulate the passenger generation */

	// time stuff
	umgebung_t::show_month = contents.get_int("show_month", umgebung_t::show_month);
	umgebung_t::bits_per_month = contents.get_int("bits_per_month", umgebung_t::bits_per_month);
	umgebung_t::use_timeline = contents.get_int("use_timeline", umgebung_t::use_timeline);
	umgebung_t::starting_year = contents.get_int("starting_year", umgebung_t::starting_year);
	umgebung_t::max_acceleration = contents.get_int("fast_forward", umgebung_t::max_acceleration);

	umgebung_t::intercity_road_length = contents.get_int("intercity_road_length", umgebung_t::intercity_road_length);
	cstring_t *test = new cstring_t(ltrim(contents.get("intercity_road_type")));
	if(test->len()>0) {
		delete umgebung_t::intercity_road_type;
		umgebung_t::intercity_road_type = test;
	}
	else {
		delete test;
	}
	test = new cstring_t(ltrim(contents.get("city_road_type")));
	if(test->len()>0) {
		delete umgebung_t::city_road_type;
		umgebung_t::city_road_type = test;
	}
	else {
		delete test;
	}

	umgebung_t::autosave = (contents.get_int("autosave", umgebung_t::autosave));

	umgebung_t::crossconnect_factories = contents.get_int("crossconnect_factories", umgebung_t::crossconnect_factories) != 0;
	umgebung_t::crossconnect_factor = contents.get_int("crossconnect_factories_percentage", umgebung_t::crossconnect_factor);
	umgebung_t::just_in_time = contents.get_int("just_in_time", umgebung_t::just_in_time) != 0;
	umgebung_t::beginner_price_factor = contents.get_int("beginner_price_factor", umgebung_t::beginner_price_factor); /* this manipulates the good prices in beginner mode */
	umgebung_t::beginner_mode_first = contents.get_int("first_beginner", umgebung_t::beginner_mode_first); /* start in beginner mode */

	/* now the cost section */
	umgebung_t::cst_multiply_dock = contents.get_int("cost_multiply_dock", umgebung_t::cst_multiply_dock/(-100) ) * -100;
	umgebung_t::cst_multiply_station = contents.get_int("cost_multiply_station", umgebung_t::cst_multiply_station/(-100) ) * -100;
	umgebung_t::cst_multiply_roadstop = contents.get_int("cost_multiply_roadstop", umgebung_t::cst_multiply_roadstop/(-100) ) * -100;
	umgebung_t::cst_multiply_airterminal = contents.get_int("cost_multiply_airterminal", umgebung_t::cst_multiply_airterminal/(-100) ) * -100;
	umgebung_t::cst_multiply_post = contents.get_int("cost_multiply_post", umgebung_t::cst_multiply_post/(-100) ) * -100;
	umgebung_t::cst_multiply_headquarter = contents.get_int("cost_multiply_headquarter", umgebung_t::cst_multiply_headquarter/(-100) ) * -100;
	umgebung_t::cst_depot_rail = contents.get_int("cost_depot_rail", umgebung_t::cst_depot_rail/(-100) ) * -100;
	umgebung_t::cst_depot_road = contents.get_int("cost_depot_road", umgebung_t::cst_depot_road/(-100) ) * -100;
	umgebung_t::cst_depot_ship = contents.get_int("cost_depot_ship", umgebung_t::cst_depot_ship/(-100) ) * -100;
	umgebung_t::cst_signal = contents.get_int("cost_signal", umgebung_t::cst_signal/(-100) ) * -100;
	umgebung_t::cst_tunnel = contents.get_int("cost_tunnel", umgebung_t::cst_tunnel/(-100) ) * -100;
	umgebung_t::cst_third_rail = contents.get_int("cost_third_rail", umgebung_t::cst_third_rail/(-100) ) * -100;

	// alter landscape
	umgebung_t::cst_buy_land = contents.get_int("cost_buy_land", umgebung_t::cst_buy_land/(-100) ) * -100;
	umgebung_t::cst_alter_land = contents.get_int("cost_alter_land", umgebung_t::cst_alter_land/(-100) ) * -100;
	umgebung_t::cst_set_slope = contents.get_int("cost_set_slope", umgebung_t::cst_set_slope/(-100) ) * -100;
	umgebung_t::cst_found_city = contents.get_int("cost_found_city", umgebung_t::cst_found_city/(-100) ) * -100;
	umgebung_t::cst_multiply_found_industry = contents.get_int("cost_multiply_found_industry", umgebung_t::cst_multiply_found_industry/(-100) ) * -100;
	umgebung_t::cst_remove_tree = contents.get_int("cost_remove_tree", umgebung_t::cst_remove_tree/(-100) ) * -100;
	umgebung_t::cst_multiply_remove_haus = contents.get_int("cost_multiply_remove_haus", umgebung_t::cst_multiply_remove_haus/(-100) ) * -100;
	umgebung_t::cst_multiply_remove_field = contents.get_int("cost_multiply_remove_field", umgebung_t::cst_multiply_remove_field/(-100) ) * -100;
	// powerlines
	umgebung_t::cst_transformer = contents.get_int("cost_transformer", umgebung_t::cst_transformer/(-100) ) * -100;
	umgebung_t::cst_maintain_transformer = contents.get_int("cost_maintain_transformer", umgebung_t::cst_maintain_transformer/(-100) ) * -100;

	/* now the way builder */
	umgebung_t::way_count_straight = contents.get_int("way_straight", umgebung_t::way_count_straight);
	umgebung_t::way_count_curve = contents.get_int("way_curve", umgebung_t::way_count_curve);
	umgebung_t::way_count_double_curve = contents.get_int("way_double_curve", umgebung_t::way_count_double_curve);
	umgebung_t::way_count_90_curve = contents.get_int("way_90_curve", umgebung_t::way_count_90_curve);
	umgebung_t::way_count_slope = contents.get_int("way_slope", umgebung_t::way_count_slope);
	umgebung_t::way_count_tunnel = contents.get_int("way_tunnel", umgebung_t::way_count_tunnel);
	umgebung_t::way_max_bridge_len = contents.get_int("way_max_bridge_len", umgebung_t::way_max_bridge_len);
	umgebung_t::way_count_leaving_road = contents.get_int("way_leaving_road", umgebung_t::way_count_leaving_road);

	/*
	* Selection of savegame format through inifile
	*/
	const char *str = contents.get("saveformat");
	while (*str == ' ') str++;
	if (strcmp(str, "binary") == 0) {
		loadsave_t::set_savemode(loadsave_t::binary);
	} else if(strcmp(str, "zipped") == 0) {
		loadsave_t::set_savemode(loadsave_t::zipped);
	} else if(strcmp(str, "text") == 0) {
		loadsave_t::set_savemode(loadsave_t::text);
	}

	/*
	 * Default resolution
	 */
	disp_width = contents.get_int("display_width", disp_width);
	disp_height = contents.get_int("display_height", disp_height);
	fullscreen = contents.get_int("fullscreen", fullscreen);
	umgebung_t::fps = contents.get_int("frames_per_second",umgebung_t::fps);

	// Default pak file path
	objfilename = ltrim(contents.get_string("pak_file_path", DEFAULT_OBJPATH));

	// use different save directories
	multiuser &= contents.get_int("singleuser_install", 1)==1;

	print("Reading simuconf.tab successful!\n");

	simuconf.close();
}



/**
 * Dies wird in main mittels set_new_handler gesetzt und von der
 * Laufzeitumgebung im Falle des Speichermangels bei new() aufgerufen
 */
#ifdef _MSC_VER
int sim_new_handler(unsigned int)
#else
void sim_new_handler()
#endif
{
	dbg->fatal("sim_new_handler()", "OUT OF MEMORY");
#ifdef _MSC_VER
	return 0;
#endif
}


static const char *gimme_arg(int argc, char *argv[], const char *arg, int off)
{
	for (int i = 1; i < argc; i++) {
		if (tstrequ(argv[i], arg) && i < argc - off) return argv[i + off];
	}
	return NULL;
}


extern "C" int simu_main(int argc, char** argv)
{
	static const int resolutions[][2] = {
		{  640,  480 },
		{  800,  600 },
		{ 1024,  768 },
		{ 1280, 1024 },
		{  672,  496 } // try to force window mode with allegro
	};

	FILE * config = NULL; // die konfigurationsdatei

	int disp_width = 800;
	int disp_height = 600;
	int fullscreen = false;

#ifdef _MSC_VER
	_set_new_handler(sim_new_handler);
#else
	std::set_new_handler(sim_new_handler);
#endif

	// you really want help with this?
	if (gimme_arg(argc, argv, "-h",     0) ||
			gimme_arg(argc, argv, "-?",     0) ||
			gimme_arg(argc, argv, "-help",  0) ||
			gimme_arg(argc, argv, "--help", 0)) {
		printf(
			"\n"
			"---------------------------------------\n"
			"  Simutrans " VERSION_NUMBER "\n"
			"  released " VERSION_DATE "\n"
			"  developed\n"
			"  by the Simutrans team.\n"
			"\n"
			"  Send feedback and questions to:\n"
			"  <markus@pristovsek.de>\n"
			"\n"
			"  Based on Simutrans 0.84.21.2\n"
			"  by Hansj�rg Malthaner et. al.\n"
			"  <hansjoerg.malthaner@gmx.de>\n"
			"---------------------------------------\n"
		);
		return 0;
	}

#ifdef __BEOS__
	if(1) { // since BeOS only supports relative paths ...
#else
	// use current dir as basedir, else use program_dir
	if (gimme_arg(argc, argv, "-use_workdir",0)) {
#endif
		// save the current directories
		getcwd( umgebung_t::program_dir, 1024 );
#ifdef _WIN32
		strcat( umgebung_t::program_dir, "\\" );
#else
		strcat( umgebung_t::program_dir, "/" );
#endif
	}
	else {
		strcpy( umgebung_t::program_dir, argv[0] );
#ifdef _WIN32
		*(strrchr( umgebung_t::program_dir, '\\' )+1) = 0;
#else
		*(strrchr( umgebung_t::program_dir, '/' )+1) = 0;
#endif
		chdir( umgebung_t::program_dir );
	}

	// unmgebung init
	umgebung_t::testlauf      = (gimme_arg(argc, argv, "-test",     0) != NULL);
	umgebung_t::freeplay      = (gimme_arg(argc, argv, "-freeplay", 0) != NULL);
	umgebung_t::verbose_debug = (gimme_arg(argc, argv, "-debug",    0) != NULL);

	// parsing config/simuconf.tab
	print("Reading low level config data ...\n");
	bool found_simuconf=false;
	bool multiuser = (gimme_arg(argc, argv, "-singleuser", 0) == NULL);

	tabfile_t simuconf;
	if(simuconf.open("config/simuconf.tab")) {
		parse_simuconf( simuconf, disp_width, disp_height, fullscreen, umgebung_t::objfilename, multiuser );
		found_simuconf = true;
		simuconf.close();
	}

	// if set for multiuser, then parses the users config (if there)
	// but just retrieve the pak-path
	if(multiuser) {
		umgebung_t::user_dir = dr_query_homedir();

		cstring_t obj_conf = umgebung_t::user_dir;
		if(simuconf.open(obj_conf + "simuconf.tab")) {
			// determine my pak file path
			tabfileobj_t contents;
			simuconf.read(contents);
			umgebung_t::objfilename = ltrim(contents.get_string("pak_file_path", DEFAULT_OBJPATH));
			found_simuconf = true;
			simuconf.close();
		}
	}
	else {
		// save in program directory
		umgebung_t::user_dir = umgebung_t::program_dir;
	}

	// now set the desired objectfilename (overide all previous settings)
	if (gimme_arg(argc, argv, "-objects", 1)) {
		umgebung_t::objfilename = gimme_arg(argc, argv, "-objects", 1);
	}

	// now find the pak specific tab file ...
	cstring_t obj_conf = umgebung_t::objfilename + "config/simuconf.tab";
	cstring_t dummy("");
	if(simuconf.open((const char *)obj_conf)) {
		parse_simuconf( simuconf, disp_width, disp_height, fullscreen, dummy, multiuser );
		found_simuconf = true;
	}

	// now parse the user settings
	if(umgebung_t::user_dir!=umgebung_t::program_dir) {
		cstring_t obj_conf = umgebung_t::user_dir;
		if(simuconf.open(obj_conf + "simuconf.tab")) {
			parse_simuconf( simuconf, disp_width, disp_height, fullscreen, dummy, multiuser );
			found_simuconf = true;
		}
	}
	simuconf.close();

	chdir( umgebung_t::user_dir );
	if (gimme_arg(argc, argv, "-log", 0)) {
		init_logging("simu.log", true, gimme_arg(argc, argv, "-debug", 0) != NULL);
	} else if (gimme_arg(argc, argv, "-debug", 0) != NULL) {
		init_logging("stderr", true, gimme_arg(argc, argv, "-debug", 0) != NULL);
	} else {
		init_logging(NULL, false, false);
	}
	DBG_MESSAGE("simmain::main()", "Version: " VERSION_NUMBER "  Date: " VERSION_DATE);
	DBG_MESSAGE( "program_dir", umgebung_t::program_dir );
	DBG_MESSAGE( "home_dir", umgebung_t::user_dir );
#ifdef DEBUG
	if (gimme_arg(argc, argv, "-sizes", 0) != NULL) {
		// show the size of some structures ...
		show_sizes();
	}
#endif
	chdir( umgebung_t::program_dir );

	convoihandle_t::init( umgebung_t::max_convoihandles );
	linehandle_t::init( umgebung_t::max_linehandles );
	halthandle_t::init( umgebung_t::max_halthandles );
	// Max number of steps in goods pathfinding
	haltestelle_t::set_max_hops( umgebung_t::set_max_hops );

	// likely only the programm without graphics was downloaded
	if(!found_simuconf) {
		fprintf(stderr, "*** No simuconf.tab found ***\n\nPlease install a complete system\n");
		getc(stdin);
		return 0;
	}

	if (gimme_arg(argc, argv, "-res", 0) != NULL) {
		const char* res_str = gimme_arg(argc, argv, "-res", 1);
		const int res = *res_str - '1';

		switch (res) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				disp_width  = resolutions[res][0];
				disp_height = resolutions[res][1];
				fullscreen = true;
				break;

			case 5:
				fullscreen = false;
				// XXX missing break?

			default:
				fprintf(stderr,
					"invalid resolution, argument must be 1,2,3 or 4\n"
					"1=640x480, 2=800x600, 3=1024x768, 4=1280x1024\n"
				);
				return 0;
		}
	}

	fullscreen |= (gimme_arg(argc, argv, "-fullscreen", 0) != NULL);

	if (gimme_arg(argc, argv, "-screensize", 0) != NULL) {
		const char* res_str = gimme_arg(argc, argv, "-screensize", 1);
		int n = 0;

		if (res_str != NULL) {
			n = sscanf(res_str, "%dx%d", &disp_width, &disp_height);
		}

		if (n != 2) {
			fprintf(stderr,
				"invalid argument for -screensize option\n"
				"argument must be of format like 800x600\n"
			);
			return 1;
		}
	}

	const char* use_shm = gimme_arg(argc, argv, "-net",   0);
	const char* do_sync = gimme_arg(argc, argv, "-async", 0);

	print("Preparing display ...\n");
	simgraph_init(disp_width, disp_height, use_shm == NULL, do_sync == NULL, fullscreen);

	// just check before loading objects
	if (!gimme_arg(argc, argv, "-nosound", 0)) {
		print("Reading compatibility sound data ...\n");
		sound_besch_t::init();
	}

	// Adam - Moved away loading from simmain and placed into translator for better modularisation
	if (!translator::load(umgebung_t::objfilename)) {
		// installation error: likely only program started
		dbg->fatal("simmain::main()", "Unable to load any language files\n*** PLEASE INSTALL PROPER BASE FILES ***\n");
		exit(11);
	}

	print("Reading city configuration ...\n");
	stadt_t::cityrules_init(umgebung_t::objfilename);

	print("Reading forest configuration ...\n");
	baum_t::forestrules_init(umgebung_t::objfilename);

	// loading all paks
	print("Reading object data from %s...\n", (const char*)umgebung_t::objfilename);
	if (!obj_reader_t::init(umgebung_t::objfilename)) {
		fprintf(stderr, "reading object data failed.\n");
		exit(11);
	}

	bool new_world = true;
	cstring_t loadgame = "";

	if (gimme_arg(argc, argv, "-load", 0) != NULL) {
		char buf[256];
		chdir( umgebung_t::user_dir );
		/**
		 * Added automatic adding of extension
		 */
		sprintf(buf, SAVE_PATH_X "%s", (const char*)searchfolder_t::complete(gimme_arg(argc, argv, "-load", 1), "sve"));
		loadgame = buf;
		new_world = false;
	}
	else {
		char buffer[256];
		sprintf(buffer, "%sdemo.sve", (const char*)umgebung_t::objfilename);
		// access did not work!
		FILE *f=fopen(umgebung_t::objfilename+"demo.sve","rb");
		if(f) {
			// there is a demo game to load
			loadgame = buffer;
			fclose(f);
DBG_MESSAGE("simmain","loadgame file found at %s",buffer);
		}
	}

	if (gimme_arg(argc, argv, "-timeline", 0) != NULL) {
		const char* ref_str = gimme_arg(argc, argv, "-timeline", 1);
		if (ref_str != NULL) umgebung_t::use_timeline = atoi(ref_str);
	}

	if (gimme_arg(argc, argv, "-startyear", 0) != NULL) {
		const char * ref_str = gimme_arg(argc, argv, "-startyear", 1); //1930
		if (ref_str != NULL) umgebung_t::starting_year = atoi(ref_str);
	}

	// now always writing in user dir (which points the the program dir in multiuser mode)
	chdir( umgebung_t::user_dir );

	/* Jetzt, nachdem die Kommandozeile ausgewertet ist, koennen wir die
	 * Konfigurationsdatei lesen, und ggf. einige Einstellungen setzen
	 */
	config = fopen("simworld.cfg", "rb");
	int sprache = -1;
	if (config) {
		fscanf(config, "Lang=%d\n", &sprache);

		int dn = 0;
		fscanf(config, "DayNight=%d\n", &dn);
		umgebung_t::night_shift = (dn != 0);

		int b[6];
		fscanf(config, "AIs=%i,%i,%i,%i,%i,%i\n", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
		for (int i = 0; i < 6; i++) umgebung_t::automaten[i] = b[i];

		fscanf(
			config, "Messages=%d,%d,%d,%d\n",
			&umgebung_t::message_flags[0],
			&umgebung_t::message_flags[1],
			&umgebung_t::message_flags[2],
			&umgebung_t::message_flags[3]
		);

		fscanf(config, "Visual=%i,%i,%i,%i,%i\n", &b[0], &b[1], &b[2], &b[3], &b[4] );
		umgebung_t::hide_with_transparency = b[0]!=0;
		umgebung_t::hide_trees = b[1]!=0;
		umgebung_t::hide_buildings = b[2];
		umgebung_t::use_transparency_station_coverage = b[3]!=0;
		umgebung_t::station_coverage_show = b[4]!=0;

		int midi_volume=128, sound_volume=128;
		fscanf(config, "SoundMidiVolume=%d,%d\n", &sound_volume, &midi_volume );
		sound_set_global_volume(sound_volume);
		sound_set_midi_volume(midi_volume);

		int i_shuffle_music=false;
		fscanf(config, "SoundShuffle=%d\n", &i_shuffle_music );
		sound_set_shuffle_midi( i_shuffle_music!=0 );
		fclose(config);
	}

	karte_t *welt = new karte_t();
	karte_ansicht_t *view = new karte_ansicht_t(welt);
	welt->setze_ansicht( view );

	// some messages about old vehicle may appear ...
	message_t *msg = new message_t(welt);
	// to hide all messages
	message_t::get_instance()->set_flags(0, 0, 0, 0);

	if (!gimme_arg(argc, argv, "-nomidi", 0)) {
		print("Reading midi data ...\n");
		if(midi_init(umgebung_t::user_dir)) {
			midi_play(0);
		} else if(midi_init(umgebung_t::program_dir)) {
			midi_play(0);
		}
	}

	// set the frame per second
	const char *ref_str = gimme_arg(argc, argv, "-fps", 1);
	if (ref_str != NULL) {
		int want_refresh = atoi(ref_str);
		umgebung_t::fps = want_refresh < 5 ? 5 : (want_refresh > 100 ? 100 : want_refresh);
	}

	chdir(umgebung_t::program_dir);
	if(loadgame==""  ||  !welt->laden(loadgame)) {
		// create a default map
DBG_MESSAGE("init","map");
		welt->init(welt->gib_einstellungen());
	}

#ifdef DEBUG
	// do a render test?
	if (gimme_arg(argc, argv, "-times", 0) != NULL) {
		show_times(welt, view);
	}
#endif

	intr_set(welt, view);

	win_setze_welt(welt);
	win_display_menu();
	view->display(true);
	welt->set_fast_forward(true);

	// Bringe welt in ansehnlichen Zustand
	// bevor sie als Hintergrund f�r das intro dient
	if (loadgame=="") {
		welt->sync_step(welt->gib_zeit_ms() + welt->ticks_per_tag / 2);
		welt->step();
		welt->step();
		welt->step();
		welt->step();
	}
	welt->set_fast_forward(false);
	intr_refresh_display(true);

#ifdef USE_SOFTPOINTER
	// Hajo: give user a mouse to work with
	if (skinverwaltung_t::mouse_cursor != NULL) {
		// we must use our softpointer (only Allegro!)
		display_set_pointer(skinverwaltung_t::mouse_cursor->gib_bild_nr(0));
	}
#endif
	display_show_pointer(TRUE);

	welt->setze_dirty();

	translator::set_language("en");

	ticker::add_msg("Welcome to Simutrans, a game created by Hj. Malthaner and the Simutrans community.", koord::invalid, PLAYER_FLAG + 1);

	zeige_banner(welt);

	intr_set(welt, view);

	// Hajo: simgraph init loads default fonts, now we need to load
	// the real fonts for the current language
	if (sprache != -1) {
		translator::set_language(sprache);
	}
	sprachengui_t::init_font_from_lang();

	einstellungen_t *sets = new einstellungen_t(*welt->gib_einstellungen());
	if(new_world) {
		// load the default settings for new maps
		loadsave_t  file;
		if(file.rd_open("default.sve")) {
			sets->rdwr(&file);
			file.close();
		}
		else {
			// default without a matching file ...
			sets->setze_groesse(256, 256);
			sets->setze_anzahl_staedte(16);
			sets->setze_land_industry_chains(6);
			sets->setze_city_industry_chains(0);
			sets->setze_tourist_attractions(12);
			sets->setze_karte_nummer(simrand(999));
			sets->setze_allow_player_change(true);
			sets->setze_station_coverage(umgebung_t::station_coverage_size);
			sets->setze_use_timeline(umgebung_t::use_timeline == 1);
			sets->setze_starting_year(umgebung_t::starting_year);
		}
		sets->setze_station_coverage(umgebung_t::station_coverage_size);
		sets->setze_bits_per_month(umgebung_t::bits_per_month);
		sets->setze_beginner_mode(umgebung_t::beginner_mode_first);
		sets->setze_just_in_time(umgebung_t::just_in_time);
	}
	delete msg;

	do {
		// play next tune?
		check_midi();

		// to purge all previous old messages
		message_t *msg = new message_t(welt);
		msg->set_flags(umgebung_t::message_flags[0], umgebung_t::message_flags[1], umgebung_t::message_flags[2], umgebung_t::message_flags[3]);

		if (!umgebung_t::testlauf && new_world) {
			welt_gui_t *wg = new welt_gui_t(welt, sets);
			sprachengui_t* sg = new sprachengui_t();
			climate_gui_t* cg = new climate_gui_t(wg, sets);
			event_t ev;

			view->display(true);

			// we want to center wg (width 260) between sg (width 220) and cg (176)

			create_win(10, 40, -1, sg, w_info);
			create_win((disp_width - 220 - 176 -10 -10- 260)/2 + 220 + 10, (disp_height - 300) / 2, -1, wg, w_info);
			create_win((disp_width - 176-10), 40, -1, cg, w_info);

			setsimrand(dr_time(), dr_time());

			do {
				INT_CHECK("simmain 803");
				win_poll_event(&ev);
				INT_CHECK("simmain 805");
				check_pos_win(&ev);
				INT_CHECK("simmain 807");
				dr_sleep(5);
			} while(
				!wg->gib_load() &&
				!wg->gib_load_heightfield() &&
				!wg->gib_start() &&
				!wg->gib_close() &&
				!wg->gib_quit()
			);

			if (IS_LEFTCLICK(&ev)) {
				do {
					display_get_event(&ev);
				} while (!IS_LEFTRELEASE(&ev));
			}

			// Neue Karte erzeugen
			if (wg->gib_start()) {
				destroy_win(wg);
				destroy_win(sg);
				destroy_win(cg);

				nachrichtenfenster_t *nd = new nachrichtenfenster_t(welt, "Erzeuge neue Karte.\n", skinverwaltung_t::neueweltsymbol->gib_bild_nr(0));
				create_win(200, 100, nd, w_autodelete);
				intr_refresh_display(true);

				sets->heightfield = "";
				welt->init(sets);
				// save setting ...
				loadsave_t file;
				if(file.wr_open("default.sve",loadsave_t::binary,"settings only")) {
					// save default setting
					sets->rdwr(&file);
					file.close();
				}
				destroy_all_win();
			} else if(wg->gib_load()) {
				destroy_win(wg);
				destroy_win(sg);
				destroy_win(cg);
				create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
			} else if(wg->gib_load_heightfield()) {
				destroy_win(wg);
				destroy_win(sg);
				destroy_win(cg);
				einstellungen_t *sets = wg->gib_sets();
				welt->load_heightfield(sets);
			} else {
				destroy_win(wg);
				destroy_win(sg);
				destroy_win(cg);
				// quit the game
				if (wg->gib_quit()) break;
			}
		}

		loadgame = ""; // only first time

		// run the loop
		while(welt->interactive())
			;

		msg->get_flags(&umgebung_t::message_flags[0], &umgebung_t::message_flags[1], &umgebung_t::message_flags[2], &umgebung_t::message_flags[3]);
		welt->set_fast_forward(false);
		delete msg;

	} while (!umgebung_t::testlauf && !umgebung_t::quit_simutrans);

	intr_disable();

	delete welt;
	welt = NULL;

	delete view;
	view = 0;


	simgraph_exit();

	/* Zu guter Letzt schreiben wir noch die akuellen Einstellungen in eine
	 * Konfigurationsdatei
	 */
	config = fopen("simworld.cfg", "wb");
	if (config != NULL) {
		fprintf(config, "Lang=%d\n", translator::get_language());
		fprintf(config, "DayNight=%d\n", umgebung_t::night_shift);
		fprintf(
			config, "AIs=%d,%d,%d,%d,%d,%d\n",
			umgebung_t::automaten[0],
			umgebung_t::automaten[1],
			umgebung_t::automaten[2],
			umgebung_t::automaten[3],
			umgebung_t::automaten[4],
			umgebung_t::automaten[5]
		);
		fprintf(
			config, "Messages=%d,%d,%d,%d\n",
			umgebung_t::message_flags[0],
			umgebung_t::message_flags[1],
			umgebung_t::message_flags[2],
			umgebung_t::message_flags[3]
		);
		fprintf(
			config, "Visual=%d,%d,%d,%d,%d\n",
			umgebung_t::hide_with_transparency,
			umgebung_t::hide_trees,
			umgebung_t::hide_buildings,
			umgebung_t::use_transparency_station_coverage,
			umgebung_t::station_coverage_show
		);
		fprintf(config, "SoundMidiVolume=%d,%d\n", sound_get_global_volume(), sound_get_midi_volume() );
		fprintf(config, "SoundShuffle=%d,%d\n", sound_get_shuffle_midi(), 0 );
		fclose(config);
	}

	close_midi();

#if 0
	// free all list memories (not working, since there seems to be unitialized list still waiting for automated destruction)
	freelist_t::free_all_nodes();
#endif

	return 0;
}

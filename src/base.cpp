/* ============================================================================ *
 * Name        : base.cpp
 * Author      : Dirk Brinkmeier
 * Date        : 17.08.2014 Start of project
 * Changed     : 23.08.2019 Finished framework and music player application
 *               18.12.2019 Finished radio streaming feature (5Y 123D)
 * Description : <project>.cpp file
 * ============================================================================ */

/*
 * Standard header files
 */
#include "inc/application.h"
#include "inc/localization.h"
#include "inc/translation.h"
#include "inc/system.h"
#include "config.h"

/*
 * Application dependent header files
 */
#include "app/upnp.h"
#include "app/player.h"
#include "app/explorer.h"
#include "app/auxiliary.h"
#include "app/resources.h"
#include "app/network.h"
#include "app/sysconf.h"

using namespace app;
using namespace upnp;

/*
 * Instantiate application class and system data
 */
TSystemData sysdat;
TApplication application;
TLocale syslocale(ELT_SYSTEM);
TTranslator nls;

/*
 * Reference Music Player main application
 */
int main(int argc, char *argv[]) {
	// Local instantiation of application modules
	TUPnP upnp;
	TExplorer explorer;
	TAuxiliary auxiliary;
	TResources resources;
	TSystemConfiguration sysconf;
	TNetworkShares network;
	TPlayer player;

	// Initialize framework
	application.initialize(argc, argv, nls);

	// Execute optional modules

	// Execute application modules
	application.execute(upnp);
	application.execute(explorer);
	application.execute(auxiliary);
	application.execute(resources);
	application.execute(sysconf);
	application.execute(network);

	// Enable application events
	application.enableEvents();

	// Execute main application module
	application.execute(player);

	// Shut down framework
	application.finalize();
	return application.result();
}

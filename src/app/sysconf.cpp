/*
 * sysconf.cpp
 *
 *  Created on: 17.07.2020
 *      Author: dirk
 */

#include "sysconf.h"
#include "../inc/globals.h"
#include "storeconsts.h"

namespace app {

TSystemConfiguration::TSystemConfiguration() {
	debug = false;
}

TSystemConfiguration::~TSystemConfiguration() {
	cleanup();
}

int TSystemConfiguration::execute() {

	// Add actions and data links to webserver instance
	if (application.hasWebServer()) {

		// Setupt virtual display elements
		setupListBoxes();

		// Add virtual RESTful data URLs to webserver
		application.addWebLink("sysconf.json", &app::TSystemConfiguration::getSystemConfig, this, true);

		// Add named web actions
		application.addWebAction("OnSystemData", &app::TSystemConfiguration::onSystemConfigData, this, WAM_SYNC);
	}

	// Leave after initialization
	return EXIT_SUCCESS;
}

void TSystemConfiguration::cleanup() {
}

void TSystemConfiguration::setupListBoxes() {

	// Setup combo box for refresh interval in seconds
	lbxInterval.setID("lbxRefreshInterval");
	lbxInterval.setName("CONFIG_REFRESH_INTERVAL");
	lbxInterval.setOwner(sysdat.obj.webServer);
	lbxInterval.setFocus(html::ELF_FULL);
	lbxInterval.setStyle(html::ECS_HTML);
	lbxInterval.elements().add("1");
	lbxInterval.elements().add("2");
	lbxInterval.elements().add("5");
	lbxInterval.elements().add("10");

	// Setup combo box for JavaScript refresh timer in milliseconds
	lbxTimer.setID("lbxRefreshTimer");
	lbxTimer.setName("CONFIG_REFRESH_TIMER");
	lbxTimer.setOwner(sysdat.obj.webServer);
	lbxTimer.setFocus(html::ELF_FULL);
	lbxTimer.setStyle(html::ECS_HTML);
	lbxTimer.elements().add("1000");
	lbxTimer.elements().add("2000");
	lbxTimer.elements().add("5000");
	lbxTimer.elements().add("10000");

	// Setup combo box for verbosits level
	lbxVerbosity.setID("lbxVerbosityLevel");
	lbxVerbosity.setName("CONFIG_VERBOSITY_LEVEL");
	lbxVerbosity.setOwner(sysdat.obj.webServer);
	lbxVerbosity.setFocus(html::ELF_FULL);
	lbxVerbosity.setStyle(html::ECS_HTML);
	lbxVerbosity.elements().add("0");
	lbxVerbosity.elements().add("1");
	lbxVerbosity.elements().add("2");
	lbxVerbosity.elements().add("3");
	lbxVerbosity.elements().add("4");

	// Update view state
	CWebSettings config;
	application.getWebServer().getRunningConfiguration(config);
	updateViewState(config);
}


void TSystemConfiguration::updateViewState(TWebSettings& config) {
	// Set application store configuration value
	application.writeLocalStore(STORE_REFRESH_TIMER, config.refreshTimer);
	application.writeLocalStore(STORE_REFRESH_INTERVAL, config.refreshInterval);
	application.writeLocalStore(STORE_WEB_SOCKETS, config.allowWebSockets);

	// Update HTML components
	app::TLockGuard<app::TMutex> mtx(configMtx);
	lbxVerbosity.update(std::to_string((size_s)config.verbosity));
	lbxInterval.update(std::to_string((size_s)config.refreshInterval));
	lbxTimer.update(std::to_string((size_s)config.refreshTimer));
}


void TSystemConfiguration::getSystemConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get configuration values
	CWebSettings config;
	application.getWebServer().getRunningConfiguration(config);

	// Return configuration parameters as JSON object
	util::TVariantValues response;
	response.add("Verbosity", config.verbosity);
	response.add("RefreshInterval", config.refreshInterval);
	response.add("RefreshTimer", config.refreshTimer);
	response.add("AllowWebSockets", config.allowWebSockets);

	// Build JSON response
	jsonConfig = response.asJSON().text();
	if (!jsonConfig.empty()) {
		data = jsonConfig.c_str();
		size = jsonConfig.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getSystemConfig() JSON = " << jsonConfig << app::reset << std::endl;
}

void TSystemConfiguration::onSystemConfigData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	if (debug) params.debugOutput("TSystemConfiguration::onSystemConfigData()", "  ");

	// Read configuration values from parameter list
	CWebSettings config;
	application.getWebServer().getRunningConfiguration(config);

	// Do not save parameters during active library scan...
	config.allowWebSockets = params["AllowWebSockets"].asBoolean(config.allowWebSockets);
	config.refreshInterval = params["RefreshInterval"].asInteger(config.refreshInterval);
	config.refreshTimer = params["RefreshTimer"].asInteger(config.refreshTimer);
	config.verbosity = params["Verbosity"].asInteger(config.verbosity);

	// Save configuration and update view state
	application.getWebServer().setRunningConfiguration(config);
	updateViewState(config);
}

} /* namespace app */

/*
 * network.cpp
 *
 *  Created on: 17.07.2020
 *      Author: dirk
 */

#include "network.h"
#include "../inc/globals.h"
#include "../inc/encoding.h"
#include "../inc/sysutils.h"
#include "storeconsts.h"

namespace app {

TNetworkShares::TNetworkShares() {
	debug = false;
	initialized = false;
	wtMountStatusColor = nil;
	wtMountStatusCaption = nil;
}

TNetworkShares::~TNetworkShares() {
	cleanup();
}

int TNetworkShares::execute() {

	// Add actions and data links to webserver instance
	if (application.hasWebServer()) {

		// Setupt virtual display elements
		setupListBoxes();
		setupButtons();

		// Add prepare handler to catch URI parameters...
		application.addWebPrepareHandler(&app::TNetworkShares::prepareWebRequest, this);

		// Add virtual RESTful data URLs to webserver
		application.addWebLink("mount.json", &app::TNetworkShares::getMountState, this, true);
		application.addWebLink("network.json", &app::TNetworkShares::getNetworkConfig, this, true);

		// Add named web actions
		application.addWebAction("OnNetworkData", &app::TNetworkShares::onNetworkData, this, WAM_SYNC);
		application.addWebAction("OnNetworkButtonClick", &app::TNetworkShares::onButtonClick, this, WAM_SYNC);
	}

	// Leave after initialization
	return EXIT_SUCCESS;
}

void TNetworkShares::cleanup() {
}

int TNetworkShares::prepare() {
	int r = EXIT_SUCCESS;

	// Read configuration file
	openConfig(application.getConfigFolder());
	reWriteConfig();

	// Add data links to webserver instance before property values set by mount actions
	if (application.hasWebServer()) {
		wtMountStatusCaption = application.addWebToken("MOUNT_STATUS_CAPTION", "Undefined");
		wtMountStatusColor = application.addWebToken("MOUNT_STATUS_COLOR", "label-default");
	}

	// Create local mount folder
	CNetworkValues config;
	getNetworkValues(config);
	if (!util::folderExists(config.localpath)) {
		if (util::createDirektory(config.localpath)) {
			writeLog(util::csnprintf("Local mount folder <%> created.", config.localpath));
		} else {
			writeLog(util::csnprintf("Creating local mount folder <%> created failed $", config.localpath, sysutil::getSysErrorMessage(errno)));
			r = EXIT_FAILURE;
		}
	} else {
		writeLog(util::csnprintf("Using local mount folder <%>", config.localpath));
	}

	// Mount remote file system
	doMountRemoteFilesSystem(config);

	return r;
}

void TNetworkShares::unprepare() {
	// Mount remote file system
	// doUnmountRemoteFilesSystem();
	if (initialized) {
		umountRemoteFilesSystem();
		writeConfig();
	}
}

void TNetworkShares::openConfig(const std::string& configPath) {
	std::string path = util::validPath(configPath);
	std::string file = path + "network.conf";
	config.open(file);
	reWriteConfig();
	initialized = true;
}

void TNetworkShares::readConfig() {
	config.setSection("Network");
	shares.hostname = config.readString("ExternalHostName", "example.dyndns.org:80");

	config.setSection("Shares");
	debug = config.readBool("Debug", debug);
	shares.type  = config.readString("MountType", "NFS");
	shares.options  = config.readString("MountOptions", "vers=3,nolock");
	shares.localpath = config.readPath("LocalMountPath", application.getDataRootFolder() + "media/");
	shares.remotepath = config.readString("RemoteMountPath", "192.168.1.2:/music");
	shares.username = config.readString("RemoteUserName", "");
	shares.password = config.readString("RemotePassword", "");
	shares.allowmount = config.readBool("AllowMount", false);
}

void TNetworkShares::writeConfig() {
	config.setSection("Network");
	config.writeString("ExternalHostName", shares.hostname);

	config.setSection("Shares");
	config.writeString("MountType", shares.type);
	config.writeString("MountOptions",shares.options);
	config.writePath("LocalMountPath", shares.localpath);
	config.writeString("RemoteMountPath", shares.remotepath);
	config.writeString("RemoteUserName", shares.username);
	config.writeString("RemotePassword", shares.password);
	config.writeBool("AllowMount", shares.allowmount, app::INI_BLYES);
	config.deleteKey("ExternalHostName");

	// Write configuration file
	config.flush();

	// Set application store configuration value
	application.writeLocalStore(STORE_HOST_NAME, shares.hostname);
}

void TNetworkShares::reWriteConfig() {
	readConfig();
	writeConfig();
}

void TNetworkShares::writeDebug(const std::string& text) const {
	if (debug) {
		application.writeLog("[Mount] " + text);
	}
}

void TNetworkShares::writeLog(const std::string& text) const {
	application.writeLog("[Mount] " + text);
}

void TNetworkShares::setupListBoxes() {

	// Setup combo box for remote filesystem mounts
	lbxMounts.setID("lbxFileSystems");
	lbxMounts.setName("REMOTE_FILE_SYSTEMS");
	lbxMounts.setOwner(sysdat.obj.webServer);
	lbxMounts.setFocus(html::ELF_PARTIAL);
	lbxMounts.setStyle(html::ECS_HTML);
	lbxMounts.elements().add("NFS (Network File System / Linux)");
	lbxMounts.elements().add("SMB (Server Message Block / Microsoft Windows)");

	// Set current mount type
	CNetworkValues config;
	getNetworkValues(config);
	setRemoteMountType(config.type);
}

void TNetworkShares::setupButtons() {

	btnMount.setOwner(sysdat.obj.webServer);
	btnMount.setHint("Click to mount remote filesystem");
	btnMount.setGlyphicon("glyphicon-ok");
	btnMount.setName("BTN_MOUNT");
	btnMount.setID("btnMountPoint");
	btnMount.setValue("MOUNTFS");
	btnMount.setClick("onButtonClick(event);");
	btnMount.setType(html::ECT_DEFAULT);
	btnMount.setSize(html::ESZ_MEDIUM);
	btnMount.setAlign(html::ECA_LEFT);
	btnMount.update();

	btnUnmount.setOwner(sysdat.obj.webServer);
	btnUnmount.setHint("Click to unmount remote filesystem");
	btnUnmount.setGlyphicon("glyphicon-remove");
	btnUnmount.setName("BTN_UNMOUNT");
	btnUnmount.setID("btnUnmountPoint");
	btnUnmount.setValue("UNMOUNTFS");
	btnUnmount.setClick("onButtonClick(event);");
	btnUnmount.setType(html::ECT_DEFAULT);
	btnUnmount.setSize(html::ESZ_MEDIUM);
	btnUnmount.setAlign(html::ECA_LEFT);
	btnUnmount.update();
}

void TNetworkShares::prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared) {
	if (query["prepare"].asString() == "yes") {
		std::string title = util::tolower(query["title"].asString());
		bool found = false;

		// Prepare detection of transition of mount state
		if (!found && title == "mount") {

			// Get current state
			size_t current = getMountChanged();
			size_t last = session["HTML_CURRENT_MOUNT"].asInteger(0);

			// Set defaults...
			if (last == 0)
				last = current;

			// Store values for songs in session
			session.add("HTML_CURRENT_MOUNT", current);
			session.add("HTML_LAST_MOUNT", last);
			found = true;
		}
	}
}

void TNetworkShares::addMountConfig(util::TVariantValues& response) {

	// Get configuration values
	CNetworkValues config;
	getNetworkValues(config);

	// Add hostname parameter
	response.add("ExternalHostName", config.hostname);

	// Return configuration parameters as JSON object
	response.add("MountType", config.type);
	response.add("MountOptions", config.options);
	response.add("LocalMountPath", config.localpath);
	response.add("RemoteMountPath", config.remotepath);
	response.add("RemoteUserName", util::TURL::encode(util::strToStr(config.username, "Username")));
	response.add("RemotePassword", util::TURL::encode(util::strToStr(config.password, "12345")));
	response.add("AllowMount", config.allowmount);
}

void TNetworkShares::getNetworkConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

    // Return configuration parameters as JSON object
    util::TVariantValues response;
    addMountConfig(response);

    // Build JSON response
    jsonNetwork = response.asJSON().text();
    if (!jsonNetwork.empty()) {
        data = jsonNetwork.c_str();
        size = jsonNetwork.size();
    }

    if (debug) aout << app::yellow << "TPlayer::getNetworkConfig() JSON = " << jsonNetwork << app::reset << std::endl;
}

void TNetworkShares::getMountState(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Get curent state of mount
	TMountState state;
	getMountValues(state);

	// Compare stored session hash value with current file hash
	size_t current = session["HTML_CURRENT_MOUNT"].asInteger(0);
	size_t last = session["HTML_LAST_MOUNT"].asInteger(0);
	bool transition = current != last;

	// Return state as JSON
	util::TVariantValues response;
	response.add("State", state.state);
	response.add("Message", util::TURL::encode(state.message));
	response.add("Transition", transition);

	// Add current configuration parameters on mount state changed
	if (transition) {
		addMountConfig(response);
	}

	// Build JSON response
	jsonMount = response.asJSON().text();
	if (!jsonMount.empty()) {
		data = jsonMount.c_str();
		size = jsonMount.size();
	}

	if (debug) aout << app::yellow << "TNetworkShares::getMountState() JSON = " << jsonMount << app::reset << std::endl;
}


void TNetworkShares::onNetworkData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	writeLog("[Event] Set network configuration for [" + key + "] = [" + value + "]");
	if (debug) params.debugOutput("TNetworkShares::onNetworkData()", "  ");
	bool ok = false;

	// Read configuration values from parameter list
	CNetworkValues running, values;
	getNetworkValues(values);
	running = values;

	// Do not save parameters during active library scan...
	values.hostname   = util::trim(params["ExternalHostName"].asString(values.hostname));
	values.type       = util::trim(params["MountType"].asString(values.type));
	values.options    = util::trim(params["MountOptions"].asString(values.options));
	values.localpath  = util::validPath(util::trim(params["LocalMountPath"].asString(values.localpath)));
	values.remotepath = util::trim(params["RemoteMountPath"].asString(values.remotepath));
	values.username   = util::trim(params["RemoteUserName"].asString(values.username));
	values.password   = util::trim(params["RemotePassword"].asString(values.password));
	values.allowmount = params["AllowMount"].asBoolean(values.allowmount);

	// Log new values
	std::string enabled = values.allowmount ? "yes" : "no";
	writeLog("[Network] Hostname [" + values.hostname + "]");
	writeLog("[Network] Type     [" + values.type + "]");
	writeLog("[Network] Local    [" + values.localpath + "]");
	writeLog("[Network] Remote   [" + values.remotepath + "]");
	writeLog("[Network] Options  [" + values.options + "]");
	writeLog("[Network] Username [" + values.username + "]");
	writeLog("[Network] Password [" + values.password + "]");
	writeLog("[Network] Enabled  [" + enabled + "]");

	// Set new values
	setNetworkValues(values);
	setRemoteMountType(values.type);

	// Configuration changed?
	if (running != values) {
		if (running.allowmount /*&& !values.allowmount*/) {
			running.allowmount = false;
			writeLog(util::csnprintf("Unmount network share on <%>", running.localpath));
			doUnmountRemoteFilesSystem(running);
			ok = true;
		}
		if (values.allowmount) {
			writeLog(util::csnprintf("Mount network share on <%>", values.localpath));
			doMountRemoteFilesSystem(values);
			ok = true;
		}
	}

	// Check if enabled and mounted when no parameters changed
	if (values.allowmount && !ok) {
		if (!isRemoteFilesSystemMounted(values)) {
			writeLog(util::csnprintf("Mount network share on <%>", values.localpath));
			doMountRemoteFilesSystem(values);
		}
	}
}

void TNetworkShares::onButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	writeLog("[Event] [Button] [" + key + "] = [" + value + "]");
	std::string action = util::toupper(value);
	if (action == "MOUNTFS") {
		doMountRemoteFilesSystem();
	} else if (action == "UNMOUNTFS") {
		doUnmountRemoteFilesSystem();
	}
}

void TNetworkShares::setNetworkValues(const CNetworkValues& values) {
	app::TLockGuard<app::TMutex> lock(configMtx);
	shares = values;
	writeConfig();
}

void TNetworkShares::getNetworkValues(CNetworkValues& values) {
	app::TLockGuard<app::TMutex> lock(configMtx);
	values = shares;
}

void TNetworkShares::setRemoteMountType(const std::string filesystem) {
	app::TLockGuard<app::TMutex> lock(mountMtx);
	setRemoteMountTypeWithNolock(filesystem);
}

void TNetworkShares::setRemoteMountTypeWithNolock(const std::string filesystem) {
	if (lbxMounts.elements().size() > 0) {
		size_t idx = lbxMounts.elements().find(filesystem, util::EC_COMPARE_HEADING);
		if (std::string::npos == idx) {
			idx = 0;
		}
		lbxMounts.update(lbxMounts.elements().at(idx));
	}
}

bool TNetworkShares::mountRemoteFilesSystem(const CNetworkValues& values) {
	bool r = false;
	errno = EINVAL;
	if (values.allowmount) {
		bool found = false;
		std::string remote, type, options;
		std::string local = util::validPath(values.localpath);
		if (!found && values.type == "NFS") {
			std::string host = util::nfsHostName(values.remotepath);
			std::string ip = inet::getInet4Address(host);
			if (!ip.empty()) {
				type = "nfs";
				options = "addr=" + ip;
				if (!values.options.empty())
					options += "," + values.options;
				remote = values.remotepath;
				found = true;
			}
		}
		if (!found && values.type == "SMB") {
			std::string host = util::urlHostName(values.remotepath);
			std::string ip = inet::getInet4Address(host);
			if (!ip.empty()) {
				type = "cifs";
				options = "ip=" + ip + ",username=" + values.username + ",password=" + values.password;
				remote = values.remotepath;
				found = true;
			}
		}
		if (found) {
			writeLog(util::csnprintf("Mount command --> sudo mount -v -t % -o $ % %", type, options, remote, local));
			r = util::mount(remote, local, type, options, MS_DEFAULT);
		}
	} else {
		umountRemoteFilesSystem(values);
		r = true;
	}
	return r;
}

bool TNetworkShares::mountRemoteFilesSystem() {
	CNetworkValues values;
	getNetworkValues(values);
	return mountRemoteFilesSystem(values);
}

bool TNetworkShares::umountRemoteFilesSystem(const CNetworkValues& values) {
	return util::umount(values.localpath, 0);
}

bool TNetworkShares::umountRemoteFilesSystem() {
	CNetworkValues values;
	getNetworkValues(values);
	return umountRemoteFilesSystem(values);
}

void TNetworkShares::doMountRemoteFilesSystem() {
	CNetworkValues values;
	getNetworkValues(values);
	doMountRemoteFilesSystem(values);
}

void TNetworkShares::doMountRemoteFilesSystem(const CNetworkValues& values) {
	std::string text = "undefined";
	EMountState state = EMS_UNDEFINED;
	if (values.allowmount) {
		if (!isRemoteFilesSystemMounted(values)) {
			if (util::folderExists(values.localpath)) {
				if (mountRemoteFilesSystem(values)) {
					state = EMS_CONNECTED;
					text = "Network share mounted";
					writeLog(util::csnprintf("Mounted network share <%>", values.remotepath));
				} else {
					std::string error = sysutil::getSysErrorMessage(errno);
					state = EMS_ERROR;
					if (errno != EXIT_SUCCESS)
						text = util::csnprintf("Mount network share failed : $ (%)", error, errno);
					else
						text = "Mount network share failed";
					writeLog(util::csnprintf("Mounting network share <%> failed $", values.remotepath, error));
				}
			} else {
				std::string error = "Local path does not exists";
				state = EMS_ERROR;
					text = util::csnprintf("Mount network share failed : $", error);
					writeLog(util::csnprintf("Mounting network share <%> failed $", values.remotepath, error));
			}
		} else {
			state = EMS_CONNECTED;
			text = "Network share mounted";
			writeLog(util::csnprintf("Network share <%> is already mounted.", values.remotepath));
		}
	} else {
		state = EMS_DISCONNECTED;
		text = "Network mount disabled";
		writeLog(util::csnprintf("Mount <%> disabled by configuration", values.remotepath));
	}
	updateMountStatusLabel(text, state);
	updateNetworkMountButtons(values.allowmount, state);
	setMountValues(state, text);
}

void TNetworkShares::doUnmountRemoteFilesSystem() {
	CNetworkValues netconf;
	getNetworkValues(netconf);
	doUnmountRemoteFilesSystem(netconf);
}

void TNetworkShares::doUnmountRemoteFilesSystem(const CNetworkValues& values) {
	std::string text = "undefined";
	EMountState state = EMS_UNDEFINED;
	if (isRemoteFilesSystemMounted(values)) {
		if (umountRemoteFilesSystem(values)) {
			state = EMS_DISCONNECTED;
			text = values.allowmount ? "Network mount disconnected" : "Network mount disabled";
			writeLog(util::csnprintf("Unmounted network share <%>", values.remotepath));
		} else {
			std::string error = sysutil::getSysErrorMessage(errno);
			state = EMS_ERROR;
			text = util::csnprintf("Unmounting network share failed : $ (%)", error, errno);
			writeLog(util::csnprintf("Unmounting network share <%> failed $", values.remotepath, error));
		}
	} else {
		state = EMS_DISCONNECTED;
		text = values.allowmount ? "Network mount disconnected" : "Network mount disabled";
		writeLog(util::csnprintf("Network share <%> is not mounted.", values.remotepath));
	}
	updateMountStatusLabel(text, state);
	updateNetworkMountButtons(values.allowmount, state);
	setMountValues(state, text);
}

size_t TNetworkShares::scanStorageMounts(util::TStringList& mounts) {
	mounts.clear();
	util::getMountPoints(mounts, util::MT_DISK);
	if (!mounts.empty()) mounts.sort();
	return mounts.size();
}

bool TNetworkShares::isRemoteFilesSystemMounted(const CNetworkValues& values) {
	util::TStringList drives;
	if (scanStorageMounts(drives) > 0) {
		size_t idx = drives.find(util::stripLastPathSeparator(values.localpath), util::EC_COMPARE_FULL);
		if (std::string::npos != idx)
			return true;
	}
	return false;
}

bool TNetworkShares::isRemoteFilesSystemMounted() {
	CNetworkValues values;
	getNetworkValues(values);
	return isRemoteFilesSystemMounted(values);
}

void TNetworkShares::updateNetworkMountButtons(const bool enabled, const EMountState state) {
	app::TLockGuard<app::TMutex> lock(componentMtx);
	updateNetworkMountButtonsWithNolock(enabled, state);
}

void TNetworkShares::updateNetworkMountButtonsWithNolock(const bool enabled, const EMountState state) {
	if (enabled) {
		bool mounted = state == EMS_CONNECTED;
		btnMount.setEnabled(!mounted);
		btnUnmount.setEnabled(mounted);
	} else {
		btnMount.setEnabled(false);
		btnUnmount.setEnabled(false);
	}
	btnMount.update();
	btnUnmount.update();
}

void TNetworkShares::setMountValues(const EMountState state, const std::string& message) {
	app::TLockGuard<app::TMutex> lock(stateMtx);
	this->state.state = state;
	this->state.message = message;
	this->state.invalidate();
}

void TNetworkShares::getMountValues(TMountState& state) {
	app::TLockGuard<app::TMutex> lock(stateMtx);
	state.state = this->state.state;
	state.message = this->state.message;
	state.changed = this->state.changed;
}

size_t TNetworkShares::getMountChanged() {
	app::TLockGuard<app::TMutex> lock(stateMtx);
	return state.changed;
}

void TNetworkShares::updateMountStatusLabel(const std::string text, const EMountState state) {
	if (util::assigned(wtMountStatusColor)) {
		app::TLockGuard<app::TMutex> lock(componentMtx);
		*wtMountStatusCaption = text;
		std::string color;
		switch (state) {
			case EMS_CONNECTED:
				color = "label-primary";
				break;
			case EMS_DISCONNECTED:
				color = "label-default";
				break;
			case EMS_ERROR:
				color = "label-danger";
				break;
			default:
				color = "label-warning";
				break;
		}
		*wtMountStatusColor = color;
		wtMountStatusColor->invalidate();
	}
}

} /* namespace app */

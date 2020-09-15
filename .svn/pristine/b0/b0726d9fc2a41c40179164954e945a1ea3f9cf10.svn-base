/*
 * network.h
 *
 *  Created on: 17.07.2020
 *      Author: dirk
 */

#ifndef APP_NETWORK_H_
#define APP_NETWORK_H_

#include "../inc/application.h"
#include "../inc/component.h"
#include "../inc/fileutils.h"
#include "../inc/inifile.h"
#include <sys/mount.h>

namespace app {

#define MS_DEFAULT_RW MS_NOATIME | MS_NOEXEC | MS_NODIRATIME | MS_NOSUID
#define MS_DEFAULT_RO MS_RDONLY | MS_DEFAULT_RW
#define MS_DEFAULT MS_DEFAULT_RO

enum EMountState {
	EMS_UNDEFINED,
	EMS_CONNECTED,
	EMS_DISCONNECTED,
	EMS_ERROR
};

typedef struct CMountState {
	app::EMountState state;
	std::string message;
	size_t changed;

	void prime() {
		state = EMS_DISCONNECTED;
		changed = 0;
	}

	void clear() {
		prime();
		message.clear();
	}

	void invalidate() {
		++changed;
		if (changed > 0xFFFF)
			changed = 1;
	}

	CMountState() {
		prime();
	};

} TMountState;


struct CNetworkValues {
	std::string type;
	std::string options;
	std::string localpath;
	std::string remotepath;
	std::string username;
	std::string password;
	std::string hostname;
	bool allowmount;
	size_t changed;

	void prime() {
		allowmount = false;
		changed = 0;
	}

	void clear() {
		prime();
		type.clear();
		options.clear();
		localpath.clear();
		remotepath.clear();
		username.clear();
		password.clear();
		hostname.clear();
	}

	void invalidate() {
		++changed;
		if (changed > 0xFFFF)
			changed = 1;
	}

	bool compare(const CNetworkValues &value) const {
		return type == value.type &&
			options == value.options &&
			localpath == value.localpath &&
			remotepath == value.remotepath &&
			username == value.username &&
			password == value.password &&
			allowmount == value.allowmount;
	}

	bool operator == (const CNetworkValues &value) const { return compare(value); };
	bool operator != (const CNetworkValues &value) const { return !compare(value); };

	CNetworkValues& operator = (const CNetworkValues &value) {
		type = value.type;
		options = value.options;
		localpath = value.localpath;
		remotepath = value.remotepath;
		username = value.username;
		password = value.password;
		allowmount = value.allowmount;
		hostname = value.hostname;
		invalidate();
		return *this;
	}

	CNetworkValues() {
		prime();
	}
};

class TNetworkShares : public app::TModule {
private:
	html::TButton btnMount;
	html::TButton btnUnmount;
	html::TListBox lbxMounts;

	PWebToken wtMountStatusColor;
	PWebToken wtMountStatusCaption;

	app::TMutex componentMtx;
	app::TMutex configMtx;
	app::TMutex mountMtx;
	app::TMutex stateMtx;
	CNetworkValues shares;
	TMountState state;
	TIniFile config;

	std::string jsonNetwork;
	std::string jsonMount;
	bool initialized;
	bool debug;

	void addMountConfig(util::TVariantValues& response);
	void getMountState(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getNetworkConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void onNetworkData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared);

	void setNetworkValues(const CNetworkValues& values);
	void getNetworkValues(CNetworkValues& values);

	void setupListBoxes();
	void setupButtons();

	void setRemoteMountType(const std::string filesystem);
	void setRemoteMountTypeWithNolock(const std::string filesystem);
	bool mountRemoteFilesSystem(const CNetworkValues& values);
	bool mountRemoteFilesSystem();
	bool umountRemoteFilesSystem(const CNetworkValues& values);
	bool umountRemoteFilesSystem();
	void doMountRemoteFilesSystem();
	void doMountRemoteFilesSystem(const CNetworkValues& values);
	void doUnmountRemoteFilesSystem();
	void doUnmountRemoteFilesSystem(const CNetworkValues& values);
	size_t scanStorageMounts(util::TStringList& mounts);
	bool isRemoteFilesSystemMounted(const CNetworkValues& values);
	bool isRemoteFilesSystemMounted();
	void updateNetworkMountButtons(const bool enabled, const EMountState state);
	void updateNetworkMountButtonsWithNolock(const bool enabled, const EMountState state);
	void updateMountStatusLabel(const std::string text, const EMountState state);
	void setMountValues(const EMountState state, const std::string& message);
	void getMountValues(TMountState& state);
	size_t getMountChanged();

	void openConfig(const std::string& configPath);
	void readConfig();
	void writeConfig();
	void reWriteConfig();

	void writeDebug(const std::string& text) const;
	void writeLog(const std::string& text) const;

public:
	int execute();
	void cleanup();

	int prepare();
	void unprepare();

	TNetworkShares();
	virtual ~TNetworkShares();
};

} /* namespace app */

#endif /* APP_NETWORK_H_ */

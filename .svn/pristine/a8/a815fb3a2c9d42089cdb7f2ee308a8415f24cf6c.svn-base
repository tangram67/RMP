/*
 * sysconf.h
 *
 *  Created on: 17.07.2020
 *      Author: dirk
 */

#ifndef APP_SYSCONF_H_
#define APP_SYSCONF_H_

#include "../inc/application.h"
#include "../inc/component.h"

namespace app {

class TSystemConfiguration : public app::TModule {
	html::TListBox lbxTimer;
	html::TListBox lbxInterval;
	html::TListBox lbxVerbosity;

	app::TMutex configMtx;
	std::string jsonConfig;
	bool debug;

	void setupListBoxes();
	void updateViewState(TWebSettings& config);

	void getSystemConfig(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void onSystemConfigData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);

public:
	int execute();
	void cleanup();

	TSystemConfiguration();
	virtual ~TSystemConfiguration();
};

} /* namespace app */

#endif /* APP_SYSCONF_H_ */

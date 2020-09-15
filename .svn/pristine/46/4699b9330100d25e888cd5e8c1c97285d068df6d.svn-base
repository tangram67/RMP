/*
 * auxiliary.h
 *
 *  Created on: 21.04.2019
 *      Author: dirk
 */

#ifndef APP_AUXILIARY_H_
#define APP_AUXILIARY_H_

#include "../inc/application.h"
#include "../inc/component.h"
#include "../inc/bitmap.h"

namespace app {

STATIC_CONST size_t DEFAULT_HISTORY_DEPTH = 100;
STATIC_CONST util::TColor CL_PROGRESS = { 0x0C, 0x0C, 0x0C, 0x00 };

class TAuxiliary : public app::TModule {
private:
	bool debug;

	size_t c_size;
	size_t c_width;
	size_t c_height;
	size_t c_outline;
	ssize_t c_progress;
	util::TPNG canvas;
	char* image;

	std::string jsonSessions;
	std::string jsonRequests;
	std::string jsonCredentials;
	std::string jsonEditUser;
	std::string chartRequests;

	PWebToken wtRequestsHeader;
	PWebToken wtSessionsHeader;
	PWebToken wtCredentialsHeader;
	PWebToken wtApplicationLog;
	PWebToken wtExceptionLog;
	PWebToken wtWebserverLog;


	html::TContextMenu mnStations;

	void prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared);

	void drawColorGradientFiles();

	ssize_t drawNaturalColors(util::TPNG& canvas, size_t width, size_t height);
	ssize_t drawRainbowColors(util::TPNG& canvas, size_t width, size_t height);
	ssize_t drawTemperatureColors(util::TPNG& canvas, size_t width, size_t height);
	ssize_t drawColorGradient(util::TPNG& canvas, size_t width, size_t height, const util::TColor from, const util::TColor to);
	ssize_t drawProgressBar(util::TPNG& canvas, size_t width, size_t height, size_t outline, size_t progress, const util::TColor& color, const bool dryRun = false);
	ssize_t drawRectangle(util::TPNG& canvas, size_t width, size_t height, size_t outline, const util::TColor& color);

	std::string sessionsAsJSON(size_t index, size_t count);
	std::string requestsAsJSON(size_t index, size_t count);
	std::string requestsAsChart(size_t index, size_t count);

	void getPercentBitmap(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getProgresssBitmap(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSessions(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRequests(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getRequestsChart(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getCredentials(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getEditUser(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void onCredentialData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);

	void setupContextMenus();
	void setCredentialsHeader(const size_t count);
	void updateLoggerView();
	void logger(const std::string& text) const;

	void onButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);

public:
	int execute();
	void cleanup();

	TAuxiliary();
	virtual ~TAuxiliary();
};

} /* namespace app */

#endif /* APP_AUXILIARY_H_ */

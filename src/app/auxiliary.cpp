/*
 * auxiliary.cpp
 *
 *  Created on: 21.04.2019
 *      Author: dirk
 */

#include <algorithm>
#include "../inc/globals.h"
#include "../inc/compare.h"
#include "../inc/templates.h"
#include "auxiliary.h"

using namespace std;
using namespace util;

namespace app {

TAuxiliary::TAuxiliary() {
	debug = false;
	image = nil;
	c_size = 0;
	c_width = 0;
	c_height = 0;
	c_outline = 0;
	c_progress = -10;
	wtRequestsHeader = nil;
	wtSessionsHeader = nil;
	wtCredentialsHeader = nil;
	wtApplicationLog = nil;
	wtExceptionLog = nil;
	wtWebserverLog = nil;
}

TAuxiliary::~TAuxiliary() {
	cleanup();
}

int TAuxiliary::execute() {

	// Check if debugging to stdout possible
	if (debug && application.isDaemonized()) {
		debug = false;
	}

	// Test outputs...
	if (debug || application.arguments().hasKey("Q")) {
		drawColorGradientFiles();
	}

	// Add progress color to system colors
	util::addColorByName("progress", CL_PROGRESS);

	// Add web data links
	if (application.hasWebServer()) {

		// Prepare web requests...
		application.addWebPrepareHandler(&app::TAuxiliary::prepareWebRequest, this);

		// Set log file history for web display
		application.getApplicationLogger().setHistoryDepth(DEFAULT_HISTORY_DEPTH);
		application.getExceptionLogger().setHistoryDepth(DEFAULT_HISTORY_DEPTH);
		application.getWebLogger().setHistoryDepth(DEFAULT_HISTORY_DEPTH);

		// Add web token for logfile content
		wtApplicationLog = application.addWebToken("MSG_APPLICATION_LOG", "-");
		wtExceptionLog   = application.addWebToken("MSG_EXCEPTION_LOG", "-");
		wtWebserverLog   = application.addWebToken("MSG_WEBSERVER_LOG", "-");

		// Add heder text as web token content
		wtRequestsHeader = application.addWebToken("REQUESTS_HEADER", "Server workload");
		wtSessionsHeader = application.addWebToken("SESSIONS_HEADER", "User sessions");
		wtCredentialsHeader = application.addWebToken("CREDENTIALS_HEADER", "User credentials");

		// Progress bars
		application.addWebLink("percent.png", &app::TAuxiliary::getPercentBitmap, this, false, false);
		application.addWebLink("progress.png", &app::TAuxiliary::getProgresssBitmap, this, false, false);

		// Internal web server lists
		application.addWebLink("sessions.json", &app::TAuxiliary::getSessions, this, true);
		application.addWebLink("requests.json", &app::TAuxiliary::getRequests, this, true);
		application.addWebLink("requests-chart.json", &app::TAuxiliary::getRequestsChart, this, true);

		// User credential management
		application.addWebLink("credentials.json", &app::TAuxiliary::getCredentials, this, true);
		application.addWebLink("edit-user.json", &app::TAuxiliary::getEditUser, this);

		// Add web actions
		application.addWebAction("OnCredentialData", &app::TAuxiliary::onCredentialData, this, WAM_SYNC);
		application.addWebAction("OnLoggerButtonClick", &app::TAuxiliary::onButtonClick, this, WAM_SYNC);

		setupContextMenus();
		updateLoggerView();
	}

	// Leave after initialization
	return EXIT_SUCCESS;
}

void TAuxiliary::cleanup() {
	canvas.clear();
	if (util::assigned(image))
		delete[] image;
	image = nil;
}

void TAuxiliary::logger(const std::string& text) const {
	application.getApplicationLogger().write(text);
}

void TAuxiliary::setupContextMenus() {

	// Add user account context menu
	mnStations.setTitle("Manage user accounts...");
	mnStations.setID("credentials-context-menu");
	mnStations.setName("CREDENTIALS_CONTEXT_MENU");
	mnStations.setOwner(sysdat.obj.webServer);
	mnStations.setStyle(html::ECS_HTML);
	mnStations.addItem("EDITUSER", "Edit user", 3, "glyphicon-pencil");
	mnStations.addItem("CREATEUSER", "New user", 3, "glyphicon-plus");
	mnStations.addSeparator();
	mnStations.addItem("DELETEUSER", "Remove user", 3,  "glyphicon-trash");
	mnStations.update();
	if (debug) {
		aout << "User account context menu:" << endl;
		aout << mnStations.html() << endl << endl;
	}
}

void TAuxiliary::prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared) {
	bool found = false;
	std::string title = util::tolower(query["title"].asString());

	// Update log file view
	if (!found && title == "logger") {
		updateLoggerView();
		found = true;
	}

	if (!found && util::strcasestr(uri, "requests.html")) {
		size_t count = application.getWebServer().getWebApiCount();
		if (count > 0) {
			std::string text = count > 1 ? "APIs" : "API";
			*wtRequestsHeader = util::csnprintf("Server workload (% active %)", count, text);
		} else {
			*wtRequestsHeader = "Server workload";
		}
		wtRequestsHeader->invalidate();
		found = true;
	}

	if (!found && util::strcasestr(uri, "sessions.html")) {
		size_t count = application.getWebServer().getWebSessionCount();
		if (count > 0) {
			std::string text = count > 1 ? "sessions" : "session";
			*wtSessionsHeader = util::csnprintf("User sessions (% %)", count, text);
		} else {
			*wtSessionsHeader = "User sessions";
		}
		wtSessionsHeader->invalidate();
		found = true;
	}

	if (!found && util::strcasestr(uri, "credentials.html")) {
		size_t count = application.getCredentials().size();
		setCredentialsHeader(count);
		found = true;
	}
}

void TAuxiliary::onButtonClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	logger("[Event] [Button] [" + key + "] = [" + value + "]");
	bool found = false;
	if (!found && util::strcasestr(value, "LOADLOG")) {
		updateLoggerView();
		found = true;
	}
}


void TAuxiliary::updateLoggerView() {
	if (application.hasWebServer()) {
		*wtApplicationLog = application.getApplicationLogger().getHistory().text('\n');
		*wtExceptionLog = application.getExceptionLogger().getHistory().text('\n');
		*wtWebserverLog = application.getWebLogger().getHistory().text('\n');
		wtApplicationLog->invalidate();
		wtExceptionLog->invalidate();
		wtWebserverLog->invalidate();
	}
}


void TAuxiliary::setCredentialsHeader(const size_t count) {
	if (count > 0) {
		std::string text = count > 1 ? "users" : "users";
		*wtCredentialsHeader = util::csnprintf("User credentials (% %)", count, text);
	} else {
		*wtCredentialsHeader = "User credentials";
	}
	wtCredentialsHeader->invalidate();
}


void TAuxiliary::getSessions(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;
	jsonSessions = sessionsAsJSON(index, count);
	if (jsonSessions.empty())
		jsonSessions = JSON_EMPTY_TABLE;
	if (!jsonSessions.empty()) {
		data = jsonSessions.c_str();
		size = jsonSessions.size();
	}
}

void TAuxiliary::getRequests(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;
	jsonRequests = requestsAsJSON(index, count);
	if (jsonRequests.empty())
		jsonRequests = JSON_EMPTY_TABLE;
	if (!jsonRequests.empty()) {
		data = jsonRequests.c_str();
		size = jsonRequests.size();
	}
}


void TAuxiliary::getRequestsChart(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	size_t count = params["limit"].asInteger64(15);
	chartRequests = requestsAsChart(0, count);
	if (chartRequests.empty())
		chartRequests = JSON_EMPTY_TABLE;
	if (!chartRequests.empty()) {
		data = chartRequests.c_str();
		size = chartRequests.size();
	}
}


void TAuxiliary::getCredentials(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	std::string filter = params["search"].asString();

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;
	jsonCredentials = application.getCredentials().asJSON(filter);
	if (jsonCredentials.empty())
		jsonCredentials = JSON_EMPTY_TABLE;
	if (!jsonCredentials.empty()) {
		data = jsonCredentials.c_str();
		size = jsonCredentials.size();
	}
}

void TAuxiliary::getEditUser(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	jsonEditUser.clear();

	// Get username from parameters
	std::string username = params["value"].asString();

	// Get user credentials
	TCredential credential;
	if (application.getCredentials().getUserCredentials(username, credential)) {
		util::TJsonList json;
		json.open(EJT_OBJECT);
		json.add("Givenname", credential.givenname);
		json.add("Lastname", credential.lastname);
		json.add("Username", credential.username);
		json.add("Password", credential.password);
		json.add("Priviledge", TCredentials::getPriviledge(credential.level), EJE_LAST);
		json.close();
		jsonEditUser = json.text();
	}
	if (jsonEditUser.empty())
		jsonEditUser = JSON_EMPTY_TABLE;
	if (!jsonEditUser.empty()) {
		data = jsonEditUser.c_str();
		size = jsonEditUser.size();
	}
}

void TAuxiliary::onCredentialData(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	bool found = false;
	bool modified = false;
	logger(util::csnprintf("[Event] User credentials for [%] = [%]", key, value));

	//	"OnCredentialData":"EDITUSER",
	//	"prepare":"no",
	//	"value":"admin",
	//	"Givenname":"Default",
	//	"Lastname":"admin",
	//	"Username":"admin",
	//	"Password":"!\"§$%&/()=?",
	//	"Priviledge":"Administrator"
	std::string username = params["Username"].asString();

	// Create a new user credential
	if (!found && 0 == util::strcasecmp("CREATEUSER", value)) {
		TCredential credential;
		credential.username = username;
		credential.givenname = params["Givenname"].asString();
		credential.lastname = params["Lastname"].asString();
		credential.password = params["Password"].asString();
		credential.level = TCredentials::getUserLevel(params["Priviledge"].asString());
		if (credential.check()) {
			if (application.getCredentials().insert(credential)) {
				logger(util::csnprintf("[Credentials] New user $ created.", username));
				modified = true;
			} else {
				logger(util::csnprintf("[Credentials] Create new user $ failed.", username));
				error = WSC_InternalServerError;
}
		} else {
			logger(util::csnprintf("[Credentials] Invalid credentials for user $", username));
			error = WSC_NotAcceptable;
		}
		found = true;
	}

	// Edit user credential
	if (!found && 0 == util::strcasecmp("EDITUSER", value)) {
		TCredential credential;
		if (application.getCredentials().getUserCredentials(username, credential)) {
			credential.username = username;
			credential.givenname = params["Givenname"].asString();
			credential.lastname = params["Lastname"].asString();
			credential.password = params["Password"].asString();
			credential.level = TCredentials::getUserLevel(params["Priviledge"].asString());
			if (credential.check()) {
				if (application.getCredentials().modify(credential)) {
					logger(util::csnprintf("[Credentials] User $ updated.", username));
					modified = true;
				} else {
					logger(util::csnprintf("[Credentials] Update user $ failed.", username));
					error = WSC_InternalServerError;
				}
			} else {
				logger(util::csnprintf("[Credentials] Invalid update credentials for user $", username));
				error = WSC_NotAcceptable;
			}
		} else {
			logger(util::csnprintf("[Credentials] Getting update credentials for user $ failed.", username));
			error = WSC_NotFound;
		}
		found = true;
	}

	// Delete user credential
	if (!found && 0 == util::strcasecmp("DELETEUSER", value)) {
		if (0 != util::strcasecmp("admin", username)) {
			if (application.getCredentials().remove(username)) {
				logger(util::csnprintf("[Credentials] User $ deleted.", username));
				modified = true;
			} else {
				logger(util::csnprintf("[Credentials] Delete user $ failed.", username));
				error = WSC_InternalServerError;
			}
		} else {
			logger(util::csnprintf("[Credentials] Cannot delete user $", username));
			error = WSC_NotAcceptable;
		}
		found = true;
	}

	// Delete user credential
	if (!found && 0 == util::strcasecmp("REVERTUSER", value)) {
		found = true;
	}

	// Logoff current user session
	if (!found && 0 == util::strcasecmp("LOGONUSER", value)) {
		std::string sid = session["SESSION_ID"].asString();
		if (application.getWebServer().logoffSessionUser(sid)) {
			logger(util::csnprintf("[Credentials] Session $ logged off.", sid));
		} else {
			logger(util::csnprintf("[Credentials] Logging off session $ failed.", sid));
		}
		found = true;
	}

	// Update webserver credential list
	if (modified) {
		TCredentialMap users;
		application.getCredentials().getCredentialMap(users);
		application.getWebServer().assignUserCredentials(users);
		size_t count = users.size();
		setCredentialsHeader(count);
	}

}


std::string TAuxiliary::sessionsAsJSON(size_t index, size_t count) {
	util::TJsonList json;
	app::TWebSessionInfoList sessions;

	if (application.hasWebServer() && count > 0) {
		application.getWebServer().getWebSessionInfoList(sessions);
		if (!sessions.empty() && index < sessions.size()) {

			// Begin new JSON object
			json.add("{");

			// Begin new JSON array
			json.add("\"total\": " + std::to_string((size_u)sessions.size()) + ",");
			json.add("\"rows\": [");

			// Add JSON object with trailing separator
			PWebSessionInfo o;
			size_t idx = index;
			size_t end = index + count;
			size_t last = index + count - 1;
			if (last >= sessions.size())
				last = sessions.size() - 1;

			for(; idx<end; ++idx) {
				if (idx >= sessions.size())
					break;
				o = sessions[idx];
				if (util::assigned(o)) {
					std::string state = (o->authenticated || !(HAT_DIGEST_NONE != application.getWebServer().getDigestType())) ? "Valid" : "Rejected";
					json.open(EJT_OBJECT);
					json.add("SID", o->sid);
					json.add("Address", o->remote);
					json.add("Browser", app::userAgentToStr(o->userAgent));
					json.add("Datetime", util::ISO8601DateTimeToStr(o->timestamp));
					json.add("Requested", std::to_string((size_u)o->useC));
					json.add("Username", o->username);
					json.add("State", state, EJE_LAST);
					json.close((idx < last) ? EJE_LIST : EJE_LAST);
				}
			}

			// Close JSON array and object
			json.add("]}");
		}
	}

	return json.text();
}


std::string TAuxiliary::requestsAsJSON(size_t index, size_t count) {
	util::TJsonList json;
	TWebRequestInfoList requests;

	if (application.hasWebServer() && count > 0) {
		size_t requested;
		application.getWebServer().getWebRequestInfoList(requests, requested);
		if (!requests.empty() && index < requests.size()) {

			// Begin new JSON object
			json.add("{");

			// Begin new JSON array
			json.add("\"total\": " + std::to_string((size_u)requests.size()) + ",");
			json.add("\"rows\": [");

			// Add JSON object with trailing separator
			PWebRequestInfo o;
			size_t idx = index;
			size_t end = index + count;
			size_t last = index + count - 1;
			if (last >= requests.size())
				last = requests.size() - 1;

			for(; idx<end; ++idx) {
				if (idx >= requests.size())
					break;
				o = requests[idx];
				if (util::assigned(o)) {
					json.open(EJT_OBJECT);
					json.add("Type", o->type);
					json.add("URL", o->url);
					json.add("Percent", std::to_string((size_u)o->percent));
					json.add("Requested", std::to_string((size_u)o->requested));
					json.add("Datetime", util::ISO8601DateTimeToStr(o->timestamp));
					json.add("Total", std::to_string((size_u)requested), EJE_LAST);
					json.close((idx < last) ? EJE_LIST : EJE_LAST);
				}
			}

			// Close JSON array and object
			json.add("]}");
		}
	}

	return json.text();
}


inline int isPathWhiteSpace(char c) {
	unsigned char u = (unsigned char)c;
	if (u == (unsigned char)'/')
		return true;
	if (u == (unsigned char)'\\')
		return true;
	return (u <= USPC);
}

std::string TAuxiliary::requestsAsChart(size_t index, size_t count) {
	util::TJsonList json;
	TWebRequestInfoList requests;

	if (application.hasWebServer() && count > 0) {
		size_t requested;
		application.getWebServer().getWebRequestInfoList(requests, requested, 2);
		if (!requests.empty() && index < requests.size()) {

			// Begin new JSON object
			json.add("{");
			json.add("\"rows\": [");

			// Add JSON object with trailing separator
			PWebRequestInfo o;
			size_t idx = index;
			size_t end = index + count;
			size_t last = index + count - 1;
			if (last >= requests.size())
				last = requests.size() - 1;

			for(; idx<end; ++idx) {
				if (idx >= requests.size())
					break;
				o = requests[idx];
				if (util::assigned(o)) {
					std::string url = o->url.size() > 1 ? util::fileExtName(util::trimRight(o->url, isPathWhiteSpace)) : o->url;
					json.open(EJT_OBJECT);
					json.add("URL", url);
					json.add("Requested", std::to_string((size_u)o->requested));
					json.add("Datetime", util::ISO8601DateTimeToStr(o->timestamp), EJE_LAST);
					json.close((idx < last) ? EJE_LIST : EJE_LAST);
				}
			}

			// Close JSON array and object
			json.add("]}");
		}
	}

	return json.text();
}


void TAuxiliary::getProgresssBitmap(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	size_t encoded = 0;

	// Get parameter for progress bar
	size_t progress = params["progress"].asInteger(0);
	size_t length   = params["size"].asInteger(4);
	size_t height   = params["height"].asInteger(6);
	size_t width    = params["width"].asInteger(length < 5 ? 28 : 36);
	size_t outline  = params["outline"].asInteger(1);
	std::string cl  = params["color"].asString("progress");

	// Get color by name
	TColor col, color = CL_PROGRESS;
	if (util::getColorByName(cl, col)) {
		color = col;
	}

	// Check last values
	ssize_t n_progress = drawProgressBar(canvas, width, height, outline, progress, color, true);
	if (util::assigned(image) && c_width == width && c_height == height && c_outline == outline && c_progress == (ssize_t)n_progress) {
		size = c_size;
		data = image;
		if (debug) {
			aout << "TAuxiliary::getProgresssBitmap() Return same PNG data for progress = " << progress << "%" << endl;
		}
		return;
	}

	// Set compare values
	c_width = width;
	c_height = height;
	c_outline = outline;

	// Get progressbar data
	c_progress = drawProgressBar(canvas, width, height, outline, progress, color);
	canvas.encode(image, encoded);
	if (util::assigned(image) && encoded > 0) {
		data = image;
		c_size = size = encoded;
	}

	if (debug) {
		aout << "TAuxiliary::getProgresssBitmap() Encoded " << encoded << " bytes of PNG data for progress = " << progress << "%" << endl;
	}
}

void TAuxiliary::getPercentBitmap(app::TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	size_t encoded = 0;
	util::TPNG canvas;
	char* image = nil;

	// Get parameter for progress bar
	size_t progress = params["percent"].asInteger(0);
	size_t length   = params["size"].asInteger(4);
	size_t height   = params["height"].asInteger(6);
	size_t width    = params["width"].asInteger(length < 5 ? 28 : 36);
	size_t outline  = params["outline"].asInteger(1);
	std::string cl  = params["color"].asString("progress");

	// Get color by name
	TColor col, color = CL_PROGRESS;
	if (util::getColorByName(cl, col)) {
		color = col;
	}

	// Get progressbar data
	if (drawProgressBar(canvas, width, height, outline, progress, color) > 0) {
		canvas.encode(image, encoded);
		if (util::assigned(image) && encoded > 0) {
			data = image;
			size = encoded;
		}
	}

	if (debug) {
		aout << "TAuxiliary::getPercentBitmap() Encoded " << encoded << " bytes of PNG data for progress = " << progress << "%" << endl;
	}
}

ssize_t TAuxiliary::drawProgressBar(util::TPNG& canvas, size_t width, size_t height, size_t outline, size_t progress, const util::TColor& color, const bool dryRun) {

	// Check ranges
	if (width <= 0 || height <= 0)
		return (ssize_t)-1;

	// Normalize values
	outline = std::max(outline, (size_t)1);
	progress = std::min(std::max(progress, (size_t)0), (size_t)100);
	size_t limit = std::max(std::min(height, width) / (size_t)2, (size_t)1);
	size_t yOutline = std::min(width / (size_t)2, outline);
	size_t xOutline = std::min(height / (size_t)2, outline);

	size_t xLast = util::pred(width);
	size_t yLast = util::pred(height);
	size_t yBorder = height - yOutline;
	size_t xBorder = width - xOutline;

	// Calculate progress value
	size_t xArea = xLast - (2 * xOutline);
	size_t xProgress = xOutline + (xArea * progress / 100);
	if (dryRun) {
		return (ssize_t)xProgress;
	}

	// Create bitmap of requested size
	canvas.fill(width, height, CL_WHITE);

	// Draw upper border
	for (size_t y=0; y<outline && y<limit; ++y) {
		canvas.line(0,y, xLast,y, color);
	}
	// Draw lower border
	for (size_t y=yLast; y>=yBorder && y>limit; --y) {
		canvas.line(0,y, xLast,y, color);
	}

	// Draw left border
	for (size_t x=0; x<outline && x<limit; ++x) {
		canvas.line(x,0, x,yLast, color);
	}
	// Draw right border
	for (size_t x=xLast; x>=xBorder && x>limit; --x) {
		canvas.line(x,0, x,yLast, color);
	}

	// Draw progess bar
	if (progress > 0) {
		for (size_t y=0; y<height; ++y) {
			if (y >= yOutline && y < yBorder) {
				canvas.line(xOutline,y, xProgress,y, color);
			}
		}
	}

	return (ssize_t)xProgress;
}

ssize_t TAuxiliary::drawRectangle(util::TPNG& canvas, size_t width, size_t height, size_t outline, const util::TColor& color) {

	// Check ranges
	if (width <= 0 || height <= 0)
		return (ssize_t)-1;

	// Normalize values
	outline = std::max(outline, (size_t)1);
	size_t limit = std::max(std::min(height, width) / (size_t)2, (size_t)1);
	size_t yOutline = std::min(width / (size_t)2, outline);
	size_t xOutline = std::min(height / (size_t)2, outline);

	size_t xLast = util::pred(width);
	size_t yLast = util::pred(height);
	size_t yBorder = height - yOutline;
	size_t xBorder = width - xOutline;

	// Create bitmap of requested size
	canvas.fill(width, height, CL_WHITE);

	// Draw upper border
	for (size_t y=0; y<outline && y<limit; ++y) {
		canvas.line(0,y, xLast,y, color);
	}
	// Draw lower border
	for (size_t y=yLast; y>=yBorder && y>limit; --y) {
		canvas.line(0,y, xLast,y, color);
	}

	// Draw left border
	for (size_t x=0; x<outline && x<limit; ++x) {
		canvas.line(x,0, x,yLast, color);
	}
	// Draw right border
	for (size_t x=xLast; x>=xBorder && x>limit; --x) {
		canvas.line(x,0, x,yLast, color);
	}

	return (ssize_t)(width * height);
}

ssize_t TAuxiliary::drawColorGradient(util::TPNG& canvas, size_t width, size_t height, const util::TColor from, const util::TColor to) {

	// Check ranges
	if (width <= 0 || height <= 0)
		return (ssize_t)-1;

	// Create bitmap of requested size
	canvas.fill(width, height, CL_NONE);

	// Draw color gradient on x axis (width)
	size_t yLast = util::pred(height);
	for (size_t x=0; x<width; ++x) {
		double percent = (double)x * 100.0 / (double)width;
		util::TColor color = canvas.gradient(percent, from, to);
		canvas.line(x,0, x,yLast, color);
	}

	return (ssize_t)(width * height);
}

ssize_t TAuxiliary::drawTemperatureColors(util::TPNG& canvas, size_t width, size_t height) {

	// Check ranges
	if (width <= 0 || height <= 0)
		return (ssize_t)-1;

	// Create bitmap of requested size
	canvas.fill(width, height, CL_NONE);

	// Draw temperature colors on x axis (width)
	size_t yLast = util::pred(height);
	const CColorSystem* system = canvas.getColorSystem("REC709");
	for (size_t x=0; x<width; ++x) {
		double percent = (double)x * 100.0 / (double)width;
		util::TColor color = canvas.temperature(percent, 1000.0, 10000.0, system);
		canvas.line(x,0, x,yLast, color);
	}

	return (ssize_t)(width * height);
}

ssize_t TAuxiliary::drawRainbowColors(util::TPNG& canvas, size_t width, size_t height) {

	// Check ranges
	if (width <= 0 || height <= 0)
		return (ssize_t)-1;

	// Create bitmap of requested size
	canvas.fill(width, height, CL_NONE);

	// Draw rainbow colors on x axis (width)
	size_t yLast = util::pred(height);
	for (size_t x=0; x<width; ++x) {
		double percent = (double)x * 100.0 / (double)width;
		util::TColor color = canvas.rainbow(percent);
		canvas.line(x,0, x,yLast, color);
	}

	return (ssize_t)(width * height);
}

ssize_t TAuxiliary::drawNaturalColors(util::TPNG& canvas, size_t width, size_t height) {

	// Check ranges
	if (width <= 0 || height <= 0)
		return (ssize_t)-1;

	// Create bitmap of requested size
	canvas.fill(width, height, CL_NONE);

	// Draw rainbow colors on x axis (width)
	size_t yLast = util::pred(height);
	for (size_t x=0; x<width; ++x) {
		double percent = (double)x * 100.0 / (double)width;
		util::TColor color = canvas.colors(percent);
		canvas.line(x,0, x,yLast, color);
	}

	return (ssize_t)(width * height);
}


void TAuxiliary::drawColorGradientFiles() {
	drawRainbowColors(canvas, 576, 35);
	canvas.saveToFile(application.getTempFolder() + "rainbow-1.png");
	drawRainbowColors(canvas, 576, 5);
	canvas.saveToFile("rainbow-2.png");

	drawNaturalColors(canvas, 576, 35);
	canvas.saveToFile(application.getTempFolder() + "natural-1.png");
	drawNaturalColors(canvas, 576, 5);
	canvas.saveToFile(application.getTempFolder() + "natural-2.png");

	drawTemperatureColors(canvas, 576, 35);
	canvas.saveToFile(application.getTempFolder() + "temperature-1.png");
	drawTemperatureColors(canvas, 576, 5);
	canvas.saveToFile(application.getTempFolder() + "temperature-2.png");

	drawColorGradient(canvas, 576, 35, CL_WHITE, CL_BLACK);
	canvas.saveToFile(application.getTempFolder() + "gradient-bw.png");
	drawColorGradient(canvas, 576, 35, CL_GREEN, CL_RED);
	canvas.saveToFile(application.getTempFolder() + "gradient-co.png");

	drawRectangle(canvas, 48, 48, 1, CL_GRAY);
	canvas.saveToFile(application.getTempFolder() + "rectangular-48.png");

	drawRectangle(canvas, 200, 200, 1, CL_GRAY);
	canvas.saveToFile(application.getTempFolder() + "rectangular-20.png");

	for (ssize_t progress = 100; progress >= 0; progress -= 10) {
		std::string fileName = application.getTempFolder() + "progress-" + std::to_string(progress) + ".png";
		drawProgressBar(canvas, 576, 35, 4, progress, CL_PRIMARY);
		size_t encoded = canvas.saveToFile(fileName);
		if (debug)
			std::cout << "TAuxiliary::execute() Write " << encoded << " bytes of PNG data to file \"" << fileName << "\" Ratio = 1 : " << 576 * 35 * 3 / encoded << endl;
	}
}

} /* namespace app */

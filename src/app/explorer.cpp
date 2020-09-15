/*
 * explorer.cpp
 *
 *  Created on: 20.10.2017
 *      Author: Dirk Brinkmeier
 */

#include <stdlib.h>
#include <fnmatch.h>
#include "../inc/templates.h"
#include "../inc/mimetypes.h"
#include "../inc/datetime.h"
#include "../inc/encoding.h"
#include "../inc/convert.h"
#include "../inc/compare.h"
#include "../inc/globals.h"
#include "../inc/json.h"
#include "../config.h"
#include "explorer.h"

namespace app {

TExplorer::TExplorer() {
	jail = true;
	debug = false;
	useHTML5 = true;
	wtExplorerImages = nil;
	wtExplorerHeaderText = nil;
	wtExplorerHeaderPath = nil;
	wtExplorerPathButtons = nil;
	wtExplorerAudioElements = nil;
}

TExplorer::~TExplorer() {
	util::clearObjectList(parts);
	util::clearObjectList(items);
}


int TExplorer::execute() {
	explore(application.getWorkingFolder());

	// Add actions and data links to webserver instance
	if (application.hasWebServer()) {

		// Read configuration file
		openConfig(application.getConfigFolder());
		reWriteConfig();

		// Add prepare handler to catch URI parameters...
		application.addWebPrepareHandler(&app::TExplorer::prepareWebRequest, this);
		application.addFileUploadEventHandler(&app::TExplorer::onFileUploaded, this);

		// Add named web actions
		application.addWebAction("OnExplorerAction", &app::TExplorer::onExplorerAction, this, WAM_SYNC);
		application.addWebAction("OnExplorerClick",  &app::TExplorer::onExplorerClick,  this, WAM_SYNC);

		// Add virtual data URLs to webserver instance
		application.addWebLink("explorer.json",  &app::TExplorer::getExplorerContent, this, true);

		// Add webtoken to webserver instance
		wtExplorerHeaderText = application.addWebToken("EXPLORER_HEADER_TEXT", "Explore \"" + getPath() + "\"");
		wtExplorerHeaderPath = application.addWebToken("EXPLORER_HEADER_PATH", getPath());
		wtExplorerPathButtons = application.addWebToken("EXPLORER_PATH_BUTTONS", asButtons());
		wtExplorerAudioElements = application.addWebToken("EXPLORER_AUDIO_ELEMENTS", asAudioElements());
		wtExplorerImages = application.addWebToken("EXPLORER_IMAGES", asImages());

		// Get configured web root to "jail" navigation
		explorerRootPath = application.getWorkingFolder();
	}

	// Leave file explorer after initialization
	return EXIT_SUCCESS;
}

void TExplorer::cleanup() {
	// Not needed here...
}


void TExplorer::writeDebug(const std::string& text) const {
	if (debug) {
		application.writeLog(text);
	}
}

void TExplorer::writeLog(const std::string& text) const {
	application.writeLog(text);
}


void TExplorer::openConfig(const std::string& configPath) {
	std::string path = util::validPath(configPath);
	std::string file = path + "explorer.conf";
	config.open(file);
	reWriteConfig();
}

void TExplorer::readConfig() {
	config.setSection("Explorer");
	debug = config.readBool("Debug", debug);
	jail = config.readBool("JailUserToWorkingDir", jail);
	jail = config.readBool("JailUserToEnvironment", jail);
}

void TExplorer::writeConfig() {
	config.setSection("Explorer");
	config.writeBool("Debug", debug, app::INI_BLYES);
	config.writeBool("JailUserToEnvironment", jail, app::INI_BLYES);
	config.deleteKey("JailUserToWorkingDir");

	// Save changes to disk
	config.flush();
}

void TExplorer::reWriteConfig() {
	readConfig();
	writeConfig();
}


void TExplorer::getExplorerContent(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {
	int64_t id = params["_"].asInteger64(0);
	size_t count = params["limit"].asInteger64(0);
	size_t index = params["offset"].asInteger64(0);
	std::string filter = params["search"].asString();

	// Check for valid bootstrap table request
	if (id < util::BOOTSTRAP_TABLE_INDEX)
		return;

	// Get file content as JSON
	content = getJSON(filter, index, count);
	writeDebug(util::csnprintf("[Request] TExplorer::getExplorerContent() Get JSON content for Filter: $, Index: %, Count: %", filter, index, count));

	// Return JSON data
	if (content.empty())
		content = util::JSON_EMPTY_TABLE;
	if (!content.empty()) {
		data = content.c_str();
		size = content.size();
	}
}

void TExplorer::prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared) {
	bool found = false;

	// Prepare explorer table request
	if (util::strcasestr(uri, "explorer.json") && query["_"].asInteger64(0) >= util::BOOTSTRAP_TABLE_INDEX) {

		// Get last path from given session
		std::string current = session["HTML_CURRENT_EXPLORER_PATH"].asString();
		if (current.empty())
			current = application.getWorkingFolder();

		// Reread folder content for current path
		current = explore(current);

		// Store current path in session
		writeDebug("[Prepare] TExplorer::prepareWebRequest() Explore content for \"" + current + "\"");
		session.add("HTML_CURRENT_EXPLORER_PATH", current);
		found = prepared = true;
	}

	// Prepare HTML page content
	if (!found && query["prepare"].asString() == "yes") {

			// Get page title from URI parameters
		std::string title = util::tolower(query["title"].asString());

		// Set explorer view items
		if (!found && title == "explorer") {

			// Get path from given session or from URI parameter list
			std::string current = session["HTML_CURRENT_EXPLORER_PATH"].asString();
 			std::string root = query["path"].asString();
			if (!root.empty()) {
				// Use given URI path parameter
				current = setPath(root);
			}
			if (current.empty())
				current = application.getWorkingFolder();

			// Reread folder content for current path
			current = explore(current);

			// Store current path in session
			writeDebug("[Prepare] TExplorer::prepareWebRequest() Set HTML properties for \"" + current + "\"");
			session.add("HTML_CURRENT_EXPLORER_PATH", current);
			found = prepared = true;
		}

		// Set browser properties
		if (!found && title == "browse") {

			// Set given path
			std::string current = query["path"].asString();
			if (util::fileExists(current)) {

				// Set web token to current path and reread folder content
				current = explore(setPath(current));

				// Store changed path in session
				writeDebug("[Prepare] TExplorer::prepareWebRequest() Set browser path to \"" + current + "\"");
				session.add("HTML_CURRENT_EXPLORER_PATH", current);
				found = prepared = true;
			}
		}

	}
}

void TExplorer::onFileUploaded(const app::TWebServer& sender, const util::TVariantValues& session, const std::string& fileName, const size_t& size) {
	std::string ext = util::fileExt(fileName);
	std::string path = session["HTML_CURRENT_EXPLORER_PATH"].asString();
	if (ext != "lic") {
		application.writeLog("[Event] [Uploaded] File \"" + fileName + "\" uploaded (" + util::sizeToStr(size) + ") for current folder \"" + path + "\"");
	}
}

void TExplorer::updatePathToken() {
	if (util::assigned(wtExplorerHeaderPath)) {
		*wtExplorerHeaderPath = path;
		*wtExplorerHeaderText = "Explore \"" + path + "\"";
		*wtExplorerPathButtons = asButtons();
		wtExplorerHeaderPath->invalidate();
		wtExplorerHeaderText->invalidate();
		wtExplorerPathButtons->invalidate();
	}
}

void TExplorer::updateContentToken() {
	if (util::assigned(wtExplorerImages) && util::assigned(wtExplorerAudioElements)) {
		*wtExplorerImages = asImages();
		*wtExplorerAudioElements = asAudioElements();
		wtExplorerImages->invalidate();
		wtExplorerAudioElements->invalidate();
	}
}

void TExplorer::onExplorerClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	application.writeLog("[Event] [Explorer] Row clicked for key = \"" + key + "\", value = \"" + value + "\"");
}

void TExplorer::onExplorerAction(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error) {
	bool found = false;
	writeLog("[Event] [Explorer] Action called for key = \"" + key + "\", value = \"" + value + "\"");

	std::string source = util::TURL::decode(params["source"].asString());
	std::string destination = util::TURL::decode(params["destination"].asString());
	std::string path = util::validPath(session["HTML_CURRENT_EXPLORER_PATH"].asString());

	// Execute requested file operation
	if (!found && 0 == util::strcasecmp("DELETEFILE", value)) {
		std::string file = path + source;
		writeLog("[Action] Delete file \"" + file + "\"");
		error = WSC_NotFound;
		found = true;
	}
	if (!found && 0 == util::strcasecmp("DELETEFOLDER", value)) {
		std::string folder = path + source;
		writeLog("[Action] Delete Folder \"" + folder + "\"");
		error = WSC_NotFound;
		found = true;
	}
	if (!found && 0 == util::strcasecmp("RENAMEFILE", value)) {
		std::string from = path + source;
		std::string to = path + destination;
		writeLog("[Action] Rename file \"" + from + "\" --> \"" + to + "\"");
		error = WSC_NotFound;
		found = true;
	}
	if (!found && 0 == util::strcasecmp("RENAMEFOLDER", value)) {
		std::string from = util::validPath(path + source);
		std::string to = util::validPath(path + destination);
		writeLog("[Action] Rename folder \"" + from + "\" --> \"" + to + "\"");
		error = WSC_NotFound;
		found = true;
	}

	// Fallback
	if (!found) {
		error = WSC_NotImplemented;
	}
}


int TExplorer::readDirektory(const std::string& path, const app::TStringVector& patterns, const bool recursive, const bool hidden) {
	util::TDirectory dir;
	struct dirent *file;
	std::string name;
	std::string pattern;
	std::string root = validPath(path);
	util::TStringList folders;
	size_t size;
	int r = 0;

	if (!recursive)
		clear();

	dir.open(root);
	if (dir.isOpen()) {
		while(util::assigned((file = readdir(dir())))) {
			size = strnlen(file->d_name, 255);
			if (size <= 0 || size >= 255)
				continue;

			// Get filename
			name = std::string(file->d_name, size);

			// Ignore hiden and special files
			if (name.size() == 1 && name[0] == '.')
				continue;
			if (name.size() == 2 && name == "..")
				continue;
			if (!hidden && name.size() > 1 && name[0] == '.')
				continue;

			// Add file to list if pattern matches filename
			if (file->d_type == DT_REG) {
				if (!pattern.empty()) {
					app::TStringVector::const_iterator it = patterns.begin();
					while (it != patterns.end()) {
						// Add matching files only
						pattern = *it;
						if (match(file, pattern)) {
							addInode(root, name, EFT_FILE);
							++r;
						}
						++it;
					}
				} else {
					// Add every file on empty pattern
					addInode(root, name, EFT_FILE);
					++r;
				}
			}

			// Prepare recursive folder scan
			if (file->d_type == DT_DIR) {
				addInode(root, file->d_name, EFT_FOLDER);
				if (recursive)
					folders.add(validPath(root + name));
				++r;
			}
		}
		dir.close();
	}

	// Scan sub folders
	if (!folders.empty()) {
		for (size_t i=0; i<folders.size(); ++i) {
			r += readDirektory(folders[i], patterns, recursive);
		}
	}

	// Sort result set
	sort();
	writeDebug(util::csnprintf("[Debug] TExplorer::readDirektory() Found % items in path $", items.size(), root));
	return r;
}

bool TExplorer::match(struct dirent *file, const std::string& pattern) {
	if (!pattern.empty())
		return (0 == ::fnmatch(pattern.c_str(), file->d_name, FNM_NOESCAPE | FNM_CASEFOLD));
	return false;
}

std::string TExplorer::validPath(const std::string& directoryName) {
	std::string path = directoryName;
	util::trim(path);
	if (!path.empty())
		if (path[0] != sysutil::PATH_SEPERATOR)
			path = sysutil::PATH_SEPERATOR + directoryName;
	if (path.size() >= 1)
		if (path[util::pred(path.size())] != sysutil::PATH_SEPERATOR)
			path += sysutil::PATH_SEPERATOR;
	return path;
}


void TExplorer::addInode(const std::string& path, const std::string& fileName, EFileType type) {
	PExplorerItem o = new TExplorerItem;
	o->type = type;

	// Set file properties
	o->file = path + fileName;
	o->name = fileName;
	o->ext  = util::tolower(util::fileExt(fileName));
	o->sort = util::tolower(o->file);

	// Set file hash value
	util::TDigest MD5(util::EDT_MD5);
	MD5.setFormat(util::ERT_HEX_SHORT);
	o->hash = MD5(o->name);

	// Set type dependent values
	switch (type) {
		case EFT_FILE:
			o->path = path;
			o->mime = util::getMimeType(o->ext);
			o->strType = "File";
			o->regular = util::JSON_TRUE;
			o->folder = util::JSON_FALSE;
			o->isAudio = 0 == util::strncasecmp(o->mime, "audio", 5);
			o->isVideo = 0 == util::strncasecmp(o->mime, "video", 5);
			o->isImage = 0 == util::strncasecmp(o->mime, "image", 5);
			o->image = util::TJsonValue::boolToStr(o->isImage);
			o->audio = util::TJsonValue::boolToStr(o->isAudio);
			o->video = util::TJsonValue::boolToStr(o->isVideo);
			break;
		case EFT_FOLDER:
			o->path = validPath(o->file);
			o->mime = "application/folder";
			o->strType = "Folder";
			o->regular = util::JSON_FALSE;
			o->folder = util::JSON_TRUE;
			o->image = util::JSON_FALSE;
			o->audio = util::JSON_FALSE;
			o->video = util::JSON_FALSE;
			break;
		default:
			o->path = sysutil::PATH_SEPERATOR;
			o->mime = "application/unknown";
			o->strType = "Unknown";
			o->regular = util::JSON_FALSE;
			o->folder = util::JSON_FALSE;
			o->image = util::JSON_FALSE;
			o->audio = util::JSON_FALSE;
			o->video = util::JSON_FALSE;
			break;
	}

	// Store URL encoded values
	o->urlPath = util::TURL::encode(o->path);
	o->urlFile = util::TURL::encode(o->file);
	o->urlName = util::TURL::encode(o->name);
	o->urlExt  = util::TURL::encode(o->ext);

	// Get file properties
	stat(o->file, type, o);
	items.push_back(o);
}

bool TExplorer::stat(const std::string& fileName, const EFileType type, PExplorerItem item) {
    struct stat buf;
    if (util::fileStatus(fileName, &buf) && util::assigned(item)) {
    	item->time = buf.st_mtime;
    	item->utcTime = util::ISO8601DateTimeToStr(item->time);
    	item->localTime = util::dateTimeToStr(item->time);
    	switch (type) {
    		case EFT_FILE:
    	    	item->size = buf.st_size;
    	    	item->strSize = util::sizeToStr(item->size, 1, util::VD_SI);
    			break;
    		case EFT_FOLDER:
    	    	item->size = (size_t)0;
    	    	item->strSize = "Folder";
    			break;
    		default:
    	    	item->size = (size_t)0;
    	    	item->strSize = "n.a.";
    			break;
    	}
    	return true;
    }
	return false;
}


void TExplorer::clear() {
	util::clearObjectList(items);
}


bool inodeItemSorter(PExplorerItem o, PExplorerItem p) {
	// Folders at top position
	if (o->type == EFT_FOLDER && p->type == EFT_FILE)
		return true;
	if (o->type == EFT_FILE && p->type == EFT_FOLDER)
		return false;
	return util::strnatsort(o->sort, p->sort);
}

void TExplorer::sort() {
	std::sort(items.begin(), items.end(), inodeItemSorter);
}


std::string TExplorer::getJSON(const std::string filter, size_t index, size_t count) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return asJSON(filter, index, count);
}

std::string TExplorer::asJSON(const std::string filter, size_t index, size_t count) const {
	util::TJsonList json;

	if (!items.empty()) {
		bool ok;
		PExplorerItem o;

		// Filter items for given filter string
		const TExplorerList* list = &items;
		TExplorerList query;
		if (!filter.empty()) {
			for(size_t i=0; i<items.size(); ++i) {
				o = items[i];
				if (util::assigned(o)) {
					ok = util::strcasestr(o->name, filter);
					if (ok) {
						query.push_back(o);
					}
				}
			}
			list = &query;
		}

		// Begin new JSON object
		json.add("{");

		// Begin new JSON array
		json.add("\"total\": " + std::to_string((size_u)list->size()) + ",");
		json.add("\"rows\": [");

		// Add items as JSON objects
		size_t end = index + count;
		if (end > list->size())
			end = list->size();
		size_t last = util::pred(end);
		for(size_t idx = index; idx<end; idx++) {
			o = list->at(idx);
			if (util::assigned(o)) {
				json.open(util::EJT_OBJECT);
				json.add("Hash", o->hash);
				json.add("File", util::TJsonValue::escape(o->file));
				json.add("Path", util::TJsonValue::escape(o->path));
				json.add("Name", util::TJsonValue::escape(o->name));
				json.add("Ext", util::TJsonValue::escape(o->ext));
				json.add("FileURL", o->urlFile);
				json.add("PathURL", o->urlPath);
				json.add("NameURL", o->urlName);
				json.add("ExtURL", o->urlExt);
				json.add("Mime", o->mime);
				json.add("Size", o->strSize);
				json.add("Time", o->utcTime);
				json.add("Date", o->localTime);
				json.add("Type", o->strType);
				json.append("HTML5", util::TJsonValue::boolToStr(useHTML5));
				json.append("Regular", o->regular);
				json.append("Folder", o->folder);
				json.append("Image", o->image);
				json.append("Audio", o->audio);
				json.append("Video", o->video, util::EJE_LAST);
				json.close(idx == last);
			}
		}

		// Close JSON array and object
		json.add("]}");
	}

	return json.text();
}


std::string TExplorer::getButtons(const bool bold) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return asButtons(bold);
}

std::string TExplorer::asButtons(const bool bold) const {
	util::TStringList html;
	if (!parts.empty()) {
		PPathItem item;
		for (size_t i=0; i<parts.size(); ++i) {
			item = parts[i];
			if (util::assigned(item)) {
				const std::string& path = item->url;
				const std::string& hint = item->path;
				const std::string& caption = item->folder;
				if (bold)
					html.add("    <a folder=\"" + path + "\" title=\"" + hint + "\" onclick=\"onPathButtonClick(event)\" class=\"btn btn-default\"><strong>&nbsp;" + caption + "&nbsp;</strong></a>");
				else
					html.add("    <a folder=\"" + path + "\" title=\"" + hint + "\" onclick=\"onPathButtonClick(event)\" class=\"btn btn-default\">&nbsp;" + caption + "&nbsp;</a>");
			}
		}
	}
	return html.text('\n');
}

std::string TExplorer::getImages(bool collection) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return asImages(collection);
}

std::string TExplorer::asImages(bool collection) const {
	util::TStringList html;
	PExplorerItem o;
	std::string tag;
	for(size_t i=0; i<items.size(); ++i) {
		o = items[i];
		if (util::assigned(o)) {
			if (o->isImage) {
				tag = collection ? "lightbox-collection" : o->hash;
				html.add("<a id=\"" + o->hash + "\" href=\"/fs0" + o->file + "\" data-lightbox=\"" + tag + "\" data-title=\"" + o->path + "<br><i>" + o->name + "</i> (" + o->localTime + ")\"></a>");
			}
		}
	}
	return html.text('\n');
}

std::string TExplorer::getAudioElements(bool collection) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return asAudioElements(collection);
}

std::string TExplorer::asAudioElements(bool collection) const {
	util::TStringList html;
	PExplorerItem o;
	std::string tag, type, codec;
	for(size_t i=0; i<items.size(); ++i) {
		o = items[i];
		if (util::assigned(o)) {
			if (o->isAudio) {
				// Add HTML5 audio element
				// <audio
				//   id="EE01F44DDB0676128650777F50DD1742"
				//   data-media5="EE01F44DDB0676128650777F50DD1742" or data-media5="<placeholder for collection name>"
				//   title-media5="11 - Wallies"
				//   cover-media5="/fs1/musik/Anne Clark/The Best of Anne Clark/folder.jpg"
				//   src="/fs0/musik/Anne Clark/The Best of Anne Clark/11 - Wallies.flac"
				//   preload="none">
				// </audio>
				if (util::getAudioCodec(o->ext, type, codec)) {
					tag = collection ? "media-collection" : o->hash;
					if (codec.empty()) {
						html.add("<audio id=\"" + o->hash + "\" data-media5=\"" + tag + "\" title-media5=\"" + o->name + "\" cover-media5=\"/fs1" + o->path + "folder.jpg\" src=\"/fs0" + o->file + "\" type=\"" + type + "\" preload=\"none\"></audio>");
					} else {
						html.add("<audio id=\"" + o->hash + "\" data-media5=\"" + tag + "\" title-media5=\"" + o->name + "\" cover-media5=\"/fs1" + o->path + "folder.jpg\" src=\"/fs0" + o->file + "\" type=\'" + type + "; codecs=" + codec + "\' preload=\"none\"></audio>");
					}
				}
			}
		}
	}
	return html.text('\n');
}

void TExplorer::assign(const std::string& root) {
	// Jail browser to root path?
	util::TStringList folders;
	if (application.getWorkingFolders(folders)) {
		explorerRootPath = folders[0];
	}
	if (jail && !folders.empty()) {
		bool found = false;
		if (!root.empty()) {
			std::string directory = validPath(root);
			for (size_t i=0; i<folders.size(); ++i) {
				std::string folder = folders[i];
				if (directory.size() >= folder.size()) {
					if (0 == strncmp(directory.c_str(), folder.c_str(), folder.size())) {
						found = true;
					}
				}
				if (found) {
					explorerRootPath = folder;
					path = directory;
					break;
				}
			}
		}
	} else {
		// No folders set to check agains or jail disabled...
		path = root.empty() ? std::string(1, sysutil::PATH_SEPERATOR) : validPath(root);
	}
	parse(path);
}

void TExplorer::parse(const std::string& root) {
	util::clearObjectList(parts);

	// Add default item for filesystem root folder
	PPathItem item;
	item = new TPathItem;
	item->path = sysutil::PATH_SEPERATOR;
	item->folder = item->path;
	item->url = util::TURL::encode(item->path);
	parts.push_back(item);

	// Add current subfolders
	if (!root.empty()) {
		char c;
		int state = 0;
		size_t pos = 0;

		// Walk through path string...
		for (size_t i=0; i<root.size(); ++i) {
			c = root[i];
			switch (state) {
				case 0:
					if (c != sysutil::PATH_SEPERATOR) {
						pos = i;
						state = 10;
					}
					break;

				case 10:
					if (c == sysutil::PATH_SEPERATOR) {
						item = new TPathItem;
						item->path = validPath(root.substr(0, i));
						item->folder = root.substr(pos, i - pos);
						item->url = util::TURL::encode(item->path);
						parts.push_back(item);
						state = 0;
					}
					break;

				default:
					state = 0;
					break;
			}
		}

		// Loop finished without adding last folder entry
		if (state == 10) {
			size_t i = root.size() - 1;
			item = new TPathItem;
			item->path = validPath(root.substr(0, i));
			item->folder = root.substr(pos, std::string::npos);
			item->url = util::TURL::encode(item->path);
			parts.push_back(item);
		}
	}
}

void TExplorer::browse(const std::string& root, const app::TStringVector& patterns, const bool recursive, const bool hidden) {
	assign(root);
	readDirektory(path, patterns, recursive, hidden);
	updateContentToken();
	updatePathToken();
}


const std::string& TExplorer::setPath(const std::string& path) {
	app::TLockGuard<app::TMutex> lock(mtx);
	assign(path);
	updatePathToken();
	return this->path;
}

const std::string& TExplorer::getPath() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return path;
}

const std::string& TExplorer::setRoot(const std::string& root) {
	app::TLockGuard<app::TMutex> lock(mtx);
	explorerRootPath = util::validPath(root);
	return explorerRootPath;
}

const std::string& TExplorer::getRoot() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return explorerRootPath;
}


const std::string& TExplorer::explore() {
	app::TLockGuard<app::TMutex> lock(mtx);
	app::TStringVector patterns;
	browse("", patterns, false);
	return path;
}

const std::string& TExplorer::explore(const std::string& root) {
	app::TLockGuard<app::TMutex> lock(mtx);
	app::TStringVector patterns;
	browse(root, patterns, false);
	return path;
}

const std::string& TExplorer::explore(const std::string& root, const app::TStringVector& patterns) {
	app::TLockGuard<app::TMutex> lock(mtx);
	browse(root, patterns, false);
	return path;
}

} /* namespace app */

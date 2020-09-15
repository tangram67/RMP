/*
 * explorer.h
 *
 *  Created on: 20.10.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef EXPLORER_H_
#define EXPLORER_H_

#include "../inc/application.h"
#include "../inc/stringutils.h"
#include "../inc/semaphores.h"
#include "../inc/webserver.h"

namespace app {

enum EFileType {
	EFT_UNKNOWN,
	EFT_FILE,
	EFT_FOLDER
};

typedef struct CExplorerItem {
	size_t size;
	EFileType type;
	std::time_t time;
	std::string mime;
	std::string sort;
	std::string hash;

	std::string path;
	std::string file;
	std::string name;
	std::string ext;

	std::string urlPath;
	std::string urlFile;
	std::string urlName;
	std::string urlExt;

	std::string strType;
	std::string strSize;

	std::string utcTime;
	std::string localTime;

	std::string regular;
	std::string folder;
	std::string image;
	std::string audio;
	std::string video;

	bool isImage;
	bool isAudio;
	bool isVideo;

	void prime() {
		size = (size_t)0;
		time = (std::time_t)0;
		type = EFT_UNKNOWN;
		isImage = false;
		isAudio = false;
		isVideo = false;
	}

	void clear() {
		prime();
		mime.clear();
		sort.clear();
		hash.clear();

		path.clear();
		file.clear();
		name.clear();
		ext.clear();

		urlPath.clear();
		urlFile.clear();
		urlName.clear();
		urlExt.clear();

		strType.clear();
		strSize.clear();

		utcTime.clear();
		localTime.clear();

		regular.clear();
		folder.clear();
		image.clear();
		audio.clear();
		video.clear();
	}

	CExplorerItem() {
		prime();
	};

} TExplorerItem;


typedef struct CPathItem {
	std::string path;
	std::string folder;
	std::string url;

	void clear() {
		path.clear();
		folder.clear();
		url.clear();
	}

} TPathItem;


#ifdef STL_HAS_TEMPLATE_ALIAS

using PExplorerItem = TExplorerItem*;
using TExplorerList = std::vector<PExplorerItem>;
using PPathItem = TPathItem*;
using TPathList = std::vector<PPathItem>;

#else

typedef TExplorerItem* PExplorerItem;
typedef std::vector<PExplorerItem> TExplorerList;
typedef TPathItem* PPathItem;
typedef std::vector<PPathItem> TPathList;

#endif

class TExplorer : public TModule {
private:
	mutable app::TMutex mtx;
	std::string explorerRootPath;
	TExplorerList items;
	TPathList parts;
	TIniFile config;
	std::string path;
	std::string content;
	PWebToken wtExplorerImages;
	PWebToken wtExplorerHeaderText;
	PWebToken wtExplorerHeaderPath;
	PWebToken wtExplorerPathButtons;
	PWebToken wtExplorerAudioElements;
	bool useHTML5;
	bool debug;
	bool jail;

	void clear();
	void sort();
	void parse(const std::string& root);
	void assign(const std::string& root);
	void addInode(const std::string& path, const std::string& fileName, EFileType type);
	bool match(struct dirent *file, const std::string& pattern);
	bool stat(const std::string& fileName, const EFileType type, PExplorerItem item);
	std::string validPath(const std::string& directoryName);

	void browse(const std::string& root, const app::TStringVector& patterns, const bool recursive, const bool hidden = false);
	int readDirektory(const std::string& path, const app::TStringVector& patterns, const bool recursive, const bool hidden = false);

	void onExplorerClick(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void onExplorerAction(const std::string& key, const std::string& value, const util::TVariantValues& params, const util::TVariantValues& session, int& error);
	void prepareWebRequest(const std::string& uri, const util::TVariantValues& query, util::TVariantValues& session, bool& prepared);
	void onFileUploaded(const app::TWebServer& sender, const util::TVariantValues& session, const std::string& fileName, const size_t& size);
	void getExplorerContent(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void updateContentToken();
	void updatePathToken();

	std::string asJSON(const std::string filter, size_t index, size_t count) const;
	std::string asButtons(const bool bold = false) const;
	std::string asImages(bool collection = true) const;
	std::string asAudioElements(bool collection = true) const;

	void openConfig(const std::string& configPath);
	void readConfig();
	void writeConfig();
	void reWriteConfig();

public:
	int execute();
	void cleanup();

	const std::string& setPath(const std::string& path);
	const std::string& getPath() const;

	const std::string& setRoot(const std::string& root);
	const std::string& getRoot() const;

	const std::string& explore();
	const std::string& explore(const std::string& root);
	const std::string& explore(const std::string& root, const app::TStringVector& patterns);

	std::string getJSON(const std::string filter, size_t index, size_t count) const;
	std::string getButtons(const bool bold = false) const;
	std::string getImages(bool collection = true) const;
	std::string getAudioElements(bool collection = true) const;

	void writeDebug(const std::string& text) const;
	void writeLog(const std::string& text) const;

	TExplorer();
	virtual ~TExplorer();
};

} /* namespace app */

#endif /* EXPLORER_H_ */

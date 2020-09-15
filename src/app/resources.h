/*
 * resources.h
 *
 *  Created on: 31.12.2018
 *      Author: dirk
 */

#ifndef APP_RESOURCES_H_
#define APP_RESOURCES_H_

#include "../inc/application.h"
#include "../inc/stringutils.h"

namespace app {

class TResources : public TModule {
private:
	bool debug;
	PWebToken wtHTMLSystemInfo;

	std::string htmlCredits;
	std::string htmlSysinfo;
	std::string htmlSearchHelp;
	std::string htmlLibraryHelp;
	std::string htmlStreamHelp;
	std::string htmlTracksHelp;
	std::string htmlExplorerHelp;
	std::string htmlSettingsHelp;
	std::string htmlErroneousHelp;
	std::string htmlApplicationHelp;
	std::string htmlSessionData;

	void getCreditsData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSystemData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getApplicationHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getExplorerHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSearchHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getLibraryHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getStreamHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getTracksHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSettingsHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getErroneousHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);
	void getSessionData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error);

	std::string getCreditsList(size_t padding = 0);
	std::string getSystemInfo(size_t padding = 0);
	std::string getSearchHelp(size_t padding = 0);
	std::string getErroneousHelp(size_t padding = 0);
	std::string getApplicationHelp(size_t padding = 0);
	std::string getExplorerHelp(size_t padding = 0);
	std::string getTracksHelp(int page, size_t padding = 0);
	std::string getLibraryHelp(int page, size_t padding = 0);
	std::string getSettingsHelp(int page, size_t padding = 0);
	std::string getStreamHelp(size_t padding = 0);
	std::string getSessionInfo(const std::string& sid, size_t padding);

	void createContentTable(util::TStringList& html, const util::TStringList& headers, size_t padding);
	void addPadding(util::TStringList& html, size_t padding);
	void addAnchor(util::TStringList& html, size_t& anchor);
	void addHeadline(util::TStringList& html, const std::string& header, size_t& anchor, size_t padding, bool addHeader);
	void addPageBreak(util::TStringList& html);

	void addExplorerHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addUPnPNotes(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addTaggingNotes(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addSearchTagsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addUpdateHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addThmbnailActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addTrackActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addTrackLinksHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addPlayerStatusHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addPlayerControlHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addPlayerSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addNetworkSharingHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addNetworkSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addRemoteSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addSystemSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addPlaylistActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addLibraryActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addStreamActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addStreamInfoHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addStreamCommentHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	void addErroneousExplanationHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader);
	bool getWhoisResult(const std::string& address, util::TStringList& result);
	void addVendorFootnote(util::TStringList& html, size_t padding);

public:
	int execute();
	void cleanup();

	TResources();
	virtual ~TResources();
};

} /* namespace app */

#endif /* APP_RESOURCES_H_ */

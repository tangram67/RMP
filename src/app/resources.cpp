/*
 * resources.cpp
 *
 *  Created on: 31.12.2018
 *      Author: dirk
 */

#include "resources.h"
#include "../inc/globals.h"
#include "../inc/sockets.h"
#include "../inc/process.h"
#include "../inc/htmlutils.h"
#include "../inc/sysutils.h"

namespace app {

TResources::TResources() {
	wtHTMLSystemInfo = nil;
	debug = false;
}

TResources::~TResources() {
}

int TResources::execute() {

	// Add actions and data links to webserver instance
	if (application.hasWebServer()) {

		// Add virtual data URLs to webserver
		application.addWebLink("credits.html",       &app::TResources::getCreditsData,         this);
		application.addWebLink("systeminfo.html",    &app::TResources::getSystemData,          this);
		application.addWebLink("searchhelp.html",    &app::TResources::getSearchHelpData,      this);
		application.addWebLink("explorerhelp.html",  &app::TResources::getExplorerHelpData,    this);
		application.addWebLink("mainhelp.html",      &app::TResources::getApplicationHelpData, this);
		application.addWebLink("libraryhelp.html",   &app::TResources::getLibraryHelpData,     this);
		application.addWebLink("streamhelp.html",    &app::TResources::getStreamHelpData,      this);
		application.addWebLink("trackshelp.html",    &app::TResources::getTracksHelpData,      this);
		application.addWebLink("settingshelp.html",  &app::TResources::getSettingsHelpData,    this);
		application.addWebLink("erroneoushelp.html", &app::TResources::getErroneousHelpData,   this);
		application.addWebLink("sessiondata.html",   &app::TResources::getSessionData,         this);

		// Add and fill web token with default data
		wtHTMLSystemInfo  = application.addWebToken("HTML_SYSTEM_INFO", getSystemInfo());
	}

	// Leave resources after initialization
	return EXIT_SUCCESS;
}

void TResources::cleanup() {
	// Not needed here...
}

void TResources::createContentTable(util::TStringList& html, const util::TStringList& headers, size_t padding) {
	if (!headers.empty()) {
		addPadding(html, padding);

		html.add("<a id=\"anchor-top\"></a>");
		html.add("<div class=\"bs-callout bs-callout-default\">");
		html.add("<h4>Table of contents</h4>");
		html.add("<ul style=\"list-style-type:circle\">");
		for (size_t i=0, a=1; i<headers.size(); ++i,++a) {
			html.add("  <li><a href=\"#anchor-" + std::to_string((size_u)a) + "\">" + headers[i] + "</a></li>");
		}
		html.add("</ul>");
		html.add("</div>");

		addPadding(html, 2 * padding);
	}
}

void TResources::addPadding(util::TStringList& html, size_t padding) {
	if (padding > 0) {
		html.add("<div style=\"margin:0; padding:0; height:" + std::to_string((size_u)padding) + "px;\"></div>");
	}
}

void TResources::addAnchor(util::TStringList& html, size_t& anchor) {
	if (anchor > 0) {
		html.add("<a id=\"anchor-" + std::to_string((size_u)anchor++) + "\"></a>");
	}
}

void TResources::addHeadline(util::TStringList& html, const std::string& header, size_t& anchor, size_t padding, bool addHeader) {
	if (anchor == 0 || anchor > 1) {
		addPadding(html, padding);
	}
	if (addHeader) {
		addAnchor(html, anchor);
		html.add("<h4><b>" + header + "</b>");
		if (anchor > 0) {
			html.add("  <a href=\"#anchor-top\" class=\"hidden-print pull-right\" style=\"display:inline-block; margin-right:5px;\">");
			html.add("    <span style=\"color:black;\" class=\"glyphicon glyphicon-circle-arrow-up\" title=\"Goto top...\"></span>");
			html.add("  </a>");
			html.add("</h4>");
		} else {
			html.add("</h4>");
		}
	}
}


void TResources::getCreditsData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	htmlCredits = getCreditsList(5);
	if (!htmlCredits.empty()) {
		data = htmlCredits.c_str();
		size = htmlCredits.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getCreditsData() HTML = " << htmlCredits << app::reset << std::endl;
}

void TResources::getSystemData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	htmlSysinfo = getSystemInfo(5);
	if (!htmlSysinfo.empty()) {
		data = htmlSysinfo.c_str();
		size = htmlSysinfo.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getSystemData() HTML = " << htmlSysinfo << app::reset << std::endl;
}

void TResources::getApplicationHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	if (htmlApplicationHelp.empty()) {
		htmlApplicationHelp = getApplicationHelp(5);
	}
	if (!htmlApplicationHelp.empty()) {
		data = htmlApplicationHelp.c_str();
		size = htmlApplicationHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getApplicationHelpData() HTML = " << htmlApplicationHelp << app::reset << std::endl;
}

void TResources::getSearchHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	if (htmlSearchHelp.empty()) {
		htmlSearchHelp = getSearchHelp(5);
	}
	if (!htmlSearchHelp.empty()) {
		data = htmlSearchHelp.c_str();
		size = htmlSearchHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getSearchHelpData() HTML = " << htmlSearchHelp << app::reset << std::endl;
}

void TResources::getExplorerHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	if (htmlExplorerHelp.empty()) {
		htmlExplorerHelp = getExplorerHelp(5);
	}
	if (!htmlExplorerHelp.empty()) {
		data = htmlExplorerHelp.c_str();
		size = htmlExplorerHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getExplorerHelpData() HTML = " << htmlExplorerHelp << app::reset << std::endl;
}

void TResources::getLibraryHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	int page = params["page"].asInteger(999);
	htmlLibraryHelp = getLibraryHelp(page, 5);
	if (!htmlLibraryHelp.empty()) {
		data = htmlLibraryHelp.c_str();
		size = htmlLibraryHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getLibraryHelpData() HTML = " << htmlLibraryHelp << app::reset << std::endl;
}

void TResources::getStreamHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	htmlStreamHelp = getStreamHelp(5);
	if (!htmlStreamHelp.empty()) {
		data = htmlStreamHelp.c_str();
		size = htmlStreamHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getStreamHelpData() HTML = " << htmlStreamHelp << app::reset << std::endl;
}

void TResources::getTracksHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	int page = params["page"].asInteger(999);
	htmlTracksHelp = getTracksHelp(page, 5);
	if (!htmlTracksHelp.empty()) {
		data = htmlTracksHelp.c_str();
		size = htmlTracksHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getTracksHelpData() HTML = " << htmlTracksHelp << app::reset << std::endl;
}

void TResources::getSettingsHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	int page = params["page"].asInteger(999);
	htmlSettingsHelp = getSettingsHelp(page, 5);
	if (!htmlSettingsHelp.empty()) {
		data = htmlSettingsHelp.c_str();
		size = htmlSettingsHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getSettingsHelpData() HTML = " << htmlSettingsHelp << app::reset << std::endl;
}

void TResources::getErroneousHelpData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	if (htmlErroneousHelp.empty()) {
		htmlErroneousHelp = getErroneousHelp(5);
	}
	if (!htmlErroneousHelp.empty()) {
		data = htmlErroneousHelp.c_str();
		size = htmlErroneousHelp.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getErroneousHelpData() HTML = " << htmlErroneousHelp << app::reset << std::endl;
}

void TResources::getSessionData(TThreadData& sender, const void*& data, size_t& size, const util::TVariantValues& params, const util::TVariantValues& session, util::TVariantValues& headers, bool& zipped, bool& cached, int& error) {

	// Build HTML response
	std::string sid = params["SID"].asString();
	if (!sid.empty()) {
		htmlSessionData = getSessionInfo(sid, 5);
	} else {
		htmlSessionData = "<h3><strong><i>Failure...</i></strong></h3><h4>Invalid or missing session ID.</h4>";
	}
	if (!htmlSessionData.empty()) {
		data = htmlSessionData.c_str();
		size = htmlSessionData.size();
	}

	if (debug) aout << app::yellow << "TPlayer::getSessionData() HTML = " << htmlSessionData << app::reset << std::endl;
}


std::string TResources::getSystemInfo(size_t padding) {
	std::string address;
	std::string hostname = sysutil::getHostName();
	std::string domainname = sysutil::getDomainName();
	util::TStringList addresses;
	sysutil::getLocalIpAddresses(addresses);

	// Sanitize hostname
	if (domainname.empty())
		hostname = hostname + "." + domainname;
	hostname = html::THTML::applyFlowControl(hostname);

	// Sanitize system language
	std::string language = html::THTML::applyFlowControl(syslocale.getInfo());

	// Get system name
	std::string system = "Unknown system";
	if (sysutil::isLinux()) {
		sysutil::TSysInfo info;
		if (sysutil::uname(info)) {
			// Output: e.g. Linux 3.13.0-85-generic x86_64>
			system = info.sysname + " " + info.release + " " + info.machine;
		}
	}

	util::TStringList html;
	addPadding(html, padding);
	html.add("<h4><b>General system and network information</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>General informations about the system are shown below. " \
			 "To connect to the system from devices in the local network like PCs, tablets or smartphones, please <b>use the full URL</b> (e.g. http://192.168.1.5:8099) shown in this popup window under &quot;Links&quot;. " \
			 "If more that one IP range is configured, please use the one that fits to the IP address of the mobile device. " \
			 "Please note the remarks about automatic UPnP device discovery on Microsoft Windows<sup>&reg;</sup> operating systems.</p>");
	html.add("</div>");
	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Name</th>");
	html.add("      <th>Value</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>Hostname</td>");
	html.add("      <td>" + hostname + "</td>");
	html.add("    </tr>");

	if (!addresses.empty()) {
		if (addresses.size() > 1) {
			for (size_t i=0; i<addresses.size(); ++i) {
				address = html::THTML::applyFlowControl(addresses[i]);
				html.add("    <tr>");
				if (i == 0)
					html.add("      <td rowspan=\"" + std::to_string((size_u)addresses.size())  + "\">Addresses</td>");
				html.add("      <td>" + address + "</td>");
				html.add("    </tr>");
			}
		} else {
			address = html::THTML::applyFlowControl(addresses[0]);
			html.add("    <tr>");
			html.add("      <td>Address</td>");
			html.add("      <td>" + address + "</td>");
			html.add("    </tr>");
		}
	} else {
		html.add("    <tr>");
		html.add("      <td>Address</td>");
		html.add("      <td>No network connection</td>");
		html.add("    </tr>");
	}

	if (!addresses.empty() && application.hasWebServer()) {
		std::string link, url;
		std::string proto = application.getWebServer().isSecure() ? "https" : "http";
		size_t offset = proto.size() + 3; // "://" --> 3
		if (addresses.size() > 1) {
			for (size_t i=0; i<addresses.size(); ++i) {
				address = addresses[i];
				if (inet::isIPv6Address(address))
					address = "[" + address + "]";
				url = util::csnprintf("%://%:%", proto, address, application.getWebServer().getPort());
				link = html::THTML::applyFlowControl(url, offset);
				html.add("    <tr>");
				if (i == 0)
					html.add("      <td rowspan=\"" + std::to_string((size_u)addresses.size())  + "\">Links</td>");
				html.add("      <td>");
				html.add("        <a target=\"_blank\" href=\"" + url + "\">");
				html.add("          <div style=\"height: 100%; width: 100%\">" + link + "</div>");
				html.add("        </a>");
				html.add("      </td>");
				html.add("    </tr>");
			}
		} else {
			address = addresses[0];
			if (inet::isIPv6Address(address))
				address = "[" + address + "]";
			url = util::csnprintf("%://%:%", proto, address, application.getWebServer().getPort());
			link = html::THTML::applyFlowControl(url, offset);
			html.add("    <tr>");
			html.add("      <td>Link</td>");
			html.add("      <td>");
			html.add("        <a target=\"_blank\" href=\"" + url + "\">");
			html.add("          <div style=\"height: 100%; width: 100%\">" + link + "</div>");
			html.add("        </a>");
			html.add("      </td>");
			html.add("    </tr>");
		}
	} else {
		html.add("    <tr>");
		html.add("      <td>Link</td>");
		html.add("      <td>No network connection</td>");
		html.add("    </tr>");
	}

	html.add("    <tr>");
	html.add("      <td>Language</td>");
	html.add("      <td>" + language + "</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td>System</td>");
	html.add("      <td>" + system + "</td>");
	html.add("    </tr>");
	html.add("  </tbody>");

	html.add("</table>");

	size_t anchor = 0;
	addUPnPNotes(html, anchor, 0, false);

	return html.asString('\n');
}


enum ECreditType { ECT_NONE, ECT_BACKEND, ECT_FRONTEND };

struct CCredits {
	const ECreditType type;
	const char* library;
	const char* description;
	const char* website;
	const char* url;
};

static const struct CCredits credits[] =
{
		{ ECT_BACKEND,  "GCC",                  "GNU Compiler Collection",                  "gcc.gnu.org",                   "https://gcc.gnu.org/" },
		{ ECT_BACKEND,  "glibc",                "GNU C Library",                            "www.gnu.org",                   "https://www.gnu.org/software/libc/" },
		{ ECT_BACKEND,  "libstdc++",            "GNU C++ Library",                          "gcc.gnu.org",                   "https://gcc.gnu.org/onlinedocs/libstdc++/" },
		{ ECT_BACKEND,  "Libmicro&shy;httpd",   "GNU Libmicrohttpd",                        "www.gnu.org",                   "https://www.gnu.org/software/libmicrohttpd/" },
		{ ECT_BACKEND,  "GnuTLS",               "GnuTLS Transport Layer Security Library",  "www.gnutls.org",                "https://www.gnutls.org/" },
		{ ECT_BACKEND,  "OpenSSL",              "OpenSSL Cryptography and SSL/TLS Toolkit", "www.openssl.org",               "https://www.openssl.org/" },
		{ ECT_BACKEND,  "Eclipse Paho",         "Eclipse Paho MQTT client library",         "www.eclipse.org",               "https://www.eclipse.org/paho/" },
		{ ECT_BACKEND,  "mpg123",               "MPEG Audio Player and decoder library",    "www.mpg123.de",                 "https://www.mpg123.de/" },
		{ ECT_BACKEND,  "zlib",                 "zlib Compression Library",                 "zlib.net",                      "https://zlib.net/" },
		{ ECT_BACKEND,  "libPNG",               "PNG Reference Library",                    "www.libpng.org",                "http://www.libpng.org/pub/png/libpng.html" },
		{ ECT_BACKEND,  "libjpeg",              "Independent JPEG Group",                   "ijg.org",                       "https://ijg.org/" },
		{ ECT_BACKEND,  "libexif",              "The C EXIF library",                       "libexif.github.io",             "https://libexif.github.io/" },
		{ ECT_BACKEND,  "libFLAC",              "Free Lossless Audio Codec",                "xiph.org",                      "https://xiph.org/flac/" },
		{ ECT_BACKEND,  "libcurl",              "Multiprotocol file transfer library",      "curl.haxx.se",                  "https://curl.haxx.se/libcurl/" },
		{ ECT_BACKEND,  "SQLite",               "SQLite database",                          "www.sqlite.org",                "https://www.sqlite.org/index.html" },
		{ ECT_BACKEND,  "libfaad",              "Advanced MPEG-4/AAC Audio Codec",          "www.audiocoding.com",           "https://www.audiocoding.com/faad2.html" },
		{ ECT_BACKEND,  "ALAC",                 "Apple Lossless Format Decoder",            "github.com/macosforge",         "https://github.com/macosforge/alac" },
		{ ECT_BACKEND,  "id3v2lib",             "Read and edit ID3 tags from MP3 files",    "github.com/larsbs",             "https://github.com/larsbs/id3v2lib" },
		{ ECT_BACKEND,  "Color&shy;space",      "Color Rendering of Spectra",               "www.fourmilab.ch",              "https://www.fourmilab.ch/documents/specrend/" },
		{ ECT_BACKEND,  "SHA1",                 "Public domain SHA1 algorithm",             "www.dominik-reichl.de",         "https://www.dominik-reichl.de/software.html#csha1" },
		{ ECT_BACKEND,  "SHA2",                 "Public domain SHA2 algorithm",             "github.com/ogay",               "https://github.com/ogay/sha2" },
		{ ECT_BACKEND,  "FFT (C)",              "Fixed point Fast Fourier Transform",       "www.jjj.de",                    "https://www.jjj.de/fft/fftpage.html" },
		{ ECT_BACKEND,  "FFT (C++)",            "Double precision Fast Fourier Transform",  "rosettacode.org",               "https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B" },
		{ ECT_BACKEND,  "Bits",                 "Bit Twiddling Hacks",                      "graphics.stanford.edu/seander", "https://graphics.stanford.edu/~seander/bithacks.html" },
		{ ECT_FRONTEND, "jQuery",               "JavaScript DOM manipulation library",      "jquery.com",                    "https://jquery.com/" },
		{ ECT_FRONTEND, "Bootstrap",            "HTML front-end component library",         "getbootstrap.com",              "https://getbootstrap.com/docs/3.4/" },
		{ ECT_FRONTEND, "Bootstrap Table",      "An extended table for Bootstrap",          "bootstrap-table.com",           "https://bootstrap-table.com/" },
		{ ECT_FRONTEND, "Bootstrap Notify",     "Bootstrap notification library",           "github.com/mouse0270",          "https://github.com/mouse0270/bootstrap-growl" },
		{ ECT_FRONTEND, "Context Menu",         "Bootstrap Table context menu",             "github.com/prograhammer",       "https://github.com/prograhammer/bootstrap-table-contextmenu" },
		{ ECT_FRONTEND, "Drag&amp;&#8203;Drop", "Bootstrap Table Drag &amp; Drop",          "github.com/isocra",             "https://github.com/isocra/TableDnD" },
		{ ECT_FRONTEND, "Animate",              "CSS animation library",                    "daneden.github.io",             "https://daneden.github.io/animate.css/" },
		{ ECT_FRONTEND, "Light&shy;box",        "The original lightbox script",             "lokeshdhakar.com",              "https://lokeshdhakar.com/projects/lightbox2/" },
		{ ECT_FRONTEND, "Javascrypt",           "Browser-Based Cryptography Tools",         "www.fourmilab.ch",              "https://www.fourmilab.ch/javascrypt/" },
		{ ECT_FRONTEND, "Chart.js",             "Simple yet flexible JavaScript charting",  "www.chartjs.org",               "https://www.chartjs.org/" },
		{ ECT_FRONTEND, "PACE",                 "Automatic page load progress bar",         "codebyzach.github.io",          "https://codebyzach.github.io/pace/" },
		{ ECT_NONE, nil, nil, nil, nil }
};

void addCredits(util::TStringList& html, const ECreditType type) {
	std::string library, description, website, link;
	const struct CCredits *it;

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Library</th>");
	html.add("      <th>Description</th>");
	html.add("      <th>Website</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");

	for (it = credits; util::assigned(it->library); ++it) {
		if (type == it->type) {
			library = it->library;
			link = it->url;
			description = it->description;
			website = html::THTML::applyFlowControl(it->website);
			html.add("    <tr>");
			html.add("      <td>" + library + "</td>");
			html.add("      <td>" + description + "</td>");
			html.add("      <td><a href=\"" + link + "\" target=\"_blank\" title=\"Goto " + link + "\">" + website + "</a></td>");
			html.add("    </tr>");
		}
	}

	html.add("  </tbody>");
	html.add("</table>");
}

std::string TResources::getCreditsList(size_t padding) {
	util::TStringList html;

	addPadding(html, padding);

	html.add("<div class=\"well\">");
	html.add("<p>This application uses external libraries and/or makes use of following persons or companies code. "\
			 "The author of this application would like to thank all the people and companies for their work in the public domain.</p>");
	html.add("</div>");
	html.add("<div class=\"caption\">");

	html.add("<h4><b>C/C++ Backend</b></h4>");
	addCredits(html, ECT_BACKEND);

	html.add("<h4><b>HTML/JavaScript Frontend</b></h4>");
	addCredits(html, ECT_FRONTEND);

	html.add("</div>");

	return html.asString('\n');
}


std::string TResources::getApplicationHelp(size_t padding) {
	util::TStringList html;

	addPadding(html, padding);

	html.add("<div class=\"well\">");
	html.add("<p>This is the summary of all context sensitive help screens. "\
			 "The main information tables are collected in this help screen. " \
			 "<strong>See the context help for further details on each individual screen.</strong> " \
			 "The help dialogs can be opened by clicking on the &quot;Context Help&quot; icon <span class=\"glyphicon glyphicon-question-sign\"></span> on the top right corner of the pages. " \
			 "Print the help pages by clicking on the &quot;Printer&quot; icon <span class=\"glyphicon glyphicon-print\"></span> at the top right corner of this page.</p>");
	html.add("</div>");

	util::TStringList items;
	items.add("Player status");
	items.add("Player mode controls");
	items.add("Notes on album tagging");
	items.add("UPnP/DLNA support");
	items.add("Tagging and search fields");
	items.add("Managing the thumbnail cache");
	items.add("Playlist management");
	items.add("Library actions");
	items.add("Internet radio management");
	items.add("Song context menu actions");
	items.add("Advanced song information");
	items.add("Player settings");
	items.add("Basic network settings");
	items.add("Network file sharing settings");
	items.add("Remote control settings");
	items.add("File explorer view");
	items.add("Hints on corrupt files");
	items.add("Online software updates");
	createContentTable(html, items, padding);

	padding = 0;
	size_t anchor = 1;
	addPageBreak(html);
	addPlayerStatusHelp(html, anchor, padding, true);
	addPageBreak(html);
	addPlayerControlHelp(html, anchor, padding, true);
	addPageBreak(html);
	addTaggingNotes(html, anchor, padding, true);
	addUPnPNotes(html, anchor, padding, true);
	addPageBreak(html);
	addSearchTagsHelp(html, anchor, padding, true);
	addPageBreak(html);
	addThmbnailActionsHelp(html, anchor, padding, true);
	addPageBreak(html);
	addPlaylistActionsHelp(html, anchor, padding, true);
	addLibraryActionsHelp(html, anchor, padding, true);
	addPageBreak(html);
	addStreamActionsHelp(html, anchor, padding, true);
	addStreamInfoHelp(html, anchor, padding, false);
	addStreamCommentHelp(html, anchor, padding, false);
	addPageBreak(html);
	addTrackActionsHelp(html, anchor, padding, true);
	addPageBreak(html);
	addTrackLinksHelp(html, anchor, padding, true);
	addPageBreak(html);
	addPlayerSettingsHelp(html, anchor, padding, true);
	addPageBreak(html);
	addNetworkSettingsHelp(html, anchor, padding, true);
	addNetworkSharingHelp(html, anchor, padding, true);
	addPageBreak(html);
	addRemoteSettingsHelp(html, anchor, padding, true);
	addExplorerHelp(html, anchor, padding, true);
	addPageBreak(html);
	addErroneousExplanationHelp(html, anchor, padding, true);
	addUpdateHelp(html, anchor, padding, true);
	addPageBreak(html);
	addVendorFootnote(html, padding);

	return html.asString('\n');
}


void TResources::addPageBreak(util::TStringList& html) {
	html.add("<div class=\"pagebreak\"> </div>");
}


std::string TResources::getSearchHelp(size_t padding) {
	util::TStringList html;
	if (padding > 0) {
		html.add("<div style=\"margin:0; padding:0; height:" + std::to_string((size_u)padding) + "px;\"></div>");
	}
	html.add("<h4><b>Assignment of ID3 tags to search fields</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>The following table shows how ID3 tags are assigned to search selections. Multiple assignements are possible. "\
			 "This means searching for artist also searches for album artist. This might also be taken as a hint of how to use tags as suggested by ");
	html.add("<a href=\"http://id3.org/id3v2.4.0-frames\" target=\"_blank\" title=\"Goto ID3 Home...\">ID3.org</a></p>");
	html.add("</div>");

	size_t anchor = 0;
	addSearchTagsHelp(html, anchor, 0, false);
	addTaggingNotes(html, anchor, 0, false);

	html.add("<div class=\"bs-callout bs-callout-primary\">");
	html.add("<h4>Addendum</h4>");
	html.add("The &quot;Search&quot; icon <span class=\"glyphicon glyphicon-search\"></span> on the top right corner of the page can be used initiate a new search with the current settings.");
	html.add("</div>");

	return html.asString('\n');
}


void TResources::addUPnPNotes(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Information on UPnP/DLNA support", anchor, padding, addHeader);

	std::string callout = "bs-callout-primary";

	html.add("<div class=\"bs-callout " + callout + "\">");
	html.add("<h4>UPnP device discovery</h4>");
	html.add("The UPnP/DLNA support <b>is limited</b> to device discovery. " \
			 "The &quot;" + application.getDescription() + "&quot; shows up in the network neighbourhood as a media server.<br>" \
			 "The management web page (&quot;this page&quot;) is shown when double clicking the icon in the network section of the file explorer on Microsoft Windows<sup>&reg;</sup> operating systems. " \
			 "This feature is intended to be used to open the management web interface without typing any URL into the browser.");
	html.add("</div>");
}


void TResources::addTaggingNotes(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Notes on album tagging", anchor, padding, addHeader);

	std::string callout = addHeader ? "bs-callout-primary" : "bs-callout-danger";

	html.add("<div class=\"bs-callout " + callout + "\">");
	html.add("<h4>Album grouping</h4>");
	html.add("Albums are grouped primarily on the albumartist tag. If there is no albumartist tag found, as a fallback the artist tag is used. " \
			 "In practice that means the artist substitutes the missing albumartist tag. In general it is recommended to use the albumartist tag to group albums together.<br>" \
			 "An album by &quot;The Miles Davis Quintet&quot; should be tagged as albumartist &quot;Miles Davis&quot;. " \
			 "This causes the album to be shown under &quot;Miles Davis&quot; with each individual artist &quot;The Miles Davis Quintet&quot;.");
	html.add("</div>");

	html.add("<div class=\"bs-callout " + callout + "\">");
	html.add("<h4>Compilations and tribute albums</h4>");
	html.add("The albumartist naming convention is also useful for tribute compilations. The albumartist is the tributed artist and each song has the artist tag of individual the performing artist.<br>" \
			 "Simple compilations should be tagged as &quot;Various Artistst&quot;, &quot;Soundtracks&quot;, etc. as the albumartist tag. " \
			 "The comilation category names can be configured under <a href=\"/app/system/settings.html?prepare=yes\" title=\"Media Player Settings\">&quot;Media Player Settings&quot;</a>. " \
			 "The categories are grouped together in the <a href=\"/app/library/artists.html?prepare=yes&title=artists\" title=\"Artist View\">&quot;Artist View&quot;</a> as " \
			 "the <a  href=\"/app/library/artists.html?prepare=yes&title=artists&filter=2\"><span class=\"glyphicon glyphicon-menu-hamburger\"></span></a> entry.");
	html.add("</div>");
}


void TResources::addUpdateHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Online software updates", anchor, padding, addHeader);

	html.add("<div class=\"bs-callout bs-callout-primary\">");
	html.add("<h4>Getting updates</h4>");
	html.add("Open the <a href=\"/app/system/system.html\" title=\"Application Overview\">&quot;Application Overview&quot;</a> screen in the browser of the Reference Media Player device, " \
			 "<b>not from a portable or any other device or browser</b>. As soon as a newer version is available a popup message for the new version is shown. " \
			 "Open the link to the Reference Media Player hompage <a href=\"http://www.dbrinkmeier.de\" target=\"_blank\" title=\"db Application\">&quot;db Application&quot;</a> on the left bottom side " \
			 "or visit the Reference Media Player homepage <a href=\"http://www.dbrinkmeier.de\" target=\"_blank\" title=\"db Application\">&quot;db Application&quot;</a> directly. " \
			 "Under &quot;Downloads&quot; you will find the current version for the Reference Media Player. " \
			 "The current version can also be downoladed under  <a href=\"http://www.dbrinkmeier.de/download/hmp.update.tar.gz\" title=\"Reference Media Player Update\">&quot;hmp.update.tar.gz&quot;</a>.");
	html.add("</div>");

	html.add("<div class=\"bs-callout bs-callout-danger\">");
	html.add("<h4>Attention</h4>");
	html.add("Shut down the Reference Media Player device, <b>not the portable remote device</b>, after downloading an update file for the Reference Media Player. " \
			 "The update will be installed during the next reboot. " \
			 "The new version number will be shown under <a href=\"/app/system/system.html\" title=\"Application Overview\">&quot;Application Overview&quot;</a>. " \
			 "Delete the browser cache when experiencing an unexpected behaviour in the user interface after updating the Reference Media Player version. " \
			 "Read your browser specific manual how to do this.");
	html.add("</div>");
}


void TResources::addSearchTagsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Assignment of ID3 tags to search fields", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>ID3 Tag</th>");
	html.add("      <th>Name</th>");
	html.add("      <th>Description</th>");
	html.add("      <th>Search field</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>TPE1</td>");
	html.add("      <td>Artist</td>");
	html.add("      <td>");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Main artist</li>");
	html.add("          <li>Performing artist</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("      <td>Artist & Albumartist</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>TPE2</td>");
	html.add("      <td>Albumartist</td>");
	html.add("      <td>");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Band</li>");
	html.add("          <li>Orchestra</li>");
	html.add("          <li>Tribute artist</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("      <td>Artist & Albumartist<br>Albumartist</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>TPE3</td>");
	html.add("      <td>Conductor</td>");
	html.add("      <td>");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Conductor</li>");
	html.add("          <li>Performer refinement</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("      <td>Conductor</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>TALB</td>");
	html.add("      <td>Albumname</td>");
	html.add("      <td>");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Album</li>");
	html.add("          <li>Movie / Show title</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("      <td>Album & Songs</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>TIT2</td>");
	html.add("      <td>Title</td>");
	html.add("      <td>");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Title / Songname</li>");
	html.add("          <li>Content description</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("      <td>Album & Songs</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>TCOM</td>");
	html.add("      <td>Composer</td>");
	html.add("      <td>Composer</td>");
	html.add("      <td>Composer</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}


void TResources::addNetworkSharingHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Network sharing settings", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Setting</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>Mount remote file system</td>");
	html.add("      <td>Enable or disable automatic mounting of the share on application startup.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Select remote filesystem</td>");
	html.add("      <td>Valid types are:");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>SMB for Microsoft Windows based shares</li>");
	html.add("          <li>NFS for native Linux based shared</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Remote path (Share)</td>");
	html.add("      <td>This is the exported path on the remote host:");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>SMB //&lt;Hostname or IP address&gt;/&lt;Name of share&gt;</li>");
	html.add("          <li>NFS &lt;Hostname or IP address&gt;:&lt;Exported (remote) path of share&gt;</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Local path (Target)</td>");
	html.add("      <td>The local path is the mount point in the local filesystem. The default path is &quot;/usr/local/dbApps/media/&quot;. " \
						"After mounting a network share, this path can be used in the " \
						"<a href=\"/app/system/settings.html?prepare=yes\" title=\"Media Player Settings\">&quot;Media Player Settings&quot;</a> " \
						"to select the media mount. Select the local mount path to use the mounted remote file system as the media root.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Mount options (NFS)</td>");
	html.add("      <td>Mount options are only valid for Linux NFS shares. The default setting is &quot;vers=3,nolock&quot;.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Username and password (SMB)</td>");
	html.add("      <td>These are the security credential for Microsoft Windows based SMB share and Samba based shares on Linux systems.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Mount state</td>");
	html.add("      <td>The current state of the mount is shown here. If an error during the mount process occured, a short error message is also shown here.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");

	html.add("<div class=\"bs-callout bs-callout-warning\">");
	html.add("<h4>Addendum</h4>");
	html.add("Please note that the path separator must always be &quot;/&quot; for Linux based shares as well as for Microsoft Windows based shares.<br>");
	html.add("Prefer using IP addresses instead of hostnames unless you are sure your local name resolution (DNS) is working properly.");
	html.add("</div>");
}


void TResources::addThmbnailActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Managing the thumbnail cache", anchor, padding, addHeader);

	std::string width = "36px";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Icon</th>");
	html.add("      <th>Action</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-remove\"></span></td>");
	html.add("      <td>Clear cached image</td>");
	html.add("      <td>Clearing the coverart image can be done in the album detail view. " \
					   "The cache context menu is opened on rigth clicking the image or tapping longer than one second on the image on a mobile device. " \
					   "The menu entry &quot;Clear cached image&quot; <span class=\"glyphicon glyphicon-remove\"></span> clears the cached image for the given album.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-trash\"></span></td>");
	html.add("      <td>Clear all cached images</td>");
	html.add("      <td>Clearing the complete application image cache can be done in the album detail view. " \
					   "The cache context menu is opened on rigth clicking the image or tapping longer than one second on the image on a mobile device. " \
					   "The menu entry &quot;Clear all cached images&quot; <span class=\"glyphicon glyphicon-trash\"></span> clears all cached images for all albums.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");

	std::string callout = addHeader ? "bs-callout-primary" : "bs-callout-danger";

	html.add("<div class=\"bs-callout " + callout + "\">");
	html.add("<h4>Browser caching</h4>");
	html.add("Clearing a cover art image in the application cache does not autonatically means, that the browser display canges. " \
			 "To view the newly loaded image after clearing the application image(s) it is nessacary to empty the browser cache. " \
			 "Please refer to the help of the browser (Chrome, Firefox, Microsoft Internet Explorer, etc.) to empty the browser file cache.");
	html.add("</div>");

}


std::string TResources::getLibraryHelp(int page, size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Album library display and playlist management</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>The library display allows direct access to play content and to create playlists. " \
			 "The artist display and also the album display support direct library actions " \
			 "to add complete artists <span class=\"glyphicon glyphicon-duplicate\"></span> or a single album <span class=\"glyphicon glyphicon-file\"></span> to a given playlist. " \
			 "An album can also be played by a single click on the <span class=\"glyphicon glyphicon-play\"></span> button, just like a classical CD player device. " \
			 "The whole user interface layout is designed to work on single user actions. This is a useful feature for all kinds of mobile devices.</p>");
	html.add("</div>");

	size_t anchor = 0;
	switch (page) {
		case 2002:
			addPlaylistActionsHelp(html, anchor, 0, true);
			addLibraryActionsHelp(html, anchor, 0, true);
			break;
		case 2001:
			addPlaylistActionsHelp(html, anchor, 0, true);
			break;
		case 2000:
			addLibraryActionsHelp(html, anchor, 0, true);
			html.add("<div class=\"bs-callout bs-callout-primary\">");
			html.add("<h4>Addendum</h4>");
			html.add("Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding albums and songs.");
			html.add("</div>");
			break;
		default:
			html.add("<div class=\"alert alert-danger\" role=\"alert\">");
			html.add("  <strong>Not found:</strong> Invalid help page index (" + std::to_string((size_s)page) + ")");
			html.add("</div>");
			break;
	}

	return html.asString('\n');
}


void TResources::addPlaylistActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Playlist management", anchor, padding, addHeader);

	std::string width = "36px";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Icon</th>");
	html.add("      <th>Action</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-star\"></span></td>");
	html.add("      <td nowrap>Select playlist</td>");
	html.add("      <td>A playlist can be selected via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span>.<br> " \
					"Either the submenu &quot;Select playlist&quot; <span class=\"glyphicon glyphicon-star\"></span> is displayed and when clicked a selection menu is shown " \
					"or, if there are less that 5 playlist created, the playlist name can be selected directly from the main menu.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-pencil\"></span></td>");
	html.add("      <td nowrap>Create playlist</td>");
	html.add("      <td>A new playlist can be created via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span>.<br> " \
					"The submenu &quot;New playlist&quot; <span class=\"glyphicon glyphicon-pencil\"></span> asks for the name of the new playlist and creates an empty body.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-flash\"></span></td>");
	html.add("      <td nowrap>Rename playlist</td>");
	html.add("      <td>A playlist can be renamed via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span>.<br> " \
					"The submenu &quot;Rename playlist&quot; <span class=\"glyphicon glyphicon-flash\"></span> selects the playlist and expects the new name.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-trash\"></span></td>");
	html.add("      <td nowrap>Delete playlist</td>");
	html.add("      <td>A playlist can be deleted via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span>.<br> " \
					"The submenu &quot;Delete playlist&quot; <span class=\"glyphicon glyphicon-trash\"></span> selects the playlist and delete the chosen one.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addLibraryActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Library actions", anchor, padding, addHeader);

	std::string width = "36px";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Icon</th>");
	html.add("      <th>Action</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-play\"></span></td>");
	html.add("      <td nowrap>Play album</td>");
	html.add("      <td>The button <span class=\"glyphicon glyphicon-play\"></span> plays an album immediately.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-cd\"></span></td>");
	html.add("      <td nowrap>Show tracks</td>");
	html.add("      <td>The button <span class=\"glyphicon glyphicon-cd\"></span> shows the tracks of an album.<br>" \
					"Also clicking on the album cover art links to the track view.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-file\"></span></td>");
	html.add("      <td nowrap>Add album</td>");
	html.add("      <td>The button <span class=\"glyphicon glyphicon-file\"></span> adds a single album to the selected playlist.<br> " \
					"This is done with no further confirmation. The songs are added at the bottom of the playlist.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-duplicate\"></span></td>");
	html.add("      <td nowrap>Add artist</td>");
	html.add("      <td>The button <span class=\"glyphicon glyphicon-duplicate\"></span> adds a complete artist to the selected playlist.<br> " \
					"This is done with no further confirmation. The albums for the given artist are added at the bottom of the playlist.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}


void TResources::addPlayerStatusHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Player status information", anchor, padding, addHeader);

	std::string width = "88px";
	std::string libraryActionAnchor = "#anchor-6";
	std::string playerSettingsAnchor = "#anchor-9";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th width=\"" + width + "\">State</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-default\">Undefined</span></h5></td>");
	html.add("      <td>This is the state after starting the application when no valid state of the sound card or USB device is known.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-success\">Playing</span></h5></td>");
	html.add("      <td>This is the normal playing state. After selecting a song or an album to play " \
					"or just pressing the &quot;Play&quot; button <span class=\"glyphicon glyphicon-play\"></span> the mode is changing to &quot;Playing&quot;. " \
					"See <a href=\"" + libraryActionAnchor + "\">&quot;Library actions&quot;</a> for further help on playlist management.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-primary\">Paused</span></h5></td>");
	html.add("      <td>After pressing the &quot;Pause&quot; button <span class=\"glyphicon glyphicon-pause\"></span> the mode is changed to paused. " \
					"Changing the mode can be delayed, because buffered data will be played by the operating system until the mode change can take place.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-default\">Stopped</span></h5></td>");
	html.add("      <td>After pressing the &quot;Stop&quot; button <span class=\"glyphicon glyphicon-stop\"></span> the play back stops. " \
					"Stop play back can be delayed, because buffered data will be played by the operating system until the mode change can take place.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-default\">Closed</span></h5></td>");
	html.add("      <td>After pressing the &quot;Stop&quot; button <span class=\"glyphicon glyphicon-stop\"></span> <u>twice</u> the playback is first stopped and then the sound device is closed. " \
					"A stopped sound device will receive further data, the device is playing &quot;Silence&quot; until it is closed.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-warning\">Waiting</span></h5></td>");
	html.add("      <td>The system is waiting for data to be buffered. In the meantime &quot;Silence&quot; data is send to the sound device.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-warning\">Reopen</span></h5></td>");
	html.add("      <td>If the hardware parameters changes between 2 songs, the application must close the sound device to reopen it with the correct settings for the next song. " \
					"This may take a short time and the interim state is shown.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><h5 class=\"nomargins\"><span class=\"label label-default label-danger\">Failure</span></h5></td>");
	html.add("      <td>A failure on the sound device had occured. To reset this state press the &quot;Stop&quot; button <span class=\"glyphicon glyphicon-stop\"></span> " \
					"until the state shows &quot;Closed&quot;. Please check the output device settings. See <a href=\"" + playerSettingsAnchor + "\">&quot;Player settings&quot;</a> for further help on this topic. " \
					"Also the <a href=\"/app/system/messages.html\" title=\"System Message Viewer\">&quot;System Message Viewer&quot;</a> will contain some useful information about the nature of the problem.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}


void TResources::addPlayerControlHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Player mode controls", anchor, padding, addHeader);

	std::string width = "54px";
	std::string libraryActionAnchor = "#anchor-6";
	std::string playerSettingsAnchor = "#anchor-9";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th width=\"" + width + "\">Button</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><button title=\"Stop after current song\" class=\"btn btn-xs btn-default\"><span class=\"glyphicon glyphicon-record\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></button></td>");
	html.add("      <td>This button stops playback after finishing the current song. The stop mode disables itself after triggered once.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><button title=\"Randomize playlist or album\" class=\"btn btn-xs btn-default\"><span class=\"glyphicon glyphicon-random\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></button></td>");
	html.add("      <td>This button activates the shuffle mode. The songs of the current playlist are played in random order. If the disk mode (see below) is enabled, the songs of the current album are randomized.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><button title=\"Repeat playlist or album\" class=\"btn btn-xs btn-default\"><span class=\"glyphicon glyphicon-retweet\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></button></td>");
	html.add("      <td>This button activates the repeat mode. All songs of the current playlist are are repeated. If the disk mode (see below) is enabled, the songs of the current album are are repeated. " \
					"Repeat and random mode can be combined. Playback ends if the last song of the playlist or album is randomized and the repeat mode is disabled.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><button title=\"Repeat current song\" class=\"btn btn-xs btn-default\"><span class=\"glyphicon glyphicon-repeat\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></button></td>");
	html.add("      <td>This button activates the repeat mode for the current song. The current song will be repeated until doomsday.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><button title=\"Normal mode: Stop playback after last song of album&#10;Repeat mode: Repeat songs of current album only\" class=\"btn btn-xs btn-default\"><span class=\"glyphicon glyphicon-cd\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></button></td>");
	html.add("      <td>This button activates the disk mode. In normal mode playback ends with the last song of the album. In repeat or shuffle mode the songs of the current album are repeated or played in random order.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><button title=\"Play next random song\" class=\"btn btn-xs btn-default\"><span class=\"glyphicon glyphicon-chevron-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span></button></td>");
	html.add("      <td>This button is active in shuffle mode only and skips the current song to select the next random song for playback.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}


std::string TResources::getStreamHelp(size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Internet radio management</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>Internet radio stations can be streamed. For now <b>only MP3</b> streams are supported. " \
			 "Please note that media playing has to be stopped before starting an internet stream.</p>");
	html.add("</div>");

	size_t anchor = 0;
	addStreamActionsHelp(html, anchor, 0, false);
	addStreamInfoHelp(html, anchor, 0, false);
	addStreamCommentHelp(html, anchor, 0, false);

	return html.asString('\n');
}


void TResources::addStreamActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Internet radio management", anchor, padding, addHeader);

	std::string width = "36px";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Icon</th>");
	html.add("      <th>Action</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-play\"></span></td>");
	html.add("      <td nowrap>Play stream</td>");
	html.add("      <td>Start playing the selected stream. The button in the upper menu bar starts playing the last active stream. " \
					"If a stream is already playing, selecting a different stream to play stops the current one and starts the new selected stream. " \
					"<b>Please be patient, buffering a new stream can take a second.</b></td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-stop\"></span></td>");
	html.add("      <td nowrap>Stop stream</td>");
	html.add("      <td>Stop playing the active stream.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-plus\"></span></td>");
	html.add("      <td nowrap>New stream</td>");
	html.add("      <td>Create a new stream for a given name and URL.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-trash\"></span></td>");
	html.add("      <td nowrap>Delete stream</td>");
	html.add("      <td>Delete the selected stream from the list.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-pencil\"></span></td>");
	html.add("      <td nowrap>Edit stream</td>");
	html.add("      <td>Edit the stream settings. The name of the station and the URL can be changed at any time, but " \
					"changing the URL of the current playing stream has no effect until the stream is started next time.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-arrow-left\"></span></td>");
	html.add("      <td nowrap>Undo chages</td>");
	html.add("      <td>Undo the latest change. The &quot;undo list&quot; is limited to the last 5 changes. " \
					"This functions restores the values to ones before editing, deletes a newly created stream or restores a deleted entry. " \
					"The &quot;undo list&quot; clears itself after 60 seconds.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-option-horizontal\"></span></td>");
	html.add("      <td nowrap>Reorder stations</td>");
	html.add("      <td>When enabled the rows can be dragged upward or downward in the list. " \
					"Edit mode  is disabled by pressing the reorder button a secon time.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-music\"></span></td>");
	html.add("      <td nowrap>Export stations</td>");
	html.add("      <td>Export stations as extended M3U file.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addStreamInfoHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Internet radio management", anchor, padding, addHeader);

	html.add("<div class=\"bs-callout bs-callout-danger\">");
	html.add("<h4>Attention</h4>");
	html.add("Please also note that the stream can be interrupted by low bandwith or latencies of your internet access. " \
			 "Stream playback ist stopped after 5 unsuccessful retries to reestablish a connection to the selected stream. " \
			 "See the <a href=\"/app/system/messages.html\" title=\"System Message Viewer\">&quot;System Message Viewer&quot;</a> in case of stream interrupts.");
	html.add("</div>");
	html.add("</div>");
}

void TResources::addStreamCommentHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Internet radio management", anchor, padding, addHeader);

	html.add("<div class=\"bs-callout bs-callout-warning\">");
	html.add("<h4>Addendum</h4>");
	html.add("Common internet streaming software or internet radio devices do resample all streams from 44.1 or 48 kSamples/sec to the internal sample rate. " \
			 "This causes sonic degression in any case. This application sets the output stream to the nate sample rate of the input stream. " \
			 "This means the playback is as bit perfect as allowed by the input data. " \
			 "To compensate and to realign to the input stream drop outs are accepted to maintain the sonic quality, " \
			 "instead of injecting artificial samples or adjusting the resample rate.");
	html.add("</div>");
	html.add("</div>");
}


std::string TResources::getTracksHelp(int page, size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Track display and playlist management</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>The album track display allows direct access to play content and to create playlists. " \
			 "The context menu of the song table supports library actions to add a complete album <span class=\"glyphicon glyphicon-cd\"></span> " \
			 "or a single song <span class=\"glyphicon glyphicon-file\"></span> to a given playlist. " \
			 "A complete album can played by choosing the <span class=\"glyphicon glyphicon-play\"></span> menu entry, " \
			 "or just play a single song by choosing the <span class=\"glyphicon glyphicon-volume-up\"></span> menu entry.</p>");
	html.add("</div>");

	size_t anchor = 0;
	switch (page) {
		case 3000:
			addTrackActionsHelp(html, anchor, 0, true);
			addTrackLinksHelp(html, anchor, 0, true);
			break;
		case 3001:
			addTrackActionsHelp(html, anchor, 0, true);
			addTrackLinksHelp(html, anchor, 0, true);
			html.add("<div class=\"bs-callout bs-callout-primary\">");
			html.add("<h4>Addendum</h4>");
			html.add("The selected playlist can be saved with the icon &quot;Music&quot; icon <span class=\"glyphicon glyphicon-music\"></span> on the top right corner of the page. " \
					 "The format is <a href=\"https://en.wikipedia.org/wiki/M3U\" target=\"_blank\" title=\"M3U\">M3U</a>. " \
					 "When accessing the web interface from an external internet address, " \
					 "the playlist is created by using the external DNS hostname configured via <a href=\"/app/system/network.html\" title=\"Network Settings\">Network Settings</a>.");
			html.add("</div>");
			break;
		default:
			html.add("<div class=\"alert alert-danger\" role=\"alert\">");
			html.add("  <strong>Not found:</strong> Invalid help page index (" + std::to_string((size_s)page) + ")");
			html.add("</div>");
			break;
	}

	return html.asString('\n');
}


void TResources::addTrackActionsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Explanation of song context menu actions", anchor, padding, addHeader);

	std::string width = "36px";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Icon</th>");
	html.add("      <th>Action</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-play\"></span></td>");
	html.add("      <td nowrap>Play song</td>");
	html.add("      <td>The menu item &quot;Play song&quot; <span class=\"glyphicon glyphicon-play\"></span> plays the selected song immediately. " \
					"The song is added to the list of current songs under &quot;Recent songs...&quot; <span class=\"glyphicon glyphicon-music\"></span>.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-volume-up\"></span></td>");
	html.add("      <td nowrap>Play album</td>");
	html.add("      <td>The menu item &quot;Play album&quot; <span class=\"glyphicon glyphicon-volume-up\"></span> plays the first song of the selected album immediately. " \
					"All songs of the album are added to the list of current songs to be played next.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-play-circle\"></span></td>");
	html.add("      <td nowrap>Enqueue song</td>");
	html.add("      <td>The menu item &quot;Enqueue song&quot; <span class=\"glyphicon glyphicon-play-circle\"></span> marks the selected song as to be played after the current song ends. " \
					"The song is added to the list of current songs.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-ok-circle\"></span></td>");
	html.add("      <td nowrap>Enqueue album</td>");
	html.add("      <td>The menu item &quot;Enqueue album&quot; <span class=\"glyphicon glyphicon-ok-circle\"></span> marks the first song of the selected album to be played after the current song ends. " \
					"All songs of the album are added to the list of current songs and will be played next.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-file\"></span></td>");
	html.add("      <td nowrap>Add song</td>");
	html.add("      <td>The menu item &quot;Add song&quot; <span class=\"glyphicon glyphicon-file\"></span> adds the selected song to the current playlist. " \
					"Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding songs. " \
					"The song is added at the bottom of the selected playlist.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-cd\"></span></td>");
	html.add("      <td nowrap>Add album</td>");
	html.add("      <td>The menu item &quot;Add album&quot; <span class=\"glyphicon glyphicon-cd\"></span> adds the selected album to the current playlist. " \
					"Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding album. " \
					"The songs of the album are added at the bottom of the selected playlist.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-play\"></span></td>");
	html.add("      <td nowrap>Play song now</td>");
	html.add("      <td>The menu item &quot;Play song now&quot; <span class=\"glyphicon glyphicon-play\"></span> adds the selected song to the current playlist and starts playing the song immidiately. " \
					"Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding songs. " \
					"The song is added at the bottom of the selected playlist.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-volume-up\"></span></td>");
	html.add("      <td nowrap>Play album now</td>");
	html.add("      <td>The menu item &quot;Play album now&quot; <span class=\"glyphicon glyphicon-volume-up\"></span> adds the selected album to the current playlist and starts playing the first song of the album. " \
					"Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding album. " \
					"The songs of the album are added at the bottom of the selected playlist.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-play-circle\"></span></td>");
	html.add("      <td>Enqueue song to playlist</td>");
	html.add("      <td>The menu item &quot;Enqueue song to playlist&quot; <span class=\"glyphicon glyphicon-play-circle\"></span> marks the selected song as to be played after the current song ends. " \
					"The song is also added at the bottom of the current playlist." \
					"Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding songs.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-ok-circle\"></span></td>");
	html.add("      <td>Enqueue album to playlist</td>");
	html.add("      <td>The menu item &quot;Enqueue album to playlist&quot; <span class=\"glyphicon glyphicon-ok-circle\"></span> marks the first song of the selected album to be played after the current song ends. " \
					"The songs of the album are also added at the bottom of the current playlist." \
					"Remember to select a playlist via the main menu &quot;Playlists&quot; <span class=\"glyphicon glyphicon-file\"></span> before adding albums.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-remove\"></span></td>");
	html.add("      <td nowrap>Remove song</td>");
	html.add("      <td>The menu item &quot;Remove song&quot; <span class=\"glyphicon glyphicon-remove\"></span> removes the selected song from the current playlist.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-trash\"></span></td>");
	html.add("      <td nowrap>Remove album</td>");
	html.add("      <td>The menu item &quot;Remove album&quot; <span class=\"glyphicon glyphicon-trash\"></span> removes all songs selected album from the current playlist.</td>");
	html.add("    </tr>");

	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addTrackLinksHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Advanced song information", anchor, padding, addHeader);

	std::string width = "36px";

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Icon</th>");
	html.add("      <th>Action</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-th\"></span></td>");
	html.add("      <td nowrap>View artist</td>");
	html.add("      <td>The menu item &quot;View artist&quot; <span class=\"glyphicon glyphicon-th\"></span> shows all albums that contains the same artist. " \
					"This feature relies on the content of the &quot;Artist&quot; tag.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-pencil\"></span></td>");
	html.add("      <td nowrap>View composer</td>");
	html.add("      <td>The menu item &quot;View composer&quot; <span class=\"glyphicon glyphicon-pencil\"></span> shows all albums that contains the same composer. " \
					"This feature relies on the content of the &quot;Composer&quot; tag.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-user\"></span></td>");
	html.add("      <td nowrap>View conductor</td>");
	html.add("      <td>The menu item &quot;View conductor&quot; <span class=\"glyphicon glyphicon-user\"></span> shows all albums that contains the same conductor. " \
					"This feature relies on the content of the &quot;Conductor&quot; tag.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-music\"></span></td>");
	html.add("      <td nowrap>View albumartist</td>");
	html.add("      <td>The menu item &quot;View albumartist&quot; <span class=\"glyphicon glyphicon-music\"></span> shows all albums that contains the same albumartist. " \
					"When tagged as a compilation this menu item will show the albums corresponding to the various artist category. " \
					"This feature relies on the content of the &quot;Albumartist&quot; tag.</td>");
	html.add("    </tr>");

	html.add("    <tr>");
	html.add("      <td width=\"" + width + "\"><span class=\"glyphicon glyphicon-folder-open\"></td>");
	html.add("      <td>Browse file location</td>");
	html.add("      <td>The menu item &quot;Browse file location&quot; <span class=\"glyphicon glyphicon-folder-open\"></span>&nbsp;&nbsp;redirects to a directory view for the given file.</td>");
	html.add("    </tr>");

	html.add("  </tbody>");
	html.add("</table>");
}


std::string TResources::getSettingsHelp(int page, size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Hints to use settings pages</b></h4>");
	html.add("<div class=\"well\">");
	if (page < 1005) {
		html.add("<p>Please use &quot;Save settings&quot; button <span class=\"glyphicon glyphicon-check\"></span> at the bottom of the settings page to store changes permanently. "\
				"If you want to setup a new device or a different content folder, it is recommended to rescan the hardware with the button <span class=\"glyphicon glyphicon-refresh\"></span> on the left of the input field. " \
				"Usually a hardware rescan is done automatically when opening the settings page.</p>");
	} else {
		html.add("<p>Please use &quot;Save settings&quot; button <span class=\"glyphicon glyphicon-check\"></span> at the bottom of the settings page to store changes permanently.</p>");
	}
	html.add("</div>");
	//html.add("<h4><b>Explanation of settings</b></h4>");

	size_t anchor = 0;
	switch (page) {
		case 1005:
			addSystemSettingsHelp(html, anchor, 0, true);
			break;
		case 1002:
			addRemoteSettingsHelp(html, anchor, 0, true);
			html.add("<div class=\"bs-callout bs-callout-primary\">");
			html.add("<h4>Addendum</h4>");
			html.add("The &quot;Refresh&quot; icon <span class=\"glyphicon glyphicon-refresh\"></span> on the top right corner of the page can be used to send a status request via the serial port to the connected device " \
					 "(only if a valid serial device is selected and the serial port is enabled). " \
					 "The property display for word clock sample rate and selected input is refreshed according to the status response of the connected device. " \
					 "See the <a href=\"/app/system/messages.html\" title=\"System Message Viewer\">&quot;System Message Viewer&quot;</a> when experiencing problems with the status update request.");
			html.add("</div>");
			addVendorFootnote(html, padding);
			break;
		case 1001:
			addNetworkSettingsHelp(html, anchor, 0, true);
			addNetworkSharingHelp(html, anchor, 0, true);
			break;
		case 1000:
			addPlayerSettingsHelp(html, anchor, 0, true);
			html.add("<div class=\"bs-callout bs-callout-primary\">");
			html.add("<h4>Addendum</h4>");
			html.add("When using the &quot;Rescan&quot; buttons <span class=\"glyphicon glyphicon-refresh\"></span> or <span class=\"glyphicon glyphicon-repeat\"></span>, the progress is displayed on the label right to the button group. " \
					 "If there are any corrupt files found, a direct link to these files can be used by clicking on the label " \
					 "or use <a href=\"/app/system/erroneous.html\" title=\"Erroneous Library Items\">Erroneous Library Items</a> to show a table of the invalid files. " \
					 "Remember to do a fast rescan of the library content first.");
			html.add("</div>");
			break;
		default:
			html.add("<div class=\"alert alert-danger\" role=\"alert\">");
			html.add("  <strong>Not found:</strong> Invalid help page index (" + std::to_string((size_s)page) + ")");
			html.add("</div>");
			break;
	}

	return html.asString('\n');
}

void TResources::addPlayerSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Explanation of player settings", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Setting</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>Rescan library</td>");
	html.add("      <td>There are 3 options to rescan the library:");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>The left button <span class=\"glyphicon glyphicon-refresh\"></span> is doing a quick rescan for modified and newly added or deleted content</li>");
	html.add("          <li>The button in the middle <span class=\"glyphicon glyphicon-repeat\"></span> is doing a full rescan on the content folder an recreates the whole database</li>");
	html.add("          <li>The delete button <span class=\"glyphicon glyphicon-trash\"></span> removes all library entries</li>");
	html.add("        </ul>" \
					 "Usually it is sufficient to use the left button to do a quick content scan.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Audio device</td>");
	html.add("      <td>The list box entries consists of 2 parts:");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Hardware device, e.g. &quot;hw:CARD=d2,DEV=0&quot;</li>");
	html.add("          <li>Description for device in brackets</li>");
	html.add("        </ul>" \
					 "By saving the settings, only the hardware part is stored. " \
					 "It is recommended to choose devices marked with &quot;BitPerfect&quot; for best results.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Media mount</td>");
	html.add("      <td>This is the native mountpoint of the harddrive.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Library path</td>");
	html.add("      <td>The native mountpoint of the chosen device can be refined by the path of a content subfolder. " \
					   "<br><b>This path is used for library scans.</b></td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>File extensions</td>");
	html.add("      <td>These file extensions are used when executing a rescan of the library. All other files are ignored.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Watch for library changes</td>");
	html.add("      <td>This option enables an automatic watch for changes on the library. " \
					"In case changes are detected the information label rigth from &quot;Rescan library&quot; " \
					"changes the color to blue and &quot;Database update required&quot; is displayed. " \
					"Please do start a library update in that case. " \
					"<b>The library watch is limited to 8192 folders (~80000 songs) by the operating system.</b></td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Search result limit</td>");
	html.add("      <td>This limits the search results and the recently added file display to the given album count value.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Various artists categories</td>");
	html.add("      <td>This categories are the filters used to limit the various artists display " \
					"<a href=\"/app/library/artists.html?prepare=yes&title=artists&filter=2\"><span class=\"glyphicon glyphicon-menu-hamburger\"></span></a> " \
					"in the alphabetical artis view. " \
					"If the albumartist tag contains one of the given filters, then the album is grouped as a various artist.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addNetworkSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Host name settings", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Setting</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>DNS hostname</td>");
	html.add("      <td>This is the external DNS name (see <a href=\"https://en.wikipedia.org/wiki/Dynamic_DNS\" target=\"_blank\" title=\"Dynamic DNS\">&quot;Dynamic DNS&quot;</a>) " \
					"when forwarding the application web interface to the internet. " \
					"Please include the port number when non-standard port is used. " \
					"<br><b>This network name is used to generate extended M3U playlists on external access.</b></td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addRemoteSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Explanation of remote control settings", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Setting</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>Serial control port</td>");
	html.add("      <td>This is the serial device that is used to send remote control commands to a connected device (e.g. DAC). " \
				   "<br><b>For now only dCS<sup>&reg;</sup> Debussy DAC <sup><a href=\"#fn1\" id=\"ref1\">1)</a></sup> is supported.</b></td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Word clock device</td>");
	html.add("      <td>This is the USB device that is used to control an external word clock. " \
					"The device is opened for the sample rate given by the content to be played. " \
					"<br><b>For now dCS<sup>&reg;</sup> Puccini U-Clock <sup><a href=\"#fn1\" id=\"ref2\">1)</a></sup> is confirmed to work.</b></td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Enable word clock</td>");
	html.add("      <td>Enable or disable word clock usage. " \
					"Also the output sample rate for the word clock can be limited to the canonical frequency. " \
					"This means the lowest possible sample rate ist used (e.g. 48k instead of 192k). </td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addSystemSettingsHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Explanation of system configuration", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Setting</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>Use web sockets</td>");
	html.add("      <td>Web sockets are used to refresh the display instantly when needed. Some browsers, mostly on mobile devices, might cause problems when reactivated from standby mode. "\
					"Disable web sockets work around such display problems. If sockets are disabled, the display ist refreshed on a regular bisis by the refresh timer delay (see description below).</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Web refresh delay</td>");
	html.add("      <td>The different views are refreshed on a regular basis by the refresh timer delay. The delay is measured in milliseconds (1/1000 of a second). " \
					"The default value is 2000 ms, which should work for most common use cases.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Application refresh interval</td>");
	html.add("      <td>This interval is used for internal purposes in the application itself. This value is mesured in seconds. Do nor modify this value unless there is a good reason to do so.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Verbosity level</td>");
	html.add("      <td>The verbosity level controls the granularity of the web server log files. Lower values means less information. A value of 0 means no logging. " \
					"The default value is 0 and therefore no logs for web server requests are written.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

void TResources::addVendorFootnote(util::TStringList& html, size_t padding) {
	addPadding(html, padding);
	html.add("<sup id=\"fn1\">1) Debussy DAC and Puccini U-Clock are registered trademarks of " \
			 "<a href=\"https://www.dcsltd.co.uk/\" target=\"_blank\" title=\"Data Conversion Systems Ltd.\">Data Conversion Systems Ltd.</a> <a href=\"#ref1\" title=\"Goto footnote...\">↩</a></sup>");
}


std::string TResources::getExplorerHelp(size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Comments on file explorer view</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>The explorer view shows the file content for a given path. " \
			 "The full path is shown splittet in the subfolder parts at the top of the screen. " \
			 "By clicking on a fragment, the view is redirected to the subfolder. " \
			 "When selecting one file by clicking on the row, a dedicated action is executed (see table below for details):</p>");
	html.add("<ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("  <li>Show text files in browser</li>");
	html.add("  <li>Open image gallery viewer</li>");
	html.add("  <li>Play an audio file</li>");
	html.add("</ul>");
	html.add("<div class=\"spacer10\"></div>");
	html.add("<p>Audio files are played local on the external mobile device using an HTML5 based media player screen. " \
			 "<b>The supported audio codecs may vary on the browser and operating system used.</b></p>");
	html.add("</div>");

	size_t anchor = 0;
	addExplorerHelp(html, anchor, 0, true);

	return html.asString('\n');
}

void TResources::addExplorerHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Explorer actions for given file types", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>File type</th>");
	html.add("      <th>Action</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>FLAC, MP3, WAV, M4A</td>");
	html.add("      <td>Internal Media player is started to play the files.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>JPG, PNG</td>");
	html.add("      <td>Internal Image Gallery Viewer displays the images.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>TXT, LOG, CONF, JSON, SH, CPP, HPP, C, H, PDF</td>");
	html.add("      <td>Open a new browser window to show file content.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Other files</td>");
	html.add("      <td>A download is forced.</td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}

std::string TResources::getErroneousHelp(size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Comments on erroneous files</b></h4>");
	html.add("<div class=\"well\">");
	html.add("<p>There are many reasons why the file scanner can detect invalid or corrpt files. "\
			 "The most common reasons are corrupt metadata information in the binary file header or missing important ID3 tags (like &quot;Artist&quot;). " \
			 "The application tries to solve such issues as best as it can, but in some cases the missing information can't be reconstructed. " \
			 "Such files are displayed in the <a href=\"/app/system/erroneous.html\" title=\"Erroneous Library Items\">Erroneous Library Items</a> view.</p>");
	html.add("</div>");

	size_t anchor = 0;
	addErroneousExplanationHelp(html, anchor, 0, true);

	return html.asString('\n');
}

void TResources::addErroneousExplanationHelp(util::TStringList& html, size_t& anchor, size_t padding, bool addHeader) {
	addHeadline(html, "Explanation of table content for corrupt files", anchor, padding, addHeader);

	html.add("<table class=\"table table-bordered\">");
	html.add("  <thead>");
	html.add("    <tr>");
	html.add("      <th>Field</th>");
	html.add("      <th>Description</th>");
	html.add("    </tr>");
	html.add("  </thead>");
	html.add("  <tbody>");
	html.add("    <tr>");
	html.add("      <td>Error</td>");
	html.add("      <td>This is the internal error number that caused the file not to be parsed.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Filename</td>");
	html.add("      <td>This is the full filename. Clicking on the filename or the row in the table will redirect to a directory view for the given file.</td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Description</td>");
	html.add("      <td>This is a brief description why the parser had failed on the peticular file. In general there are 3 cases for the parser to fail:");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Errors in binary metadata like missing stream information</li>");
	html.add("          <li>Errors in ID3 or Apple Atoms caused by erroneous file tagging</li>");
	html.add("          <li>Unsupported codec or file type (e.g. multichannel surround sound data)</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("    </tr>");
	html.add("    <tr>");
	html.add("      <td>Reason</td>");
	html.add("      <td>This is the real reason why the processing of the file had failed. The corrupt or unsupported binary stream property is displayed here. " \
					   "Some common examples are:");
	html.add("        <ul style=\"padding-left: 16px; margin-bottom: 0px;\">");
	html.add("          <li>Sample Size = 0</li>");
	html.add("          <li>Sample Rate = 0</li>");
	html.add("          <li>Invalid Channel Count = 5</li>");
	html.add("        </ul>");
	html.add("      </td>");
	html.add("    </tr>");
	html.add("  </tbody>");
	html.add("</table>");
}


std::string TResources::getSessionInfo(const std::string& sid, size_t padding) {
	util::TStringList html;
	addPadding(html, padding);

	html.add("<h4><b>Session \"" + sid + "\"</b></h4>");
	if (util::assigned(sysdat.obj.webServer)) {
		util::TVariantValues values;
		sysdat.obj.webServer->getWebSessionValues(sid, values);
		if (!values.empty()) {
			util::TStringList list;
			values.asHTML(list);
			html.add(list);
			std::string address = values["REMOTE_HOST"].asString();
			if (!address.empty()) {
				if (!inet::isPrivateIPv4AddressRange(address)) {
					util::TStringList list;
					html.add("<h4><b>Whois \"" + address + "\"</b></h4>");
					if (!getWhoisResult(address, list)) {
						list.add("Lookup failed or missing response from Whois server");
					}
					size_t size = list.size();
					if (size < 2)
						size = 2;
					else if (size > 20)
						size = 20;
					html.add("<textarea name=\"txtWhoisResult\" id=\"txtWhoisResult\" class=\"form-control\" wrap=\"soft\" rows=\"" + std::to_string((size_u)size) + "\">" + list.asString('\n') + "</textarea>");
				}
			}
		} else {
			html.add("<div class=\"alert alert-danger\" role=\"alert\">");
			html.add("  <strong>Not found:</strong> Session no longer exists.");
			html.add("</div>");
		}
	}
	return html.asString('\n');
}


bool TResources::getWhoisResult(const std::string& address, util::TStringList& result) {
	result.clear();
	int retVal;
	std::string output;
	std::string commandLine = "/usr/bin/whois " + address;
	bool r = util::executeCommandLine(commandLine, output, retVal, true, 10);
	if (r && !output.empty()) {
		if (!output.empty()) {
			result.assign(output);
		}
	}
	return result.size() > 0;
}


} /* namespace app */

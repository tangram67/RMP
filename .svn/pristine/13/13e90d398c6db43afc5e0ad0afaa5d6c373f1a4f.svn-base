/*
 * mimetypes.cpp
 *
 *  Created on: 05.02.2015
 *      Author: Dirk Brinkmeier
 */

#include "mimetypes.h"
#include "fileutils.h"
#include "stringutils.h"
#include "compare.h"
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace util;

// See also http://www.iana.org/assignments/media-types/media-types.xhtml
TMimeMap fillMimeTypes()
{
	TMimeMap mimeTypes;

	mimeTypes.insert(TMimeItem("3dm",       "x-world/x-3dmf"));
	mimeTypes.insert(TMimeItem("3dmf",      "x-world/x-3dmf"));
	mimeTypes.insert(TMimeItem("a",         "application/octet-stream"));
	mimeTypes.insert(TMimeItem("aab",       "application/x-authorware-bin"));
	mimeTypes.insert(TMimeItem("aam",       "application/x-authorware-map"));
	mimeTypes.insert(TMimeItem("aas",       "application/x-authorware-seg"));
	mimeTypes.insert(TMimeItem("abc",       "text/vnd.abc"));
	mimeTypes.insert(TMimeItem("acgi",      HTML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("afl",       "video/animaflex"));
	mimeTypes.insert(TMimeItem("ai",        "application/postscript"));
	mimeTypes.insert(TMimeItem("aif",       "audio/aiff"));
	mimeTypes.insert(TMimeItem("aifc",      "audio/x-aiff"));
	mimeTypes.insert(TMimeItem("aiff",      "audio/aiff"));
	mimeTypes.insert(TMimeItem("aim",       "application/x-aim"));
	mimeTypes.insert(TMimeItem("aip",       "text/x-audiosoft-intra"));
	mimeTypes.insert(TMimeItem("ani",       "application/x-navi-animation"));
	mimeTypes.insert(TMimeItem("aos",       "application/x-nokia-9000-communicator-add-on-software"));
	mimeTypes.insert(TMimeItem("aps",       "application/mime"));
	mimeTypes.insert(TMimeItem("arc",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("arj",       "application/arj"));
	mimeTypes.insert(TMimeItem("art",       "image/x-jg"));
	mimeTypes.insert(TMimeItem("asf",       "video/x-ms-asf"));
	mimeTypes.insert(TMimeItem("asm",       "text/x-asm"));
	mimeTypes.insert(TMimeItem("asp",       "text/asp"));
	mimeTypes.insert(TMimeItem("asx",       "video/x-ms-asf"));
	mimeTypes.insert(TMimeItem("au",        "audio/basic"));
	mimeTypes.insert(TMimeItem("avi",       "video/avi"));
	mimeTypes.insert(TMimeItem("avs",       "video/avs-video"));
	mimeTypes.insert(TMimeItem("bcpio",     "application/x-bcpio"));
	mimeTypes.insert(TMimeItem("bin",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("bm",        BMP_MIME_TYPE));
	mimeTypes.insert(TMimeItem("bmp",       BMP_MIME_TYPE));
	mimeTypes.insert(TMimeItem("boo",       "application/book"));
	mimeTypes.insert(TMimeItem("book",      "application/book"));
	mimeTypes.insert(TMimeItem("boz",       "application/x-bzip2"));
	mimeTypes.insert(TMimeItem("bsh",       "application/x-bsh"));
	mimeTypes.insert(TMimeItem("bz",        "application/x-bzip"));
	mimeTypes.insert(TMimeItem("bz2",       "application/x-bzip2"));
	mimeTypes.insert(TMimeItem("c",         TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("c++",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("cat",       "application/vnd.ms-pki.seccat"));
	mimeTypes.insert(TMimeItem("cc",        TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("ccad",      "application/clariscad"));
	mimeTypes.insert(TMimeItem("cco",       "application/x-cocoa"));
	mimeTypes.insert(TMimeItem("cdf",       "application/cdf"));
	mimeTypes.insert(TMimeItem("cer",       "application/pkix-cert"));
	mimeTypes.insert(TMimeItem("cer",       "application/x-x509-ca-cert"));
	mimeTypes.insert(TMimeItem("cha",       "application/x-chat"));
	mimeTypes.insert(TMimeItem("chat",      "application/x-chat"));
	mimeTypes.insert(TMimeItem("class",     "application/java"));
	mimeTypes.insert(TMimeItem("com",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("conf",      TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("cpio",      "application/x-cpio"));
	mimeTypes.insert(TMimeItem("cpp",       TEXT_MIME_TYPE)); //"text/x-c"));
	mimeTypes.insert(TMimeItem("cpt",       "application/x-cpt"));
	mimeTypes.insert(TMimeItem("crl",       "application/pkcs-crl"));
	mimeTypes.insert(TMimeItem("crt",       "application/pkix-cert"));
	mimeTypes.insert(TMimeItem("csh",       "application/x-csh"));
	mimeTypes.insert(TMimeItem("css",       CSS_MIME_TYPE));
	mimeTypes.insert(TMimeItem("cxx",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("dcr",       "application/x-director"));
	mimeTypes.insert(TMimeItem("deepv",     "application/x-deepv"));
	mimeTypes.insert(TMimeItem("def",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("dff",       "audio/dff"));
	mimeTypes.insert(TMimeItem("dsf",       "audio/dsf"));
	mimeTypes.insert(TMimeItem("der",       "application/x-x509-ca-cert"));
	mimeTypes.insert(TMimeItem("dif",       "video/x-dv"));
	mimeTypes.insert(TMimeItem("dir",       "application/x-director"));
	mimeTypes.insert(TMimeItem("dl",        "video/dl"));
	mimeTypes.insert(TMimeItem("divx",      "video/x-msvideo"));
	mimeTypes.insert(TMimeItem("doc",       "application/msword"));
	mimeTypes.insert(TMimeItem("docx",      "application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
	mimeTypes.insert(TMimeItem("dot",       "application/msword"));
	mimeTypes.insert(TMimeItem("dp",        "application/commonground"));
	mimeTypes.insert(TMimeItem("drw",       "application/drafting"));
	mimeTypes.insert(TMimeItem("dump",      "application/octet-stream"));
	mimeTypes.insert(TMimeItem("dv",        "video/x-dv"));
	mimeTypes.insert(TMimeItem("dvi",       "application/x-dvi"));
	mimeTypes.insert(TMimeItem("dwf",       "model/vnd.dwf"));
	mimeTypes.insert(TMimeItem("dwg",       "image/vnd.dwg"));
	mimeTypes.insert(TMimeItem("dxf",       "image/vnd.dwg"));
	mimeTypes.insert(TMimeItem("dxr",       "application/x-director"));
	mimeTypes.insert(TMimeItem("el",        "text/x-script.elisp"));
	mimeTypes.insert(TMimeItem("elc",       "application/x-elc"));
	mimeTypes.insert(TMimeItem("env",       "application/x-envoy"));
	mimeTypes.insert(TMimeItem("eps",       "application/postscript"));
	mimeTypes.insert(TMimeItem("es",        "application/x-esrehber"));
	mimeTypes.insert(TMimeItem("etx",       "text/x-setext"));
	mimeTypes.insert(TMimeItem("evy",       "application/envoy"));
	mimeTypes.insert(TMimeItem("exe",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("f",         "text/x-fortran"));
	mimeTypes.insert(TMimeItem("f77",       "text/x-fortran"));
	mimeTypes.insert(TMimeItem("f90",       "text/x-fortran"));
	mimeTypes.insert(TMimeItem("fdf",       "application/vnd.fdf"));
	mimeTypes.insert(TMimeItem("fif",       "image/fif"));
	mimeTypes.insert(TMimeItem("flac",      "audio/flac"));
	mimeTypes.insert(TMimeItem("fli",       "video/fli"));
	mimeTypes.insert(TMimeItem("flo",       "image/florian"));
	mimeTypes.insert(TMimeItem("flv",       "video/x-flv"));
	mimeTypes.insert(TMimeItem("flx",       "text/vnd.fmi.flexstor"));
	mimeTypes.insert(TMimeItem("fmf",       "video/x-atomic3d-feature"));
	mimeTypes.insert(TMimeItem("for",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("for",       "text/x-fortran"));
	mimeTypes.insert(TMimeItem("fpx",       "image/vnd.fpx"));
	mimeTypes.insert(TMimeItem("frl",       "application/freeloader"));
	mimeTypes.insert(TMimeItem("funk",      "audio/make"));
	mimeTypes.insert(TMimeItem("g",         TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("g3",        "image/g3fax"));
	mimeTypes.insert(TMimeItem("gif",       "image/gif"));
	mimeTypes.insert(TMimeItem("gl",        "video/x-gl"));
	mimeTypes.insert(TMimeItem("gsd",       "audio/x-gsm"));
	mimeTypes.insert(TMimeItem("gsm",       "audio/x-gsm"));
	mimeTypes.insert(TMimeItem("gsp",       "application/x-gsp"));
	mimeTypes.insert(TMimeItem("gss",       "application/x-gss"));
	mimeTypes.insert(TMimeItem("gtar",      "application/x-gtar"));
	mimeTypes.insert(TMimeItem("gz",        "application/x-compressed"));
	mimeTypes.insert(TMimeItem("gzip",      "application/x-gzip"));
	mimeTypes.insert(TMimeItem("h",         TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("hdf",       "application/x-hdf"));
	mimeTypes.insert(TMimeItem("help",      "application/x-helpfile"));
	mimeTypes.insert(TMimeItem("hgl",       "application/vnd.hp-hpgl"));
	mimeTypes.insert(TMimeItem("hh",        TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("hlb",       "text/x-script"));
	mimeTypes.insert(TMimeItem("hlp",       "application/hlp"));
	mimeTypes.insert(TMimeItem("hpg",       "application/vnd.hp-hpgl"));
	mimeTypes.insert(TMimeItem("hpgl",      "application/vnd.hp-hpgl"));
	mimeTypes.insert(TMimeItem("hqx",       "application/binhex"));
	mimeTypes.insert(TMimeItem("hta",       "application/hta"));
	mimeTypes.insert(TMimeItem("htc",       "text/x-component"));
	mimeTypes.insert(TMimeItem("htm",       HTML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("html",      HTML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("htmls",     HTML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("htt",       "text/webviewhtml"));
	mimeTypes.insert(TMimeItem("htx",       HTML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("ice",       "x-conference/x-cooltalk"));
	mimeTypes.insert(TMimeItem("ico",       "image/x-icon"));
	mimeTypes.insert(TMimeItem("idc",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("ief",       "image/ief"));
	mimeTypes.insert(TMimeItem("iefs",      "image/ief"));
	mimeTypes.insert(TMimeItem("iges",      "application/iges"));
	mimeTypes.insert(TMimeItem("igs",       "application/iges"));
	mimeTypes.insert(TMimeItem("igs",       "model/iges"));
	mimeTypes.insert(TMimeItem("ima",       "application/x-ima"));
	mimeTypes.insert(TMimeItem("imap",      "application/x-httpd-imap"));
	mimeTypes.insert(TMimeItem("inf",       "application/inf"));
	mimeTypes.insert(TMimeItem("ins",       "application/x-internett-signup"));
	mimeTypes.insert(TMimeItem("ip",        "application/x-ip2"));
	mimeTypes.insert(TMimeItem("isu",       "video/x-isvideo"));
	mimeTypes.insert(TMimeItem("it",        "audio/it"));
	mimeTypes.insert(TMimeItem("iv",        "application/x-inventor"));
	mimeTypes.insert(TMimeItem("ivr",       "i-world/i-vrml"));
	mimeTypes.insert(TMimeItem("ivy",       "application/x-livescreen"));
	mimeTypes.insert(TMimeItem("jam",       "audio/x-jam"));
	mimeTypes.insert(TMimeItem("jav",       "text/x-java-source"));
	mimeTypes.insert(TMimeItem("java",      "text/x-java-source"));
	mimeTypes.insert(TMimeItem("jcm",       "application/x-java-commerce"));
	mimeTypes.insert(TMimeItem("jfif",      JPEG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("jfif-tbnl", JPEG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("jpe",       JPEG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("jpeg",      JPEG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("jpg",       JPEG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("jps",       "image/x-jps"));
	mimeTypes.insert(TMimeItem("js",        JAVA_MIME_TYPE));
	mimeTypes.insert(TMimeItem("json",      JSON_MIME_TYPE));
	mimeTypes.insert(TMimeItem("jut",       "image/jutvision"));
	mimeTypes.insert(TMimeItem("kar",       "music/x-karaoke"));
	mimeTypes.insert(TMimeItem("ksh",       "application/x-ksh"));
	mimeTypes.insert(TMimeItem("ksh",       "text/x-script.ksh"));
	mimeTypes.insert(TMimeItem("la",        "audio/nspaudio"));
	mimeTypes.insert(TMimeItem("lam",       "audio/x-liveaudio"));
	mimeTypes.insert(TMimeItem("latex",     "application/x-latex"));
	mimeTypes.insert(TMimeItem("lha",       "application/lha"));
	mimeTypes.insert(TMimeItem("lhx",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("list",      TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("lma",       "audio/nspaudio"));
	mimeTypes.insert(TMimeItem("log",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("lsp",       "application/x-lisp"));
	mimeTypes.insert(TMimeItem("lst",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("lsx",       "text/x-la-asf"));
	mimeTypes.insert(TMimeItem("ltx",       "application/x-latex"));
	mimeTypes.insert(TMimeItem("lzh",       "application/x-lzh"));
	mimeTypes.insert(TMimeItem("lzx",       "application/lzx"));
	mimeTypes.insert(TMimeItem("m",         "text/x-m"));
	mimeTypes.insert(TMimeItem("m1v",       "video/mpeg"));
	mimeTypes.insert(TMimeItem("m2a",       "audio/mpeg"));
	mimeTypes.insert(TMimeItem("m2v",       "video/mpeg"));
	mimeTypes.insert(TMimeItem("m2ts",      "video/mp2t"));
	mimeTypes.insert(TMimeItem("m3u",       M3U_MIME_TYPE));
	mimeTypes.insert(TMimeItem("man",       "application/x-troff-man"));
	mimeTypes.insert(TMimeItem("map",       "application/x-navimap"));
	mimeTypes.insert(TMimeItem("mar",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("mbd",       "application/mbedlet"));
	mimeTypes.insert(TMimeItem("mc$",       "application/x-magic-cap-package-1.0"));
	mimeTypes.insert(TMimeItem("mcd",       "application/x-mathcad"));
	mimeTypes.insert(TMimeItem("mcf",       "text/mcf"));
	mimeTypes.insert(TMimeItem("mcp",       "application/netmc"));
	mimeTypes.insert(TMimeItem("me",        "application/x-troff-me"));
	mimeTypes.insert(TMimeItem("mht",       "message/rfc822"));
	mimeTypes.insert(TMimeItem("mhtml",     "message/rfc822"));
	mimeTypes.insert(TMimeItem("mid",       "audio/midi"));
	mimeTypes.insert(TMimeItem("midi",      "audio/midi"));
	mimeTypes.insert(TMimeItem("mif",       "application/x-mif"));
	mimeTypes.insert(TMimeItem("mime",      "message/rfc822"));
	mimeTypes.insert(TMimeItem("mjf",       "audio/x-vnd.audioexplosion.mjuicemediafile"));
	mimeTypes.insert(TMimeItem("mjpg",      "video/x-motion-jpeg"));
	mimeTypes.insert(TMimeItem("mka",       "audio/x-matroska"));
	mimeTypes.insert(TMimeItem("mkv",       "video/x-matroska"));
	mimeTypes.insert(TMimeItem("mk3d",      "video/x-matroska-3d"));
	mimeTypes.insert(TMimeItem("mm",        "application/x-meme"));
	mimeTypes.insert(TMimeItem("mme",       "application/base64"));
	mimeTypes.insert(TMimeItem("mod",       "audio/mod"));
	mimeTypes.insert(TMimeItem("moov",      "video/quicktime"));
	mimeTypes.insert(TMimeItem("mov",       "video/quicktime"));
	mimeTypes.insert(TMimeItem("movie",     "video/x-sgi-movie"));
	mimeTypes.insert(TMimeItem("mp2",       "audio/mpeg"));
	mimeTypes.insert(TMimeItem("mp3",       MP3_MIME_TYPE));
	mimeTypes.insert(TMimeItem("mp4",       "video/mp4"));
	mimeTypes.insert(TMimeItem("mpa",       "audio/mpeg"));
	mimeTypes.insert(TMimeItem("mpc",       "application/x-project"));
	mimeTypes.insert(TMimeItem("mpe",       "video/mpeg"));
	mimeTypes.insert(TMimeItem("mpeg",      "video/mpeg"));
	mimeTypes.insert(TMimeItem("mpg",       "video/mpeg"));
	mimeTypes.insert(TMimeItem("mpga",      "audio/mpeg"));
	mimeTypes.insert(TMimeItem("mpp",       "application/vnd.ms-project"));
	mimeTypes.insert(TMimeItem("mpt",       "application/x-project"));
	mimeTypes.insert(TMimeItem("mpv",       "application/x-project"));
	mimeTypes.insert(TMimeItem("mpx",       "application/x-project"));
	mimeTypes.insert(TMimeItem("mrc",       "application/marc"));
	mimeTypes.insert(TMimeItem("ms",        "application/x-troff-ms"));
	mimeTypes.insert(TMimeItem("mv",        "video/x-sgi-movie"));
	mimeTypes.insert(TMimeItem("my",        "audio/make"));
	mimeTypes.insert(TMimeItem("mzz",       "application/x-vnd.audioexplosion.mzz"));
	mimeTypes.insert(TMimeItem("nap",       "image/naplps"));
	mimeTypes.insert(TMimeItem("naplps",    "image/naplps"));
	mimeTypes.insert(TMimeItem("nc",        "application/x-netcdf"));
	mimeTypes.insert(TMimeItem("ncm",       "application/vnd.nokia.configuration-message"));
	mimeTypes.insert(TMimeItem("nfo",       XML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("nif",       "image/x-niff"));
	mimeTypes.insert(TMimeItem("niff",      "image/x-niff"));
	mimeTypes.insert(TMimeItem("nix",       "application/x-mix-transfer"));
	mimeTypes.insert(TMimeItem("nsc",       "application/x-conference"));
	mimeTypes.insert(TMimeItem("nvd",       "application/x-navidoc"));
	mimeTypes.insert(TMimeItem("o",         "application/octet-stream"));
	mimeTypes.insert(TMimeItem("oda",       "application/oda"));
	mimeTypes.insert(TMimeItem("ogg",       OGG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("omc",       "application/x-omc"));
	mimeTypes.insert(TMimeItem("omcd",      "application/x-omcdatamaker"));
	mimeTypes.insert(TMimeItem("omcr",      "application/x-omcregerator"));
	mimeTypes.insert(TMimeItem("out",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("p",         "text/x-pascal"));
	mimeTypes.insert(TMimeItem("p10",       "application/pkcs10"));
	mimeTypes.insert(TMimeItem("p12",       "application/pkcs-12"));
	mimeTypes.insert(TMimeItem("p7a",       "application/x-pkcs7-signature"));
	mimeTypes.insert(TMimeItem("p7c",       "application/pkcs7-mime"));
	mimeTypes.insert(TMimeItem("p7m",       "application/pkcs7-mime"));
	mimeTypes.insert(TMimeItem("p7r",       "application/x-pkcs7-certreqresp"));
	mimeTypes.insert(TMimeItem("p7s",       "application/pkcs7-signature"));
	mimeTypes.insert(TMimeItem("part",      "application/pro_eng"));
	mimeTypes.insert(TMimeItem("pas",       "text/pascal"));
	mimeTypes.insert(TMimeItem("pbm",       "image/x-portable-bitmap"));
	mimeTypes.insert(TMimeItem("pcl",       "application/vnd.hp-pcl"));
	mimeTypes.insert(TMimeItem("pct",       "image/x-pict"));
	mimeTypes.insert(TMimeItem("pcx",       "image/x-pcx"));
	mimeTypes.insert(TMimeItem("pdb",       "chemical/x-pdb"));
	mimeTypes.insert(TMimeItem("pdf",       "application/pdf"));
	mimeTypes.insert(TMimeItem("pfunk",     "audio/make.my.funk"));
	mimeTypes.insert(TMimeItem("pgm",       "image/x-portable-greymap"));
	mimeTypes.insert(TMimeItem("pic",       "image/pict"));
	mimeTypes.insert(TMimeItem("pict",      "image/pict"));
	mimeTypes.insert(TMimeItem("pkg",       "application/x-newton-compatible-pkg"));
	mimeTypes.insert(TMimeItem("pko",       "application/vnd.ms-pki.pko"));
	mimeTypes.insert(TMimeItem("pl",        "text/x-script.perl"));
	mimeTypes.insert(TMimeItem("pls",       PLS_MIME_TYPE));
	mimeTypes.insert(TMimeItem("plx",       "application/x-pixclscript"));
	mimeTypes.insert(TMimeItem("pm",        "text/x-script.perl-module"));
	mimeTypes.insert(TMimeItem("pm4",       "application/x-pagemaker"));
	mimeTypes.insert(TMimeItem("pm5",       "application/x-pagemaker"));
	mimeTypes.insert(TMimeItem("png",       PNG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("pnm",       "application/x-portable-anymap"));
	mimeTypes.insert(TMimeItem("pot",       "application/vnd.ms-powerpoint"));
	mimeTypes.insert(TMimeItem("pov",       "model/x-pov"));
	mimeTypes.insert(TMimeItem("ppa",       "application/vnd.ms-powerpoint"));
	mimeTypes.insert(TMimeItem("ppm",       "image/x-portable-pixmap"));
	mimeTypes.insert(TMimeItem("pps",       "application/mspowerpoint"));
	mimeTypes.insert(TMimeItem("ppt",       "application/mspowerpoint"));
	mimeTypes.insert(TMimeItem("ppz",       "application/mspowerpoint"));
	mimeTypes.insert(TMimeItem("pre",       "application/x-freelance"));
	mimeTypes.insert(TMimeItem("prt",       "application/pro_eng"));
	mimeTypes.insert(TMimeItem("ps",        "application/postscript"));
	mimeTypes.insert(TMimeItem("psd",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("pvu",       "paleovu/x-pv"));
	mimeTypes.insert(TMimeItem("pwz",       "application/vnd.ms-powerpoint"));
	mimeTypes.insert(TMimeItem("py",        "text/x-script.phyton"));
	mimeTypes.insert(TMimeItem("pyc",       "applicaiton/x-bytecode.python"));
	mimeTypes.insert(TMimeItem("qcp",       "audio/vnd.qcelp"));
	mimeTypes.insert(TMimeItem("qd3",       "x-world/x-3dmf"));
	mimeTypes.insert(TMimeItem("qd3d",      "x-world/x-3dmf"));
	mimeTypes.insert(TMimeItem("qif",       "image/x-quicktime"));
	mimeTypes.insert(TMimeItem("qt",        "video/quicktime"));
	mimeTypes.insert(TMimeItem("qtc",       "video/x-qtc"));
	mimeTypes.insert(TMimeItem("qti",       "image/x-quicktime"));
	mimeTypes.insert(TMimeItem("qtif",      "image/x-quicktime"));
	mimeTypes.insert(TMimeItem("ra",        "audio/x-realaudio"));
	mimeTypes.insert(TMimeItem("ram",       "audio/x-pn-realaudio"));
	mimeTypes.insert(TMimeItem("ras",       "image/cmu-raster"));
	mimeTypes.insert(TMimeItem("rast",      "image/cmu-raster"));
	mimeTypes.insert(TMimeItem("rexx",      "text/x-script.rexx"));
	mimeTypes.insert(TMimeItem("rf",        "image/vnd.rn-realflash"));
	mimeTypes.insert(TMimeItem("rgb",       "image/x-rgb"));
	mimeTypes.insert(TMimeItem("rm",        "audio/x-pn-realaudio"));
	mimeTypes.insert(TMimeItem("rmi",       "audio/mid"));
	mimeTypes.insert(TMimeItem("rmm",       "audio/x-pn-realaudio"));
	mimeTypes.insert(TMimeItem("rmp",       "audio/x-pn-realaudio"));
	mimeTypes.insert(TMimeItem("rng",       "application/ringing-tones"));
	mimeTypes.insert(TMimeItem("rnx",       "application/vnd.rn-realplayer"));
	mimeTypes.insert(TMimeItem("roff",      "application/x-troff"));
	mimeTypes.insert(TMimeItem("rp",        "image/vnd.rn-realpix"));
	mimeTypes.insert(TMimeItem("rpm",       "audio/x-pn-realaudio-plugin"));
	mimeTypes.insert(TMimeItem("rt",        "text/richtext"));
	mimeTypes.insert(TMimeItem("rtf",       "text/richtext"));
	mimeTypes.insert(TMimeItem("rtx",       "text/richtext"));
	mimeTypes.insert(TMimeItem("rv",        "video/vnd.rn-realvideo"));
	mimeTypes.insert(TMimeItem("s",         "text/x-asm"));
	mimeTypes.insert(TMimeItem("s3m",       "audio/s3m"));
	mimeTypes.insert(TMimeItem("saveme",    "application/octet-stream"));
	mimeTypes.insert(TMimeItem("sbk",       "application/x-tbook"));
	mimeTypes.insert(TMimeItem("scm",       "video/x-scm"));
	mimeTypes.insert(TMimeItem("sdml",      TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("sdp",       "application/sdp"));
	mimeTypes.insert(TMimeItem("sdr",       "application/sounder"));
	mimeTypes.insert(TMimeItem("sea",       "application/sea"));
	mimeTypes.insert(TMimeItem("set",       "application/set"));
	mimeTypes.insert(TMimeItem("sgm",       "text/sgml"));
	mimeTypes.insert(TMimeItem("sgml",      "text/sgml"));
	mimeTypes.insert(TMimeItem("sh",        "text/x-script.sh"));
	mimeTypes.insert(TMimeItem("shar",      "application/x-bsh"));
	mimeTypes.insert(TMimeItem("shtml",     HTML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("shtml",     "text/x-server-parsed-html"));
	mimeTypes.insert(TMimeItem("sid",       "audio/x-psid"));
	mimeTypes.insert(TMimeItem("sit",       "application/x-sit"));
	mimeTypes.insert(TMimeItem("sit",       "application/x-stuffit"));
	mimeTypes.insert(TMimeItem("skd",       "application/x-koan"));
	mimeTypes.insert(TMimeItem("skm",       "application/x-koan"));
	mimeTypes.insert(TMimeItem("skp",       "application/x-koan"));
	mimeTypes.insert(TMimeItem("skt",       "application/x-koan"));
	mimeTypes.insert(TMimeItem("sl",        "application/x-seelogo"));
	mimeTypes.insert(TMimeItem("smi",       "application/smil"));
	mimeTypes.insert(TMimeItem("smil",      "application/smil"));
	mimeTypes.insert(TMimeItem("snd",       "audio/basic"));
	mimeTypes.insert(TMimeItem("sol",       "application/solids"));
	mimeTypes.insert(TMimeItem("spc",       "text/x-speech"));
	mimeTypes.insert(TMimeItem("spl",       "application/futuresplash"));
	mimeTypes.insert(TMimeItem("spr",       "application/x-sprite"));
	mimeTypes.insert(TMimeItem("sprite",    "application/x-sprite"));
	mimeTypes.insert(TMimeItem("src",       "application/x-wais-source"));
	mimeTypes.insert(TMimeItem("ssi",       "text/x-server-parsed-html"));
	mimeTypes.insert(TMimeItem("ssm",       "application/streamingmedia"));
	mimeTypes.insert(TMimeItem("sst",       "application/vnd.ms-pki.certstore"));
	mimeTypes.insert(TMimeItem("step",      "application/step"));
	mimeTypes.insert(TMimeItem("stl",       "application/sla"));
	mimeTypes.insert(TMimeItem("stp",       "application/step"));
	mimeTypes.insert(TMimeItem("sv4cpio",   "application/x-sv4cpio"));
	mimeTypes.insert(TMimeItem("sv4crc",    "application/x-sv4crc"));
	mimeTypes.insert(TMimeItem("svf",       "image/vnd.dwg"));
	mimeTypes.insert(TMimeItem("svr",       "application/x-world"));
	mimeTypes.insert(TMimeItem("swf",       "application/x-shockwave-flash"));
	mimeTypes.insert(TMimeItem("t",         "application/x-troff"));
	mimeTypes.insert(TMimeItem("talk",      "text/x-speech"));
	mimeTypes.insert(TMimeItem("tar",       "application/x-tar"));
	mimeTypes.insert(TMimeItem("tbk",       "application/toolbook"));
	mimeTypes.insert(TMimeItem("tcl",       "text/x-script.tcl"));
	mimeTypes.insert(TMimeItem("tcsh",      "text/x-script.tcsh"));
	mimeTypes.insert(TMimeItem("tex",       "application/x-tex"));
	mimeTypes.insert(TMimeItem("texi",      "application/x-texinfo"));
	mimeTypes.insert(TMimeItem("texinfo",   "application/x-texinfo"));
	mimeTypes.insert(TMimeItem("text",      TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("tgz",       "application/x-compressed"));
	mimeTypes.insert(TMimeItem("tif",       "image/tiff"));
	mimeTypes.insert(TMimeItem("tiff",      "image/tiff"));
	mimeTypes.insert(TMimeItem("tr",        "application/x-troff"));
	mimeTypes.insert(TMimeItem("ts",        "video/mp2t"));
	mimeTypes.insert(TMimeItem("tsi",       "audio/tsp-audio"));
	mimeTypes.insert(TMimeItem("tsp",       "audio/tsplayer"));
	mimeTypes.insert(TMimeItem("tsv",       "text/tab-separated-values"));
	mimeTypes.insert(TMimeItem("turbot",    "image/florian"));
	mimeTypes.insert(TMimeItem("txt",       TEXT_MIME_TYPE));
	mimeTypes.insert(TMimeItem("uil",       "text/x-uil"));
	mimeTypes.insert(TMimeItem("uni",       "text/uri-list"));
	mimeTypes.insert(TMimeItem("unis",      "text/uri-list"));
	mimeTypes.insert(TMimeItem("unv",       "application/i-deas"));
	mimeTypes.insert(TMimeItem("uri",       "text/uri-list"));
	mimeTypes.insert(TMimeItem("uris",      "text/uri-list"));
	mimeTypes.insert(TMimeItem("ustar",     "application/x-ustar"));
	mimeTypes.insert(TMimeItem("uu",        "text/x-uuencode"));
	mimeTypes.insert(TMimeItem("uue",       "text/x-uuencode"));
	mimeTypes.insert(TMimeItem("vcd",       "application/x-cdlink"));
	mimeTypes.insert(TMimeItem("vcs",       "text/x-vcalendar"));
	mimeTypes.insert(TMimeItem("vda",       "application/vda"));
	mimeTypes.insert(TMimeItem("vdo",       "video/vdo"));
	mimeTypes.insert(TMimeItem("vew",       "application/groupwise"));
	mimeTypes.insert(TMimeItem("viv",       "video/vivo"));
	mimeTypes.insert(TMimeItem("vivo",      "video/vivo"));
	mimeTypes.insert(TMimeItem("vmd",       "application/vocaltec-media-desc"));
	mimeTypes.insert(TMimeItem("vmf",       "application/vocaltec-media-file"));
	mimeTypes.insert(TMimeItem("voc",       "audio/voc"));
	mimeTypes.insert(TMimeItem("vos",       "video/vosaic"));
	mimeTypes.insert(TMimeItem("vox",       "audio/voxware"));
	mimeTypes.insert(TMimeItem("vqe",       "audio/x-twinvq-plugin"));
	mimeTypes.insert(TMimeItem("vqf",       "audio/x-twinvq"));
	mimeTypes.insert(TMimeItem("vql",       "audio/x-twinvq-plugin"));
	mimeTypes.insert(TMimeItem("vrml",      "application/x-vrml"));
	mimeTypes.insert(TMimeItem("vrt",       "x-world/x-vrt"));
	mimeTypes.insert(TMimeItem("vsd",       "application/x-visio"));
	mimeTypes.insert(TMimeItem("vst",       "application/x-visio"));
	mimeTypes.insert(TMimeItem("vsw",       "application/x-visio"));
	mimeTypes.insert(TMimeItem("w60",       "application/wordperfect6.0"));
	mimeTypes.insert(TMimeItem("w61",       "application/wordperfect6.1"));
	mimeTypes.insert(TMimeItem("w6w",       "application/msword"));
	mimeTypes.insert(TMimeItem("wav",       "audio/wav"));
	mimeTypes.insert(TMimeItem("wb1",       "application/x-qpro"));
	mimeTypes.insert(TMimeItem("wbmp",      "image/vnd.wap.wbmp"));
	mimeTypes.insert(TMimeItem("web",       "application/vnd.xara"));
	mimeTypes.insert(TMimeItem("wiz",       "application/msword"));
	mimeTypes.insert(TMimeItem("wk1",       "application/x-123"));
	mimeTypes.insert(TMimeItem("wma",       "audio/x-ms-wma"));
	mimeTypes.insert(TMimeItem("wmf",       "windows/metafile"));
	mimeTypes.insert(TMimeItem("wml",       "text/vnd.wap.wml"));
	mimeTypes.insert(TMimeItem("wmlc",      "application/vnd.wap.wmlc"));
	mimeTypes.insert(TMimeItem("wmls",      "text/vnd.wap.wmlscript"));
	mimeTypes.insert(TMimeItem("wmlsc",     "application/vnd.wap.wmlscriptc"));
	mimeTypes.insert(TMimeItem("wmv",       "video/x-ms-wmv"));
	mimeTypes.insert(TMimeItem("woff",      "application/x-font-woff"));
	mimeTypes.insert(TMimeItem("woff2",     "application/x-font-woff2"));
	mimeTypes.insert(TMimeItem("word",      "application/msword"));
	mimeTypes.insert(TMimeItem("wp",        "application/wordperfect"));
	mimeTypes.insert(TMimeItem("wp5",       "application/wordperfect"));
	mimeTypes.insert(TMimeItem("wp6",       "application/wordperfect"));
	mimeTypes.insert(TMimeItem("wpd",       "application/wordperfect"));
	mimeTypes.insert(TMimeItem("wq1",       "application/x-lotus"));
	mimeTypes.insert(TMimeItem("wri",       "application/mswrite"));
	mimeTypes.insert(TMimeItem("wrl",       "model/vrml"));
	mimeTypes.insert(TMimeItem("wrz",       "model/vrml"));
	mimeTypes.insert(TMimeItem("wsc",       "text/scriplet"));
	mimeTypes.insert(TMimeItem("wsrc",      "application/x-wais-source"));
	mimeTypes.insert(TMimeItem("wtk",       "application/x-wintalk"));
	mimeTypes.insert(TMimeItem("xbm",       "image/xbm"));
	mimeTypes.insert(TMimeItem("xdr",       "video/x-amt-demorun"));
	mimeTypes.insert(TMimeItem("xgz",       "xgl/drawing"));
	mimeTypes.insert(TMimeItem("xif",       "image/vnd.xiff"));
	mimeTypes.insert(TMimeItem("xl",        "application/excel"));
	mimeTypes.insert(TMimeItem("xla",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlb",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlc",       "application/excel"));
	mimeTypes.insert(TMimeItem("xld",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlk",       "application/excel"));
	mimeTypes.insert(TMimeItem("xll",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlm",       "application/excel"));
	mimeTypes.insert(TMimeItem("xls",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlsx",      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
	mimeTypes.insert(TMimeItem("xlt",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlv",       "application/excel"));
	mimeTypes.insert(TMimeItem("xlw",       "application/excel"));
	mimeTypes.insert(TMimeItem("xm",        "audio/xm"));
	mimeTypes.insert(TMimeItem("xml",       XML_MIME_TYPE));
	mimeTypes.insert(TMimeItem("xmz",       "xgl/movie"));
	mimeTypes.insert(TMimeItem("xpix",      "application/x-vnd.ls-xpix"));
	mimeTypes.insert(TMimeItem("xpm",       "image/xpm"));
	mimeTypes.insert(TMimeItem("x-png",     PNG_MIME_TYPE));
	mimeTypes.insert(TMimeItem("xspf",      XSPF_MIME_TYPE));
	mimeTypes.insert(TMimeItem("xsr",       "video/x-amt-showrun"));
	mimeTypes.insert(TMimeItem("xvid",      "video/x-msvideo"));
	mimeTypes.insert(TMimeItem("xwd",       "image/x-xwd"));
	mimeTypes.insert(TMimeItem("xyz",       "chemical/x-pdb"));
	mimeTypes.insert(TMimeItem("z",         "application/x-compressed"));
	mimeTypes.insert(TMimeItem("zip",       "application/zip"));
	mimeTypes.insert(TMimeItem("zoo",       "application/octet-stream"));
	mimeTypes.insert(TMimeItem("zsh",       "text/x-script.zsh"));

	return mimeTypes;
}

static TMimeMap mimeTypeListLocl = fillMimeTypes();
static TMimeMap mimeTypeListExtrn;

std::string util::getMimeType(const std::string& extension) {
	if (!extension.empty()) {
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		TMimeMap::const_iterator it = mimeTypeListLocl.find(ext);
		if (it != mimeTypeListLocl.end())
			return it->second;

		it = mimeTypeListExtrn.find(ext);
		if (it != mimeTypeListExtrn.end())
			return it->second;

		// No type found --> type = application/x-...
		return "application/x-" + ext;
	}
	return DEFAULT_MIME_TYPE;
}

bool util::isMimeType(const std::string& extension, const char* mimetype) {
	if (!extension.empty() && util::assigned(mimetype)) {
		std::string mime = getMimeType(extension);
		if (0 == util::strcasecmp(mime, mimetype))
			return true;
	}
	return false;
}

bool util::isMimeType(const std::string& extension, const std::string& mimetype) {
	if (!extension.empty() && !mimetype.empty()) {
		std::string mime = getMimeType(extension);
		if (0 == util::strcasecmp(mime, mimetype))
			return true;
	}
	return false;
}

std::string util::getMimeType(const util::TFile& file) {
	std::string ext = file.getExtension();
	return getMimeType(ext);
}


int util::loadMimeTypesFromFile(const std::string& fileName) {
	mimeTypeListExtrn.clear();
	if (util::fileExists(fileName)) {
		size_t i;
		std::ifstream strm;
		std::string line;
		app::TStringVector sl;
		strm.open(fileName, std::ios_base::in);
		try {
			// Read all lines until EOF
			while (strm.good()) {
				std::getline(strm, line);
				util::trim(line);
				if (!line.empty()) {
					// Ignore comment lines
					if (line.find_first_of('#') == std::string::npos) {
						util::split(line, sl);
						if (sl.size() > 1) {
							if (sl[0].size()) {
								// Allow multiple allocations like: application/postscript  ps ai eps epsi epsf eps2 eps3
								std::string mime(sl[0]);
								std::transform(mime.begin(), mime.end(), mime.begin(), ::tolower);
								for (i=1; i<sl.size(); i++) {
									if (sl[i].size()) {
										std::string ext(sl[i]);
										std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
										mimeTypeListExtrn.insert(TMimeItem(ext, mime));
									}
								}
							}
						}
					}
				}
			}
		} catch (const std::exception& e)	{
			strm.close();
			throw e;
		}
		strm.close();
	}
	return mimeTypeListExtrn.size();
}

const TMimeMap& util::getMimeMapLocal() {
	return mimeTypeListLocl;
}

const TMimeMap& util::getMimeMapExtrn() {
	return mimeTypeListExtrn;
}

bool util::getAudioCodec(const std::string extension, std::string& type, std::string& codec) {
	const struct TWebMediaType *it = multimediatypes;
	for (; util::assigned(it->extension); ++it) {
		if (0 == util::strncasecmp(extension, it->extension, extension.size())) {
			type = it->type;
			codec = util::assigned(it->codec) ? util::quote(it->codec) : "";
			return true;
		}
	}
	return false;
}

/*
 * componenttypes.h
 *
 *  Created on: 12.02.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef COMPONENTTYPES_H_
#define COMPONENTTYPES_H_

#include <vector>
#include "gcc.h"
#include "templates.h"

namespace html {

enum EComponentStyle {
	ECS_TEXT,
	ECS_HTML,
	ECS_DEFAULT = ECS_HTML
};

enum EComponentSize {
	ESZ_XSMALL,
	ESZ_SMALL,
	ESZ_MEDIUM,
	ESZ_LARGE,
	ESZ_CUSTOM,
	ESZ_DEFAULT = ESZ_LARGE
};

enum EComponentType {
	ECT_DEFAULT,
	ECT_PRIMARY,
	ECT_SUCCESS,
	ECT_INFO,
	ECT_WARNING,
	ECT_DANGER
};

enum EComponentAlign {
	ECA_NONE,
	ECA_LEFT,
	ECA_CENTER,
	ECA_RIGHT,
	ECA_DEFAULT = ECA_NONE
};

enum EListFocus {
	ELF_NONE,
	ELF_PARTIAL,
	ELF_FULL,
	ELF_DEFAULT = ELF_NONE
};

class TMenuItem;
class TMainMenuItem;
class TContextMenuItem;
class TTileItem;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PMenuItem = TMenuItem*;
using TMenuItemList = std::vector<PMenuItem>;
using PMainMenuItem = TMainMenuItem*;
using TMainMenuItemList = std::vector<PMainMenuItem>;
using PContextMenuItem = TContextMenuItem*;
using TContextMenuItemList = std::vector<PContextMenuItem>;
using PTileItem = TTileItem*;
using TTileItemList = std::vector<PTileItem>;

#else

typedef TMenuItem* PMenuItem;
typedef std::vector<PMenuItem> TMenuItemList;
typedef TMainMenuItem* PMainMenuItem;
typedef std::vector<PMainMenuItem> TMainMenuItemList;
typedef TContextMenuItem* PContextMenuItem;
typedef std::vector<PContextMenuItem> TContextMenuItemList;
typedef TTileItem* PTileItem;
typedef std::vector<PTileItem> TTileItemList;

#endif

} /* namespace htlm */

#endif /* COMPONENTTYPES_H_ */

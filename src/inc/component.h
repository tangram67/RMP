/*
 * component.h
 *
 *  Created on: 11.02.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "classes.h"
#include "templates.h"
#include "componenttypes.h"
#include "stringutils.h"
#include "translation.h"
#include "webtypes.h"
#include "webtoken.h"

namespace html {

class TPersistent {
private:
	bool sanitized;

public:
	void setSanitized(const bool value) { sanitized = value; };
	bool getSanitized() const { return sanitized; };

	std::string encodeHtmlText(const std::string& str) const;

	TPersistent();
	virtual ~TPersistent();
};


class THtmlList : public util::TStringList, protected TPersistent {
protected:
	mutable std::string strHTML;

public:
	void clear();
	void invalidate();

	THtmlList();
	virtual ~THtmlList();
};

class TTagList : public THtmlList {
private:
	mutable	util::hash_type hashTag;
	mutable	app::TStringVector icons;
	mutable TItemTagList tags;
	bool addEmptyDefault;
	std::string tag;

	void prime();

public:
	void setTag(const std::string& tag);
	const std::string& getTag() const { return tag; };
	void allowEmptyDefault(const bool value) { addEmptyDefault = value; };
	bool isEmptyDefaultAllowed() const { return addEmptyDefault; };

	const std::string& html(const std::string caption = "") const;
	const app::TStringVector& getIcons() const { return icons; };
	const TItemTagList& getTags() const { return tags; };
	void invalidate();


	void add(const std::string& text, size_t tag = 0);
	void add(const util::TStringList& items);
	void add(const std::string& text, const std::string& glyphicon, size_t tag = 0);
	void clear();

	TTagList& operator = (const app::TStringVector& vector);
	TTagList& operator = (const TStringList& list);
	TTagList& operator = (const std::string& csv);

	TTagList();
	TTagList(const std::string& tag);
	virtual ~TTagList();
};

class TRadioButtons : public THtmlList {
public:
	void assign(const util::TStringList& list);
	const std::string& html(const std::string& checked = "") const;

	TRadioButtons();
	virtual ~TRadioButtons();
};

class TComponent : public app::TObject, protected TPersistent {
public:
	typedef app::TStringVector::iterator iterator;
	typedef app::TStringVector::const_iterator const_iterator;

private:
	app::PTranslator nls;
	app::PWebToken token;
	std::string nstr;
	std::string id;

protected:
	mutable std::string strHTML;
	mutable std::string strText;
	mutable TTagList items;
	util::ECompareType compare;
	EComponentStyle style;
	EComponentAlign align;
	EComponentSize size;
	EComponentType type;
	EListFocus focus;
	size_t width;
	std::string glyphicon;
	std::string onclick;
	std::string hint;
	bool invalidated;
	bool enabled;

	std::string padTextRight(const std::string &str, const size_t length, const std::string padding = "&nbsp;") const;
	std::string padTextLeft(const std::string &str, const size_t length, const std::string padding = "&nbsp;") const;

	bool isJavascript(const std::string& prototype) const;
	std::string tagToStr(const std::string& tag, const std::string& value) const;
	std::string tagToStr(const std::string& tag, const size_t value) const;
	std::string disbledToStr() const;
	app::PWebServer getServer() const;

	bool addWebToken();

	// Access string entries of component in derived classes
	inline const_iterator begin() const { return items.begin(); };
	inline const_iterator end() const { return items.end(); };
	inline const_iterator first() const { return begin(); };
	inline const_iterator last() const { return util::pred(end()); };
	inline size_t count() const { return items.size(); };
	inline TTagList& elements() { return items; };

	std::string translate(const size_t id, const std::string& defValue) const;

public:
	// HTML properties like id, name and text
	void setName(const std::string& name);
	void setID(const std::string& id) { this->id = id; };
	const std::string& getID() const { return id.empty() ? getName() : id; };
	void setCaption(const std::string& caption);
	const std::string& getCaption() const { return items.size() > 0 ? items[0] : nstr; };
	void setStyle(const EComponentStyle value);
	EComponentStyle getStyle() const { return style; };
	void setAlign(const EComponentAlign value);
	EComponentAlign getAlign() const { return align; };
	void setSize(const EComponentSize value);
	EComponentSize getSize() const { return size; };
	void setType(const EComponentType value);
	size_t getWidth() const { return width; };
	void setWidth(const size_t value);
	EComponentType getType() const { return type; };
	void setFocus(const EListFocus value);
	EListFocus getFocus() const { return focus; };
	void setCompare(const util::ECompareType value);
	util::ECompareType getCompare() const { return compare; };
	void setGlyphicon(const std::string& name);
	const std::string& getGlyphicon() const { return glyphicon; };
	void setHint(const std::string& tooltip);
	const std::string& getHint() const { return hint; };
	void setClick(const std::string& action);
	const std::string& getClick() const { return onclick; };
	void setEnabled(const bool value);
	bool isEnabled() const { return enabled; };
	void setTranslator(app::TTranslator& nls);
	bool hasTranslator() const { return util::assigned(nls); };
	bool isInvalidated() const { return invalidated; };

	// Let derived classes override string methods
	virtual const std::string& text() const;
	virtual const std::string& html() const;
	virtual const std::string& text(const std::string& caption) const;
	virtual const std::string& html(const std::string& caption) const;
	virtual void invalidate();

	virtual void update(const std::string& caption);
	virtual void update();

	void clear();

	TComponent();
	virtual ~TComponent();
};

class TComboBox : public TComponent {
private:
	void prime();
	void setCaption(const std::string& caption) {};

public:
	// Allow access to protected string properties of TComponent
#ifdef STL_HAS_TEMPLATE_ALIAS
	using TComponent::begin;
	using TComponent::end;
	using TComponent::first;
	using TComponent::last;
	using TComponent::count;
	using TComponent::elements;
#else
	inline const_iterator begin() const { return TComponent::begin(); };
	inline const_iterator end() const { return TComponent::end(); };
	inline const_iterator first() const { return TComponent::first(); };
	inline const_iterator last() const { return TComponent::last(); };
	inline size_t count() const { return TComponent::count(); };
	inline util::TStringList& elements() { return TComponent::elements(); };
#endif

	const std::string& html(const std::string& caption) const;
	util::TStringList& operator () () { return TComponent::elements(); };

	TComboBox();
	TComboBox(const std::string& name);
	virtual ~TComboBox();
};


class TListBox : public TComponent {
private:
	util::TStringList component;

	void prime();
	bool compare(const std::string& s1, const std::string& s2) const;
	void setCaption(const std::string& caption) {};

public:
	// Allow access to protected string properties of TComponent
#ifdef STL_HAS_TEMPLATE_ALIAS
	using TComponent::begin;
	using TComponent::end;
	using TComponent::first;
	using TComponent::last;
	using TComponent::count;
	using TComponent::elements;
#else
	inline const_iterator begin() const { return TComponent::begin(); };
	inline const_iterator end() const { return TComponent::end(); };
	inline const_iterator first() const { return TComponent::first(); };
	inline const_iterator last() const { return TComponent::last(); };
	inline size_t count() const { return TComponent::count(); };
	inline util::TStringList& elements() { return TComponent::elements(); };
#endif

	const std::string& text(const std::string& caption) const;
	const std::string& html(const std::string& caption) const;
	util::TStringList& operator () () { return TComponent::elements(); };
	void addComponent(const util::TStringList& html);

	TListBox();
	TListBox(const std::string& name);
	virtual ~TListBox();
};


class TButton : public TComponent {
private:
	bool defclick;
	std::string value;
	std::string css;

	void prime();
	void invalidate();

	std::string typeToClass() const;
	std::string sizeToClass() const;
	std::string clickToStr() const;
	std::string getClass() const;

public:
	void setDefaultClick(const bool value);
	bool getDefaultClick() const { return defclick; };
	void setValue(const std::string& value);
	std::string getValue() const { return value; };
	void setCSS(const std::string& type) { css = type; }
	std::string getCSS() const { return css; };

	const std::string& html() const;
	const std::string& text() const;

	TButton();
	TButton(const std::string& name);
	virtual ~TButton();
};


class TRadioGroup : public TComponent {
private:
	util::ECompareType partial;

	void prime();
	void setCaption(const std::string& caption) {};
	bool compare(const std::string& value1, const std::string& value2) const;

public:
	// Allow access to protected string properties of TComponent
#ifdef STL_HAS_TEMPLATE_ALIAS
	using TComponent::begin;
	using TComponent::end;
	using TComponent::first;
	using TComponent::last;
	using TComponent::count;
	using TComponent::elements;
#else
	inline const_iterator begin() const { return TComponent::begin(); };
	inline const_iterator end() const { return TComponent::end(); };
	inline const_iterator first() const { return TComponent::first(); };
	inline const_iterator last() const { return TComponent::last(); };
	inline size_t count() const { return TComponent::count(); };
	inline util::TStringList& elements() { return TComponent::elements(); };
#endif

	const std::string& html() const { return strHTML; };
	const std::string& html(const std::string& caption) const;
	util::TStringList& operator () () { return TComponent::elements(); };

	void setCompareType(const util::ECompareType value) { partial = value; };
	util::ECompareType getCompareType() const { return partial; };

	TRadioGroup();
	TRadioGroup(const std::string& name);
	virtual ~TRadioGroup();
};


class TMenuItem {
	friend class TMainMenu;
	friend class TMainMenuItem;
	friend class TContextMenu;
	friend class TContextMenuItem;

private:
	mutable app::TMutex mtx;
	std::string id;
	std::string name;
	std::string link;
	size_t captionID;
	std::string caption;
	std::string glyph;
	bool active;
	int level;

public:
	bool getActive() const { return active; };
	void setActive(const bool value) { active = value; };
	int getLevel() const { return level; };
	void setLevel(const int value) { level = value; };

	const std::string getCaption() const;
	void setCaption(const std::string& value);
	const std::string getLink() const;
	void setLink(const std::string& value);

	void getProperties(size_t& captionID, std::string& caption, std::string& link) const;

	TMenuItem();
	virtual ~TMenuItem();
};


class TContextMenuItem {
friend class TContextMenu;

private:
	TMenuItem value;
	TMenuItemList submenu;

public:
	bool hasMenu() const { return !submenu.empty(); };
	void addSeparator();
	PMenuItem addItem(const TMenuItem& item);
	void clearSubMenu();

	TContextMenuItem();
	virtual ~TContextMenuItem();
};


class TTileItem {
	friend class TTileMenu;

private:
	mutable app::TMutex mtx;
	std::string id;
	std::string name;
	std::string link;
	size_t captionID;
	std::string caption;
	size_t textID;
	std::string text;
	std::string glyph;
	EComponentSize size;
	EComponentAlign align;
	int level;

public:
	int getLevel() const { return level; };
	void setLevel(const int value) { level = value; };

	const std::string getCaption() const;
	void setCaption(const std::string& value);
	const std::string getText() const;
	void setText(const std::string& value);
	const std::string getLink() const;
	void setLink(const std::string& value);

	void getProperties(size_t& captionID, std::string& caption, size_t& textID, std::string& text, std::string& link) const;

	TTileItem();
	virtual ~TTileItem();
};


class TContextMenu : public TComponent {
private:
	size_t titleID;
	std::string title;
	std::string defVal;
	TContextMenuItemList menu;
	PContextMenuItem anchor;
	size_t multi;

	void prime();

	// Hide methods of TComponent
	const std::string& text(const std::string caption) const { return defVal; };
	const std::string& html(const std::string caption) const { return defVal; };

public:
	void setTitle(const std::string& title);
	void setTitle(const size_t titleID, const std::string& title);
	const std::string& getTitle() const { return title.empty() ? defVal : title; };

	void setAnchor(PContextMenuItem item);
	PContextMenuItem addItem(const std::string& id, const std::string& caption, const int level = 0, const std::string& glyph = "");
	PContextMenuItem addItem(const std::string& id, const size_t captionID, const std::string& caption, const int level = 0, const std::string& glyph = "");
	void addSeparator();
	PMenuItem addSubItem(const std::string& id, const std::string& caption, const int level = 0, const std::string& glyph = "");
	PMenuItem addSubItem(const std::string& id, const size_t captionID, const std::string& caption, const int level = 0, const std::string& glyph = "");
	void addSubSeparator();
	void clearSubMenu(PContextMenuItem item);

	const std::string& html() const;
	const std::string& text() const;

	TContextMenu();
	TContextMenu(const std::string& name);
	virtual ~TContextMenu();
};


class TMainMenuItem {
friend class TMainMenu;

private:
	TMenuItem value;
	TMenuItemList submenu;

public:
	bool hasMenu() const { return !submenu.empty(); };
	void addHeader(const std::string& caption);
	void addSeparator();
	PMenuItem addItem(const TMenuItem& item);
	void clearSubMenu();
	size_t getMenuItemsLength();

	TMainMenuItem();
	virtual ~TMainMenuItem();
};


class TMainMenu : public TComponent {
private:
	std::string logo;
	std::string root;
	std::string defVal;
	TMainMenuItemList menu;
	PMainMenuItem anchor;
	bool activated;
	bool fixed;
	mutable app::PWebToken css;

	void prime();
	void invalidate();

	bool addCSSToken() const;
	std::string getCSSBody() const;
	std::string completeLink(const std::string& link) const;

	// Hide methods of TComponent
	const std::string& text(const std::string caption) const { return defVal; };
	const std::string& html(const std::string caption) const { return defVal; };

public:
	void setLogo(const std::string& file) { logo = file; };
	std::string getLogo() const { return logo; };
	void setRoot(const std::string& path) { root = path; };
	std::string getRoot() const { return root; };
	void setFixed(const bool value) { fixed = value; };
	bool getFixed() const { return fixed; };

	PMainMenuItem addItem(const std::string& id, const std::string& caption, const std::string& link, const int level = 0, const std::string& glyph = "", const bool active = false);
	PMainMenuItem addItem(const std::string& id, const size_t captionID, const std::string& caption, const std::string& link, const int level = 0, const std::string& glyph = "", const bool active = false);
	PMenuItem addSubItem(const std::string& id, const std::string& caption, const std::string& link, const int level = 0, const std::string& glyph = "");
	PMenuItem addSubItem(const std::string& id, const size_t captionID, const std::string& caption, const std::string& link, const int level = 0, const std::string& glyph = "");
	void addSubHeader(const std::string& caption);
	void addSubSeparator();
	void clearSubMenu(PMainMenuItem item);
	void setAnchor(PMainMenuItem item);

	const std::string& html() const;
	const std::string& text() const;

	TMainMenu();
	TMainMenu(const std::string& name);
	virtual ~TMainMenu();
};


class TTileMenu : public TComponent {
private:
	std::string root;
	std::string defVal;
	TTileItemList menu;

	void prime();
	void invalidate();
	std::string completeLink(const std::string& link) const;

	// Hide methods of TComponent
	const std::string& text(const std::string caption) const { return defVal; };
	const std::string& html(const std::string caption) const { return defVal; };

public:
	void setRoot(const std::string& path) { root = path; };
	std::string getRoot() const { return root; };

	PTileItem addItem(const std::string& id, const std::string& caption, const std::string& text, const std::string& link,
			const EComponentSize size, const int level = 0, const std::string& glyph = "", const EComponentAlign align = ECA_DEFAULT);
	PTileItem addItem(const std::string& id, const size_t captionID, const std::string& caption, const size_t textID, const std::string& text, const std::string& link,
			const EComponentSize size, const int level = 0, const std::string& glyph = "", const EComponentAlign align = ECA_DEFAULT);

	const std::string& html() const;
	const std::string& text() const;

	TTileMenu();
	TTileMenu(const std::string& name);
	virtual ~TTileMenu();
};


} /* namespace html */

#endif /* COMPONENT_H_ */

/*
 * component.cpp
 *
 *  Created on: 11.02.2017
 *      Author: Dirk Brinkmeier
 */

#include "component.h"
#include "webserver.h"
#include "htmlconsts.h"
#include "htmlutils.h"
#include "encoding.h"
#include "compare.h"

namespace html {

TPersistent::TPersistent() {
	sanitized = false;
}

TPersistent::~TPersistent() {
}

std::string TPersistent::encodeHtmlText(const std::string& str) const {
	return sanitized ? html::THTML::encode(str) : str;
}


THtmlList::THtmlList() : util::TStringList(), TPersistent() {
}

THtmlList::~THtmlList() {
}

void THtmlList::invalidate() {
	TStringList::invalidate();
	if (!strHTML.empty())
		strHTML.clear();
}

void THtmlList::clear() {
	TStringList::clear();
	invalidate();
}


TTagList::TTagList() : THtmlList() {
	prime();
}

TTagList::TTagList(const std::string& tag) : THtmlList() {
	prime();
	setTag(tag);
}

TTagList::~TTagList() {
}

void TTagList::prime() {
	hashTag = 0;
	addEmptyDefault = false;
}

void TTagList::invalidate() {
	THtmlList::invalidate();
	hashTag = 0;
}

void TTagList::add(const std::string& text, size_t tag) {
	THtmlList::add(text);
	icons.push_back("*"); // Add one glyphicon placeholder per entry!
	tags.push_back(tag);
}

void TTagList::add(const util::TStringList& items) {
	THtmlList::add(items);
	for (size_t i=0; i<items.size(); ++i) {
		icons.push_back("*"); // Add one glyphicon placeholder per entry!
		tags.push_back(i);
	}
}

void TTagList::add(const std::string& text, const std::string& glyphicon, size_t tag) {
	THtmlList::add(text);
	icons.push_back(glyphicon.empty() ? "*" : glyphicon);
	tags.push_back(tag);
}

void TTagList::loadFromFile(const std::string& fileName) {
	clear();
	if (util::fileExists(fileName)) {
		size_t tag = 0;
		std::string line;
		std::ifstream is;
		util::TStreamGuard<std::ifstream> strm(is);
		strm.open(fileName, std::ios_base::in);
		while (is.good()) {
			std::getline(is, line);
			if (!line.empty()) {
				add(line, tag++);
			}
		}
	}
}

void TTagList::clear() {
	THtmlList::clear();
	icons.clear();
	tags.clear();
}

TTagList& TTagList::operator = (const app::TStringVector& vector) {
	assign(vector);
	return *this;
}

TTagList& TTagList::operator = (const TStringList& list) {
	assign(list);
	return *this;
}

TTagList& TTagList::operator = (const std::string& csv) {
	assign(csv);
	return *this;
}

void TTagList::setTag(const std::string& tag) {
	if (this->tag != tag) {
		invalidate();
		this->tag = tag;
	}
}

const std::string& TTagList::html(const std::string caption) const {
	if (!tag.empty() && !empty() && strHTML.empty()) {

		// Calculate estimated destination size:
		// 123456789-12345
		// < value=""></>n
		size_t n = 0, m = 2 * tag.size() + 15;
		const_iterator it = begin();
		for (; it != end(); ++it)
			n += 2 * (*it).size() + m;
		n += 2 * caption.size() + m;
		strHTML.reserve(n + size());

		// Add caption to list
		if (caption.empty()) {
			if (addEmptyDefault)
				strHTML = "<" + tag + "></" + tag + ">\n";
		} else {
			std::string text = encodeHtmlText(caption);
			strHTML = "<" + tag + " value=\"" + text + "\">" + text + "</" + tag + ">\n";
		}

		// Add list values
		std::string item, glyph;
		size_t idx = find(caption);
		size_t count = 0;
		size_t index;
		bool ok;
		for (size_t i=0; i<size(); ++i) {
			// Exclude given default value from list if it is in list!
			ok = idx == std::string::npos ? true : i != idx;
			if (ok) {
				index = 0;
				glyph.clear();
				item = encodeHtmlText(at(i));
				if (util::validListIndex(icons, i)) {
					glyph = icons[i];
				}
				if (util::validListIndex(tags, i)) {
					index = tags[i];
				}
				if (count > 0) strHTML += "\n";
				if (glyph.size() < 2) {
					// e.g. <option value="hw:CARD=USBASIO,DEV=0">hw:CARD=USBASIO,DEV=0</option>
					strHTML += "<" + tag + " data-id=\"" + std::to_string(index) + "\" value=\"" + item + "\">" + item + "</" + tag + ">";
				} else {
					strHTML += "<" + tag + " data-id=\"" + std::to_string(index) + "\" value=\"" + item + "\"><span class=\"glyphicon " + glyph + " pull-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;" + item + "</" + tag + ">";
				}
				++count;
			}
		}
	}
	return strHTML;
}


TRadioButtons::TRadioButtons() : THtmlList() {
}

TRadioButtons::~TRadioButtons() {
}

void TRadioButtons::assign(const util::TStringList& list) {
	clear();
	if (!list.empty()) {
		TStringList::const_iterator it = list.begin();
		for (; it != list.end(); ++it)
			add(*it);
	}
}

const std::string& TRadioButtons::html(const std::string& checked) const {
	if (!empty() && strHTML.empty()) {

		// Set active radio element to first entry
		bool first = false;
		if (checked.empty())
			first = true;

		// Add radio items to string list
		std::string name = util::TBase36::randomize("RD", 5, true);
		util::TStringList sl;
		sl.add("<fieldset>");
		for (size_t i=0; i<size(); ++i) {
			const std::string entry = at(i);
			const std::string text = encodeHtmlText(entry);
			std::string idx = std::to_string((size_u)i);
			sl.add("  <div class=\"radio\">");
			if (first || entry == checked) {
				sl.add("    <input type=\"radio\" name=\"" + name + "\" id=\"radio" + idx + "\" checked=\"true\">");
				first = false;
			} else {
				sl.add("    <input type=\"radio\" name=\"" + name + "\" id=\"radio" + idx + "\">");
			}
			sl.add("    <label for=\"radio" + idx + "\" index=\"" + idx + "\">" + text + "</label>");
			sl.add("  </div>");
		}
		sl.add("</fieldset>");

		// Return radio items as unmodified text
		strHTML = sl.raw('\n');

	}
	return strHTML;
}


TComponent::TComponent() : app::TObject(), TPersistent() {
	invalidated = false;
	enabled = true;
	token = nil;
	nls = nil;
	width = 0;
	height = 0;
	style = ECS_DEFAULT;
	align = ECA_NONE;
	size = ESZ_DEFAULT;
	type = ECT_DEFAULT;
	focus = ELF_NONE;
	tooltipID = 0;
	compare = util::EC_COMPARE_PARTIAL;
}

TComponent::~TComponent() {
}

void TComponent::invalidate() {
	invalidated = true;
	items.invalidate();
	if (!strHTML.empty())
		strHTML.clear();
	if (!strText.empty())
		strText.clear();
}

void TComponent::clear() {
	items.clear();
	invalidate();
}

app::PWebServer TComponent::getServer() const {
	if (util::assigned(getOwner())) {
		return util::asClass<app::TWebServer>(getOwner());
	}
	return nil;
}

const std::string& TComponent::text() const {
	return text("");
}

const std::string& TComponent::html() const {
	return html("");
}

const std::string& TComponent::text(const std::string& caption) const {
	if (strText.empty())
		strText = items.html(caption);
	return strText;
}

const std::string& TComponent::html(const std::string& caption) const {
	if (strHTML.empty())
		strHTML = text(caption);
	return strHTML;
}

void TComponent::setName(const std::string& name) {
	TObject::setName(name);
	addWebToken();
}

bool TComponent::addWebToken() {
	if (!(util::assigned(token) || getName().empty())) {
		app::PWebServer o = getServer();
		if (util::assigned(o)) {
			token = o->addWebToken(getName(), util::quote(getName()));
		}
	}
	return util::assigned(token);
}


void TComponent::setCaption(const std::string& caption) {
	clear();
	items.allowEmptyDefault(false);
	items.add(caption);
}

void TComponent::setStyle(const EComponentStyle value) {
	if (style != value)
		invalidate();
	style = value;
}

void TComponent::setAlign(const EComponentAlign value) {
	if (align != value)
		invalidate();
	align = value;
};

void TComponent::setSize(const EComponentSize value) {
	if (size != value)
		invalidate();
	size = value;
};

void TComponent::setType(const EComponentType value) {
	if (type != value)
		invalidate();
	type = value;
};

void TComponent::setWidth(const size_t value) {
	if (width != value)
		invalidate();
	width = value;
};

void TComponent::setHeight(const size_t value) {
	if (height != value)
		invalidate();
	height = value;
};

void TComponent::setFocus(const EListFocus value) {
	if (focus != value)
		invalidate();
	focus = value;
};

void TComponent::setCompare(const util::ECompareType value) {
	if (compare != value)
		invalidate();
	compare = value;
}

void TComponent::setGlyphicon(const std::string& name) {
	if (glyphicon != name)
		invalidate();
	glyphicon = name;
};

void TComponent::setTooltip(const size_t tooltipID, const std::string& tooltip) {
	if (this->tooltip != tooltip)
		invalidate();
	this->tooltipID = tooltipID;
	this->tooltip = tooltip;
}

void TComponent::setTooltip(const std::string& tooltip) {
	setTooltip(0, tooltip);
};

std::string TComponent::getTooltip() const {
	return translate(tooltipID, tooltip);
}

void TComponent::setClick(const std::string& action) {
	if (!action.empty()) {
		if (util::pred(action.size()) != ';')
			onclick = action;
		else
			onclick = action + ";";
	} else {
		onclick.clear();
	}
};

void TComponent::setEnabled(const bool value) {
	if (enabled != value)
		invalidate();
	enabled = value;
}

std::string TComponent::disbledToStr() const {
	if (!enabled)
		return "true";
	return std::string();
}

void TComponent::update() {
	update("");
}

void TComponent::update(const std::string& caption) {
	invalidate();
	if (addWebToken()) {
		switch (style) {
			case ECS_HTML:
				*token = caption.empty() ? html() : html(caption);
				break;
			case ECS_TEXT:
			default:
				*token = caption.empty() ? text() : text(caption);
				break;
		}
		token->invalidate();
	}
	invalidated = false;
}

std::string TComponent::tagToStr(const std::string& tag, const std::string& value) const {
	if (!value.empty() && !tag.empty()) {
		return " " + tag + "=\"" + value + "\"";
	}
	return std::string();
}

std::string TComponent::tagToStr(const std::string& tag, const size_t value) const {
	if (value > 0 && !tag.empty()) {
		return " " + tag + "=\"" + std::to_string((size_u)value) + "\"";
	}
	return std::string();
}

bool TComponent::isJavascript(const std::string& prototype) const {
	// Javascript example: function name(x);
	size_t len = prototype.size();
	if (len > 3) {
		char last = prototype[len - 1];
		char prev = prototype[len - 2];
		if (last == ')')
			return true;
		if (last == ';' && prev == ')')
			return true;
	}
	return false;
}


std::string TComponent::padTextRight(const std::string &str, const size_t length, const std::string padding) const {
	std::string s = str;
	if (s.size() < length) {
		size_t count = length - s.size();
		for (size_t i=0; i<count; ++i) {
			s.append(padding);
		}
	}
	return s;
}

std::string TComponent::padTextLeft(const std::string &str, const size_t length, const std::string padding) const {
	std::string s = str;
	if (s.size() < length) {
		size_t count = length - s.size();
		for (size_t i=0; i<count; ++i) {
			s.insert(0, padding);
		}
	}
	return s;
}

void TComponent::setTranslator(app::TTranslator& nls) {
	this->nls = &nls;
}

std::string TComponent::translate(const size_t id, const std::string& defValue) const {
	// Translate only text for valid IDs
	// --> e.g. setText() or setCaption() set ID = 0 to force text
	// --> forced text must be set already translated !!!
	if (util::assigned(nls) && id > 0) {
		return nls->text(id, defValue);
	}
	return defValue;
}



TComboBox::TComboBox() : TComponent() {
	prime();
}

TComboBox::TComboBox(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

void TComboBox::prime() {
	items.setTag("option");
	items.allowEmptyDefault(true);
}


const std::string& TComboBox::html(const std::string& caption) const {
	if (strHTML.empty()) {
		strHTML = "<select class=\"form-control combobox\"" + tagToStr("id", getID()) + tagToStr("name", getName()) + ">\n";
		strHTML += text(caption);
		strHTML += "\n</select>";
	}
	return strHTML;
}

TComboBox::~TComboBox() {
}



TListBox::TListBox() : TComponent() {
	prime();
}

TListBox::TListBox(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

void TListBox::prime() {
	items.setTag("li");
	items.allowEmptyDefault(false);
}

bool TListBox::compare(const std::string& s1, const std::string& s2) const {
	if (s1.size() > s2.size()) {
		if (util::strcasestr(s1, s2)) {
			return true;
		}
	} else {
		if (util::strcasestr(s2, s1)) {
			return true;
		}
	}
	return false;
}

void TListBox::addComponent(const util::TStringList& html) {
	component.assign(html);
}

const std::string& TListBox::text(const std::string& caption) const {
	const std::string& tag = items.getTag();
	if (!tag.empty() && !items.empty() && strText.empty()) {

		// Calculate estimated destination size:
		// 123456789-12345
		// < value=""></>n
		size_t n = 0, m = 2 * tag.size() + 45;
		const_iterator it = begin();
		for (; it != end(); ++it)
			n += 2 * (*it).size() + m;
		n += 2 * caption.size() + m;
		strText.reserve(n + items.size());

		// Check if given caption in list
		if (!caption.empty()) {
			size_t idx = items.find(caption, getCompare());
			if (std::string::npos == idx) {
				std::string text = encodeHtmlText(caption);
				switch (focus) {
					case ELF_NONE:
						strText += "<" + tag + " value=\"" + caption + "\"><a>" + text + "</a></" + tag + ">";
						break;
					case ELF_FULL:
					case ELF_PARTIAL:
						strText += "<" + tag + " value=\"" + caption + "\"><a><span class=\"glyphicon glyphicon-triangle-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;&nbsp;" + caption + "</a></" + tag + ">";
						break;
				}
			}
		} else {
			// Add empty entry
			strText += "<" + tag + " value=\"\"><a></a></" + tag + ">";
		}

		// Add list values
		const app::TStringVector& glyphs = items.getIcons();
		const TItemTagList& tags = items.getTags();
		std::string item, glyph;
		size_t count = 0;
		size_t index;
		for (size_t i=0; i<items.size(); ++i) {
			index = 0;
			item = items.at(i);
			glyph.clear();
			if (util::validListIndex(glyphs, i)) {
				// Ignore placeholders like "*"
				const std::string& image = glyphs[i];
				if (image.size() > 1) {
					if ('/' == image[0]) {
						// Add link to an image
						glyph = "<span style=\"pointer-events:none;\" aria-hidden=\"true\"><img src=\"" + image + "\"></span>&nbsp;&nbsp;";
					} else {
						// Add glyphicon by name
						glyph = "<span class=\"glyphicon " + image + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;&nbsp;";
					}
				}
			}
			if (count > 0) strText += "\n";
			std::string text = encodeHtmlText(item);
			if (util::validListIndex(tags, i)) {
				index = tags[i];
			}
			switch (focus) {
				case ELF_NONE:
					strText += "<" + tag + " value=\"" + item + "\"><a>" + item + "</a></" + tag + ">";
					break;
				case ELF_FULL:
					if (item.size() == caption.size() && 0 == util::strcasecmp(item, caption))
						strText += "<" + tag + " data-id=\"" + std::to_string(index) + "\" value=\"" + item + "\"><a><span class=\"glyphicon glyphicon-triangle-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;&nbsp;" + glyph + text + "</a></" + tag + ">";
					else
						strText += "<" + tag + " data-id=\"" + std::to_string(index) + "\" value=\"" + item + "\"><a><span class=\"glyphicon glyphicon-none\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;&nbsp;" + glyph + text + "</a></" + tag + ">";
					break;
				case ELF_PARTIAL:
					if (compare(item, caption))
						strText += "<" + tag + " data-id=\"" + std::to_string(index) + "\" value=\"" + item + "\"><a><span class=\"glyphicon glyphicon-triangle-right\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;&nbsp;" + glyph + text + "</a></" + tag + ">";
					else
						strText += "<" + tag + " data-id=\"" + std::to_string(index) + "\" value=\"" + item + "\"><a><span class=\"glyphicon glyphicon-none\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;&nbsp;" + glyph + text + "</a></" + tag + ">";
					break;
			}
			++count;
		}
	}
	return strText;
}

const std::string& TListBox::html(const std::string& caption) const {
	if (strHTML.empty()) {
		// Create dynamic HTML content for list box
		size_t index = 0;
		if (!caption.empty()) {
			size_t idx = items.find(caption, getCompare());
			const TItemTagList& tags = items.getTags();
			if (util::validListIndex(tags, idx)) {
				index = tags[idx];
			}
		}
		strHTML  = "<div class=\"input-group\">\n";
		strHTML += "  <span class=\"input-group-btn list-box-elements\">\n";
		strHTML += "    <a class=\"btn btn-default dropdown-toggle\" data-toggle=\"dropdown\" href=\"#\">\n";
		strHTML += "      <span class=\"caret\" style=\"pointer-events:none\" aria-hidden=\"true\"></span>\n";
		strHTML += "    </a>\n";

		if (getHeight() > 0) {
			strHTML += "    <ul id=\"ul-select\" class=\"dropdown-menu\" style=\"max-height:" + std::to_string(getHeight()) + "px; overflow:auto;\">\n";
		} else {
			strHTML += "    <ul id=\"ul-select\" class=\"dropdown-menu\">\n";
		}
		strHTML += "    " + text(caption) + "\n";
		strHTML += "    </ul>\n";

		strHTML += "  </span>\n";
		strHTML += "  <input type=\"text\" class=\"form-control list-box-input\"" + tagToStr("id", getID()) + tagToStr("name", getID()) + tagToStr("data-id", index) + " autocomplete=\"off\" autocorrect=\"off\" autocapitalize=\"off\" spellcheck=\"false\" value=\"" + caption + "\"/>\n";
		if (!component.empty()) {
			for (size_t i=0; i<component.size(); ++i) {
				strHTML += "  " + component[i] + "\n";
			}
		}
		strHTML += "</div>\n";
	}
	return strHTML;
}

TListBox::~TListBox() {
}



TButton::TButton() : TComponent() {
	prime();
}

TButton::TButton(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

TButton::~TButton() {
}

void TButton::prime() {
	defclick = false;
	items.setTag("button");
}


void TButton::invalidate() {
	if (!strHTML.empty())
		strHTML.clear();
	if (!strText.empty())
		strText.clear();
}

void TButton::setDefaultClick(const bool value) {
	if (defclick != value)
		invalidate();
	defclick = value;
};

void TButton::setValue(const std::string& value) {
	if (this->value != value)
		invalidate();
	this->value = value;
};

std::string TButton::typeToClass() const {
	switch (type) {
		case ECT_DEFAULT:
			return "btn-default";
		case ECT_PRIMARY:
			return "btn-primary";
		case ECT_SUCCESS:
			return "btn-success";
		case ECT_INFO:
			return "btn-info";
		case ECT_WARNING:
			return "btn-warning";
		case ECT_DANGER:
			return "btn-danger";
	}
	return "btn-default";
}

std::string TButton::sizeToClass() const {
	switch (size) {
		case ESZ_XSMALL:
			return "btn-xs";
		case ESZ_SMALL:
			return "btn-sm";
		case ESZ_MEDIUM:
			return "";
		case ESZ_LARGE:
			return "btn-lg";
		case ESZ_CUSTOM:
			if (!css.empty())
				return css;
	}
	return "btn-lg";
}

std::string TButton::clickToStr() const {
	if (defclick && onclick.empty())
		return "true";
	return std::string();
}

std::string TButton::getClass() const {
	std::string aln;
	switch (align) {
		case ECA_LEFT:
			aln = " pull-left";
			break;
		case ECA_RIGHT:
			aln = " pull-right";
			break;
		default:
			break;
	}
	if (size == ESZ_MEDIUM)
		return "btn " + typeToClass() + aln;
	return "btn " + sizeToClass() + " " + typeToClass() + aln;
}

const std::string& TButton::text() const {
	if (strText.empty()) {
		const std::string& icon = getGlyphicon();
		const std::string& caption = encodeHtmlText(getCaption());
		if (!icon.empty()) {
			strText += "<span class=\"glyphicon " + icon + "\" style=\"pointer-events:none\" aria-hidden=\"true\"></span>";
		}
		if (!caption.empty()) {
			if (!icon.empty()) strText += "&nbsp;";
			strText += caption;
		}
	}
	return strText;
}

const std::string& TButton::html() const {
	if (strHTML.empty()) {
		if (width > 0) {
			strHTML = "<button " + tagToStr("id", getID()) + tagToStr("name", getName()) + tagToStr("disabled", disbledToStr()) + \
								   tagToStr("value", getValue()) + tagToStr("addClick", clickToStr()) + tagToStr("onclick", getClick()) + \
								   tagToStr("data-toggle", "tooltip") + tagToStr("data-placement", "top") + tagToStr("title", getTooltip()) + tagToStr("class", getClass()) + " style=\"width: " + std::to_string((size_u)width) + "px !important;\">\n";
		} else {
			strHTML = "<button " + tagToStr("id", getID()) + tagToStr("name", getName()) + tagToStr("disabled", disbledToStr()) + \
								   tagToStr("value", getValue()) + tagToStr("addClick", clickToStr()) + tagToStr("onclick", getClick()) + \
								   tagToStr("data-toggle", "tooltip") + tagToStr("data-placement", "top") + tagToStr("title", getTooltip()) + tagToStr("class", getClass()) + ">\n";
		}
		if (!text().empty())
			strHTML += text();
		strHTML += "\n</button>";
	}
	return strHTML;
}



TRadioGroup::TRadioGroup() : TComponent() {
	prime();
}

TRadioGroup::~TRadioGroup() {
}

TRadioGroup::TRadioGroup(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

void TRadioGroup::prime() {
	items.setTag("option");
	items.allowEmptyDefault(true);
	setCompareType(util::EC_COMPARE_HEADING);
}


bool TRadioGroup::compare(const std::string& value1, const std::string& value2) const {
	if (!value1.empty() && !value2.empty()) {
		switch (partial) {
			case util::EC_COMPARE_FULL:
				if ((value1.size() == value2.size()) && (0 == util::strncasecmp(value1, value2, value2.size()))) {
					return true;
				}
				break;
			case util::EC_COMPARE_HEADING:
				if (value1.size() > value2.size()) {
					if (0 == util::strncasecmp(value1, value2, value2.size())) {
						return true;
					}
				} else {
					if (0 == util::strncasecmp(value2, value1, value1.size())) {
						return true;
					}
				}
				break;
			case util::EC_COMPARE_PARTIAL:
				if (value1.size() > value2.size()) {
					if (util::strcasestr(value1, value2)) {
						return true;
					}
				} else {
					if (util::strcasestr(value2, value1)) {
						return true;
					}
				}
				break;
			case util::EC_COMPARE_VALUE_IN_LIST:
				if (value1.size() >= value2.size()) {
					if (util::strcasestr(value1, value2)) {
						return true;
					}
				}
				break;
			case util::EC_COMPARE_LIST_IN_VALUE:
				if (value2.size() >= value1.size()) {
					if (util::strcasestr(value2, value1)) {
						return true;
					}
				}
				break;
		}
	}
	return false;
}

const std::string& TRadioGroup::html(const std::string& caption) const {
	if (!items.empty() && strHTML.empty()) {

		// Set active radio element to first entry
		bool found = false;
		bool first = false;
		if (caption.empty())
			first = true;

		// Add radio items to string list
		util::TStringList sl;
		sl.add("<fieldset>");
		for (size_t i=0; i<items.size(); ++i) {
			const std::string entry = items.at(i);
			const std::string text = encodeHtmlText(entry);
			std::string idx = std::to_string((size_u)i);
			sl.add("  <div class=\"radio\">");
			if (!found && (first || compare(entry, caption))) {
				sl.add("    <input type=\"radio\" name=\"" + getName() + "\" id=\"" + getID() + idx + "\" checked=\"true\">");
				first = false;
				found = true;
			} else {
				sl.add("    <input type=\"radio\" name=\"" + getName() + "\" id=\"" + getID() + idx + "\">");
			}
			sl.add("    <label for=\"" + getID() + idx + "\" index=\"" + idx + "\">" + text + "</label>");
			sl.add("  </div>");
		}
		sl.add("</fieldset>");

		// Return radio items as unmodified html strings
		strHTML = sl.raw('\n');

	}
	return strHTML;
}


TMenuItem::TMenuItem() {
	captionID = 0;
	active = false;
	level = 0;
}

TMenuItem::~TMenuItem() {
}

const std::string TMenuItem::getCaption() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return caption;
};

void TMenuItem::setCaption(const std::string& value) {
	app::TLockGuard<app::TMutex> lock(mtx);
	// Forced overwrite of caption text
	// --> do not firther translate by setting ID to 0 !!!
	captionID = 0;
	caption = value;
};

const std::string TMenuItem::getLink() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return link;
};

void TMenuItem::setLink(const std::string& value) {
	app::TLockGuard<app::TMutex> lock(mtx);
	link = value;
};

void TMenuItem::getProperties(size_t& captionID, std::string& caption, std::string& link) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	captionID = this->captionID;
	caption = this->caption;
	link = this->link;
}


TContextMenuItem::TContextMenuItem() {
}

TContextMenuItem::~TContextMenuItem() {
	clearSubMenu();
}

PMenuItem TContextMenuItem::addItem(const TMenuItem& item) {
	PMenuItem o = new TMenuItem;
	o->level = item.level;
	o->active = item.active;
	o->caption = item.caption;
	o->glyph = item.glyph;
	o->link = item.link;
	o->name = item.name;
	o->id = item.id;
	submenu.push_back(o);
	return o;
}

void TContextMenuItem::addSeparator() {
	PMenuItem o = new TMenuItem;
	o->id = "-";
	o->name = "-";
	o->caption = "-";
	submenu.push_back(o);
}

void TContextMenuItem::clearSubMenu() {
	if (!submenu.empty())
		util::clearObjectList(submenu);
}



TContextMenu::TContextMenu() : TComponent() {
	prime();
}

TContextMenu::TContextMenu(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

void TContextMenu::prime() {
	titleID = 0;
	defVal = "<invalid>";
	items.setTag("li");
	multi = 0;
}

void TContextMenu::setTitle(const std::string& title) {
	this->titleID = 0;
	this->title = title;
};

void TContextMenu::setTitle(const size_t titleID, const std::string& title) {
	this->titleID = titleID;
	this->title = title;
};

void TContextMenu::setAnchor(PContextMenuItem item) {
	anchor = item;
}

void TContextMenu::addSeparator() {
	addItem("-", 0, "-");
}

PContextMenuItem TContextMenu::addItem(const std::string& id, const std::string& caption, const int level, const std::string& glyph) {
	return addItem(id, 0, caption, level, glyph);
}

PContextMenuItem TContextMenu::addItem(const std::string& id, const size_t captionID, const std::string& caption, const int level, const std::string& glyph) {
	if (!id.empty() && !caption.empty()) {
		PContextMenuItem o = new TContextMenuItem;
		o->value.id = id;
		o->value.name = id;
		o->value.level = level;
		o->value.captionID = captionID;
		o->value.caption = caption;
		o->value.glyph = glyph;
		menu.push_back(o);
		anchor = o;
		return o;
	}
	if (id.empty())
		throw util::app_error("TContextMenu::addSubItem() Empty identifier.");
	if (caption.empty())
		throw util::app_error("TContextMenu::addItem() Empty caption.");
	throw util::app_error("TContextMenu::addItem() Undefined error.");
}

PMenuItem TContextMenu::addSubItem(const std::string& id, const std::string& caption, const int level, const std::string& glyph) {
	return addSubItem(id, 0, caption, level, glyph);
}

PMenuItem TContextMenu::addSubItem(const std::string& id, const size_t captionID, const std::string& caption, const int level, const std::string& glyph) {
	if (util::assigned(anchor) && !caption.empty() && !id.empty()) {
		++multi;
		TMenuItem item;
		item.id = id;
		item.name = id;
		item.level = level;
		item.caption = caption;
		item.captionID = captionID;
		item.glyph = glyph;
		return anchor->addItem(item);
	}
	if (!util::assigned(anchor))
		throw util::app_error("TContextMenu::addSubItem() Missing node to add sub item.");
	if (caption.empty())
		throw util::app_error("TContextMenu::addSubItem() Empty caption.");
	if (id.empty())
		throw util::app_error("TContextMenu::addSubItem() Empty identifier.");
	throw util::app_error("TContextMenu::addSubItem() Undefined error.");
}

void TContextMenu::addSubSeparator() {
	if (util::assigned(anchor)) {
		anchor->addSeparator();
	}
}

void TContextMenu::clearSubMenu(PContextMenuItem item) {
	if (util::assigned(item)) {
		if (multi > item->submenu.size()) {
			multi -= item->submenu.size();
		} else {
			multi = 0;
		}
		item->clearSubMenu();
	}
}


const std::string& TContextMenu::text() const {
	if (!items.getTag().empty() && strText.empty()) {

		// Fill item list
		items.clear();
		for (size_t i=0; i<menu.size(); ++i) {
			PContextMenuItem o = menu[i];
			if (util::assigned(o)) {
				if (o->value.name == "-" || o->value.id == "-") {
					items.add("  <" + items.getTag() + " class=\"divider\"></" + items.getTag() + ">");
				} else {
					//  <li><a href="#">Some action</a></li>
					//  <li><a href="#">Some other action</a></li>
					//  <li class="divider"></li>
					//  <li class="dropdown-submenu">
					//	<a tabindex="-1" href="#">Hover me for more options</a>
					//	<ul class="dropdown-menu">
					//	  <li><a tabindex="-1" href="#">Second level</a></li>
					//	  <li class="dropdown-submenu">
					//		<a href="#">Even More..</a>
					//		<ul class="dropdown-menu">
					//			<li><a href="#">3rd level</a></li>
					//			<li><a href="#">3rd level</a></li>
					//		</ul>
					//	  </li>
					//	  <li><a href="#">Second level</a></li>
					//	  <li><a href="#">Second level</a></li>
					//	</ul>
					//  </li>
					std::string caption = o->value.caption;
					size_t captionID = o->value.captionID;
					if (hasTranslator()) {
						caption = translate(captionID, caption);
					}
					std::string text = encodeHtmlText(caption);
					if (o->hasMenu()) {

						//items.add("<a tabindex=\"-1\" href=\"#\"" + tagToStr("data-item", o->value.name) + tagToStr("userlevel", std::to_string(o->value.level)) + ">" + o->value.caption + "</a>");
						items.add("  <" + items.getTag() + " class=\"dropdown-submenu\">");
						if (o->value.glyph.empty()) {
							items.add("    <a class=\"dropdown-propagate\" tabindex=\"-1\" href=\"#\"" + tagToStr("id", o->value.id) + tagToStr("data-item", o->value.name) + tagToStr("userlevel", std::to_string(o->value.level)) + ">" + text + "</a>");
						} else {
							items.add("    <a class=\"dropdown-propagate\" tabindex=\"-1\" href=\"#\"" + tagToStr("id", o->value.id) + tagToStr("data-item", o->value.name) + tagToStr("userlevel", std::to_string(o->value.level)) + "><span class=\"glyphicon " + o->value.glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + HTML_SPACE + text + "</a>");
						}
						items.add("    <ul class=\"dropdown-menu\">");

						for (size_t j=0; j<o->submenu.size(); ++j) {
							PMenuItem p = o->submenu[j];
							if (util::assigned(p)) {
								size_t captionID;
								std::string caption, link;
								p->getProperties(captionID, caption, link);
								if (hasTranslator()) {
									caption = translate(captionID, caption);
								}
								text = encodeHtmlText(caption);
								if (p->name == "-" || p->id == "-") {
									items.add("      <" + items.getTag() + " class=\"divider\"></" + items.getTag() + ">");
								} else {
									if (o->value.glyph.empty()) {
										items.add("      <" + items.getTag() + tagToStr("data-item", p->name) + tagToStr("userlevel", std::to_string(p->level)) + "><a" + tagToStr("id", p->id) + ">" + text + "</a>" + "</" + items.getTag() + ">");
									} else {
										items.add("      <" + items.getTag() + tagToStr("data-item", p->name) + tagToStr("userlevel", std::to_string(p->level)) + "><a" + tagToStr("id", p->id) + "><span class=\"glyphicon " + p->glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + HTML_SPACE + text + "</a>" + "</" + items.getTag() + ">");
									}
								}
							}
						}

						items.add("    </ul>");
						items.add("  </" + items.getTag() + ">");

					} else {
						if (o->value.glyph.empty()) {
							items.add("  <" + items.getTag() + tagToStr("data-item", o->value.name) + tagToStr("userlevel", std::to_string(o->value.level)) + "><a" + tagToStr("id", o->value.id) + ">" + text + "</a>" + "</" + items.getTag() + ">");
						} else {
							items.add("  <" + items.getTag() + tagToStr("data-item", o->value.name) + tagToStr("userlevel", std::to_string(o->value.level)) + "><a" + tagToStr("id", o->value.id) + "><span class=\"glyphicon " + o->value.glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + HTML_SPACE + text + "</a>" + "</" + items.getTag() + ">");
						}
					}
				}
			}
		}

		// Calculate estimated destination size:
		size_t n = 0;
		const_iterator it = begin();
		for (; it != end(); ++it)
			n += (*it).size() + 2;
		strText.reserve(n);

		// Concate strings...
		strText = items.raw('\n');
	}
	return strText;
}

const std::string& TContextMenu::html() const {
	if (strHTML.empty()) {
		// <ul id="playlist-context-menu" class="dropdown-menu">
		// <li class="dropdown-header">Manage playlist...</li>
		// <li class="divider"></li>
		//strHTML = "\n<div class=\"dropdown\">\n";
		if (multi > 0) {
			strHTML += "<ul" + tagToStr("id", getID()) + tagToStr("name", getName()) + " class=\"dropdown-menu multi-level\" role=\"menu\">\n";
		} else {
			strHTML += "<ul" + tagToStr("id", getID()) + tagToStr("name", getName()) + " class=\"dropdown-menu\" role=\"menu\">\n";
		}
		if (!title.empty()) {
			std::string title = getTitle();
			if (hasTranslator()) {
				title = translate(titleID, title);
			}
			std::string text = encodeHtmlText(title);
			strHTML += "  <li class=\"dropdown-header\">" + text + "</li>\n";
			strHTML += "  <li class=\"divider\"></li>\n";
		}
		strHTML += text();
		strHTML += "\n</ul>\n";
		//strHTML += "\n</ul>\n</div>\n";
	}
	return strHTML;
}

TContextMenu::~TContextMenu() {
	util::clearObjectList(menu);
}



TMainMenuItem::TMainMenuItem() {
}

TMainMenuItem::~TMainMenuItem() {
	clearSubMenu();
}

PMenuItem TMainMenuItem::addItem(const TMenuItem& item) {
	PMenuItem o = new TMenuItem;
	o->level = item.level;
	o->active = item.active;
	o->captionID = item.captionID;
	o->caption = item.caption;
	o->glyph = item.glyph;
	o->link = item.link;
	o->name = item.name;
	o->id = item.id;
	submenu.push_back(o);
	return o;
}

void TMainMenuItem::addHeader(const std::string& caption) {
	PMenuItem o = new TMenuItem;
	o->caption = caption;
	submenu.push_back(o);
}

void TMainMenuItem::addSeparator() {
	PMenuItem o = new TMenuItem;
	o->caption = "-";
	submenu.push_back(o);
}

void TMainMenuItem::clearSubMenu() {
	if (!submenu.empty())
		util::clearObjectList(submenu);
}

size_t TMainMenuItem::getMenuItemsLength() {
	// Get max length of menu entries
	size_t length = 0;
	for (size_t j=0; j<submenu.size(); ++j) {
		PMenuItem p = submenu[j];
		if (util::assigned(p)) {
			if (p->caption.size() > length)
				length = p->caption.size();
		}
	}
	return length;
}



TMainMenu::TMainMenu() : TComponent() {
	prime();
}

TMainMenu::TMainMenu(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

TMainMenu::~TMainMenu() {
	util::clearObjectList(menu);
}

void TMainMenu::prime() {
	logo = "/logo.gif";
	root = "/";
	defVal = "<invalid>";
	items.setTag("li");
	setID("navbar");
	activated = false;
	fixed = false;
	anchor = nil;
	css = nil;
}

bool TMainMenu::addCSSToken() const {
	if (!(util::assigned(css) || getName().empty())) {
		app::PWebServer o = getServer();
		if (util::assigned(o)) {
			css = o->addWebToken(getName() + "_BODY", getCSSBody());
		}
	}
	return util::assigned(css);
}

std::string TMainMenu::getCSSBody() const {
	std::string s = "body {\n";
	if (fixed) {
		s += "  min-height: 2000px;\n";
		s += "  padding-top: 70px\n";
		s += "}\n";
	} else {
		s += "  padding-top: 70px\n";
		s += "  padding-bottom: 30px\n";
		s += "}\n";
	}
	return s;
}


PMainMenuItem TMainMenu::addItem(const std::string& id, const std::string& caption, const std::string& link, const int level, const std::string& glyph, const bool active) {
	// Add entry without translation support
	return addItem(id, 0, caption, link, level, glyph, active);
}

PMainMenuItem TMainMenu::addItem(const std::string& id, const size_t captionID, const std::string& caption, const std::string& link, const int level, const std::string& glyph, const bool active) {
	if (!caption.empty() && !link.empty()) {
		PMainMenuItem o = new TMainMenuItem;
		o->value.id = id;
		o->value.level = level;
		o->value.captionID = captionID;
		o->value.caption = caption;
		o->value.glyph = glyph;
		o->value.link = link;
		if (!activated && active) {
			o->value.active = true;
			activated = true;
		}
		menu.push_back(o);
		anchor = o;
		return o;
	}
	if (caption.empty())
		throw util::app_error("TMainMenu::addItem() Empty caption.");
	if (link.empty())
		throw util::app_error("TMainMenu::addSubItem() Empty link.");
	throw util::app_error("TMainMenu::addItem() Undefined error.");
}

PMenuItem TMainMenu::addSubItem(const std::string& id, const std::string& caption, const std::string& link, const int level, const std::string& glyph) {
	// Add entry without translation support
	return addSubItem(id, 0, caption, link, level, glyph);
}

PMenuItem TMainMenu::addSubItem(const std::string& id, const size_t captionID, const std::string& caption, const std::string& link, const int level, const std::string& glyph) {
	if (util::assigned(anchor) && !caption.empty() && !link.empty()) {
		TMenuItem item;
		item.id = id;
		item.level = level;
		item.active = true;
		item.captionID = captionID;
		item.caption = caption;
		item.glyph = glyph;
		item.link = link;
		return anchor->addItem(item);
	}
	if (!util::assigned(anchor))
		throw util::app_error("TMainMenu::addSubItem() Missing node to add sub item.");
	if (caption.empty())
		throw util::app_error("TMainMenu::addSubItem() Empty caption.");
	if (link.empty())
		throw util::app_error("TMainMenu::addSubItem() Empty link.");
	throw util::app_error("TMainMenu::addSubItem() Undefined error.");
}

void TMainMenu::addSubHeader(const std::string& caption) {
	if (util::assigned(anchor) && !caption.empty()) {
		anchor->addHeader(caption);
	}
}

void TMainMenu::addSubSeparator() {
	if (util::assigned(anchor)) {
		anchor->addSeparator();
	}
}

void TMainMenu::clearSubMenu(PMainMenuItem item) {
	if (util::assigned(item)) {
		item->clearSubMenu();
	}
}

void TMainMenu::setAnchor(PMainMenuItem item) {
	anchor = item;
}


std::string TMainMenu::completeLink(const std::string& link) const {
	if (isJavascript(link)) {
		if (!link.empty()) {
			char last = link[link.size() - 1];
			if (last == ')')
				return link + ';';
		}
		return link;
	}
	if (link.empty())
		return link;
	if (std::string::npos != link.find('#'))
		return link;
	if (link[0] == '/')
		return link;
	return root + link;
}	

const std::string& TMainMenu::text() const {
	if (/*addCSSToken() &&*/ !menu.empty() && strText.empty()) {
		size_t i, j;
		std::string tag;

		// Generate new item list from given menu entries
		items.clear();
		items.add("<div class=\"navbar-header\">");
		items.add("  <button type=\"button\" class=\"navbar-toggle collapsed\" data-toggle=\"collapse\" data-target=\"#" + getID() + "\" aria-expanded=\"false\" aria-controls=\"navbar\">");
		items.add("    <span class=\"sr-only\">Toggle navigation</span>");
		items.add("    <span class=\"icon-bar\"></span>");
		items.add("    <span class=\"icon-bar\"></span>");
		items.add("    <span class=\"icon-bar\"></span>");
		items.add("  </button>");
		if (tooltip.empty()) {
			if (onclick.empty())
				items.add("  <a class=\"navbar-brand\" href=\"#\"><img alt=\"&copy; db Application\" src=\"" + logo + "\"></a>");
			else
				items.add("  <a class=\"navbar-brand\"><img alt=\"&copy; db Application\" src=\"" + logo + "\" onclick=\"" + onclick + "\"></a>");
		} else {
			std::string hint = translate(tooltipID, tooltip);
			if (onclick.empty())
				items.add("  <a class=\"navbar-brand\" href=\"#\"><img alt=\"&copy; db Application\" src=\"" + logo + "\" title=\"" + hint + "\"></a>");
			else
				items.add("  <a class=\"navbar-brand\"><img alt=\"&copy; db Application\" src=\"" + logo + "\" onclick=\"" + onclick + "\" title=\"" + hint + "\"></a>");
		}
		items.add("</div>");

		items.add("<div id=\"" + getID() + "\" class=\"navbar-collapse collapse\">");
		items.add("  <ul class=\"nav navbar-nav\">");
		for (i=0; i<menu.size(); ++i) {
			PMainMenuItem o = menu[i];
			if (util::assigned(o)) {
				bool pull = (i == util::pred(menu.size()) && (ECA_RIGHT == getAlign()));
				std::string caption = o->value.caption;
				size_t captionID = o->value.captionID;
				if (hasTranslator()) {
					caption = translate(captionID, caption);
				}
				std::string text = encodeHtmlText(caption);
				if (o->hasMenu()) {
					if (pull) {
						items.add("  </ul>");
						items.add("  <ul class=\"nav navbar-nav navbar-right\">");
					}
					items.add("    <li" + tagToStr("id", o->value.id) + tagToStr("userlevel", std::to_string(o->value.level)) + " class=\"dropdown\">");
					if (o->value.glyph.empty())
						items.add("      <a href=\"#\" class=\"dropdown-toggle\" data-toggle=\"dropdown\" role=\"button\" aria-expanded=\"false\">" + text + HTML_SPACE + "<span class=\"caret\"></span></a>");
					else
						items.add("      <a href=\"#\" class=\"dropdown-toggle\" data-toggle=\"dropdown\" role=\"button\" aria-expanded=\"false\"><span class=\"glyphicon " + o->value.glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + text + HTML_SPACE + "<span class=\"caret\"></span></a>");
					items.add("      <ul class=\"dropdown-menu\" role=\"menu\">");
					if (o->hasMenu()) {

						// Get max length of menu entries
						//size_t length = o->getMenuItemsLength();

						// Add submenu entries
						for (j=0; j<o->submenu.size(); ++j) {
							PMenuItem p = o->submenu[j];
							if (util::assigned(p)) {
								//std::string caption = padTextRight(p->caption, length);
								size_t captionID = p->captionID;
								std::string caption = p->caption;
								const std::string& glyph = p->glyph;
								const std::string& link = p->link;
								const std::string& id = p->id;
								if (hasTranslator()) {
									caption = translate(captionID, caption);
								}
								text = encodeHtmlText(caption);
								int level = p->level;
								if (caption == "-") {
									items.add("        <li class=\"divider\"></li>");
								} else {
									if (link.empty()) {
										items.add("        <li class=\"dropdown-header\">" + text + "</li>");
									} else {
										tag = isJavascript(link) ? "onclick" : "href";
										if (p->active) {
											if (glyph.empty())
												items.add("        <li" + tagToStr("id", id) + tagToStr("userlevel", std::to_string(level)) + "><a" + tagToStr(tag, completeLink(link)) + ">" + text + "</a></li>");
											else
												items.add("        <li" + tagToStr("id", id) + tagToStr("userlevel", std::to_string(level)) + "><a" + tagToStr(tag, completeLink(link)) + "><span class=\"glyphicon " + glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + HTML_SPACE + text + "</a></li>");
										} else {
											if (glyph.empty())
												items.add("        <li" + tagToStr("id", id) + tagToStr("userlevel", std::to_string(level)) + " class=\"disabled\"><a>" + text + "</a></li>");
											else
												items.add("        <li" + tagToStr("id", id) + tagToStr("userlevel", std::to_string(level)) + " class=\"disabled\"><a><span class=\"glyphicon " + glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + HTML_SPACE + text + "</a></li>");
										}
									}
								}
							}
						}
					}
					items.add("      </ul>");
					items.add("    </li>");
				} else {
					std::string caption = o->value.caption;
					size_t captionID = o->value.captionID;
					if (hasTranslator()) {
						caption = translate(captionID, caption);
					}
					std::string text = encodeHtmlText(caption);
					std::string li = "    <li" + tagToStr("id", o->value.id) + tagToStr("userlevel", std::to_string(o->value.level));
					if (o->value.active)
						li += " class=\"active\">";
					else
						li += ">";
					if (pull) {
						items.add("  </ul>");
						items.add("  <ul class=\"nav navbar-nav navbar-right\">");
					}
					tag = isJavascript(o->value.link) ? "onclick" : "href";
					if (o->value.glyph.empty())
						items.add(li + "<a" + tagToStr(tag, completeLink(o->value.link)) + ">" + text + "</a></li>");
					else
						items.add(li + "<a" + tagToStr(tag, completeLink(o->value.link)) + "><span class=\"glyphicon " + o->value.glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>" + HTML_SPACE + text + "</a></li>");
				}
			}
		}
		items.add("  </ul>");
		items.add("</div>");

		// Calculate estimated destination size:
		size_t n = 0;
		const_iterator it = begin();
		for (; it != end(); ++it)
			n += (*it).size() + 2;
		strText.reserve(n);

		// Concate strings...
		strText = items.raw('\n');

	}
	return strText;
}

const std::string& TMainMenu::html() const {
	if (strHTML.empty()) {
		strHTML = "<nav class=\"navbar navbar-inverse navbar-fixed-top\">\n";
		strHTML += "<div id=\"navbar-menu-container\" class=\"container\">\n";
		strHTML += text();
		strHTML += "\n</div>";
		strHTML += "\n</nav>";
	}
	return strHTML;
}

void TMainMenu::invalidate() {
	anchor = nil;
	activated = false;
	TComponent::invalidate();
}



TTileItem::TTileItem() {
	level = 0;
	size = ESZ_MEDIUM;
	align = ECA_DEFAULT;
	captionID = 0;
	textID = 0;
}

TTileItem::~TTileItem() {
}

const std::string TTileItem::getCaption() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return caption;
};

void TTileItem::setCaption(const std::string& value) {
	app::TLockGuard<app::TMutex> lock(mtx);
	captionID = 0;
	caption = value;
};

const std::string TTileItem::getText() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return text;
};

void TTileItem::setText(const std::string& value) {
	app::TLockGuard<app::TMutex> lock(mtx);
	textID = 0;
	text = value;
};

const std::string TTileItem::getLink() const {
	app::TLockGuard<app::TMutex> lock(mtx);
	return link;
};

void TTileItem::setLink(const std::string& value) {
	app::TLockGuard<app::TMutex> lock(mtx);
	link = value;
};

void TTileItem::getProperties(size_t& captionID, std::string& caption, size_t& textID, std::string& text, std::string& link) const {
	app::TLockGuard<app::TMutex> lock(mtx);
	captionID = this->captionID;
	caption = this->caption;
	textID = this->textID;
	text = this->text;
	link = this->link;
}


TTileMenu::TTileMenu() : TComponent() {
	prime();
}

TTileMenu::TTileMenu(const std::string& name) : TComponent() {
	prime();
	setName(name);
}

TTileMenu::~TTileMenu() {
	util::clearObjectList(menu);
}

void TTileMenu::prime() {
	root = "/";
	defVal = "<invalid>";
	items.setTag("li");
	setID("tilesbar");
}

void TTileMenu::invalidate() {
	TComponent::invalidate();
}

PTileItem TTileMenu::addItem(const std::string& id, const std::string& caption, const std::string& text, const std::string& link,
		const EComponentSize size, const int level, const std::string& glyph, const EComponentAlign align) {
	return addItem(id, 0, caption, 0, text, link, size, level, glyph, align);
}

PTileItem TTileMenu::addItem(const std::string& id, const size_t captionID, const std::string& caption, const size_t textID, const std::string& text, const std::string& link,
		const EComponentSize size, const int level, const std::string& glyph, const EComponentAlign align) {
	PTileItem o = new TTileItem;
	o->level = level;
	o->captionID = captionID;
	o->caption = caption;
	o->textID = textID;
	o->text = text;
	o->glyph = glyph;
	o->link = link;
	o->name = id;
	o->id = id;
	o->size = size;
	o->align = align;
	menu.push_back(o);
	return o;
}

std::string TTileMenu::completeLink(const std::string& link) const {
	if (link.empty())
		return link;
	if (std::string::npos != link.find('#'))
		return link;
	if (link[0] == '/')
		return link;
	return root + link;
}

const std::string& TTileMenu::text() const {
	if (!menu.empty() && strText.empty()) {
		items.clear();

		for (size_t i=0; i<menu.size(); ++i) {
			PTileItem o = menu[i];
			if (util::assigned(o)) {
				size_t captionID, textID;
				std::string caption, text, link;
				o->getProperties(captionID, caption, textID, text, link);
				if (hasTranslator()) {
					caption = translate(captionID, caption);
					text = translate(textID, text);
				}

				// Example entry:
				//	<div class='col-md-6' onclick="onTileClick('/app/system/appconf.html');">
				//		<div>
				//			<h4><span class="glyphicon glyphicon-cog" style="pointer-events:none;" aria-hidden="true"></span>&nbsp;Einstellungen</h4>
				//			<h5>Maschinenparameter editieren und Eingangsbelegung festlegen</h5>
				//		</div>
				//	</div>

				// Generate new item list from given menu entries
				if (ECA_RIGHT == o->align) {
					if (ESZ_MEDIUM == o->size) {
						items.add("    <div" + tagToStr("id", getName() + "_" + std::to_string(i)) + " class=\"col-tiles col-md-3 pull-md-right\"" + tagToStr("userlevel", std::to_string(o->level)) + " onclick=\"onTileClick('" + completeLink(link) + "');\">");
					} else {
						items.add("    <div" + tagToStr("id", getName() + "_" + std::to_string(i)) + " class=\"col-tiles col-md-6 pull-md-right\"" + tagToStr("userlevel", std::to_string(o->level)) + " onclick=\"onTileClick('" + completeLink(link) + "');\">");
					}
				} else {
					if (ESZ_MEDIUM == o->size) {
						items.add("    <div" + tagToStr("id", getName() + "_" + std::to_string(i)) + " class=\"col-tiles col-md-3\"" + tagToStr("userlevel", std::to_string(o->level)) + " onclick=\"onTileClick('" + completeLink(link) + "');\">");
					} else {
						items.add("    <div" + tagToStr("id", getName() + "_" + std::to_string(i)) + " class=\"col-tiles col-md-6\"" + tagToStr("userlevel", std::to_string(o->level)) + " onclick=\"onTileClick('" + completeLink(link) + "');\">");
					}
				}
				items.add("      <div>");
				if (o->glyph.empty()) {
					items.add("        <h4>" + caption + "</h4>");
				} else {
					items.add("        <h4><span class=\"glyphicon " + o->glyph + "\" style=\"pointer-events:none;\" aria-hidden=\"true\"></span>&nbsp;" + caption + "</h4>");
				}
				if (!o->text.empty()) {
					items.add("        <h5>"  + text + "</h5>");
				}
				items.add("      </div>");
				items.add("    </div>");
			}
		}

		// Calculate estimated destination size:
		size_t n = 0;
		const_iterator it = begin();
		for (; it != end(); ++it)
			n += (*it).size() + 2;
		strText.reserve(n);

		// Concate strings...
		strText = items.raw('\n');
	}
	return strText;
}

const std::string& TTileMenu::html() const {
	if (strHTML.empty()) {
		strHTML = "<div class=\"container-fluid\">\n";
		strHTML += "  <div" + tagToStr("id", getName()) + " class=\"row blog\">\n";
		strHTML += text();
		strHTML += "\n  </div>";
		strHTML += "\n</div>\n";
	}
	return strHTML;
}


} /* namespace util */

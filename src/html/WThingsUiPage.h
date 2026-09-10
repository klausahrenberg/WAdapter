#ifndef W_THINGS_UI_PAGE_H
#define W_THINGS_UI_PAGE_H

#include "../WDevice.h"
#include "WebPage.h"

/*
 * A control panel that builds itself from the devices of the application. The
 * cards, the rows and the controls are made here as WebControls, so the
 * browser is sent finished html and reads no description of its own. Every
 * control carries the id '<device>.<property>' and speaks over the websocket
 * the page is holding anyway: a click or an edit arrives as an onchange event
 * of that id, and the other way round every control listens to its property,
 * so a value that changes in the device is pushed to the browser the moment it
 * does. Nothing is asked for in a cycle. So every WDevice is shown and
 * operated without a line of code written for it.
 */

#define SIZE_THING_ID 64
#define SIZE_THING_VALUE 128
//An enum is a row of buttons up to this many short options, a list above.
#define MAX_SEGMENTED_OPTIONS 4
#define MAX_SEGMENTED_LENGTH 9

const static char WC_THINGS_UI_NAME[] PROGMEM = "ThingsUi";
const static char WC_PROPERTY_UPDATE[] PROGMEM = "propertyUpdate";
const static char WC_PROPERTY_SET[] PROGMEM = "propertySet";
const static char WC_ON_REFRESH[] PROGMEM = "onrefresh";
//Tags and attributes the shell does not bring along
const static char WC_ITALIC[] PROGMEM = "i";
const static char WC_MINIMUM[] PROGMEM = "min";
const static char WC_MAXIMUM[] PROGMEM = "max";
const static char WC_STEP[] PROGMEM = "step";
const static char WC_RANGE[] PROGMEM = "range";
const static char WC_NUMBER[] PROGMEM = "number";
const static char WC_ROLE[] PROGMEM = "role";
const static char WC_SWITCH_ROLE[] PROGMEM = "switch";
const static char WC_ARIA_CHECKED[] PROGMEM = "aria-checked";
const static char WC_ARIA_PRESSED[] PROGMEM = "aria-pressed";
const static char WC_ON_INPUT[] PROGMEM = "oninput";
const static char WC_DATA_UNIT[] PROGMEM = "data-u";
//The classes of the shell style sheet, the ones of the card are with it
const static char WC_CLASS_PILL[] PROGMEM = "pill";
const static char WC_CLASS_PILL_ON[] PROGMEM = "pill on";
const static char WC_CLASS_SWITCH[] PROGMEM = "sw";
const static char WC_CLASS_SEGMENT[] PROGMEM = "seg";
const static char WC_ON[] PROGMEM = "On";
const static char WC_OFF[] PROGMEM = "Off";
const static char WC_NO_DEVICE[] PROGMEM = "No device found.";
const static char WC_NO_PROPERTIES[] PROGMEM = "No properties.";
//The handlers the controls are wired to
const static char WC_CALL_THING_CHANGE[] PROGMEM = "thingChange(this)";
const static char WC_CALL_THING_SWITCH[] PROGMEM = "thingSwitch(this)";
const static char WC_CALL_THING_SEGMENT[] PROGMEM = "thingSegment(this)";
const static char WC_CALL_THING_SLIDE[] PROGMEM = "thingSlide(this)";
const static char WC_CALL_THING_RANGE[] PROGMEM = "thingRange(this)";

const static char WC_SCRIPT_THINGS_UI[] PROGMEM = R"=====(
var TLOCK = {};

//A value just sent must not be overwritten by an update that was on its way
//with the value of before, so the control is left alone for a moment.
function thingSend(id, value) {
  TLOCK[id] = Date.now() + 1500;
  sendWebSocketMessage("onchange", id, {"value": String(value)});
}

function thingChange(elem) {
  thingSend(elem.id, elem.value);
}

function thingSwitch(button) {
  var on = (button.getAttribute("aria-checked") !== "true");
  button.setAttribute("aria-checked", on);
  thingSend(button.id, on);
}

function thingMark(seg, value) {
  var buttons = seg.children;
  for (var i = 0; i < buttons.length; i++) {
    buttons[i].setAttribute("aria-pressed", buttons[i].textContent === value);
  }
}

function thingSegment(button) {
  var seg = button.parentNode;
  thingMark(seg, button.textContent);
  thingSend(seg.id, button.textContent);
}

//The box of a slider holds the slider and the number it stands at
function thingShow(box, value) {
  box.lastElementChild.textContent = value + (box.dataset.u ? " " + box.dataset.u : "");
}

function thingSlide(slider) {
  thingShow(slider.parentNode, slider.value);
}

function thingRange(slider) {
  thingShow(slider.parentNode, slider.value);
  thingSend(slider.parentNode.id, slider.value);
}

//The page was printed with the values of that moment and the socket opened a
//bit later, so what changed in between is asked for once. Everything after
//that comes on its own.
function onSocketOpen() {
  sendWebSocketMessage("onrefresh", null, null);
}

//The answer to something this browser sent itself: it is the value the device
//really took, so it counts more than the one that is shown and the lock of the
//control is dropped for it.
function propertySet(json) {
  delete TLOCK[json.id];
  propertyUpdate(json);
}

//The device pushes a new value over the socket the controls send on. What kind
//of control it is, is read from the class it was printed with.
function propertyUpdate(json) {
  var elem = document.getElementById(json.id), value = json.data;
  if ((elem === null) || (TLOCK[json.id] > Date.now()) || (elem === document.activeElement)) return;
  var cls = elem.className;
  if (cls === "sw") {
    elem.setAttribute("aria-checked", (value === "true"));
  } else if (cls === "seg") {
    thingMark(elem, value);
  } else if (cls === "ctl") {
    var slider = elem.firstElementChild;
    if (slider !== document.activeElement) {
      slider.value = value;
      thingShow(elem, value);
    }
  } else if (cls.indexOf("pill") === 0) {
    elem.className = (value === "true" ? "pill on" : "pill");
    elem.textContent = (value === "true" ? "On" : "Off");
  } else if (cls === "val") {
    elem.textContent = value + (elem.dataset.u ? " " + elem.dataset.u : "");
  } else {
    elem.value = value;
  }
}
)=====";

/**
 * A control that shows one property. It listens to it and keeps the value it
 * last told the browser, so a change is sent the moment it happens and no
 * change is sent twice.
 */
class WThingControl : public WebControl {
 public:
  WThingControl(const char* tag, const char* id, WProperty* property)
      : WebControl(tag, nullptr) {
    _property = property;
    _read();
    this->id(id);
    //the device tells the control, the control tells the browser
    _property->addListener([this]() { _propertyChanged(); }, this);
  }

  virtual ~WThingControl() {
    //the property outlives the page, it must not call into a control that is gone
    _property->removeListeners(this);
    if (_last) delete[] _last;
  }

  virtual void createScripts(WStringList* scripts) {
    WebControl::createScripts(scripts);
    //the list keeps one script per id, so it is written once for all controls
    scripts->add(WC_SCRIPT_THINGS_UI, WC_THINGS_UI_NAME);
  }

  /** Tells the browser the value, whether it has it already or not. */
  void refresh() {
    _read();
    _notify(WC_PROPERTY_UPDATE);
  }

  virtual void handleEvent(WValue* event, WList<WValue>* data) {
    if ((data == nullptr) || (!event->equals(WC_ON_CHANGE))) return;
    WValue* value = data->getById(WC_VALUE);
    if (value == nullptr) return;
    LOG->notice(F("Set property '%s' to value '%s' (web socket)"), this->id(), value->asString());
    //the answer below tells what the device really took, the listener would
    //only say the same a second time
    _setting = true;
    _property->parse(value->asString());
    _setting = false;
    //the device may have taken another value than it was sent, or none at all,
    //so the value it really has goes back in any case: the browser that asked
    //shows what it sent and would keep it otherwise
    _read();
    _notify(WC_PROPERTY_SET);
  }

  //a string is printed as it is, everything else the way json writes it
  static void printValue(Print* stream, WValue* value) {
    if (value == nullptr) return;
    if (value->type() == WDataType::STRING) {
      stream->print(value->asString());
    } else {
      WValue::toString(stream, value);
    }
  }

 protected:
  WProperty* _property;
  char* _last = nullptr;

  bool _setting = false;

  //one buffer for all controls, everything here happens in the main loop
  static WStringStream* _buffer() {
    static WStringStream buffer(SIZE_THING_VALUE);
    return &buffer;
  }

  /** Reads the property and answers whether it stands at another value than
   * the browser was told the last time. */
  bool _read() {
    _property->requestValue();
    WStringStream* stream = _buffer();
    stream->flush();
    printValue(stream, _property->value());
    if ((_last != nullptr) && (strcmp(_last, stream->c_str()) == 0)) return false;
    if (_last) delete[] _last;
    _last = WString::duplicate(stream->c_str());
    return true;
  }

  void _notify(const char* event) {
    WebAppSockets::sendMessage(event, this->id(), _last);
  }

  //the value changed in the device, the browser hears of it right away
  void _propertyChanged() {
    if ((!_setting) && (_read())) _notify(WC_PROPERTY_UPDATE);
  }

  bool _isOn() { return ((_last != nullptr) && (strcmp_P(_last, WC_TRUE) == 0)); }

  bool _isValue(const char* value) {
    //value can point to PROGMEM, _last is always in RAM
    return ((_last != nullptr) && (value != nullptr) && (strcmp_P(_last, value) == 0));
  }

  void _printValueAndUnit(Print* stream) {
    if (_last) stream->print(_last);
    if (_property->unit() != nullptr) {
      stream->print(WC_SPACE);
      //a unit can lie in the flash, so read it byte safe
      stream->print(FPSTR(_property->unit()));
    }
  }
};

/** The value of a property that is read only, with its unit behind it. */
class WThingText : public WThingControl {
 public:
  WThingText(const char* id, WProperty* property) : WThingControl(WC_SPAN, id, property) {
    param(WC_CLASS, WC_CLASS_VALUE);
    if (property->unit() != nullptr) param(WC_DATA_UNIT, property->unit());
    contentFactory([this](Print* stream) { _printValueAndUnit(stream); });
  }
};

/** A boolean that is read only: a badge that says On or Off. */
class WThingPill : public WThingControl {
 public:
  WThingPill(const char* id, WProperty* property) : WThingControl(WC_SPAN, id, property) {
    param(WC_CLASS, (_isOn() ? WC_CLASS_PILL_ON : WC_CLASS_PILL));
    //the words lie in the flash, so read them byte safe
    contentFactory([this](Print* stream) { stream->print(FPSTR(_isOn() ? WC_ON : WC_OFF)); });
  }
};

/** A boolean that can be set: the slider known from a phone. */
class WThingSwitch : public WThingControl {
 public:
  WThingSwitch(const char* id, WProperty* property) : WThingControl(WC_BUTTON, id, property) {
    param(WC_CLASS, WC_CLASS_SWITCH);
    param(WC_ROLE, WC_SWITCH_ROLE);
    param(WC_ARIA_CHECKED, (_isOn() ? WC_TRUE : WC_FALSE));
    param(WC_ON_CLICK, WC_CALL_THING_SWITCH);
    //the knob of the switch, the style sheet moves it
    this->add(new WebControl(WC_ITALIC, nullptr));
  }

  virtual void createStyles(WStringList* styles) {
    styles->add(WC_SWITCH_STYLE, WC_SWITCH_ID);
    styles->add(WC_SWITCH_STYLE_CHECKED, WC_SWITCH_ID_CHECKED);
    styles->add(WC_SWITCH_STYLE_I, WC_SWITCH_ID_I);
    styles->add(WC_SWITCH_STYLE_CHECKED_I, WC_SWITCH_ID_CHECKED_I);
    WebControl::createStyles(styles);
  }
};

/** A few short options: all of them side by side, the current one marked. */
class WThingSegment : public WThingControl {
 public:
  WThingSegment(const char* id, WProperty* property) : WThingControl(WC_DIV, id, property) {
    param(WC_CLASS, WC_CLASS_SEGMENT);
    property->enums()->forEach([this](int index, WValue* option, const char* optionId) {
      WStringStream text(SIZE_THING_VALUE);
      printValue(&text, option);
      WebControl* button = new WebControl(WC_BUTTON, WC_ON_CLICK, WC_CALL_THING_SEGMENT,
                                          WC_ARIA_PRESSED, (_isValue(text.c_str()) ? WC_TRUE : WC_FALSE), nullptr);
      this->add(button->content(text.c_str()));
    });
  }
};

/** More or longer options: a drop down list. */
class WThingSelect : public WThingControl {
 public:
  WThingSelect(const char* id, WProperty* property) : WThingControl(WC_SELECT, id, property) {
    param(WC_ON_CHANGE, WC_CALL_THING_CHANGE);
    property->enums()->forEach([this](int index, WValue* option, const char* optionId) {
      WStringStream text(SIZE_THING_VALUE);
      printValue(&text, option);
      WebControl* item = new WebControl(WC_OPTION, WC_VALUE, text.c_str(), nullptr);
      //html5 reads the bare attribute as selected="selected"
      if (_isValue(text.c_str())) item->param(WC_SELECTED, "");
      this->add(item->content(text.c_str()));
    });
  }
};

/** A number between a minimum and a maximum: a slider and the value at it. */
class WThingRange : public WThingControl {
 public:
  WThingRange(const char* id, WRangeProperty* property) : WThingControl(WC_DIV, id, property) {
    param(WC_CLASS, CSS_CARD_CTL_CLASS);
    if (property->unit() != nullptr) param(WC_DATA_UNIT, property->unit());
    bool isDouble = (property->type() == WDataType::DOUBLE);
    String minimum = (isDouble ? String(property->getMinAsDouble()) : String(property->getMinAsInteger()));
    String maximum = (isDouble ? String(property->getMaxAsDouble()) : String(property->getMaxAsInteger()));
    WebControl* slider = new WebControl(WC_INPUT, WC_TYPE, WC_RANGE, nullptr);
    slider->closing(false);
    slider->param(WC_MINIMUM, minimum.c_str());
    slider->param(WC_MAXIMUM, maximum.c_str());
    slider->param(WC_STEP, stepOf(property).c_str());
    slider->param(WC_VALUE, _last);
    slider->param(WC_ON_INPUT, WC_CALL_THING_SLIDE);
    slider->param(WC_ON_CHANGE, WC_CALL_THING_RANGE);
    this->add(slider);
    WebControl* out = new WebControl(WC_SPAN, WC_CLASS, WC_CLASS_VALUE, nullptr);
    out->contentFactory([this](Print* stream) { _printValueAndUnit(stream); });
    this->add(out);
  }

  /** How far a value may be turned at once, one step where nothing is said. */
  static String stepOf(WProperty* property) {
    if (property->hasMultipleOf()) return String(property->multipleOf());
    return String(1);
  }
};

/** Everything else: a field to write the value into. */
class WThingInput : public WThingControl {
 public:
  WThingInput(const char* id, WProperty* property) : WThingControl(WC_INPUT, id, property) {
    param(WC_TYPE, (property->type() == WDataType::STRING ? WC_TEXT : WC_NUMBER));
    if (property->hasMultipleOf()) param(WC_STEP, WThingRange::stepOf(property).c_str());
    param(WC_VALUE, _last);
    param(WC_ON_CHANGE, WC_CALL_THING_CHANGE);
    closing(false);
  }
};

/** Holds the cards of the devices, one row per property. */
class WThingsUiPage : public WebPage {
 public:
  WThingsUiPage(WList<WDevice>* devices) : WebPage(nullptr, true) {
    _devices = devices;
  }

  virtual ~WThingsUiPage() {
    if (_controls != nullptr) {
      //the controls belong to the tree of the page, not to this list
      while (!_controls->empty()) _controls->remove(0, false);
      delete _controls;
    }
  }

  virtual WebControl* createControls() {
    WebDiv* result = new WebDiv();
    if (_controls == nullptr) _controls = new WList<WThingControl>();
    byte cards = 0;
    if (_devices != nullptr) {
      _devices->forEach([this, result, &cards](int index, WDevice* device, const char* id) {
        if (device->isVisible(WEBTHING)) {
          result->add(_createCard(device));
          cards++;
        }
      });
    }
    if (cards == 0) result->add(_createMessage(WC_NO_DEVICE));
    return result;
  }

  /**
   * A browser that has just opened its socket asks for the values, so what
   * changed between the printed page and the socket is not missed.
   */
  virtual void handleEvent(WValue* event, WList<WValue>* data) {
    if ((_controls != nullptr) && (event->equals(WC_ON_REFRESH))) {
      _controls->forEach([](int index, WThingControl* control, const char* id) { control->refresh(); });
    }
  }

 private:
  WList<WDevice>* _devices;
  //the controls of the page in a row, so a refresh needs no walk of the tree
  WList<WThingControl>* _controls = nullptr;

  WebControl* _createMessage(const char* message) {
    return (new WebControl(WC_DIV, WC_CLASS, WC_CLASS_MESSAGE, nullptr))->content(message);
  }

  WebControl* _createCard(WDevice* device) {
    WebCard* card = new WebCard((device->title() != nullptr ? device->title() : device->id()), device->type());
    byte rows = 0;
    char id[SIZE_THING_ID];
    device->properties()->forEach([this, device, card, &rows, &id](int index, WProperty* property, const char* propertyId) {
      if ((property->isVisible(WEBTHING)) && (propertyId != nullptr)) {
        snprintf(id, SIZE_THING_ID, "%s.%s", device->id(), propertyId);
        WThingControl* control = _createControl(id, property);
        _controls->add(control);
        const char* title = property->title();
        //title can point to PROGMEM, so don't compare it byte wise
        card->addRow(((title != nullptr) && (strlen_P(title) != 0) ? title : propertyId), control, property->readOnly());
        rows++;
      }
    });
    if (rows == 0) card->addMessage(WC_NO_PROPERTIES);
    //the bar shows the name and the revision, the address is told here
    if (device->isMainDevice()) card->addNote(deviceIp().c_str());
    return card;
  }

  WThingControl* _createControl(const char* id, WProperty* property) {
    if (property->readOnly()) {
      return ((property->type() == WDataType::BOOLEAN)
                  ? (WThingControl*)new WThingPill(id, property)
                  : (WThingControl*)new WThingText(id, property));
    }
    if ((property->hasEnums()) && (property->enumsCount() > 0)) {
      return (_isSegmented(property)
                  ? (WThingControl*)new WThingSegment(id, property)
                  : (WThingControl*)new WThingSelect(id, property));
    }
    if (property->type() == WDataType::BOOLEAN) return new WThingSwitch(id, property);
    if (property->isRange()) return new WThingRange(id, static_cast<WRangeProperty*>(property));
    //an array of bytes is nothing to write in a field
    if (property->type() == WDataType::BYTE_ARRAY) return new WThingText(id, property);
    return new WThingInput(id, property);
  }

  /** Whether the options fit side by side rather than in a drop down list. */
  bool _isSegmented(WProperty* property) {
    if (property->enumsCount() > MAX_SEGMENTED_OPTIONS) return false;
    bool wide = false;
    property->enums()->forEach([&wide](int index, WValue* option, const char* optionId) {
      WStringStream text(SIZE_THING_VALUE);
      WThingControl::printValue(&text, option);
      if (text.length() > MAX_SEGMENTED_LENGTH) wide = true;
    });
    return (!wide);
  }
};

#endif

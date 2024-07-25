#ifndef _WJSON_PARSER_H__
#define _WJSON_PARSER_H__

#include "Arduino.h"
#include "WJson.h"
#include "WLog.h"

#define BUFFER_MAX_LENGTH 64

enum WState {
  WS_START_DOCUMENT,
  WS_DONE,
  WS_IN_ARRAY,
  WS_IN_OBJECT,
  WS_END_KEY,
  WS_AFTER_KEY,
  WS_IN_STRING,
  WS_START_ESCAPE,
  WS_UNICODE,
  WS_IN_NUMBER,
  WS_IN_TRUE,
  WS_IN_FALSE,
  WS_IN_NULL,
  WS_AFTER_VALUE,
  WS_UNICODE_SURROGATE
};

enum WType {
  WT_OBJECT,
  WT_ARRAY,
  WT_KEY,
  WT_STRING
};

struct WMapItem {
  WType type;
  String objectId;
  WList<WValue>* mapOrList = nullptr;

  WMapItem(WType type, String objectId, WList<WValue>* mapOrList = nullptr) {
    this->type = type;
    this->objectId = objectId;
    this->mapOrList = mapOrList;
  }
};

class WJsonParser {
 public:

  WJsonParser(bool logging = false) {
    _logging = logging;
    _state = WS_START_DOCUMENT;
    _stack = new WStack<WMapItem>(true);
    _bufferPos = 0;
    _unicodeEscapeBufferPos = 0;
    _unicodeBufferPos = 0;
    _characterCounter = 0;
  }

  static WList<WValue>* asMap(const char* payload) {
    WJsonParser jp = WJsonParser();    
    return jp.parse(payload);
  }

  /*WStringList* parse(const char* payload) {
    WStringList* result = new WStringList();
    parse(payload, [this, result](const char* key, const char* value) {
      char* v = new char[strlen_P(value) + 1];
      strcpy_P(v, value);
      result->add(v, key);
    });
    return _stack->peek()->mapOrList;
  }*/

  WList<WValue>* parse(const char* payload) {
    for (int i = 0; i < strlen(payload); i++) {
      _parseChar(payload[i]);
    }
    LOG->debug("parsed..");
    if (_stack->empty()) {
      LOG->debug("stack is empty");
    }
    return _stack->peek()->mapOrList;
  }

  WProperty* parse(const char* payload, WDevice* device) {
    _device = device;
    WProperty* result = nullptr;
    for (int i = 0; i < strlen(payload); i++) {
      WProperty* p = _parseChar(payload[i]);
      if (p != nullptr) {
        result = p;
      }
    }
    return result;
  }

 private:
  WState _state;
  WStack<WMapItem>* _stack;
  // int _stack[20];
  // int _stackPos = 0;
  bool _doEmitWhitespace = false;
  char _buffer[BUFFER_MAX_LENGTH];
  int _bufferPos = 0;
  char _unicodeEscapeBuffer[10];
  int _unicodeEscapeBufferPos = 0;
  char _unicodeBuffer[10];
  int _unicodeBufferPos = 0;
  int _characterCounter = 0;
  int _unicodeHighSurrogate = 0;
  bool _logging = false;
  String _currentKey = "";
  WDevice* _device = nullptr;

  void _log(String message) {
    if (_logging) {
      Serial.println(message);
    }
  }

  WProperty* _processKeyValue(const char* key, const char* value) {
    WProperty* result = nullptr;
    if (_device != nullptr) {
      result = _device->getPropertyById(key);
      if (result != nullptr) {
        if (!result->parse(value)) {
          result = nullptr;
        }
      }

    } else {
			WMapItem* peeked = _stack->peek();
      if (peeked->mapOrList != nullptr) {               
      	if (value != nullptr) {
          peeked->mapOrList->add(new WValue(value), (_currentKey.equals("") ? nullptr : _currentKey.c_str()));
        }
         _currentKey = "";
      }
    }
    return result;

		

  }

  WProperty* _parseChar(char c) {
    WProperty* result = nullptr;
    if ((c == WC_SPACE || c == '\t' || c == '\n' || c == '\r') && !(_state == WS_IN_STRING || _state == WS_UNICODE || _state == WS_START_ESCAPE || _state == WS_IN_NUMBER || _state == WS_START_DOCUMENT)) {
      return result;
    }
    switch (_state) {
      case WS_IN_STRING:
        if ((c == WC_QUOTE) || (c == WC_QUOTE2)) {
          result = _endString();
        } else if (c == '\\') {
          _state = WS_START_ESCAPE;
        } else if ((c < 0x1f) || (c == 0x7f)) {
          // throw new RuntimeException("Unescaped control character encountered: " + c + " at position" + characterCounter);
        } else {
          _buffer[_bufferPos] = c;
          _increaseBufferPointer();
        }
        break;
      case WS_IN_ARRAY:
        if (c == WC_REND) {
          _endArray();
        } else {
          _startValue(c);
        }
        break;
      case WS_IN_OBJECT:
        if (c == WC_SEND) {
          _endObject();
        } else if ((c == WC_QUOTE) || (c == WC_QUOTE2)) {
          _startKey();
        } else {
          // throw new RuntimeException("Start of string expected for object key. Instead got: " + c + " at position" + characterCounter);
        }
        break;
      case WS_END_KEY:
        if (c != WC_DPOINT) {
          // throw new RuntimeException("Expected ':' after key. Instead got " + c + " at position" + characterCounter);
        }
        _state = WS_AFTER_KEY;
        break;
      case WS_AFTER_KEY:
        _startValue(c);
        break;
      case WS_START_ESCAPE:
        _processEscapeCharacters(c);
        break;
      case WS_UNICODE:
        _processUnicodeCharacter(c);
        break;
      case WS_UNICODE_SURROGATE:
        _unicodeEscapeBuffer[_unicodeEscapeBufferPos] = c;
        _unicodeEscapeBufferPos++;
        if (_unicodeEscapeBufferPos == 2) {
          _endUnicodeSurrogateInterstitial();
        }
        break;
      case WS_AFTER_VALUE: {
        // not safe for size == 0!!!
        WType within = _stack->peek()->type;
        if (within == WT_OBJECT) {
          if (c == WC_SEND) {
            _endObject();
          } else if (c == WC_COMMA) {
            _state = WS_IN_OBJECT;
          } else {
            // throw new RuntimeException("Expected ',' or '}' while parsing object. Got: " + c + ". " + characterCounter);
          }
        } else if (within == WT_ARRAY) {
          if (c == WC_REND) {
            _endArray();
          } else if (c == WC_COMMA) {
            _state = WS_IN_ARRAY;
          } else {
            // throw new RuntimeException("Expected ',' or ']' while parsing array. Got: " + c + ". " + characterCounter);
          }
        } else {
          // throw new RuntimeException("Finished a literal, but unclear what state to move to. Last state: " + characterCounter);
        }
      } break;
      case WS_IN_NUMBER:
        if (c >= '0' && c <= '9') {
          _buffer[_bufferPos] = c;
          _increaseBufferPointer();
        } else if (c == '.') {
          if (_doesCharArrayContain(_buffer, _bufferPos, '.')) {
            // throw new RuntimeException("Cannot have multiple decimal points in a number. " + characterCounter);
          } else if (_doesCharArrayContain(_buffer, _bufferPos, 'e')) {
            // throw new RuntimeException("Cannot have a decimal point in an exponent." + characterCounter);
          }
          _buffer[_bufferPos] = c;
          _increaseBufferPointer();
        } else if (c == 'e' || c == 'E') {
          if (_doesCharArrayContain(_buffer, _bufferPos, 'e')) {
            // throw new RuntimeException("Cannot have multiple exponents in a number. " + characterCounter);
          }
          _buffer[_bufferPos] = c;
          _increaseBufferPointer();
        } else if (c == '+' || c == '-') {
          char last = _buffer[_bufferPos - 1];
          if (!(last == 'e' || last == 'E')) {
            // throw new RuntimeException("Can only have '+' or '-' after the 'e' or 'E' in a number." + characterCounter);
          }
          _buffer[_bufferPos] = c;
          _increaseBufferPointer();
        } else {
          result = _endNumber();
          // we have consumed one beyond the end of the number
          _parseChar(c);
        }
        break;
      case WS_IN_TRUE:
        _buffer[_bufferPos] = c;
        _increaseBufferPointer();
        if (_bufferPos == 4) {
          result = _endTrue();
        }
        break;
      case WS_IN_FALSE:
        _buffer[_bufferPos] = c;
        _increaseBufferPointer();
        if (_bufferPos == 5) {
          result = _endFalse();
        }
        break;
      case WS_IN_NULL:
        _buffer[_bufferPos] = c;
        _increaseBufferPointer();
        if (_bufferPos == 4) {
          _endNull();
        }
        break;
      case WS_START_DOCUMENT:
        // myListener->startDocument();
        if (c == WC_RBEGIN) {
          _startArray();
        } else if (c == WC_SBEGIN) {
          _startObject();
        }
        break;
    }
    _characterCounter++;
    return result;
  }

  void _increaseBufferPointer() {
    _bufferPos = _min(_bufferPos + 1, BUFFER_MAX_LENGTH - 1);
  }

  WProperty* _endString() {
    WProperty* result = nullptr;
    WMapItem* popped = _stack->pop();
    if (popped->type == WT_KEY) {
      _buffer[_bufferPos] = '\0';
      _currentKey = String(_buffer);
      _state = WS_END_KEY;
    } else if (popped->type == WT_STRING) {      
      _buffer[_bufferPos] = '\0';
      const char* v = _buffer;
      result = _processKeyValue(_currentKey.c_str(), _buffer);      
      _state = WS_AFTER_VALUE;
    } else {
      // throw new ParsingError($this->_line_number, $this->_char_number,
      // "Unexpected end of string.");
    }
    _bufferPos = 0;
    return result;
  }

  void _startValue(char c) {
    if (c == WC_RBEGIN) {
      _startArray();
    } else if (c == WC_SBEGIN) {
      _startObject();
    } else if ((c == WC_QUOTE) || (c == WC_QUOTE2)) {
      _startString();
    } else if (isDigit(c)) {
      _startNumber(c);
    } else if ((c == 't') || (c == 'T')) {
      _state = WS_IN_TRUE;
      _buffer[_bufferPos] = c;
      _increaseBufferPointer();
    } else if ((c == 'f') || (c == 'F')) {
      _state = WS_IN_FALSE;
      _buffer[_bufferPos] = c;
      _increaseBufferPointer();
    } else if ((c == 'n') || (c == 'N')) {
      _state = WS_IN_NULL;
      _buffer[_bufferPos] = c;
      _increaseBufferPointer();
    } else {
      // throw new ParsingError($this->_line_number, $this->_char_number,
      // "Unexpected character for value: ".$c);
    }
  }

  bool _isDigit(char c) {
    // Only concerned with the first character in a number.
    return (c >= '0' && c <= '9') || c == '-';
  }

  void _endArray() {
    WMapItem* popped = _stack->pop();
    if (popped->type != WT_ARRAY) {
      // throw new ParsingError("Unexpected end of array encountered.");
    }
    _state = WS_AFTER_VALUE;
    if (_stack->empty()) {
      _stack->push(popped);
      _endDocument();
    //} else if ((_stack.peek().map() != null) && (!LString.isEmpty(popped.objectKey()))) {
    //        _stack.peek().map().put(popped.objectKey(), popped.list());  
    }
  }

  void _startKey() {
    _stack->push(new WMapItem(WT_KEY, "", nullptr));
    _state = WS_IN_STRING;
  }

  void _endObject() {
    WMapItem* popped = _stack->pop();
    if (popped->type != WT_OBJECT) {
      LOG->error(F("jsonParser->endObject(): Unexpected end of object encountered."));
    }
    if (popped->mapOrList != nullptr) {
      // tbi
			if (_stack->empty()) {
				_stack->push(popped);
			} else {
        _stack->peek()->mapOrList->add(new WValue(popped->mapOrList), (popped->objectId.equals("") ? nullptr : popped->objectId.c_str()));
			}	
                /*    _result = (T) o;
                } else if (_stack.peek().map() != null) {
                    _stack.peek().map().put(popped.objectKey(), o);
                } else if (_stack.peek().list() != null) {
                    _stack.peek().list().add(o);
                }*/
    } else {
      LOG->error(F("jsonParser->endObject(): Stack has no object map inside, can't create object."));
    }
    _state = WS_AFTER_VALUE;
    if (_stack->empty()) {
      _endDocument();
    }
  }

  void _processEscapeCharacters(char c) {
    if (c == WC_QUOTE) {
      _buffer[_bufferPos] = WC_QUOTE;
      _increaseBufferPointer();
    } else if (c == '\\') {
      _buffer[_bufferPos] = '\\';
      _increaseBufferPointer();
    } else if (c == WC_SLASH) {
      _buffer[_bufferPos] = WC_SLASH;
      _increaseBufferPointer();
    } else if (c == 'b') {
      _buffer[_bufferPos] = 0x08;
      _increaseBufferPointer();
    } else if (c == 'f') {
      _buffer[_bufferPos] = '\f';
      _increaseBufferPointer();
    } else if (c == 'n') {
      _buffer[_bufferPos] = '\n';
      _increaseBufferPointer();
    } else if (c == 'r') {
      _buffer[_bufferPos] = '\r';
      _increaseBufferPointer();
    } else if (c == 't') {
      _buffer[_bufferPos] = '\t';
      _increaseBufferPointer();
    } else if (c == 'u') {
      _state = WS_UNICODE;
    }
    if (_state != WS_UNICODE) {
      _state = WS_IN_STRING;
    }
  }

  void _processUnicodeCharacter(char c) {
    _unicodeBuffer[_unicodeBufferPos] = c;
    _unicodeBufferPos++;
    if (_unicodeBufferPos == 4) {
      int codepoint = _getHexArrayAsDecimal(_unicodeBuffer, _unicodeBufferPos);
      _endUnicodeCharacter(codepoint);
      return;
    }
  }

  bool _isHexCharacter(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

  int _getHexArrayAsDecimal(char hexArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = hexArray[length - i - 1];
      int value = 0;
      if (current >= 'a' && current <= 'f') {
        value = current - 'a' + 10;
      } else if (current >= 'A' && current <= 'F') {
        value = current - 'A' + 10;
      } else if (current >= '0' && current <= '9') {
        value = current - '0';
      }
      result += value * 16 ^ i;
    }
    return result;
  }

  bool _doesCharArrayContain(char myArray[], int length, char c) {
    for (int i = 0; i < length; i++) {
      if (myArray[i] == c) {
        return true;
      }
    }
    return false;
  }

  void _endUnicodeSurrogateInterstitial() {
    char unicodeEscape = _unicodeEscapeBuffer[_unicodeEscapeBufferPos - 1];
    _unicodeBufferPos = 0;
    _unicodeEscapeBufferPos = 0;
    _state = WS_UNICODE;
  }

  WProperty* _endNumber() {
    WProperty* result = nullptr;
    if (_currentKey != "") {
      _buffer[_bufferPos] = '\0';
      result = _processKeyValue(_currentKey.c_str(), _buffer);
    }
    _bufferPos = 0;
    _state = WS_AFTER_VALUE;
    return result;
  }

  int _convertDecimalBufferToInt(char myArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = myArray[length - i - 1];
      result += (current - '0') * 10;
    }
    return result;
  }

  void _endDocument() {
    _state = WS_DONE;
  }

  WProperty* _endTrue() {
    WProperty* result = nullptr;
    if (_currentKey != "") {
      _buffer[_bufferPos] = '\0';
      if (strcmp_P(_buffer, WC_TRUE) == 0) {
        result = _processKeyValue(_currentKey.c_str(), WC_TRUE);
      }
    }
    _bufferPos = 0;
    _state = WS_AFTER_VALUE;
    return result;
  }

  WProperty* _endFalse() {
    WProperty* result = nullptr;
    if (_currentKey != "") {
      _buffer[_bufferPos] = '\0';
      if (strcmp_P(_buffer, WC_FALSE) == 0) {
        result = _processKeyValue(_currentKey.c_str(), WC_FALSE);
      }
    }
    _bufferPos = 0;
    _state = WS_AFTER_VALUE;
    return result;
  }

  void _endNull() {
    _buffer[_bufferPos] = '\0';
    String value = String(_buffer);
    _bufferPos = 0;
    _state = WS_AFTER_VALUE;
  }

  void _startArray() {
    LOG->debug("startArray a");
    _state = WS_IN_ARRAY;
    _stack->push(new WMapItem(WT_ARRAY, _currentKey, new WList<WValue>()));
    _currentKey = "";
    LOG->debug("startArray b");
  }

  void _startObject() {
    LOG->debug("startObject a");
    _state = WS_IN_OBJECT;
    _stack->push(new WMapItem(WT_OBJECT, _currentKey, new WList<WValue>()));
    _currentKey = "";
    LOG->debug("startObject b");
  }

  void _startString() {
    _state = WS_IN_STRING;
    _stack->push(new WMapItem(WT_STRING, "", nullptr));
  }

  void _startNumber(char c) {
    _state = WS_IN_NUMBER;
    _buffer[_bufferPos] = c;
    _increaseBufferPointer();
  }

  void _endUnicodeCharacter(int codepoint) {
    _buffer[_bufferPos] = _convertCodepointToCharacter(codepoint);
    _increaseBufferPointer();
    _unicodeBufferPos = 0;
    _unicodeHighSurrogate = -1;
    _state = WS_IN_STRING;
  }

  char _convertCodepointToCharacter(int num) {
    if (num <= 0x7F)
      return (char)(num);
    return ' ';
  }
};

#endif

#ifndef _WJSON_PARSER_H__
#define _WJSON_PARSER_H__

#include "Arduino.h"
#include "WJson.h"

#define STATE_START_DOCUMENT     0
#define STATE_DONE               -1
#define STATE_IN_ARRAY           1
#define STATE_IN_OBJECT          2
#define STATE_END_KEY            3
#define STATE_AFTER_KEY          4
#define STATE_IN_STRING          5
#define STATE_START_ESCAPE       6
#define STATE_UNICODE            7
#define STATE_IN_NUMBER          8
#define STATE_IN_TRUE            9
#define STATE_IN_FALSE           10
#define STATE_IN_NULL            11
#define STATE_AFTER_VALUE        12
#define STATE_UNICODE_SURROGATE  13

#define STACK_OBJECT             0
#define STACK_ARRAY              1
#define STACK_KEY                2
#define STACK_STRING             3

#define BUFFER_MAX_LENGTH  64

class WJsonParser {
public:
	typedef std::function<void(const char*, const char*)> TProcessKeyValueFunction;

	WJsonParser(bool logging = false) {
		_logging = logging;
		_state = STATE_START_DOCUMENT;
		_bufferPos = 0;
		_unicodeEscapeBufferPos = 0;
		_unicodeBufferPos = 0;
		_characterCounter = 0;
		_kvFunction = nullptr;
	}

	void parse(const char *payload, TProcessKeyValueFunction kvFunction) {
		_kvFunction = kvFunction;
		for (int i = 0; i < strlen(payload); i++) {
			_parseChar(payload[i]);
		}
	}

	WProperty* parse(const char *payload, WDevice *device) {
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
	int _state;
	int _stack[20];
	int _stackPos = 0;
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
	String _currentKey;
	WDevice* _device = nullptr;
	TProcessKeyValueFunction _kvFunction;

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

		} else if (_kvFunction) {
			_kvFunction(key, value);
		}
		return result;
	}

	WProperty* _parseChar(char c) {
		WProperty* result = nullptr;
		if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
				&& !(_state == STATE_IN_STRING || _state == STATE_UNICODE
						|| _state == STATE_START_ESCAPE
						|| _state == STATE_IN_NUMBER
						|| _state == STATE_START_DOCUMENT)) {
			return result;
		}
		switch (_state) {
		case STATE_IN_STRING:
			if (c == QUOTE) {
				result = _endString();
			} else if (c == '\\') {
				_state = STATE_START_ESCAPE;
			} else if ((c < 0x1f) || (c == 0x7f)) {
				//throw new RuntimeException("Unescaped control character encountered: " + c + " at position" + characterCounter);
			} else {
				_buffer[_bufferPos] = c;
				_increaseBufferPointer();
			}
			break;
		case STATE_IN_ARRAY:
			if (c == BEND) {
				_endArray();
			} else {
				_startValue(c);
			}
			break;
		case STATE_IN_OBJECT:
			if (c == SEND) {
				_endObject();
			} else if (c == QUOTE) {
				_startKey();
			} else {
				//throw new RuntimeException("Start of string expected for object key. Instead got: " + c + " at position" + characterCounter);
			}
			break;
		case STATE_END_KEY:
			if (c != DPOINT) {
				//throw new RuntimeException("Expected ':' after key. Instead got " + c + " at position" + characterCounter);
			}
			_state = STATE_AFTER_KEY;
			break;
		case STATE_AFTER_KEY:
			_startValue(c);
			break;
		case STATE_START_ESCAPE:
			_processEscapeCharacters(c);
			break;
		case STATE_UNICODE:
			_processUnicodeCharacter(c);
			break;
		case STATE_UNICODE_SURROGATE:
			_unicodeEscapeBuffer[_unicodeEscapeBufferPos] = c;
			_unicodeEscapeBufferPos++;
			if (_unicodeEscapeBufferPos == 2) {
				_endUnicodeSurrogateInterstitial();
			}
			break;
		case STATE_AFTER_VALUE: {
			// not safe for size == 0!!!
			int within = _stack[_stackPos - 1];
			if (within == STACK_OBJECT) {
				if (c == SEND) {
					_endObject();
				} else if (c == COMMA) {
					_state = STATE_IN_OBJECT;
				} else {
					//throw new RuntimeException("Expected ',' or '}' while parsing object. Got: " + c + ". " + characterCounter);
				}
			} else if (within == STACK_ARRAY) {
				if (c == BEND) {
					_endArray();
				} else if (c == COMMA) {
					_state = STATE_IN_ARRAY;
				} else {
					//throw new RuntimeException("Expected ',' or ']' while parsing array. Got: " + c + ". " + characterCounter);

				}
			} else {
				//throw new RuntimeException("Finished a literal, but unclear what state to move to. Last state: " + characterCounter);
			}
		}
			break;
		case STATE_IN_NUMBER:
			if (c >= '0' && c <= '9') {
				_buffer[_bufferPos] = c;
				_increaseBufferPointer();
			} else if (c == '.') {
				if (_doesCharArrayContain(_buffer, _bufferPos, '.')) {
					//throw new RuntimeException("Cannot have multiple decimal points in a number. " + characterCounter);
				} else if (_doesCharArrayContain(_buffer, _bufferPos, 'e')) {
					//throw new RuntimeException("Cannot have a decimal point in an exponent." + characterCounter);
				}
				_buffer[_bufferPos] = c;
				_increaseBufferPointer();
			} else if (c == 'e' || c == 'E') {
				if (_doesCharArrayContain(_buffer, _bufferPos, 'e')) {
					//throw new RuntimeException("Cannot have multiple exponents in a number. " + characterCounter);
				}
				_buffer[_bufferPos] = c;
				_increaseBufferPointer();
			} else if (c == '+' || c == '-') {
				char last = _buffer[_bufferPos - 1];
				if (!(last == 'e' || last == 'E')) {
					//throw new RuntimeException("Can only have '+' or '-' after the 'e' or 'E' in a number." + characterCounter);
				}
				_buffer[_bufferPos] = c;
				_increaseBufferPointer();
			} else {
				result = _endNumber();
				// we have consumed one beyond the end of the number
				_parseChar(c);
			}
			break;
		case STATE_IN_TRUE:
			_buffer[_bufferPos] = c;
			_increaseBufferPointer();
			if (_bufferPos == 4) {
				result = _endTrue();
			}
			break;
		case STATE_IN_FALSE:
			_buffer[_bufferPos] = c;
			_increaseBufferPointer();
			if (_bufferPos == 5) {
				result = _endFalse();
			}
			break;
		case STATE_IN_NULL:
			_buffer[_bufferPos] = c;
			_increaseBufferPointer();
			if (_bufferPos == 4) {
				_endNull();
			}
			break;
		case STATE_START_DOCUMENT:
			//myListener->startDocument();
			if (c == BBEGIN) {
				_startArray();
			} else if (c == SBEGIN) {
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
		int popped = _stack[_stackPos - 1];
		_stackPos--;
		if (popped == STACK_KEY) {
			_buffer[_bufferPos] = '\0';
			_currentKey = String(_buffer);
			_state = STATE_END_KEY;
		} else if (popped == STACK_STRING) {
			if (_currentKey != "") {
				_buffer[_bufferPos] = '\0';
				const char* v = _buffer;
				result = _processKeyValue(_currentKey.c_str(), _buffer);
			}
			_state = STATE_AFTER_VALUE;
		} else {
			// throw new ParsingError($this->_line_number, $this->_char_number,
			// "Unexpected end of string.");
		}
		_bufferPos = 0;
		return result;
	}

	void _startValue(char c) {
		if (c == '[') {
			_startArray();
		} else if (c == '{') {
			_startObject();
		} else if (c == '"') {
			_startString();
		} else if (isDigit(c)) {
			_startNumber(c);
		} else if ((c == 't') || (c == 'T')) {
			_state = STATE_IN_TRUE;
			_buffer[_bufferPos] = c;
			_increaseBufferPointer();
		} else if ((c == 'f') || (c == 'F')) {
			_state = STATE_IN_FALSE;
			_buffer[_bufferPos] = c;
			_increaseBufferPointer();
		} else if ((c == 'n') || (c == 'N')) {
			_state = STATE_IN_NULL;
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
		int popped = _stack[_stackPos - 1];
		_stackPos--;
		if (popped != STACK_ARRAY) {
			// throw new ParsingError($this->_line_number, $this->_char_number,
			// "Unexpected end of array encountered.");
		}
		_log("jsonParser->endArray()");
		_state = STATE_AFTER_VALUE;
		if (_stackPos == 0) {
			_endDocument();
		}
	}

	void _startKey() {
		_stack[_stackPos] = STACK_KEY;
		_stackPos++;
		_state = STATE_IN_STRING;
	}

	void _endObject() {
		int popped = _stack[_stackPos];
		_stackPos--;
		if (popped != STACK_OBJECT) {
			// throw new ParsingError($this->_line_number, $this->_char_number,
			// "Unexpected end of object encountered.");
		}
		_log("jsonParser->endObject()");
		_state = STATE_AFTER_VALUE;
		if (_stackPos == 0) {
			_endDocument();
		}
	}

	void _processEscapeCharacters(char c) {
		if (c == '"') {
			_buffer[_bufferPos] = '"';
			_increaseBufferPointer();
		} else if (c == '\\') {
			_buffer[_bufferPos] = '\\';
			_increaseBufferPointer();
		} else if (c == '/') {
			_buffer[_bufferPos] = '/';
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
			_state = STATE_UNICODE;		
		}
		if (_state != STATE_UNICODE) {
			_state = STATE_IN_STRING;
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
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
				|| (c >= 'A' && c <= 'F');
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
		_state = STATE_UNICODE;
	}

	WProperty* _endNumber() {
		WProperty* result = nullptr;
		if (_currentKey != "") {
			_buffer[_bufferPos] = '\0';
			result = _processKeyValue(_currentKey.c_str(), _buffer);
		}
		_bufferPos = 0;
		_state = STATE_AFTER_VALUE;
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
		_state = STATE_DONE;
	}

	WProperty* _endTrue() {
		WProperty* result = nullptr;
		if (_currentKey != "") {
			_buffer[_bufferPos] = '\0';
			if (strcmp(_buffer, "true") == 0) {
				result = _processKeyValue(_currentKey.c_str(), "true");
			}
		}
		_bufferPos = 0;
		_state = STATE_AFTER_VALUE;
		return result;
	}

	WProperty* _endFalse() {
		WProperty* result = nullptr;
		if (_currentKey != "") {
			_buffer[_bufferPos] = '\0';
			if (strcmp(_buffer, "false") == 0) {
				result = _processKeyValue(_currentKey.c_str(), "false");
			}
		}
		_bufferPos = 0;
		_state = STATE_AFTER_VALUE;
		return result;
	}

	void _endNull() {
		_buffer[_bufferPos] = '\0';
		String value = String(_buffer);
		_bufferPos = 0;
		_state = STATE_AFTER_VALUE;
	}

	void _startArray() {
		_state = STATE_IN_ARRAY;
		_stack[_stackPos] = STACK_ARRAY;
		_stackPos++;
	}

	void _startObject() {
		_state = STATE_IN_OBJECT;
		_stack[_stackPos] = STACK_OBJECT;
		_stackPos++;
	}

	void _startString() {
		_stack[_stackPos] = STACK_STRING;
		_stackPos++;
		_state = STATE_IN_STRING;
	}

	void _startNumber(char c) {
		_state = STATE_IN_NUMBER;
		_buffer[_bufferPos] = c;
		_increaseBufferPointer();
	}

	void _endUnicodeCharacter(int codepoint) {
		_buffer[_bufferPos] = _convertCodepointToCharacter(codepoint);
		_increaseBufferPointer();
		_unicodeBufferPos = 0;
		_unicodeHighSurrogate = -1;
		_state = STATE_IN_STRING;
	}

	char _convertCodepointToCharacter(int num) {
		if (num <= 0x7F)
			return (char) (num);
		return ' ';
	}

};

#endif

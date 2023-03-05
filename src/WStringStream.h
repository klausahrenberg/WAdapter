#ifndef _STRING_STREAM_H_
#define _STRING_STREAM_H_

#include <Stream.h>

class WStringStream : public Stream {
public:
  WStringStream(unsigned int maxLength) {
  	_maxLength = maxLength;
  	_string = new char[maxLength + 1];
  	this->flush();
  }

  ~WStringStream() {
  	if (_string) {
  		delete[] _string;
  	}
  }

  // Stream methods
  virtual int available() {
  	return maxLength() - _position;
  }

  virtual int read() {
  	if (_position > 0) {
  		char c = _string[0];
  		for (int i = 1; i <= _position; i++) {
  			_string[i - 1] = _string[i];
  		}
		_position--;
  		return c;
  	}
  	return -1;
  }

  virtual int peek() {
  	if (_position > 0) {
  	    char c = _string[0];
  	    return c;
  	}
  	return -1;
  }

  virtual void flush() {
  	_position = 0;
  	_string[0] = '\0';
  }

  // Print methods
  virtual size_t write(uint8_t c) {
  	if (_position < _maxLength) {
  		_string[_position] = (char) c;
  		_position++;
  		_string[_position] = '\0';
  		return 1;
  	} else {
  		return 0;
  	}
  }

  unsigned int length() {
    return _position;
  }

  unsigned int maxLength() {
  	return _maxLength;
  }

  char charAt(int index) {
  	return _string[index];
  }

  template<class T, typename ... Args> size_t printAndReplace(T msg, ...) {
    va_list args;
		va_start(args, msg);
    return printAndReplaceImpl(msg, args);
    va_end(args);
  }

  const char* c_str() {
  	return _string;
  }

  size_t printAndReplaceImpl(const __FlashStringHelper *format, va_list args) {
    size_t n = 0;
    PGM_P p = reinterpret_cast<PGM_P>(format);
		char c = pgm_read_byte(p++);
		for(;c != 0; c = pgm_read_byte(p++)) {
			if (c == '%') {
				c = pgm_read_byte(p++);
				n = n + printFormat(c, &args);
			} else {
        //just copy
  			if (write(c)) n++;
  			else break;
			}
		}
    return n;
	}

private:
  char* _string;
  unsigned int _maxLength;
  unsigned int _position;

  size_t printFormat(const char c, va_list *args) {
    size_t n = 0;
    if (c == 's') {
      //wildcard
      register char *wc = (char *)va_arg(*args, int);
      for (int b = 0; b < strlen(wc); b++) {
        if (write(wc[b])) n++;
        else break;
      }
    } else {
      if (write('%')) n++;
      if (write(c)) n++;
    }
		return n;
	}

};

#endif // _STRING_STREAM_H_

#ifndef _STRING_STREAM_H_
#define _STRING_STREAM_H_

#include <Stream.h>

class WStringStream : public Stream {
public:
  WStringStream(unsigned int maxLength) {
  	this->maxLength = maxLength;
  	this->string = new char[maxLength + 1];
  	this->flush();
  }

  ~WStringStream() {
  	if (this->string) {
  		delete[] this->string;
  	}
  }

  // Stream methods
  virtual int available() {
  	return getMaxLength() - position;
  }

  virtual int read() {
  	if (position > 0) {
  		char c = string[0];
  		for (int i = 1; i <= position; i++) {
  			string[i - 1] = string[i];
  		}
		position--;
  		return c;
  	}
  	return -1;
  }

  virtual int peek() {
  	if (position > 0) {
  	    char c = string[0];
  	    return c;
  	}
  	return -1;
  }

  virtual void flush() {
  	this->position = 0;
  	this->string[0] = '\0';
  }

  // Print methods
  virtual size_t write(uint8_t c) {
  	if (position < maxLength) {
  		string[position] = (char) c;
  		position++;
  		string[position] = '\0';
  		return 1;
  	} else {
  		return 0;
  	}
  }

  unsigned int length() {
    return this->position;
  }

  unsigned int getMaxLength() {
  	return this->maxLength;
  }

  char charAt(int index) {
  	return this->string[index];
  }

  template<class T, typename ... Args> size_t printAndReplace(T msg, ...) {
    va_list args;
		va_start(args, msg);
    return printAndReplaceImpl(msg, args);
    va_end(args);
  }

  const char* c_str() {
  	return this->string;
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
  char* string;
  unsigned int maxLength;
  unsigned int position;



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

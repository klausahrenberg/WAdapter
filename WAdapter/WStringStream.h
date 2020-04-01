#ifndef _STRING_STREAM_H_
#define _STRING_STREAM_H_

#include <Stream.h>

class WStringStream : public Stream {
public:
    WStringStream(unsigned int maxLength) {
    	this->maxLength = maxLength;
    	this->keepString = false;
    	this->string = new char[maxLength + 1];
    	this->flush();
    }

    WStringStream(unsigned int maxLength, bool keepString) {
    	this->maxLength = maxLength;
    	this->keepString = keepString;
    	this->string = new char[maxLength + 1];
    	this->flush();
    }

    ~WStringStream() {
    	if ((!keepString) && (this->string)) {
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

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1) {
    	return printAndReplace(toPrint, wc1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2) {
    	return printAndReplace(toPrint, wc1, wc2, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3) {
    	return printAndReplace(toPrint, wc1, wc2, wc3, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, wc14, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14, const char* wc15) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, wc14, wc15, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14, const char* wc15, const char* wc16) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, wc14, wc15, wc16, nullptr, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14, const char* wc15, const char* wc16, const char* wc17) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, wc14, wc15, wc16, wc17, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14, const char* wc15, const char* wc16, const char* wc17, const char* wc18) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, wc14, wc15, wc16, wc17, wc18, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14, const char* wc15, const char* wc16, const char* wc17, const char* wc18, const char* wc19) {
      return printAndReplace(toPrint, wc1, wc2, wc3, wc4, wc5, wc6, wc7, wc8, wc9, wc10, wc11, wc12, wc13, wc14, wc15, wc16, wc17, wc18, wc19, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4, const char* wc5, const char* wc6, const char* wc7, const char* wc8, const char* wc9, const char* wc10, const char* wc11, const char* wc12, const char* wc13, const char* wc14, const char* wc15, const char* wc16, const char* wc17, const char* wc18, const char* wc19, const char* wc20) {
    	PGM_P p = reinterpret_cast<PGM_P>(toPrint);
    	size_t n = 0;
    	int wcCount = (wc1 != nullptr ? 1 : 0);
    	wcCount = (wc2 != nullptr ? 2 : wcCount);
    	wcCount = (wc3 != nullptr ? 3 : wcCount);
    	wcCount = (wc4 != nullptr ? 4 : wcCount);
      wcCount = (wc5 != nullptr ? 5 : wcCount);
      wcCount = (wc6 != nullptr ? 6 : wcCount);
      wcCount = (wc7 != nullptr ? 7 : wcCount);
      wcCount = (wc8 != nullptr ? 8 : wcCount);
      wcCount = (wc9 != nullptr ? 9 : wcCount);
      wcCount = (wc10 != nullptr ? 10 : wcCount);
      wcCount = (wc11 != nullptr ? 11 : wcCount);
      wcCount = (wc12 != nullptr ? 12 : wcCount);
      wcCount = (wc13 != nullptr ? 13 : wcCount);
      wcCount = (wc14 != nullptr ? 14 : wcCount);
      wcCount = (wc15 != nullptr ? 15 : wcCount);
      wcCount = (wc16 != nullptr ? 16 : wcCount);
      wcCount = (wc17 != nullptr ? 17 : wcCount);
      wcCount = (wc18 != nullptr ? 18 : wcCount);
      wcCount = (wc19 != nullptr ? 19 : wcCount);
      wcCount = (wc20 != nullptr ? 20 : wcCount);
    	int wcIndex = 0;
    	//int lp = strlen(toPrint);
    	while (1) {
    	//for (int i = 0; i < lp; i++) {
    		unsigned char c = pgm_read_byte(p++);
    		if (c == 0) break;
    		if ((c == '%') && (wcIndex < wcCount)) {
    			c = pgm_read_byte(p++);
    			if (c == 's') {
    				//wildcard
    				const char* wc = (wcIndex == 0 ? wc1 : (wcIndex == 1 ? wc2 : (wcIndex == 2 ? wc3 : (wcIndex == 3 ? wc4 : (wcIndex == 4 ? wc5 : (wcIndex == 5 ? wc6 : (wcIndex == 6 ? wc7 : (wcIndex == 7 ? wc8 : (wcIndex == 8 ? wc9 : (wcIndex == 9 ? wc10 : (wcIndex == 10 ? wc11 : (wcIndex == 11 ? wc12 : (wcIndex == 12 ? wc13 : (wcIndex == 13 ? wc14 : (wcIndex == 14 ? wc15 : (wcIndex == 15 ? wc16 : (wcIndex == 16 ? wc17 : (wcIndex == 17 ? wc18 : (wcIndex == 18 ? wc19 : wc20)))))))))))))))))));
    				wcIndex++;
    				for (int b = 0; b < strlen(wc); b++) {
    					if (write(wc[b])) n++;
    					else break;
    				}
    			} else {
    				if (write('%')) n++;
    				else break;
    				if (write(c)) n++;
    				else break;
    			}
    		} else {
    			//just copy
    			if (write(c)) n++;
    			else break;
    		}
    	}
		return n;
    }

    const char* c_str() {
    	return this->string;
    }

private:
    char* string;
    bool keepString;
    unsigned int maxLength;
    unsigned int position;
};

#endif // _STRING_STREAM_H_

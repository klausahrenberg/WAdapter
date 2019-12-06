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

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1) {
    	return printAndReplace(toPrint, wc1, nullptr, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2) {
    	return printAndReplace(toPrint, wc1, wc2, nullptr, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3) {
    	return printAndReplace(toPrint, wc1, wc2, wc3, nullptr);
    }

    size_t printAndReplace(const __FlashStringHelper *toPrint, const char* wc1, const char* wc2, const char* wc3, const char* wc4) {
    	PGM_P p = reinterpret_cast<PGM_P>(toPrint);
    	size_t n = 0;
    	int wcCount = (wc1 != nullptr ? 1 : 0);
    	wcCount = (wc2 != nullptr ? 2 : wcCount);
    	wcCount = (wc3 != nullptr ? 3 : wcCount);
    	wcCount = (wc4 != nullptr ? 4 : wcCount);
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
    				const char* wc = (wcIndex == 0 ? wc1 : (wcIndex == 1 ? wc2 : (wcIndex == 2 ? wc3 : wc4)));
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
    unsigned int maxLength;
    unsigned int position;
};

#endif // _STRING_STREAM_H_

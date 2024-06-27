#ifndef IW_EXPANDER_h
#define IW_EXPANDER_h

class IWExpander {
public:
  virtual void mode(uint8_t pin, uint8_t mode);
  virtual bool readInput(uint8_t pin);
  virtual void writeOutput(uint8_t pin, bool value);
};  

#endif
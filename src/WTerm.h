#ifndef W_TERM_H
#define W_TERM_H

// #include <Arduino.h>

// #include "WLog.h"

#define KEYWORD_BRACKET_OPEN '('
#define KEYWORD_BRACKET_OPEN_ALTERNATIVE '{'
#define KEYWORD_BRACKET_CLOSE ')'
#define KEYWORD_BRACKET_CLOSE_ALTERNATIVE '}'
#define KEYWORD_STRING_QUOTATIONMARK '\''
#define KEYWORD_IF "if "
#define KEYWORD_THEN " then "
#define KEYWORD_ELSE " else "
#define KEYWORD_NULL "null"

enum WOperation {
  NO_OPERATION,
  IF_THEN_ELSE,
  VALUE_CONST,
  VALUE_VARIABLE,
  OR,
  AND,
  EQUAL,
  NOT_EQUAL,
  EQUAL_OR_LESS,
  EQUAL_OR_MORE
};

class WTerm {
 public:
  WTerm(String term = "", bool parseRecursive = true) {
    _parseRecursive = parseRecursive;
    _operation = NO_OPERATION;
    _subs = std::make_unique<WList<WTerm>>();
    parse(term);
  }

  WTerm(WOperation operation, WTerm* subs, ...) {
    _operation = operation;
    _subs = std::make_unique<WList<WTerm>>();
    if (subs) {
      _subs->add(subs);
      va_list args;
      va_start(args, subs);
      WTerm* term = nullptr;
      while ((term = va_arg(args, WTerm*)) != nullptr) {
        _subs->add(term);
      }
      va_end(args);
    }
  }

  WTerm(WOperation operation, WValue value) {
    _operation = operation;
    _constant = value;
  }

  WTerm(WOperation operation, WTerm* firstSub, WTerm* secondSub, va_list args)
    : _operation(operation),
      _subs(std::make_unique<WList<WTerm>>()) {
    
    if (firstSub) {
      _subs->add(firstSub);
    }
    
    if (secondSub) {
      _subs->add(secondSub);
      
      WTerm* term = nullptr;
      while ((term = va_arg(args, WTerm*)) != nullptr) {
        _subs->add(term);
      }
    }
  }

  ~WTerm() = default;

  static WTerm* And(WTerm* term1, WTerm* term2, ...) {
    va_list args;
    va_start(args, term2);
    WTerm* result = new WTerm(AND, term1, term2, args);
    va_end(args);
    return result;
  }

  static WTerm* Or(WTerm* term1, WTerm* term2, ...) {
    va_list args;
    va_start(args, term2);
    WTerm* result = new WTerm(OR, term1, term2, args);
    va_end(args);
    return result;
  }

  static WTerm* Equal(WTerm* term1, WTerm* term2) {
    return new WTerm(EQUAL, term1, term2, nullptr);
  }

  static WTerm* NotEqual(WTerm* term1, WTerm* term2) {
    return new WTerm(NOT_EQUAL, term1, term2, nullptr);
  }

  static WTerm* EqualOrLess(WTerm* term1, WTerm* term2) {
    return new WTerm(EQUAL_OR_LESS, term1, term2, nullptr);
  }

  static WTerm* EqualOrMore(WTerm* term1, WTerm* term2) {
    return new WTerm(EQUAL_OR_MORE, term1, term2, nullptr);
  }

  static WTerm* IfThenElse(WTerm* condition, WTerm* thenTerm, WTerm* elseTerm = nullptr) {
    WTerm* result = new WTerm(IF_THEN_ELSE, condition, thenTerm, nullptr);
    if (elseTerm) {
      result->_subs->add(elseTerm);
    }
    return result;
  }

  static WTerm* IfThen(WTerm* condition, WTerm* thenTerm) {
    return new WTerm(IF_THEN_ELSE, condition, thenTerm, nullptr);
  }

  static WTerm* Constant(WValue constant) {
    return new WTerm(VALUE_CONST, constant);
  }

  static WTerm* Variable(WValue variable) {
    return new WTerm(VALUE_VARIABLE, variable);
  }

  void parse(String term) {
    _operation = NO_OPERATION;
    _subs->clear();
    //_valueConstant = nullptr;
    if (!term.isEmpty()) {
      term = _prepareFormulaRemoveOuterBrackets(term);
      if (!term.isEmpty()) {
        // Looking for IF_THEN_ELSE
        int bracketLevel = 0;
        bool insideString = false;
        if (term.startsWith(KEYWORD_IF)) {
        }
      }
    }
  }

  typedef std::function<WValue(String)> TConditionListener;

  WValue value(TConditionListener conditionListener) {
    if (_operation == NO_OPERATION) {
        return WValue();
    }
    
    if (!_subs && _operation != VALUE_CONST && _operation != VALUE_VARIABLE) {
        LOG->error(F("No subterms for operation %d"), _operation);
        return WValue();
    }

    WValue result;
    switch (_operation) {
      case IF_THEN_ELSE:
        if (_subs->get(0)->value(conditionListener).asBool()) {
          result = _subs->get(1)->value(conditionListener);
        } else if (_subs->get(2) != nullptr) {
          result = _subs->get(2)->value(conditionListener);
        }
        break;
      case VALUE_CONST:
        result = _constant;
        break;
      case VALUE_VARIABLE:
        if (conditionListener == nullptr) {
          LOG->error(F("Listener for variable value '%s' is missing"), _constant.asString());
        }
        result = conditionListener(_constant.asString());
        break;
      case OR:
        result = WValue(false);
        for (int i = 0; (result.asBool() == false) && (i < _subs->size()); i++) {
          result = _subs->get(i)->value(conditionListener);
        }
        break;
      case AND:
        result = WValue(true);
        for (int i = 0; (result.asBool() == true) && (i < _subs->size()); i++) {
          result = _subs->get(i)->value(conditionListener);
        }
        break;
      case EQUAL:
        result = _subs->get(0)->value(conditionListener).equals(_subs->get(1)->value(conditionListener));
        break;
      case NOT_EQUAL:
        result = !_subs->get(0)->value(conditionListener).equals(_subs->get(1)->value(conditionListener));
        break;
      case EQUAL_OR_LESS:
        result = _subs->get(0)->value(conditionListener).equalOrLess(_subs->get(1)->value(conditionListener));
        break;
      case EQUAL_OR_MORE:
        result = _subs->get(0)->value(conditionListener).equalOrMore(_subs->get(1)->value(conditionListener));
        break;
    }
    return result;
  }

 protected:
  
 private:
  WTerm(const WTerm&) = delete;
  WTerm& operator=(const WTerm&) = delete;
  bool _parseRecursive;
  WOperation _operation;
  std::unique_ptr<WList<WTerm>> _subs = nullptr;
  // WList<WTerm>* _subs;
  WValue _constant;

  String _prepareFormulaRemoveOuterBrackets(String subFormula) {
    subFormula.trim();
    // Remove possible outer brackets
    while ((subFormula.length() > 1) && ((KEYWORD_BRACKET_OPEN == subFormula.charAt(0)) || (KEYWORD_BRACKET_OPEN_ALTERNATIVE == subFormula.charAt(0))) && ((KEYWORD_BRACKET_CLOSE == subFormula.charAt(subFormula.length() - 1)) || (KEYWORD_BRACKET_CLOSE_ALTERNATIVE == subFormula.charAt(subFormula.length() - 1))) && (_checkIntegry(subFormula.substring(1, subFormula.length() - 1)))) {
      subFormula = subFormula.substring(1, subFormula.length() - 1);
      subFormula.trim();
    }
    return subFormula;
  }

  bool _checkIntegry(String value) {
    int bracketLevel = 0;
    bool insideString = false;
    for (int i = 0; i < value.length(); i++) {
      char currentChar = value.charAt(i);
      insideString = _isInsideString(currentChar, insideString);
      if (!insideString) {
        bracketLevel = _getBracketLevel(currentChar, bracketLevel);
        bracketLevel = (bracketLevel < 0 ? 0 : bracketLevel);
      }
    }
    return (bracketLevel == 0);
  }

  bool _isInsideString(char currentChar, bool currentInsideString) {
    if (currentChar == KEYWORD_STRING_QUOTATIONMARK) {
      currentInsideString = !currentInsideString;
    }
    return currentInsideString;
  }

  int _getBracketLevel(char currentChar, int currentBracketLevel) {
    if ((KEYWORD_BRACKET_OPEN == currentChar) || (KEYWORD_BRACKET_OPEN_ALTERNATIVE == currentChar)) {
      currentBracketLevel++;
    }
    if ((KEYWORD_BRACKET_CLOSE == currentChar) || (KEYWORD_BRACKET_CLOSE_ALTERNATIVE == currentChar)) {
      currentBracketLevel--;
    }
    return currentBracketLevel;
  }
};

#endif
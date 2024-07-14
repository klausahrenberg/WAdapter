#ifndef W_LIST_H
#define W_LIST_H

/*
  Inspired by Stefan Kremser github.com/spacehuhn
  https://github.com/spacehuhn/SimpleList
  https://github.com/spacehuhn/SimpleList/blob/master/src/SimpleList.h

  In opposite to the SimpleList this list is for storing pointers do values only,
  No sorting, no deleting of objects at remove, etc.

  Added features:
  - method forEach for fast iteration through the list
  - method getIf
*/

template <class T>
struct WListNode {
  WListNode(const char* id) {
    if (id) {
      this->id = new char[strlen_P(id) + 1];
      strcpy_P(this->id, id);
    }
  }

  virtual ~WListNode() {
    if (id) delete id;
  }

  T* value;
  char* id = nullptr;
  WListNode<T>* next = nullptr; 
};

template <typename T>
class WIterator;

template <typename T>
class WList {
 public:
  typedef std::function<void(T* value, const char* id)> TOnValue;
  typedef std::function<bool(T* value)> TOnCompare;
  typedef std::function<void(WListNode<T>* listNode)> TOnListNode;

  WList(bool noDoubleIds = false) {
    _noDoubleIds = noDoubleIds;
    _size = 0;
    _firstNode = nullptr;
    _resetCaching();
  };

  virtual ~WList() {
    clear();
  }

  void add(T* value, const char* id = nullptr) { this->insert(value, _size, id); }

  virtual void insert(T* value, int index, const char* id = nullptr) {    
    WListNode<T>* newNode = (_noDoubleIds ? _getListNodeById(id) : nullptr);    
    if (newNode == nullptr) {      
      WListNode<T>* newNode = new WListNode<T>(id);    

      bool isString = std::is_same<T, const char>::value;
      newNode->value = value;        
      if (index == 0) {
          newNode->next = _firstNode;
          _firstNode = newNode;
      } else {
          WListNode<T>* prevNode = _getNode(index - 1);
          newNode->next  = prevNode->next;
          prevNode->next = newNode;
      }
      _isCached = true;
      _lastIndexGot = index;
      _lastNodeGot = newNode;
      _size++;
    } else {
      //if (newNode->value) delete newNode->value;
      newNode->value = value;
    }
  };

  void clear() {
    while (_size > 0) {
      this->remove(0, true);      
    }
  }

  void remove(int index, bool freeMemoryForValues = false) {
    if ((index >= 0) && (index < _size)) {
      WListNode<T>* nodePrev = _getNode(index - 1);
      WListNode<T>* nodeToDelete = _getNode(index);
      if (index == 0) {
        _firstNode = nodeToDelete->next;
      } else {
        nodePrev->next = nodeToDelete->next;
      }
      if ((freeMemoryForValues) && (nodeToDelete) && (nodeToDelete->value)) delete nodeToDelete->value;
      delete nodeToDelete;
      _size--;
      _resetCaching();
    }
  }

  bool removeById(const char* id) {
    bool result = false;
    WListNode<T>* nodePrev = nullptr;
    WListNode<T>* node = _firstNode;
    while (node != nullptr) {        
      if ((node->id != nullptr) && (strcmp_P(node->id, id) == 0)) {
        WListNode<T>* nodeToDelete = node;
          if (nodePrev == nullptr) {
            _firstNode = nodeToDelete->next;
          } else {
            nodePrev->next = nodeToDelete->next;
          }
          node = nodeToDelete->next;
          delete nodeToDelete;          
          result = true;
      }
      node = node->next;        
    }
    return result;
  }

  bool removeIf(TOnCompare comparator) {
    bool result = false;
    if (comparator != nullptr) {
      WListNode<T>* nodePrev = nullptr;
      WListNode<T>* node = _firstNode;
      while (node != nullptr) {
        if (comparator(node->value)) {
          WListNode<T>* nodeToDelete = node;
          if (nodePrev == nullptr) {
            _firstNode = nodeToDelete->next;
          } else {
            nodePrev->next = nodeToDelete->next;
          }
          node = nodeToDelete->next;
          delete nodeToDelete;          
          result = true;
        } else {
          nodePrev = node;
          node = node->next;
        }
      }
    }
    return result;
  }

  virtual void forEach(TOnValue consumer) {
    if (consumer) {
      WListNode<T>* node = _firstNode;
      while (node != nullptr) {
        consumer(node->value, node->id);
        node = node->next;
      }
    }
  }

  T* getIf(TOnCompare comparator) {
    if (comparator) {
      WListNode<T>* node = _firstNode;
      while (node != nullptr) {        
        if (comparator(node->value)) {
          return node->value;
        }
        node = node->next;        
      }
    }
    return nullptr;
  }

  T* get(int index) {
    WListNode<T>* node = _getNode(index);
    return (node != nullptr ? node->value : nullptr);
  }

  T* getById(const char* id) {
    WListNode<T>* ln = _getListNodeById(id);
    return (ln != nullptr ? ln->value : nullptr);
  }

  WListNode<T>* _getListNodeById(const char* id) {
    if (id != nullptr) {
      WListNode<T>* node = _firstNode;
      while (node != nullptr) {    
        if ((node->id != nullptr) && (strcmp_P(node->id, id) == 0)) {
          return node;
        }
        node = node->next;        
      }
    }
    return nullptr;
  }

  bool exists(T* value) {    
    return (indexOf(value) > -1);
  }  

  int indexOf(T* value) {
    if (value != nullptr) {
      int index = 0;
      WListNode<T>* node = _firstNode;
      while (node != nullptr) {
        if (node->value == value) {
          return index;
        }
        index++;
        node = node->next;
      }
    }
    return -1;
  }  

  int size() { return _size; }

  bool empty() { return (_size == 0); }

  WIterator<T>* iterator() {
    return new WIterator<T>(this);
  }

  WListNode<T>* _getNode(int index) {
    if ((index >= 0) && (index < _size)) {
      WListNode<T>* node = _firstNode;
      int c = 0;
      if ((_isCached) && (index >= _lastIndexGot)) {
        c = _lastIndexGot;
        node = _lastNodeGot;
      }
      while ((node != nullptr) && (c < index)) {
        node = node->next;
        c++;
      }
      if (node != nullptr) {
        _isCached = true;
        _lastIndexGot = c;
        _lastNodeGot = node;
      }
      return node;
    } else {
      return nullptr;
    }
  }

 protected:
  int _size;
  bool _noDoubleIds;
  WListNode<T>* _firstNode;
  // caching for get() method
  bool _isCached;
  int _lastIndexGot;
  WListNode<T>* _lastNodeGot;

  void _resetCaching() {
    _isCached = false;
    _lastIndexGot = -1;
    _lastNodeGot = nullptr;
  }

};  

template <typename T>
class WIterator {
 public: 
  WIterator(WList<T>* list) {
    _list = list;
    _currentNode = nullptr;
    _initial = true;
  }

  virtual ~WIterator() {
    _list = nullptr;
    _currentNode = nullptr;   
  }

  bool hasNext() {
    return (((_initial) && (_list->_getNode(0) != nullptr)) || (_currentNode != nullptr));    
  }

  T* next() {
    if (_initial) {
      _currentNode = _list->_getNode(0);
      _initial = false;
    }
    T* result = _currentNode->value;    
    _currentNode = _currentNode->next;    
    return result;
  }

 private:
  WList<T>* _list; 
  WListNode<T>* _currentNode;
  bool _initial;  
};

class WStringList : public WList<const char> {
 public:
  WStringList() : WList<const char>(true) {

  }  

  virtual void insert(const char* value, int index, const char* id = nullptr) { 
    if (value) {
      char* temp = new char[strlen_P(value) + 1];
      strcpy_P(temp, value);
      WList::insert(temp, index, id);
    }
  }  
};

#endif
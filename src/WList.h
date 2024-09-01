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

template <typename T>
class IWIterable {
 public:
  typedef std::function<void(int, T*, const char*)> TOnIteration;
  virtual void forEach(TOnIteration consumer);
};  

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
class WList : public IWIterable<T> {
 public:  
  typedef std::function<void(int, T*, const char*)> TOnIteration;
  typedef std::function<void(T* value)> TOnExists;
  typedef std::function<bool(T* value)> TOnCompare;
  typedef std::function<void(WListNode<T>* listNode)> TOnListNode;

  WList(bool noDoubleIds = false) {
    _noDoubleIds = noDoubleIds;
    _size = 0;
    _firstNode = nullptr;
    _resetCaching();
  };

  virtual ~WList() {
    this->clear();
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
      if (newNode->value) delete newNode->value;
      newNode->value = value;
    }
  };

  virtual void clear() {
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
      if ((freeMemoryForValues) && (nodeToDelete) && (nodeToDelete->value)) {
        delete nodeToDelete->value;
      }  
      delete nodeToDelete;
      _size--;
      _resetCaching();
    }
  }

  int indexOfId(const char* id) {
    WListNode<T>* node = _firstNode;
    int index = 0;
    while (node != nullptr) {
      if ((node->id != nullptr) && (strcmp_P(node->id, id) == 0)) {
        return index;
      }
      index++;
      node = node->next;
    }    
    return -1;
  }

  T* removeById(const char* id) {
    int index = indexOfId(id);
    if (index > -1) {
      T* result = get(index);
      remove(index, false);
      return result;
    } else {
      return nullptr;
    }
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

  virtual void forEach(TOnIteration consumer) {
    if (consumer) {
      WListNode<T>* node = _firstNode;
      int i = 0;
      while (node != nullptr) {
        consumer(i, node->value, node->id);
        i++;
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

  const char* getId(int index) {
    WListNode<T>* node = _getNode(index);
    return (node != nullptr ? node->id : nullptr);
  }

  T* getById(const char* id) {
    WListNode<T>* ln = _getListNodeById(id);
    return (ln != nullptr ? ln->value : nullptr);
  }

  bool existsId(const char* id) {    
    return (getById(id) != nullptr);
  }

  bool existsIdAndIf(const char* id, TOnCompare onCompare) {
    T* item = getById(id);    
    return ((item != nullptr) && (onCompare) && (onCompare(item)));
  }  

  void ifExistsId(const char* id, TOnExists onExists) {
    T* item = getById(id);
    if ((item != nullptr) && (onExists)) {
      onExists(item);
    }
  }

  void ifExists(const char* id, TOnExists onExists) {
    T* item = getById(id);
    if ((item != nullptr) && (onExists)) {
      onExists(item);
    }
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

  void changeId(const char* id, const char* newId) {
    T* lv = removeById(id);
    this->add(lv, newId);
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

class WStringList : public WList<const char> {
 public:
  WStringList() : WList<const char>(true) {

  }  

  virtual ~WStringList() {    
  }  

  virtual void insert(const char* value, int index, const char* id = nullptr) {     
    if (value) {
      char* temp = new char[strlen_P(value) + 1];
      strcpy_P(temp, value);
      WList::insert(temp, index, id);          
    }
  }  
};

template <typename T>
class WStack : public WList<T> {
 public:
  WStack(boolean lifo = true) : WList<T>() {
    _lifo = lifo;
  }

  T* peek() {    
    if (this->size() != 0) {    
      if (_lifo) {
        return this->get(this->size() - 1);
      } else {
        return this->get(0);
      }
    } else {
      return nullptr;
    }
  }

  T* pop() {
    T* obj = peek();
    if (_lifo) {
      this->remove(this->size() - 1);
    } else {
      this->remove(0);
    }
    return obj;
  }

  void push(T* item) { this->add(item); }

 private:  
  bool _lifo;
};

#endif
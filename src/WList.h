#ifndef W_LIST_H
#define W_LIST_H

//uint8_t, pgm_read_byte and strcmp_P must be known, this header can be included first
#include <Arduino.h>

/*
  Inspired by Stefan Kremser github.com/spacehuhn
  https://github.com/spacehuhn/SimpleList
  https://github.com/spacehuhn/SimpleList/blob/master/src/SimpleList.h

  In opposite to the SimpleList this list is for storing pointers do values only,
  No sorting, no deleting of objects at remove, etc.

  Added features:
  - method forEach for fast iteration through the list
  - method getIf
  - optional hash index on the ids, see WLIST_HASH_... below
*/

/*
  Hash index for the id based access (getById, existsId, indexOfId and the
  check for double ids at insert).
  To keep the footprint small the table is created lazily: as long as the list
  holds less than WLIST_HASH_MIN_SIZE entries a linear search is cheaper than
  the table itself. The table holds only pointers, so it costs
  16 slots = 64 bytes, 32 slots = 128 bytes, plus 4 bytes per node for the
  chaining. Collisions are chained, so the lookup stays correct at any size.
*/
#ifndef WLIST_HASH_MIN_SIZE
#define WLIST_HASH_MIN_SIZE 8
#endif
#ifndef WLIST_HASH_SLOTS_SMALL
#define WLIST_HASH_SLOTS_SMALL 16
#endif
#ifndef WLIST_HASH_SLOTS_LARGE
#define WLIST_HASH_SLOTS_LARGE 32
#endif
#ifndef WLIST_HASH_GROW_SIZE
#define WLIST_HASH_GROW_SIZE 32
#endif

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
    if (id) delete[] id;
  }

  T* value;
  char* id = nullptr;
  WListNode<T>* next = nullptr;
  //chaining of the id hash index, only used for nodes with an id
  WListNode<T>* hashNext = nullptr;
};

enum class WListChangeType {
  ADDED,
  REMOVED,
  CHANGED
};

template <typename T>
struct WListChange {
  WListChange(WListChangeType type, T* item, T* oldItem, int index) {
    this->type = type;
    this->item = item;
    this->oldItem = oldItem;
    this->index = index;
  }

  WListChangeType type;
  T* item;
  T* oldItem;
  int index;

  bool isAdded() {
    return (type == WListChangeType::ADDED);
  }

  bool isRemoved() {
    return (type == WListChangeType::REMOVED);
  }

  bool isChanged() {
    return (type == WListChangeType::CHANGED);
  }
  
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
  typedef std::function<void(WListChange<T> change)> WListListener;

 protected:
  //hash index of the ids, nullptr as long as the list is small.
  //declared before the first usage, some compilers don't look ahead in templates
  WListNode<T>** _hash;
  uint8_t _hashMask;

  //FNV-1a folded to 8 bits, pgm_read_byte keeps ids in the flash (PSTR) valid
  static uint8_t _hashOf(const char* id) {
    uint32_t h = 2166136261u;
    char c;
    while ((c = (char) pgm_read_byte(id++)) != '\0') {
      h ^= (uint8_t) c;
      h *= 16777619u;
    }
    return (uint8_t) (h ^ (h >> 8) ^ (h >> 16) ^ (h >> 24));
  }

  uint8_t _hashIndex(const char* id) { return (_hashOf(id) & _hashMask); }

  void _hashDrop() {
    if (_hash != nullptr) {
      delete[] _hash;
      _hash = nullptr;
      _hashMask = 0;
    }
  }

  //appends at the end of the slot, so the first added id is found first
  void _hashPut(WListNode<T>* node) {
    if ((_hash == nullptr) || (node->id == nullptr)) return;
    node->hashNext = nullptr;
    WListNode<T>** slot = &_hash[_hashIndex(node->id)];
    while (*slot != nullptr) slot = &((*slot)->hashNext);
    *slot = node;
  }

  //(re)builds the table for all nodes, the slot order stays the list order
  void _hashCreate(uint8_t slots) {
    _hashDrop();
    _hash = new WListNode<T>*[slots];
    if (_hash == nullptr) return;
    for (uint8_t i = 0; i < slots; i++) _hash[i] = nullptr;
    _hashMask = slots - 1;
    WListNode<T>* node = _firstNode;
    while (node != nullptr) {
      _hashPut(node);
      node = node->next;
    }
  }

  //called after a node was linked into the list
  void _hashInsert(WListNode<T>* node) {
    if (node->id == nullptr) return;
    if (_hash == nullptr) {
      if (_size >= WLIST_HASH_MIN_SIZE) _hashCreate(WLIST_HASH_SLOTS_SMALL);
    } else if ((_hashMask < WLIST_HASH_SLOTS_LARGE - 1) && (_size > WLIST_HASH_GROW_SIZE)) {
      _hashCreate(WLIST_HASH_SLOTS_LARGE);
    } else {
      _hashPut(node);
    }
  }

  //called before a node is unlinked from the list
  void _hashRemove(WListNode<T>* node) {
    if ((_hash == nullptr) || (node->id == nullptr)) return;
    WListNode<T>** slot = &_hash[_hashIndex(node->id)];
    while (*slot != nullptr) {
      if (*slot == node) {
        *slot = node->hashNext;
        node->hashNext = nullptr;
        return;
      }
      slot = &((*slot)->hashNext);
    }
  }

 public:
  WList(bool noDoubleIds = false) {
    _noDoubleIds = noDoubleIds;
    _size = 0;
    _firstNode = nullptr;
    _hash = nullptr;
    _hashMask = 0;
    _resetCaching();
  };

  virtual ~WList() {
    this->clear();
  }

  void add(T* value, const char* id = nullptr) { this->insert(value, _size, id); }

  virtual void insert(T* value, int index, const char* id = nullptr) {
    WListNode<T>* existingNode = (_noDoubleIds ? _getListNodeById(id) : nullptr);
    if (existingNode == nullptr) {
      WListNode<T>* newNode = new WListNode<T>(id);
      newNode->value = value;
      if (index == 0) {
        newNode->next = _firstNode;
        _firstNode = newNode;
      } else {
        WListNode<T>* prevNode = _getNode(index - 1);
        newNode->next = prevNode->next;
        prevNode->next = newNode;
      }
      _isCached = true;
      _lastIndexGot = index;
      _lastNodeGot = newNode;
      _size++;
      this->_hashInsert(newNode);
      _notifyAdd(index, newNode->value);
    } else {
      T* oldItem = existingNode->value;
      existingNode->value = value;
      _notifyChanged(index, value, oldItem);
      if (oldItem) delete oldItem;      
    }    
  };

  virtual void clear() {
    while (_size > 0) {
      this->remove(0, true);
    }
    this->_hashDrop();
  }

  //how a value is freed, WStringList stores char arrays and overrides it
  virtual void _deleteValue(T* value) { delete value; }

  void remove(int index, bool freeMemoryForValues = false) {
    if ((index >= 0) && (index < _size)) {
      WListNode<T>* nodePrev = _getNode(index - 1);
      WListNode<T>* nodeToDelete = _getNode(index);
      if (index == 0) {
        _firstNode = nodeToDelete->next;
      } else {
        nodePrev->next = nodeToDelete->next;
      }
      this->_hashRemove(nodeToDelete);
      _notifyRemove(index, nodeToDelete->value);
      if ((freeMemoryForValues) && (nodeToDelete) && (nodeToDelete->value)) {
        this->_deleteValue(nodeToDelete->value);
      }
      delete nodeToDelete;
      _size--;
      if (_size == 0) this->_hashDrop();
      _resetCaching();
    }
  }

  int indexOfId(const char* id) {
    if (id == nullptr) return -1;
    if (_hash != nullptr) {
      //hash finds the node, the list is walked for the index only (pointer compare)
      WListNode<T>* found = _getListNodeById(id);
      if (found == nullptr) return -1;
      WListNode<T>* node = _firstNode;
      int index = 0;
      while (node != nullptr) {
        if (node == found) return index;
        index++;
        node = node->next;
      }
      return -1;
    }
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
      int index = 0;
      while (node != nullptr) {
        if (comparator(node->value)) {
          WListNode<T>* nodeToDelete = node;
          if (nodePrev == nullptr) {
            _firstNode = nodeToDelete->next;
          } else {
            nodePrev->next = nodeToDelete->next;
          }
          node = nodeToDelete->next;
          this->_hashRemove(nodeToDelete);
          _notifyRemove(index, nodeToDelete->value);
          delete nodeToDelete;
          _size--;
          result = true;
        } else {
          nodePrev = node;
          node = node->next;
          index++;
        }
      }
      if (result) {
        if (_size == 0) this->_hashDrop();
        _resetCaching();
      }
    }
    return result;
  }

  virtual void forEach(TOnIteration consumer) {
    if (consumer) {
      WListNode<T>* node = _firstNode;
      int i = 0;
      while (node != nullptr) {
        if (node->value != nullptr)
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
      if (_hash != nullptr) {
        //only the nodes of one slot have to be compared
        WListNode<T>* node = _hash[this->_hashIndex(id)];
        while (node != nullptr) {
          if (strcmp_P(node->id, id) == 0) {
            return node;
          }
          node = node->hashNext;
        }
      } else {
        WListNode<T>* node = _firstNode;
        while (node != nullptr) {
          if ((node->id != nullptr) && (strcmp_P(node->id, id) == 0)) {
            return node;
          }
          node = node->next;
        }
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

  void addListener(WListListener listener) {
    _listener = listener;
  }

  void removeListener() {
    _listener = nullptr;
  }

 protected:
  int _size;
  bool _noDoubleIds;
  WListNode<T>* _firstNode;
  // caching for get() method
  bool _isCached;
  int _lastIndexGot;
  WListNode<T>* _lastNodeGot;
  WListListener _listener = nullptr;

  void _resetCaching() {
    _isCached = false;
    _lastIndexGot = -1;
    _lastNodeGot = nullptr;
  }

  void _notifyAdd(int index, T* item) {       
    if (_listener != nullptr) 
      _listener(WListChange<T>(WListChangeType::ADDED, item, nullptr, index));
  }

  void _notifyRemove(int index, T* item) {       
    if (_listener != nullptr) 
      _listener(WListChange<T>(WListChangeType::REMOVED, nullptr, item, index));
  }

  void _notifyChanged(int index, T* item, T* oldItem) {       
    if (_listener != nullptr) 
      _listener(WListChange<T>(WListChangeType::CHANGED, item, oldItem, index));
  }

};

class WStringList : public WList<const char> {
 public:
  WStringList() : WList<const char>(true) {
  }

  virtual ~WStringList() {
    //clear() here, in ~WList the override of _deleteValue is not reached anymore
    this->clear();
  }

  //insert() stores copies made with new char[]
  virtual void _deleteValue(const char* value) { delete[] value; }

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
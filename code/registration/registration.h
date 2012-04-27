#ifndef __SLIB_REGISTRATION_H__
#define __SLIB_REGISTRATION_H__

#include <map>
#include <string>

#define REGISTER_CATEGORY(CATEGORY)					\
  									\
  class CATEGORY ## Registry {						\
  private:								\
  static std::map<std::string, Allocator<CATEGORY>* > registry;		\
  public:								\
  									\
  inline static CATEGORY* CreateByName(const std::string& name) {	\
    std::map<std::string, Allocator<CATEGORY>* >::iterator iter		\
      = registry.find(name);						\
    if (iter == registry.end()) {					\
      return NULL;							\
    } else {								\
      return (*iter).second->Allocate();				\
    }									\
  }									\
									\
  template <class T>							\
  inline static bool Register(const std::string& name,			\
			      Allocator<T>* allocator) {		\
    if (registry.find(name) == registry.end()) {			\
      std::pair<std::string, Allocator<T>* > entry(name,		\
						   allocator);		\
      registry.insert(entry);						\
      return true;							\
    } else {								\
      return false;							\
    }									\
  }									\
  };

#define REGISTER_TYPE(TYPE, CATEGORY)				\
  bool ignore_##__FILE__ = CATEGORY##Registry::Register(#TYPE, new Allocator<TYPE>());

template<class T>
class Allocator {
public:
  Allocator() {}
  
  T* Allocate() {
    //return new T();
    return NULL;
  }
};

#endif

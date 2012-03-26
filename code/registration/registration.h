#ifndef __SLIB_REGISTRATION_H__
#define __SLIB_REGISTRATION_H__

#include <map>
#include <string>

#define REGISTER_CATEGORY(CATEGORY)					\
									\
  class CATEGORY ## Registry {						\
  private:								\
  public:								\
  inline static CATEGORY* CreateByName(const std::string& name) {	\
    std::map<std::string, Pointer>* registry				\
      = slib::Registration::GetRegistry();				\
    std::map<std::string, Pointer>::iterator iter			\
      = registry->find(#CATEGORY "_" + name);				\
    if (iter == registry->end()) {					\
      return NULL;							\
    } else {								\
      return (*iter).second.Get<CATEGORY>();				\
    }									\
  }									\
									\
  inline static bool Register(const std::string& name,			\
			      CATEGORY* obj) {				\
    std::map<std::string, slib::Pointer>* registry			\
      = slib::Registration::GetRegistry();				\
    if (registry->find(#CATEGORY "_" + name) == registry->end()) {	\
      slib::Pointer pointer(reinterpret_cast<void*>(obj));		\
      std::pair<std::string, slib::Pointer> entry(#CATEGORY "_" + name,	\
						  pointer);		\
      registry->insert(entry);						\
      return true;							\
    } else {								\
      return false;							\
    }									\
  }									\
  };

#define REGISTER_TYPE(TYPE, CATEGORY)					\
  bool ignore_##TYPE##_##CATEGORY					\
  = CATEGORY##Registry::Register(#TYPE, slib::Allocator<TYPE>::Allocate());

namespace slib {
  template <class T>
  class Allocator {
  public:
    static T* Allocate() {
      return new T;
    }
  };
  
  class Pointer {
  private:
    void* _ptr;
  public:
    Pointer(void* ptr) {
      _ptr = ptr;
    }
    
    template <class T>
    inline T* Get() {
      return reinterpret_cast<T*>(_ptr);
    }
  };
      
  class Registration {
  private:
    class Registry {
    private:
      std::map<std::string, Pointer> _map;
      
    public:
      inline std::map<std::string, Pointer>* Get() {
	return &_map;
      }
    };

    static Registry* _registry;

  public:
    static std::map<std::string, Pointer>* GetRegistry();
  };
}  // namespace slib

#endif

// Abstract interface for a callback.  When calling an RPC, you must provide
// a Closure to call when the procedure completes.  See the Service interface
// in service.h.
//
// To automatically construct a Closure which calls a particular function or
// method with a particular set of parameters, use the NewCallback() function.
// Example:
//   void FooDone(const FooResponse* response) {
//     ...
//   }
//
//   void CallFoo() {
//     ...
//     // When done, call FooDone() and pass it a pointer to the response.
//     Closure* callback = NewCallback(&FooDone, response);
//     // Make the call.
//     service->Foo(controller, request, response, callback);
//   }
//
// Example that calls a method:
//   class Handler {
//    public:
//     ...
//
//     void FooDone(const FooResponse* response) {
//       ...
//     }
//
//     void CallFoo() {
//       ...
//       // When done, call FooDone() and pass it a pointer to the response.
//       Closure* callback = NewCallback(this, &Handler::FooDone, response);
//       // Make the call.
//       service->Foo(controller, request, response, callback);
//     }
//   };
//
// Currently NewCallback() supports binding zero, one, or two arguments.
//
// Callbacks created with NewCallback() automatically delete themselves when
// executed.  They should be used when a callback is to be called exactly
// once (usually the case with RPC callbacks).  If a callback may be called
// a different number of times (including zero), create it with
// NewPermanentCallback() instead.  You are then responsible for deleting the
// callback (using the "delete" keyword as normal).
//
// Note that NewCallback() is a bit touchy regarding argument types.  Generally,
// the values you provide for the parameter bindings must exactly match the
// types accepted by the callback function.  For example:
//   void Foo(string s);
//   NewCallback(&Foo, "foo");          // WON'T WORK:  const char* != string
//   NewCallback(&Foo, string("foo"));  // WORKS
// Also note that the arguments cannot be references:
//   void Foo(const string& s);
//   string my_str;
//   NewCallback(&Foo, my_str);  // WON'T WORK:  Can't use referecnes.
// However, correctly-typed pointers will work just fine.

#undef GOOGLE_DISALLOW_EVIL_CONSTRUCTORS
#define GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                           \
  void operator=(const TypeName&)

namespace slib {
  namespace common {
    
    class Closure {
    public:
      Closure() {}
      virtual ~Closure() {}
      
      virtual void Run() = 0;
      
    private:
      GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Closure);
    };
    
    namespace internal {
      
      class FunctionClosure0 : public Closure {
      public:
	typedef void (*FunctionType)();
	
	FunctionClosure0(FunctionType function, bool self_deleting)
	  : function_(function), self_deleting_(self_deleting) {}
	~FunctionClosure0();
	
	void Run() {
	  bool needs_delete = self_deleting_;  // read in case callback deletes
	  function_();
	  if (needs_delete) delete this;
	}
	
      private:
	FunctionType function_;
	bool self_deleting_;
      };
      
      template <typename Class>
      class MethodClosure0 : public Closure {
      public:
	typedef void (Class::*MethodType)();
	
	MethodClosure0(Class* object, MethodType method, bool self_deleting)
	  : object_(object), method_(method), self_deleting_(self_deleting) {}
	~MethodClosure0() {}
	
	void Run() {
	  bool needs_delete = self_deleting_;  // read in case callback deletes
	  (object_->*method_)();
	  if (needs_delete) delete this;
	}
	
      private:
	Class* object_;
	MethodType method_;
	bool self_deleting_;
      };
      
      template <typename Arg1>
      class FunctionClosure1 : public Closure {
      public:
	typedef void (*FunctionType)(Arg1 arg1);
	
	FunctionClosure1(FunctionType function, bool self_deleting,
			 Arg1 arg1)
	  : function_(function), self_deleting_(self_deleting),
	    arg1_(arg1) {}
	~FunctionClosure1() {}
	
	void Run() {
	  bool needs_delete = self_deleting_;  // read in case callback deletes
	  function_(arg1_);
	  if (needs_delete) delete this;
	}
	
      private:
	FunctionType function_;
	bool self_deleting_;
	Arg1 arg1_;
      };
      
      template <typename Class, typename Arg1>
      class MethodClosure1 : public Closure {
      public:
	typedef void (Class::*MethodType)(Arg1 arg1);
	
	MethodClosure1(Class* object, MethodType method, bool self_deleting,
		       Arg1 arg1)
	  : object_(object), method_(method), self_deleting_(self_deleting),
	    arg1_(arg1) {}
	~MethodClosure1() {}
	
	void Run() {
	  bool needs_delete = self_deleting_;  // read in case callback deletes
	  (object_->*method_)(arg1_);
	  if (needs_delete) delete this;
	}
	
      private:
	Class* object_;
	MethodType method_;
	bool self_deleting_;
	Arg1 arg1_;
      };
      
      template <typename Arg1, typename Arg2>
      class FunctionClosure2 : public Closure {
      public:
	typedef void (*FunctionType)(Arg1 arg1, Arg2 arg2);
	
	FunctionClosure2(FunctionType function, bool self_deleting,
			 Arg1 arg1, Arg2 arg2)
	  : function_(function), self_deleting_(self_deleting),
	    arg1_(arg1), arg2_(arg2) {}
	~FunctionClosure2() {}
	
	void Run() {
	  bool needs_delete = self_deleting_;  // read in case callback deletes
	  function_(arg1_, arg2_);
	  if (needs_delete) delete this;
	}
	
      private:
	FunctionType function_;
	bool self_deleting_;
	Arg1 arg1_;
	Arg2 arg2_;
      };
      
      template <typename Class, typename Arg1, typename Arg2>
      class MethodClosure2 : public Closure {
      public:
	typedef void (Class::*MethodType)(Arg1 arg1, Arg2 arg2);
	
	MethodClosure2(Class* object, MethodType method, bool self_deleting,
		       Arg1 arg1, Arg2 arg2)
	  : object_(object), method_(method), self_deleting_(self_deleting),
	    arg1_(arg1), arg2_(arg2) {}
	~MethodClosure2() {}
	
	void Run() {
	  bool needs_delete = self_deleting_;  // read in case callback deletes
	  (object_->*method_)(arg1_, arg2_);
	  if (needs_delete) delete this;
	}
	
      private:
	Class* object_;
	MethodType method_;
	bool self_deleting_;
	Arg1 arg1_;
	Arg2 arg2_;
      };         
    }  // namespace internal
  }  // namespace common
}  // namespace slib

// See Closure.
inline slib::common::Closure* NewCallback(void (*function)()) {
  return new slib::common::internal::FunctionClosure0(function, true);
}

// See Closure.
inline slib::common::Closure* NewPermanentCallback(void (*function)()) {
  return new slib::common::internal::FunctionClosure0(function, false);
}

// See Closure.
template <typename Class>
inline slib::common::Closure* NewCallback(Class* object, void (Class::*method)()) {
  return new slib::common::internal::MethodClosure0<Class>(object, method, true);
}

// See Closure.
template <typename Class>
inline slib::common::Closure* NewPermanentCallback(Class* object, void (Class::*method)()) {
  return new slib::common::internal::MethodClosure0<Class>(object, method, false);
}

// See Closure.
template <typename Arg1>
inline slib::common::Closure* NewCallback(void (*function)(Arg1),
			    Arg1 arg1) {
  return new slib::common::internal::FunctionClosure1<Arg1>(function, true, arg1);
}

// See Closure.
template <typename Arg1>
inline slib::common::Closure* NewPermanentCallback(void (*function)(Arg1),
				     Arg1 arg1) {
  return new slib::common::internal::FunctionClosure1<Arg1>(function, false, arg1);
}

// See Closure.
template <typename Class, typename Arg1>
inline slib::common::Closure* NewCallback(Class* object, void (Class::*method)(Arg1),
			    Arg1 arg1) {
  return new slib::common::internal::MethodClosure1<Class, Arg1>(object, method, true, arg1);
}

// See Closure.
template <typename Class, typename Arg1>
inline slib::common::Closure* NewPermanentCallback(Class* object, void (Class::*method)(Arg1),
				     Arg1 arg1) {
  return new slib::common::internal::MethodClosure1<Class, Arg1>(object, method, false, arg1);
}

// See Closure.
template <typename Arg1, typename Arg2>
inline slib::common::Closure* NewCallback(void (*function)(Arg1, Arg2),
			    Arg1 arg1, Arg2 arg2) {
  return new slib::common::internal::FunctionClosure2<Arg1, Arg2>(function, true, arg1, arg2);
}

// See Closure.
template <typename Arg1, typename Arg2>
inline slib::common::Closure* NewPermanentCallback(void (*function)(Arg1, Arg2),
				     Arg1 arg1, Arg2 arg2) {
  return new slib::common::internal::FunctionClosure2<Arg1, Arg2>(function, false, arg1, arg2);
}

// See Closure.
template <typename Class, typename Arg1, typename Arg2>
inline slib::common::Closure* NewCallback(Class* object, void (Class::*method)(Arg1, Arg2),
			    Arg1 arg1, Arg2 arg2) {
  return new slib::common::internal::MethodClosure2<Class, Arg1, Arg2>(object, method, true, arg1, arg2);
}

// See Closure.
template <typename Class, typename Arg1, typename Arg2>
inline slib::common::Closure* NewPermanentCallback(
				     Class* object, void (Class::*method)(Arg1, Arg2),
				     Arg1 arg1, Arg2 arg2) {
  return new slib::common::internal::MethodClosure2<Class, Arg1, Arg2>(object, method, false, arg1, arg2);
}

// A function which does nothing.  Useful for creating no-op callbacks, e.g.:
//   Closure* nothing = NewCallback(&DoNothing);
void DoNothing();

#ifndef __SLIB_UTIL_STL_H__
#define __SLIB_UTIL_STL_H__

#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::stringstream;
using std::vector;

namespace slib {
  namespace util {

    template <typename T>
    struct STLIndexedEntry {
      T value;
      int index;
    };
    
    template <typename T>
    bool AscendingOrder(const STLIndexedEntry<T>& left, const STLIndexedEntry<T>& right) {
      return (left.value < right.value);
    }

    template <typename T>
    vector<STLIndexedEntry<T> > CreateIndexedContainer(const vector<T>& V) {
      vector<STLIndexedEntry<T> > VI(V.size());
      for (int i = 0; i < (int) V.size(); i++) {
	VI[i].value = V[i];
	VI[i].index = i;
      }
      return VI;
    }

    template <typename T>
    vector<int> UnwrapContainer(const vector<STLIndexedEntry<T> >& VI, vector<T>* V) {
      vector<int> indices(VI.size());
      V->clear();
      V->resize(VI.size());
      for (int i = 0; i < (int) VI.size(); i++) {
	indices[i] = VI[i].index;
	(*V)[i] = VI[i].value;
      }
      return indices;
    }

    // Just returns the indices. DOES NOT ACTUALLY SORT!
    template <typename T>
    vector<int> UnwrapContainer(const vector<STLIndexedEntry<T> >& VI, const vector<T>& V) {
      vector<int> indices(VI.size());
      for (int i = 0; i < (int) VI.size(); i++) {
	indices[i] = VI[i].index;
      }
      return indices;
    }

    // Create a vector of values from [start, end) with a stride of
    // step. The templated type must be well-defined for the operators
    // < and ++, and must be able to cast the value 1 appropriately if
    // you want to use the default argument list.
    template <typename T>
    vector<T> Range(const T& start, const T& end, const T& step = 1) {
      vector<T> values(end - start);
      for (T value = start; value < end; value += step) {
	values[value - start] = value;
      }
      return values;
    }

    template <typename T>
    vector<int> Sort(vector<T>* V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(*V);
      sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    template <typename T>
    vector<int> Sort(const vector<T>& V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    template <typename T>
    vector<int> Sort(const vector<T>& V, bool(&compare)(const T& left, const T& right)) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      sort(VI.begin(), VI.end(), compare);
      return UnwrapContainer<T>(VI, V);
    }

    template <typename T>
    std::string PrintVector(const vector<T>& V) {
      stringstream s(stringstream::out);
      s << "[";
      if (V.size() > 0) {
	s << V[0];
	for (int i = 1; i < (int) V.size(); i++) {
	  s << " " << V[i];
	}
      }
      s << "]";
      return s.str();
    }

    // Trims the input vector (V) to contain only the indices
    // specified in the input vector of indices.
    template <typename T>
    void TrimVectorEntries(const vector<int>& indices, vector<T>* V) {
      if (V == NULL || V->size() == 0) {
	return;
      }
      if (indices.size() == 0) {
	V->clear();
	return;
      }
      int previous_index = V->size();
  
      for (int i = ((int) indices.size()) - 1; i >= 0; i--) {
	V->erase(V->begin() + indices[i] + 1, V->begin() + previous_index);
	previous_index = indices[i];
      }
      V->erase(V->begin(), V->begin() + previous_index);
    }

    // Quick initialization from a pointer array.
    template <typename T>
    vector<T> PointerToVector(const T* array, const int& entries) {
      vector<T> V(entries);
      for (int i = 0; i < entries; i++) {
	V[i] = array[i];
      }
      return V;
    }

  }  // namespace util
}  // namespace slib

#endif

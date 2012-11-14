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

    template <typename T>
    vector<int> Sort(vector<T>* V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(*V);
      sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    template <typename T>
    vector<int> Sort(const vector<T>& V, const bool(&compare)(const T& left, const T& right)) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(*V);
      sort(VI.begin(), VI.end(), compare);
      return UnwrapContainer<T>(VI, V);
    }

    template <typename T>
    string PrintVector(const vector<T>& V) {
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
      if (indices.size() == 0 || V == NULL || V->size() == 0) {
	return;
      }
      int previous_index = V->size();
  
      for (int i = ((int) indices.size()) - 1; i >= 0; i--) {
	V->erase(V->begin() + indices[i] + 1, V->begin() + previous_index);
	previous_index = indices[i];
      }
      V->erase(V->begin(), V->begin() + previous_index);
    }

  }  // namespace util
}  // namespace slib

#endif

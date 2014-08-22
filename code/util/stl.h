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
    bool DescendingOrder(const STLIndexedEntry<T>& left, const STLIndexedEntry<T>& right) {
      return (left.value > right.value);
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
    //
    // There are two template parameters to handle the case where
    // start and end might not be exactly the same type, but are both
    // compatible with int.
    template <typename T, typename U>
    vector<T> Range(const T& start, const U& end, const T& step = 1) {
      const int num = static_cast<int>((end - start) / step);
      const T num_T = static_cast<T>(num);
      vector<T> values(num);
      for (int i = 0; i < num; ++i) {
	values[i] = start + step * static_cast<T>(i);
      }
      return values;
    }

    // Sorts a vector in ascending order. The return vector are a
    // mapping from the sorted vector to the original vector such that
    // vector<int> i = Sort(V_original) --> V_sorted = V_original(i).
    template <typename T>
    vector<T> Intersect(const vector<T>& V1, const vector<T>& V2) {
      vector<T> V1_(V1);
      vector<T> V2_(V2);
      
      vector<T> intersection(V1_.size() + V2_.size());
      
      std::sort(V1_.begin(), V1_.end());
      std::sort(V2_.begin(), V2_.end());
      typename vector<T>::iterator it = std::set_intersection(V1_.begin(), V1_.end(),
							      V2_.begin(), V2_.end(),
							      intersection.begin());
      intersection.resize(it - intersection.begin());
      return intersection;
    }

    template <typename T>
    vector<T> SetDifference(const vector<T>& V1, const vector<T>& V2) {
      vector<T> V1_(V1);
      vector<T> V2_(V2);

      vector<T> difference(V1_.size() + V2_.size());

      std::sort(V1_.begin(), V1_.end());
      std::sort(V2_.begin(), V2_.end());
      typename vector<T>::iterator it = std::set_difference(V1_.begin(), V1_.end(),
							    V2_.begin(), V2_.end(),
							    difference.begin());
      difference.resize(it - difference.begin());
      return difference;
    }

    template <typename T>
    vector<int> Sort(vector<T>* V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(*V);
      sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    // Returns the sorted indices (ascending order) but doesn't
    // actually sort the vector (hence the const). This is useful if
    // you just want to know how to reorder a vector without actually
    // reordering it.
    template <typename T>
    vector<int> Sort(const vector<T>& V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    // Same as above except you get to pass your own compare function
    // in.  TODO(sean): Make it so users can just specify a functor on
    // const T& rather than the IndexedEntry.
    template <typename T>
    vector<int> Sort(vector<T>* V, 
		     bool(&compare)(const STLIndexedEntry<T>& left, const STLIndexedEntry<T>& right)) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(*V);
      sort(VI.begin(), VI.end(), compare);
      return UnwrapContainer<T>(VI, V);
    }

    // This enables users to use the pre-defined sorters in this file
    // directly as arguments.
    template <typename T>
    vector<int> Sort(const vector<T>& V, bool(&compare)(const STLIndexedEntry<T>& left, 
							const STLIndexedEntry<T>& right)) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      sort(VI.begin(), VI.end(), compare);
      return UnwrapContainer<T>(VI, V);
    }
    
    // The next couple of methods are the same as above but they are
    // the stable sort versions.
    template <typename T>
    vector<int> StableSort(vector<T>* V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(*V);
      stable_sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    template <typename T>
    vector<int> StableSort(const vector<T>& V) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      stable_sort(VI.begin(), VI.end(), AscendingOrder<T>);
      return UnwrapContainer<T>(VI, V);
    }

    // For situations where you want to define your own sorter.
    template <typename T>
    vector<int> StableSort(const vector<T>& V, bool(&compare)(const T& left, const T& right)) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      stable_sort(VI.begin(), VI.end(), compare);
      return UnwrapContainer<T>(VI, V);
    }

    // This enables users to use the pre-defined sorters in this file
    // directly as arguments.
    template <typename T>
    vector<int> StableSort(const vector<T>& V, bool(&compare)(const STLIndexedEntry<T>& left, 
							      const STLIndexedEntry<T>& right)) {
      vector<STLIndexedEntry<T> > VI = CreateIndexedContainer<T>(V);
      stable_sort(VI.begin(), VI.end(), compare);
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

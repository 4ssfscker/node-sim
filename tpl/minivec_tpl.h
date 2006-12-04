/*
 * minivec_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_minivec_h
#define tpl_minivec_h

#include <stdlib.h>

#include "../simdebug.h"
#include "../simtypes.h"

/**
 * A template class for a simple vector type.
 *
 * @date November 2000
 * @author Hj. Malthaner
 */

template<class T> class minivec_tpl
{
protected:

    T * data;

    /**
     * Capacity.
     * @author Hj. Malthaner
     */
    uint8 size;


    /**
     * Number of elements in vector.
     * @author Hj. Malthaner
     */
    uint8 count;

public:

    /**
     * Construct a vector for size elements.
     * @param size The capacity of the new vector
     * @author Hj. Malthaner
     */
	minivec_tpl(uint8 size)
	{
		this->size  = size;
		count = 0;
		if(size>0) {
			data = new T[size];
		} else {
			data = 0;
		}
		count = 0;
	}


    /**
     * Destructor.
     * @author Hj. Malthaner
     */
	~minivec_tpl()
	{
		if(data) {
			delete [] data;
		}
	}

    /**
     * sets the vector to empty
     * @author Hj. Malthaner
     */
    void clear()
    {
	count = 0;
    }

    /**
     * Resizes the maximum data that can be hold by this vector.
     * Existing entries are preserved, new_size must be big enough
     * to hold them.
     *
     * @author prissi
     */
	bool resize(unsigned new_size)
	{
		if(new_size>255) {
			dbg->fatal("minivec_tpl<T>::resize()", "new size %i too large (>255).");
			return false;
		}

		if(new_size<=size) {
			return true;	// do nothing
		}
//MESSAGE("<minivec_tpl>::resize()","old size %i, new size %i",size,new_size);
		if(count > new_size) {
			dbg->fatal("minivec_tpl<T>::resize()", "cannot preserve elements.");
			return false;
		}
		T *new_data = new T[new_size];

		if(size>0  &&  data) {
			for(uint8 i = 0; i < count; i++) {
				new_data[i] = data[i];
			}
			delete [] data;
		}
		size  = new_size;
		data = new_data;
		return true;
	}

    /**
     * Checks if element elem is contained in vector.
     * Uses the == operator for comparison.
     * @author Hj. Malthaner
     */
    bool is_contained(T elem) const
    {
	for(uint8 i=0; i<count; i++) {
	    if(data[i] == elem) {
		return true;
	    }
	}
	return false;
    }


    /**
     * Appends the element at the end of the vector.
     * if out of space, extend with by add element(s)
     * @author prissi
     */
	bool append(T elem,unsigned short add)
	{
		if(count>=size) {
			if(  !resize( count+add )  ) {
				return false;
			}
		}
		data[count ++] = elem;
		return true;
	}

    /**
     * Appends the element at the end of the vector.
     * if out of space, extend with by one element
     * @author prissi
     */
	bool append(T elem)
	{
		return append(elem,1);
	}

    /**
     * Checks if element is contained. Appends only new elements.
     * @author Hj. Malthaner
     */
    bool append_unique(T elem)
    {
		if(!is_contained(elem)) {
			return append(elem,1);
		} else {
			return true;
		}
    }

    /**
     * Checks if element is contained. Appends only new elements.
     * @author Hj. Malthaner
     */
    bool append_unique(T elem,unsigned short add)
    {
		if(!is_contained(elem)) {
			return append(elem,add);
		} else {
			return true;
		}
    }

	/**
	* removes element, if contained
	* @author prisse
	*/
	bool remove(T elem)
	{
		uint8 i,j;
		for(i=j=0;  i<count;  i++,j++  ) {
			if(data[i] == elem) {
				// skip this one
				j++;
				count --;
			}
	///###
			// maybe we copy too often ...
			if(j<size) {
				data[i] = data[j];
			}
		}
		return true;
	}


	/**
	* insets data at a certain pos
	* @author prissi
	*/
	bool insert_at(uint8 pos,T elem)
	{
		if(  pos<count  ) {
			if(count<size  ||  resize( count+1 )) {
				// ok, a valid position, make space
				const long num_elements = (count-pos)*sizeof(T);
				memmove( data+pos+1, data+pos, num_elements );
				data[pos] = elem;
				count ++;
				return true;
			}
			else {
				// could not extend
				return false;
			}
		}
		else if(pos==count) {
			return append(elem,1);
		}
		else {
			dbg->fatal("minivec_tpl<T>::append()","cannot insert at %i! Only %i elements.", pos, count);
			return false;
		}
	}


	/**
	* removes element at position
	* @author prissi
	*/
	bool remove_at(uint8 pos)
	{
		if(  pos<size  &&  pos<count  ) {
			uint8 i,j;
			for(i=pos, j=pos+1;  j<count;  i++,j++  ) {
				data[i] = data[j];
			}
			count --;
			return true;
		}
		return false;
	}

	T& operator [](uint8 i)
	{
		if (i >= count) dbg->fatal("minivec_tpl<T>::[]","index out of bounds: %i not in 0..%d", i, count - 1);
		return data[i];
	}

	const T& operator [](uint8 i) const
	{
		if (i >= count) dbg->fatal("minivec_tpl<T>::[]","index out of bounds: %i not in 0..%d", i, count - 1);
		return data[i];
	}

		T& back() const { return data[count - 1]; }

    /**
     * Gets the number of elements in the vector.
     * @author Hj. Malthaner
     */
    uint8 get_count() const { return count; }


    /**
     * Gets the capacity.
     * @author Hj. Malthaner
     */
    uint8 get_size() const { return size; }

		bool empty() const { return count == 0; }
} GCC_PACKED;

#endif

/*
  Crystal Space Smart Pointers
  Copyright (C) 2002 by Jorrit Tyberghein and Matthias Braun

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CS_REFARR_H__
#define __CS_REFARR_H__

#include "csutil/ref.h"

/**
 * An array of smart pointers.
 */
template <class T>
class csRefArray
{
private:
  int count, limit, threshold;
  csRef<T>* root;

public:
  typedef int ArrayCompareFunction (T* item1, T* item2);
  typedef int ArrayCompareKeyFunction (T* item, void* key);
  
  /**
   * Initialize object to hold initially 'ilimit' elements, and increase
   * storage by 'ithreshold' each time the upper bound is exceeded.
   */
  csRefArray (int ilimit = 0, int ithreshold = 0)
  {
    count = 0;
    limit = (ilimit > 0 ? ilimit : 0);
    threshold = (ithreshold > 0 ? ithreshold : 16);
    if (limit != 0)
      root = (csRef<T>*)calloc (limit, sizeof(csRef<T>));
    else
      root = 0;
  }

  csRefArray (const csRefArray<T>& source)
  {
    count = 0;
    limit = source.limit;
    threshold = source.threshold;
    root = (csRef<T>*)calloc (limit, sizeof(csRef<T>));
    while (count < source.Length ())
    {
      Push (source[count]);
    }
  }
  
  /**
   * Transfer the entire contents of one array to the other. The end
   * result will be that this array will be completely empty and the
   * other array will have all items that originally were in this array.
   * This operation is very efficient.
   */
  void TransferTo (csRefArray<T>& destination)
  {
    destination.DeleteAll ();
    destination.root = root;
    destination.count = count;
    destination.limit = limit;
    destination.threshold = threshold;
    root = 0;
    limit = count = 0;
  }

  /**
   * Clear entire vector.
   */
  void DeleteAll ()
  {
    if (root)
    {
      int i;
      for (i = 0 ; i < limit ; i++)
        root[i] = 0;	// Clear ref.
      free (root);
      root = 0;
      limit = count = 0;
    }
  }

  /**
   * Destroy the container and release all contained references.
   */
  ~csRefArray ()
  {
    DeleteAll ();
  }

  /// Set vector length to n.
  void SetLength (int n)
  {
    // Free all items between new count and old count.
    int i;
    for (i = n ; i < count ; i++) root[i] = 0;

    int old_count = count;
    count = n;

    if (n > limit || (limit > threshold && n < limit - threshold))
    {
      n = ((n + threshold - 1) / threshold ) * threshold;
      if (!n)
      {
        DeleteAll ();
      }
      else if (root == 0)
        root = (csRef<T>*)calloc (n, sizeof(csRef<T>));
      else
      {
        csRef<T>* newroot = (csRef<T>*)calloc (n, sizeof(csRef<T>));
	memcpy (newroot, root, old_count * sizeof (csRef<T>));
	free (root);
	root = newroot;
      }
      limit = n;
    }
  }

  /// Query vector length.
  int Length () const
  {
    return count;
  }

  /// Query vector limit.
  int Limit () const
  {
    return limit;
  }

  /// Get a const reference.
  const csRef<T>& Get (int n) const
  {
    CS_ASSERT (n >= 0 && n < count);
    return root[n];
  }

  /// Get a const reference.
  const csRef<T>& operator [] (int n) const
  {
    CS_ASSERT (n >= 0 && n < count);
    return root[n];
  }

  /// Get a reference.
  csRef<T>& operator [] (int n)
  {
    CS_ASSERT (n >= 0);
    if (n >= count)
      SetLength (n + 1);
    return root[n];
  }

  /// Find a element in array and return its index (or -1 if not found).
  int Find (T* which) const
  {
    int i;
    for (i = 0 ; i < Length () ; i++)
      if (root[i] == which)
        return i;
    return -1;
  }

  /// Push a element on 'top' of vector.
  int Push (T* what)
  {
    SetLength (count + 1);
    root [count - 1] = what;
    return (count - 1);
  }

  /// Push a element on 'top' of vector if it is not already there.
  int PushSmart (T* what)
  {
    int n = Find (what);
    return (n == -1) ? Push (what) : n;
  }

  /// Pop an element from vector 'top'.
  csPtr<T> Pop ()
  {
    csRef<T> ret = root [count - 1];
    SetLength (count - 1);
    return csPtr<T> (ret);
  }

  /// Return the top element but don't remove it.
  T* Top () const
  {
    return root [count - 1];
  }

  /// Delete element number 'n' from vector.
  bool Delete (int n)
  {
    if (n >= 0 && n < count)
    {
      root[n] = 0;	// Clear element.
      const int ncount = count - 1;
      const int nmove = ncount - n;
      if (nmove > 0)
      {
        memmove (&root [n], &root [n + 1], nmove * sizeof (csRef<T>));
	// The following manual IncRef() is to make sure that
	// the element will not get deleted by the SetLength() below.
	if (root[ncount])
	  root[ncount]->IncRef ();
      }
      SetLength (ncount);
      return true;
    }
    else
      return false;
  }

  /// Delete the given element from vector.
  bool Delete (T* item)
  {
    int n = Find (item);
    if (n == -1) return false;
    else return Delete (n);
  }

  /// Insert element 'Item' before element 'n'.
  bool Insert (int n, T* item)
  {
    if (n <= count)
    {
      SetLength (count + 1); // Increments 'count' as a side-effect.
      const int nmove = (count - n - 1);
      if (nmove > 0)
      {
        memmove (&root [n + 1], &root [n], nmove * sizeof (csRef<T>));
	// The following manual IncRef() is to make sure that
	// the element will not get deleted later. This element is
	// currently (temporarily) duplicated in the root array so
	// that's why it needs an additional ref.
	if (root[n])
	  root[n]->IncRef ();
      }
      root [n] = item;
      return true;
    }
    else
     return false;
  }

  /**
   * Find an element based on some key.
   */
  int FindSortedKey (void* key, ArrayCompareKeyFunction* comparekey) const
  {
    int l = 0, r = Length () - 1;
    while (l <= r)
    {
      int m = (l + r) / 2;
      int cmp = comparekey (root [m], key);

      if (cmp == 0)
        return m;
      else if (cmp < 0)
        l = m + 1;
      else
        r = m - 1;
    }
    return -1;
  }

  /**
   * Insert an element at a sorted position.
   * Assumes array is already sorted.
   */
  int InsertSorted (T* item, ArrayCompareFunction* compare)
  {
    int m = 0, l = 0, r = Length () - 1;
    while (l <= r)
    {
      m = (l + r) / 2;
      int cmp = compare (root [m], item);

      if (cmp == 0)
      {
        Insert (++m, item);
        return m;
      }
      else if (cmp < 0)
        l = m + 1;
      else
        r = m - 1;
    }
    if (r == m)
      m++;
    Insert (m, item);
    return m;
  }

  /// Same but for all elements
  void QuickSort (ArrayCompareFunction* compare)
  {
    if (count > 0)
      QuickSort (0, count - 1, compare);
  }
  
  /// Partially sort the array
  void QuickSort (int Left, int Right, ArrayCompareFunction* compare)
  {
  recurse:
    int i = Left, j = Right;
    int x = (Left + Right) / 2;
    do
    {
      while ((i != x) && (compare (root[i], root[x]) < 0))
	i++;
      while ((j != x) && (compare (root[j], root[x]) > 0))
	j--;
      if (i < j)
      {
	csRef<T> swap;
	swap = root[i];
	root[i] = root[j];
	root[j] = swap;
	if (x == i)
	  x = j;
	else if (x == j)
	  x = i;
      }
      if (i <= j)
      {
	i++;
	if (j > Left)
	  j--;
      }
    } while (i <= j);

    if (j - Left < Right - i)
    {
      if (Left < j)
	QuickSort (Left, j, compare);
      if (i < Right)
      {
	Left = i;
	goto recurse;
      }
    }
    else
    {
      if (i < Right)
	QuickSort (i, Right, compare);
      if (Left < j)
      {
	Right = j;
	goto recurse;
      }
    }
  }
  
};

#endif // __CS_REFARR_H__


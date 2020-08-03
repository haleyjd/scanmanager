/*
  Scan Manager

  Purpose: Generic linked list, useful for hash chaining
  Author:  James Haley
*/

#ifndef M_DLLIST_H__
#define M_DLLIST_H__

// 
// DLListItem
//
// This template class is an evolution of the original mdllistitem_t.
// However rather than using an is-a relationship, this functions best
// in a has-a relationship (which is the same role it could already
// play via use of the object member pointer).
//
// This class is intentionally a POD and will most likely remain that way
// for speed and efficiency concerns.
//
template<typename T> class DLListItem
{
public:
   DLListItem<T>  *dllNext;
   DLListItem<T> **dllPrev;
   T              *dllObject; // 08/02/09: pointer back to object
   unsigned int    dllData;   // 02/07/10: arbitrary data cached at node

   inline void insert(T *parentObject, DLListItem<T> **head)
   {
      DLListItem<T> *next = *head;

      if((dllNext = next))
         next->dllPrev = &dllNext;
      dllPrev = head;
      *head = this;

      dllObject = parentObject; // set to object, which is generally distinct
   }

   inline void remove()
   {
      DLListItem<T> **prev = dllPrev;
      DLListItem<T>  *next = dllNext;

      // haleyjd 05/07/13: safety #1: only if prev is non-null
      if(prev && (*prev = next))
         next->dllPrev = prev;

      // haleyjd 05/07/13: safety #2: clear links.
      dllPrev = nullptr;
      dllNext = nullptr;
   }
};

//
// DLList
//
// haleyjd 05/07/13: Added a list type which makes use of DLListItem more
// regulated. Use is strictly optional. Provide the type and a member to
// pointer to the DLListItem field in the class the list will use for links.
//
template<typename T, DLListItem<T> T::* link> class DLList
{
public:
   DLListItem<T> *head;
   inline void insert(T *object) { (object->*link).insert(object, &head); }
   inline void remove(T *object) { (object->*link).remove();              }
   inline void insert(T &object) { insert(&object);                       }
   inline void remove(T &object) { remove(&object);                       }

   inline void tailInsert(T *object)
   {
      auto item = head;

      while(item && item->dllNext)
         item = item->dllNext;

      (object->*link).insert(object, item ? &item->dllNext : &head);
   }
   inline void tailInsert(T &object) { tailInsert(&object); }
};

#endif

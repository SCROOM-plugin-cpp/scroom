/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2011 Kees-Jan Dijkzeul
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License, version 2, as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef OBSERVABLE_HH
#define OBSERVABLE_HH

#include <list>
#include <map>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <scroom/utilities.hh>
#include <scroom/registration.hh>

namespace Scroom
{
  namespace Utils
  {
    template<typename T>
    class Observable;
    
    namespace Detail
    {
      /**
       * Contain all info regarding the registration.
       *
       * Performs de-registration on destruction
       */
      template<typename T>
      class Registration
      {
      public:
        boost::weak_ptr<Observable<T> > observable;
        boost::shared_ptr<T> o;      /**< Reference to the observer (for non-weak registrations) */
        boost::weak_ptr<T> observer; /**< Reference to the observer */
        Registrations registrations; /**< Recursive registrations */

        typedef boost::shared_ptr<Registration> Ptr;
        
      public:
        Registration();
        Registration(boost::weak_ptr<Observable<T> > observable, boost::shared_ptr<T> observer);
        Registration(boost::weak_ptr<Observable<T> > observable, boost::weak_ptr<T> observer);
        ~Registration();

        static Ptr create(boost::weak_ptr<Observable<T> > observable, boost::shared_ptr<T> observer);
        static Ptr create(boost::weak_ptr<Observable<T> > observable, boost::weak_ptr<T> observer);
      };
    }

    /**
     * Base class for something that accepts observers.
     *
     * Upon registering, observers are given a Registration. If the
     * same Observer registers twice (using the same method), he'll
     * still receive events only once. When the Observer destroys his
     * Registration, he'll be unregistered.
     *
     * There are two ways of registering: One can register a @c
     * shared_ptr, or a @c weak_ptr, depending on whether you want the
     * Observable to keep the Observer from being destructed.
     *
     * @note When you register one Observer with both a @c shared_ptr
     *    and a @c weak_ptr, unpredictable things will happen.
     */
    template<typename T>
    class Observable : public boost::noncopyable,
                       public virtual Base
    {
    public:
      typedef boost::shared_ptr<T> Observer;

    private:
      typedef boost::weak_ptr<Detail::Registration<T> > RegistrationWeak;
      typedef boost::weak_ptr<T> ObserverWeak;
      typedef std::map<ObserverWeak, RegistrationWeak> RegistrationMap;
      
    private:
      /** Map all registrations to their registration data */
      RegistrationMap registrationMap;

      /** Mutex protecting the two maps */
      boost::mutex mut;
  
    protected:
      /**
       * Retrieve a list of current observers.
       *
       * Always, a list of @c shared_ptr objects is returned,
       * regardless of whether the Observer registered a @c shared_ptr
       * or a @c weak_ptr.
       */
      std::list<Observer> getObservers();
  
    public:
      Observable();
      virtual ~Observable();

      /**
       * Override this function if you want to be notified of new
       * observers registering.
       */
      virtual void observerAdded(Observer observer);

      /**
       * Add the given @c registration to the Detail::Registration of
       * the given @c observer, such that they have the same lifetime.
       *
       * You'll typically use this if you want to register an Observer
       * with a bunch of child observables. Registering with child
       * observables will result in you being given a bunch of
       * Registration objects. This is where you store them :-)
       *
       * @pre @c observer's Registration must exist.
       *
       * @throw boost::bad_weak_ptr if the registration doesn't exist
       */
      void addRecursiveRegistration(Observer observer, Registration registration);

    public:
      Registration registerStrongObserver(Observer observer);
      Registration registerObserver(ObserverWeak observer);

    private:
      void unregisterObserver(ObserverWeak observer);

      friend class Detail::Registration<T>;
    };

    ////////////////////////////////////////////////////////////////////////
    // Detail::Registration implementation

    template<typename T>
    Detail::Registration<T>::Registration()
      : observable(), o(), observer()
    {
    }
    
    template<typename T>
    Detail::Registration<T>::Registration(boost::weak_ptr<Observable<T> > observable, boost::shared_ptr<T> observer)
      : observable(observable), o(observer), observer(observer)
    {
    }
    
    template<typename T>
    Detail::Registration<T>::Registration(boost::weak_ptr<Observable<T> > observable, boost::weak_ptr<T> observer)
      : observable(observable), o(), observer(observer)
    {
      // We don't want to store a "hard" reference, so field o is intentionally empty.
    }

    template<typename T>
    typename Detail::Registration<T>::Ptr Detail::Registration<T>::create(boost::weak_ptr<Observable<T> > observable,
                                                           boost::shared_ptr<T> observer)
    {
      return typename Detail::Registration<T>::Ptr(new Detail::Registration<T>(observable, observer));
    }
        
    template<typename T>
    typename Detail::Registration<T>::Ptr Detail::Registration<T>::create(boost::weak_ptr<Observable<T> > observable,
                                                           boost::weak_ptr<T> observer)
    {
      return typename Detail::Registration<T>::Ptr(new Detail::Registration<T>(observable, observer));
    }
    
    template<typename T>
    Detail::Registration<T>::~Registration()
    {
      boost::shared_ptr<Observable<T> > observable = this->observable.lock();
      if(observable)
      {
        observable->unregisterObserver(observer);
      }
    }
    
    ////////////////////////////////////////////////////////////////////////
    // Observable implementation
    template<typename T>
    Observable<T>::Observable()
    {
    }

    template<typename T>
    Observable<T>::~Observable()
    {
      // Destroy strong references to any observers
      //
      // Normally, I'd be concerned that destroying references to
      // observers would cause them to try to unregister themselves,
      // resulting in deadlock. But because we are in our destructor,
      // noone has a reference to us any more, so no unregistration
      // attempts will be made.
      boost::mutex::scoped_lock lock(mut);
      typename RegistrationMap::iterator cur = registrationMap.begin();
      typename RegistrationMap::iterator end = registrationMap.end();

      for(;cur!=end; ++cur)
      {
        typename Detail::Registration<T>::Ptr registration = cur->second.lock();
        if(registration)
        {
          registration->o.reset();
        }
      }
    }

    template<typename T>
    std::list<typename Observable<T>::Observer> Observable<T>::getObservers()
    {
      boost::mutex::scoped_lock lock(mut);
      std::list<typename Observable<T>::Observer> result;

      typename RegistrationMap::iterator cur = registrationMap.begin();
      typename RegistrationMap::iterator end = registrationMap.end();

      for(;cur!=end; ++cur)
      {
        typename Detail::Registration<T>::Ptr registration = cur->second.lock();
        if(registration)
        {
          Observer o = registration->observer.lock();
          if(o)
            result.push_back(o);
        }
      }

      return result;
    }

    template<typename T>
    Registration Observable<T>::registerStrongObserver(Observable<T>::Observer observer)
    {
      boost::mutex::scoped_lock lock(mut);
      typename Detail::Registration<T>::Ptr r = registrationMap[observer].lock();
      if(!r)
      {
        r = Detail::Registration<T>::create(shared_from_this<Observable<T> >(), observer);
        registrationMap[observer] = r;
        lock.unlock();
        observerAdded(observer);
      }
      
      return r;
    }

    template<typename T>
    Registration Observable<T>::registerObserver(Observable<T>::ObserverWeak observer)
    {
      boost::mutex::scoped_lock lock(mut);
      typename Detail::Registration<T>::Ptr r = registrationMap[observer].lock();
      if(!r)
      {
        r = Detail::Registration<T>::create(shared_from_this<Observable<T> >(), observer);
        registrationMap[observer] = r;
        lock.unlock();
        observerAdded(typename Observable<T>::Observer(observer));
      }
        
      return r;
    }

    template<typename T>
    void Observable<T>::unregisterObserver(Observable<T>::ObserverWeak observer)
    {
      boost::mutex::scoped_lock lock(mut);
      registrationMap.erase(observer);
    }

    template<typename T>
    void Observable<T>::observerAdded(Observable<T>::Observer)
    {
      // Do nothing
    }

    template<typename T>
    void Observable<T>::addRecursiveRegistration(Observable<T>::Observer observer, Registration registration)
    {
      boost::mutex::scoped_lock lock(mut);
      
      typename Detail::Registration<T>::Ptr(registrationMap[observer])->registrations.push_back(registration);
    }
  }
}

#endif
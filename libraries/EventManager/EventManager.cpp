/*
 * EventManager.cpp
 *

 * An event handling system for Arduino.
 *
 * Author: igormt@alumni.caltech.edu
 * Copyright (c) 2013 Igor Mikolic-Torreira
 * 
 * Inspired by and adapted from the 
 * Arduino Event System library by
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 * This library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser
 * General Public License along with this library; if not,
 * write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


#include "EventManager.h"
#include <util/atomic.h>

void StreamPrint_progmem(Print &out,PGM_P format,...)
{
  // program memory version of printf - copy of format string and result share a buffer
  // so as to avoid too much memory use
  char formatString[128], *ptr;
  strncpy_P( formatString, format, sizeof(formatString) ); // copy in from program mem
  // null terminate - leave last char since we might need it in worst case for result's \0
  formatString[ sizeof(formatString)-2 ]='\0';
  ptr=&formatString[ strlen(formatString)+1 ]; // our result buffer...
  va_list args;
  va_start (args,format);
  vsnprintf(ptr, sizeof(formatString)-1-strlen(formatString), formatString, args );
  va_end (args);
  formatString[ sizeof(formatString)-1 ]='\0';
  out.print(ptr);
}

#if EVENTMANAGER_DEBUG
#define EVTMGR_DEBUG_PRINT( x, ... )		Serialprint( x, ##__VA_ARGS__ );
#define EVTMGR_DEBUG_PRINT_PTR( x )	Serialprint( "%p", x );
#define EVTMGR_DEBUG_PRINTLN_PTR( x )	Serialprint( "%p\n", x );
#else
#define EVTMGR_DEBUG_PRINT( x, ... )
#define EVTMGR_DEBUG_PRINTLN( x, ... )
#define EVTMGR_DEBUG_PRINT_PTR( x )	
#define EVTMGR_DEBUG_PRINTLN_PTR( x )	
#endif


EventManager::EventManager( SafetyMode safety ) : 
mHighPriorityQueue( ( safety == EventManager::kInterruptSafe ) ), 
mLowPriorityQueue( ( safety == EventManager::kInterruptSafe ) )
{
}


int EventManager::processEvent() 
{
    EventQueue::EventElement event;
    int handledCount = 0;

    if ( mHighPriorityQueue.popEvent( &event ) )
    {
        handledCount = mListeners.sendEvent( event.code, event.param );
        
        EVTMGR_DEBUG_PRINT( "processEvent() hi-pri event %d, %d sent to %d", event.code, event.param, handledCount );

        delete event.param;
    }
    
    // If no high-pri events handled (either because there are no high-pri events or 
    // because there are no listeners for them), then try low-pri events
    if ( !handledCount && mLowPriorityQueue.popEvent( &event ) )
    {
        handledCount = mListeners.sendEvent( event.code, event.param );
        
        EVTMGR_DEBUG_PRINT( "processEvent() lo-pri event %d, %d sent to %d", event.code, event.param, handledCount );

        delete event.param;
    }
    
    return handledCount;
}


int EventManager::processAllEvents() 
{
    EventQueue::EventElement event;
    int handledCount = 0;

    while ( mHighPriorityQueue.popEvent( &event ) )
    {
        handledCount += mListeners.sendEvent( event.code, event.param );

        EVTMGR_DEBUG_PRINT( "processEvent() hi-pri event %d, %d sent to %d", event.code, event.param, handledCount );

        delete event.param;
    }
    
    while ( mLowPriorityQueue.popEvent( &event ) )
    {
        handledCount += mListeners.sendEvent( event.code, event.param );

        EVTMGR_DEBUG_PRINT( "processEvent() lo-pri event %d, %d sent to %d", event.code, event.param, handledCount );

        delete event.param;
    }
    
    return handledCount;
}



/********************************************************************/



EventManager::ListenerList::ListenerList() : 
mNumListeners( 0 ), mDefaultCallback( 0 )
{
    mListeners.reserve(kMaxListeners);
}

int EventManager::ListenerList::numListeners()
{
    return mListeners.size();
}

int EventManager::numListeners()
{
    return mListeners.numListeners();
}

boolean EventManager::ListenerList::addListener( int eventCode, EventListener* listener ) 
{
    EVTMGR_DEBUG_PRINT( "addListener() enter %d, ", eventCode );
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    // Argument check
    if ( !listener ) 
    {
        return false;
    }

    ListenerItem item;
    item.callback  = listener;
    item.eventCode = eventCode;
    item.enabled   = true;
    mListeners.push_back(item);

    EVTMGR_DEBUG_PRINT( "addListener() listener added\n" )

    return true;
}


boolean EventManager::ListenerList::removeListener( int eventCode, EventListener* listener ) 
{
    EVTMGR_DEBUG_PRINT( "removeListener() enter %d, ", eventCode );
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    if ( isEmpty() )
    {
        EVTMGR_DEBUG_PRINT( "removeListener() no listeners\n" )
        return false;
    }

    for(SimpleList<ListenerItem>::iterator itr = mListeners.begin(); itr != mListeners.end(); ++itr)
    {
        if(itr->eventCode == eventCode && itr->callback == listener)
        {
            mListeners.erase(itr);
            EVTMGR_DEBUG_PRINT( "removeListener() removed\n" )
            return true;
        }
    }

    EVTMGR_DEBUG_PRINT( "removeListener() not found\n" )
    return false;
}


int EventManager::ListenerList::removeListener( EventListener* listener ) 
{
    EVTMGR_DEBUG_PRINT( "removeListener() enter " )
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    if ( isEmpty() )
    {
        EVTMGR_DEBUG_PRINT( "removeListener() no listeners\n" )
        return 0;
    }

    int removed = 0;
    for(SimpleList<ListenerItem>::iterator itr = mListeners.begin(); itr != mListeners.end(); ++itr)
    {
        if(itr->callback == listener)
        {
            itr = mListeners.erase(itr);
            removed++;
        }
    }
    
    EVTMGR_DEBUG_PRINT( "  removeListener() removed " )
    EVTMGR_DEBUG_PRINT( "%d\n", removed )
    
    return removed;
}


boolean EventManager::ListenerList::enableListener( int eventCode, EventListener* listener, boolean enable ) 
{
    EVTMGR_DEBUG_PRINT( "enableListener() enter %d, ", eventCode )
    EVTMGR_DEBUG_PRINT_PTR( listener )
    EVTMGR_DEBUG_PRINT( ", %d\n", enable )

    if ( isEmpty() )
    {
        EVTMGR_DEBUG_PRINT( "removeListener() no listeners\n" )
        return false;
    }

    for(SimpleList<ListenerItem>::iterator itr = mListeners.begin(); itr != mListeners.end(); ++itr)
    {
        if(itr->eventCode == eventCode && itr->callback == listener)
        {
            itr->enabled = enable;
            EVTMGR_DEBUG_PRINT( "enableListener() success\n" )
            return true;
        }
    }

    EVTMGR_DEBUG_PRINT( "enableListener() not found fail\n" )
    return false;
}


boolean EventManager::ListenerList::isListenerEnabled( int eventCode, EventListener* listener ) 
{
    if ( isEmpty() )
    {
        return false;
    }

    for(SimpleList<ListenerItem>::iterator itr = mListeners.begin(); itr != mListeners.end(); ++itr)
    {
        if(itr->eventCode == eventCode && itr->callback == listener)
        {
            return itr->enabled;
        }
    }

    return false;
}


int EventManager::ListenerList::sendEvent( int eventCode, EventParam *param )
{
    EVTMGR_DEBUG_PRINT( "sendEvent() enter %d, ", eventCode )
    EVTMGR_DEBUG_PRINTLN_PTR( param )

    int handlerCount = 0;
    for(SimpleList<ListenerItem>::iterator itr = mListeners.begin(); itr != mListeners.end(); ++itr)
    {
        if ( ( itr->callback != 0 ) && ( itr->eventCode == eventCode ) && itr->enabled )
        {
            handlerCount++;
            (*itr->callback)( eventCode, param );
        }
    }
    
    EVTMGR_DEBUG_PRINT( "sendEvent() sent to %d\n",  handlerCount )

    if ( !handlerCount ) 
    {
        handlerCount += callDefaultListener( eventCode, param );

    }
#if EVENTMANAGER_DEBUG
    else
    {
        EVTMGR_DEBUG_PRINT( "sendEvent() no default\n" )
    }
#endif
    
    return handlerCount;
}


boolean EventManager::ListenerList::setDefaultListener( EventListener* listener ) 
{
    EVTMGR_DEBUG_PRINT( "setDefaultListener() enter " )
    EVTMGR_DEBUG_PRINTLN_PTR( listener )
    
    if ( listener == 0 ) 
    {
        return false;
    }

    mDefaultCallback = listener;
    mDefaultCallbackEnabled = true;
    return true;
}


void EventManager::ListenerList::removeDefaultListener() 
{
    mDefaultCallback = 0;
    mDefaultCallbackEnabled = false;
}


void EventManager::ListenerList::enableDefaultListener( boolean enable ) 
{
    mDefaultCallbackEnabled = enable;
}

int EventManager::ListenerList::callDefaultListener(int eventCode, EventParam *param)
{
    if ( ( mDefaultCallback != 0 ) && mDefaultCallbackEnabled )
    {
        (*mDefaultCallback)( eventCode, param );

        EVTMGR_DEBUG_PRINT( "sendEvent() event sent to default\n" );
        return 1;
    }
    return 0;
}



/******************************************************************************/




EventManager::EventQueue::EventQueue( boolean beSafe ) :
mInterruptSafeMode( beSafe )
{
    mEventQueue.reserve(kEventQueueSize);
}



boolean EventManager::EventQueue::queueEvent(int eventCode, int eventParam, uint32_t eventTime)
{  
    return queueEvent(eventCode, new EventParam(eventParam), eventTime);
}

boolean EventManager::EventQueue::queueEvent(int eventCode, EventParam *eventParam, uint32_t eventTime)
{
    /*
    * The call to noInterrupts() MUST come BEFORE the full queue check.
    *
    * If the call to isFull() returns FALSE but an asynchronous interrupt queues
    * an event, making the queue full, before we finish inserting here, we will then
    * corrupt the queue (we'll add an event to an already full queue). So the entire
    * operation, from the call to isFull() to completing the inserting (if not full)
    * must be atomic.
    *
    * Note that this race condition can only arise IF both interrupt and non-interrupt (normal)
    * code add events to the queue.  If only normal code adds events, this can't happen
    * because then there are no asynchronous additions to the queue.  If only interrupt
    * handlers add events to the queue, this can't happen because further interrupts are
    * blocked while an interrupt handler is executing.  This race condition can only happen
    * when an event is added to the queue by normal (non-interrupt) code and simultaneously
    * an interrupt handler tries to add an event to the queue.  This is the case that the
    * cli() (= noInterrupts()) call protects against.
    *
    * Contrast this with the logic in popEvent().
    *
    */

    // Because this function may be called from interrupt handlers, debugging is
    // only available in when NOT in interrupt safe mode.

#if EVENTMANAGER_DEBUG
    if ( !mInterruptSafeMode )
    {
        EVTMGR_DEBUG_PRINT( "queueEvent() enter %d, ", eventCode )
        EVTMGR_DEBUG_PRINTLN_PTR( eventParam )
    }
#endif

    uint8_t sregSave;
    if ( mInterruptSafeMode )
    {
        // Set up the atomic section by saving SREG and turning off interrupts
        // (because we might or might not be called inside an interrupt handler)
        sregSave = SREG;
        cli();
    }

    // ATOMIC BLOCK BEGIN (only atomic **if** mInterruptSafeMode is on)
    EventElement event;

    event.code  = eventCode;
    event.param = eventParam;
    event.time  = eventTime;

    mEventQueue.push_back(event);
    boolean retVal = true;
    // ATOMIC BLOCK END

    if ( mInterruptSafeMode )
    {
        // Restore previous state of interrupts
        __iRestore( &sregSave );
    }

#if EVENTMANAGER_DEBUG
    if ( !mInterruptSafeMode )
    {
        EVTMGR_DEBUG_PRINT( "queueEvent() " );
        if(retVal)
            EVTMGR_DEBUG_PRINT( "added\n" )
        else
            EVTMGR_DEBUG_PRINT( "full\n" );
    }
#endif

    return retVal;
}


boolean EventManager::EventQueue::popEvent( EventElement *event )
{
    /*
    * The call to noInterrupts() MUST come AFTER the empty queue check.  
    * 
    * There is no harm if the isEmpty() call returns an "incorrect" TRUE response because 
    * an asynchronous interrupt queued an event after isEmpty() was called but before the 
    * return is executed.  We'll pick up that asynchronously queued event the next time 
    * popEvent() is called. 
    * 
    * If noInterrupts() is set before the isEmpty() check, we pretty much lock-up the Arduino.  
    * This is because popEvent(), via processEvents(), is normally called inside loop(), which
    * means it is called VERY OFTEN.  Most of the time (>99%), the event queue will be empty.
    * But that means that we'll have interrupts turned off for a significant fraction of the 
    * time.  We don't want to do that.  We only want interrupts turned off when we are 
    * actually manipulating the queue.
    * 
    * Contrast this with the logic in queueEvent().
    * 
    */

    if ( isEmpty() ) 
    {
        return false;
    }
    
    if ( mInterruptSafeMode )
    {
        EVTMGR_DEBUG_PRINT( "popEvent() interrupts off\n" )
        cli();
    }

    bool found = false;
    for(SimpleList<EventElement>::iterator itr = mEventQueue.begin(); itr != mEventQueue.end(); ++itr)
    {
        if(itr->time <= millis())
        {
            event->code = itr->code;
            event->param = itr->param;
            event->time = itr->time;
            mEventQueue.erase(itr);
            found = true;
            break;
        }
    }
    if(!found)
        return false;

    if ( mInterruptSafeMode )
    {
        // This function is NOT designed to called from interrupt handlers, so
        // it is safe to turn interrupts on instead of restoring a previous state.
        sei();
        EVTMGR_DEBUG_PRINT( "popEvent() interrupts on\n" )
    }

    EVTMGR_DEBUG_PRINT( "popEvent() return %d, ", event->code)
    EVTMGR_DEBUG_PRINTLN_PTR( event->param)

    return true;
}

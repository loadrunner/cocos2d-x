/****************************************************************************
 Copyright (c) 2013-2014 Chukong Technologies Inc.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __cocos2d_libs__CCGyroscopeListener__
#define __cocos2d_libs__CCGyroscopeListener__

#include "base/CCEventListener.h"
#include "base/ccTypes.h"

/**
 * @addtogroup base
 * @{
 */

NS_CC_BEGIN

/** @class EventListenerGyroscope
 * @brief Gyroscope event listener.
 * @js NA
 */
class CC_DLL EventListenerGyroscope : public EventListener
{
public:
    static const std::string LISTENER_ID;
    
    /** Create a gyroscope EventListener.
     *
     * @param callback The gyroscope callback method.
     * @return An autoreleased EventListenerGyroscope object.
     */
    static EventListenerGyroscope* create(const std::function<void(Gyroscope*, Event*)>& callback);
    
    /** Destructor.
     */
    virtual ~EventListenerGyroscope();
    
    /// Overrides
    virtual EventListenerGyroscope* clone() override;
    virtual bool checkAvailable() override;
    
CC_CONSTRUCTOR_ACCESS:
    EventListenerGyroscope();
    
    bool init(const std::function<void(Gyroscope*, Event* event)>& callback);
    
private:
    std::function<void(Gyroscope*, Event*)> onGyroscopeEvent;
    
    //friend class LuaEventListenerGyroscope;
};

NS_CC_END

// end of base group
/// @}

#endif /* defined(__cocos2d_libs__CCGyroscopeListener__) */

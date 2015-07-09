/****************************************************************************
 Copyright (c) 2015 Chris Hannon http://www.channon.us
 Copyright (c) 2013-2015 Chukong Technologies Inc.

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

*based on the SocketIO library created by LearnBoost at http://socket.io
*using spec version 1 found at https://github.com/LearnBoost/socket.io-spec

Usage is described below, a full working example can be found in TestCpp under ExtionsTest/NetworkTest/SocketIOTest

creating a new connection to a socket.io server running at localhost:3000

    SIOClient *client = SocketIO::connect(*delegate, "ws://localhost:3000");

the connection process will begin and if successful delegate::onOpen will be called
if the connection process results in an error, delegate::onError will be called with the err msg

sending a message to the server

    client->send("Hello!");

emitting an event to be handled by the server, argument json formatting is up to you

    client->emit("eventname", "[{\"arg\":\"value\"}]");

registering an event callback, target should be a member function in a subclass of SIODelegate
CC_CALLBACK_2 is used to wrap the callback with std::bind and store as an SIOEvent

    client->on("eventname", CC_CALLBACK_2(TargetClass::targetfunc, *targetclass_instance));

event target function should match this pattern, *this pointer will be made available

    void TargetClass::targetfunc(SIOClient *, const std::string&)

disconnect from the endpoint by calling disconnect(), onClose will be called on the delegate once complete
in the onClose method the pointer should be set to NULL or used to connect to a new endpoint

    client->disconnect();

 ****************************************************************************/

#ifndef __CC_SOCKETIO_H__
#define __CC_SOCKETIO_H__

#include "platform/CCPlatformMacros.h"
#include "base/CCMap.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "WebSocket.h"
#include "HttpClient.h"
#include <algorithm>
#include <sstream>
#include <string>

#include "json/rapidjson.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"

/**
 * @addtogroup core
 * @{
 */

NS_CC_BEGIN

namespace network {

//forward declarations
class SIOClientImpl;
class SIOClient;

/**
 * Singleton and wrapper class to provide static creation method as well as registry of all sockets.
 *
 * @lua NA
 */
class CC_DLL SocketIO
{
public:
    /**
     * Get instance of SocketIO.
     *
     * @return SocketIO* the instance of SocketIO.
     */
    static SocketIO* getInstance();
    static void destroyInstance();

    /**
     * The delegate class to process socket.io events.
     * @lua NA
     */
    class SIODelegate
    {
    public:
        /** Destructor of SIODelegate. */
        virtual ~SIODelegate() {}
        /**
         * This is kept for backwards compatibility, connect is now fired as a socket.io event "connect"
         * 
         * This function would be called when the related SIOClient object recevie messages that mean it have connected to endpoint sucessfully.
         *
         * @param client the connected SIOClient object.
         */
        virtual void onConnect(SIOClient* client) { };
        /**
         * This is kept for backwards compatibility, message is now fired as a socket.io event "message"
         *
         * This function would be called wwhen the related SIOClient object recevie message or json message.
         *
         * @param client the connected SIOClient object.
         * @param data the message,it could be json message
         */
        virtual void onMessage(SIOClient* client, const std::string& data) { };
        /**
         * Pure virtual callback function, this function should be overrided by the subclass.
         *
         * This function would be called when the related SIOClient object disconnect or recevie disconnect signal.
         *
         * @param client the connected SIOClient object.
         */
        virtual void onClose(SIOClient* client) = 0;
        /**
         * Pure virtual callback function, this function should be overrided by the subclass.
         *
         * This function would be called wwhen the related SIOClient object recevie error signal or didn't connect the endpoint but do some network operation,eg.,send and emit,etc.
         *
         * @param client the connected SIOClient object.
         * @param data the error message
         */
        virtual void onError(SIOClient* client, const std::string& data) = 0;
        /**
         * Fire event to script when the related SIOClient object receive the fire event signal.
         *
         * @param client the connected SIOClient object.
         * @param eventName the event's name.
         * @param data the event's data information.
         */
        virtual void fireEventToScript(SIOClient* client, const std::string& eventName, const std::string& data) { };
    };

    /**
     *  Static client creation method, similar to socketio.connect(uri) in JS.
     *  @param  uri      the URI of the socket.io server.
     *  @param  delegate the delegate which want to receive events from the socket.io client.
     *  @return SIOClient* an initialized SIOClient if connected successfully, otherwise nullptr.
     */
    static SIOClient* connect(const std::string& uri, SocketIO::SIODelegate& delegate);

    /**
     *  Static client creation method, similar to socketio.connect(uri) in JS.
     *  @param  delegate the delegate which want to receive events from the socket.io client.
     *  @param  uri      the URI of the socket.io server.
     *  @return SIOClient* an initialized SIOClient if connected successfully, otherwise nullptr.
     */
    CC_DEPRECATED_ATTRIBUTE  static SIOClient* connect(SocketIO::SIODelegate& delegate, const std::string& uri);

private:

    SocketIO();
    virtual ~SocketIO(void);

    static SocketIO *_inst;

    cocos2d::Map<std::string, SIOClientImpl*> _sockets;

    SIOClientImpl* getSocket(const std::string& uri);
    void addSocket(const std::string& uri, SIOClientImpl* socket);
    void removeSocket(const std::string& uri);

    friend class SIOClientImpl;
private:
    CC_DISALLOW_COPY_AND_ASSIGN(SocketIO)
};

//c++11 style callbacks entities will be created using CC_CALLBACK (which uses std::bind)
typedef std::function<void(SIOClient*, const std::string&)> SIOEvent;
//c++11 map to callbacks
typedef std::unordered_map<std::string, SIOEvent> EventRegistry;

/**
 * A single connection to a socket.io endpoint.
 *
 * @lua NA
 */
class CC_DLL SIOClient
    : public cocos2d::Ref
{
private:
    int _port;
    std::string _host, _path, _tag;
    bool _connected;
    SIOClientImpl* _socket;

    SocketIO::SIODelegate* _delegate;

    EventRegistry _eventRegistry;

    void fireEvent(const std::string& eventName, const std::string& data);

    void onOpen();
    void onConnect();
	void socketClosed();

    friend class SIOClientImpl;

public:
    /**
     * Construtor of SIOClient class.
     *
     * @param host the string that represent the host address.
     * @param port the int value represent the port number.
     * @param path the string that represent endpoint.
     * @param impl the SIOClientImpl object.
     * @param delegate the SIODelegate object.
     */
    SIOClient(const std::string& host, int port, const std::string& path, SIOClientImpl* impl, SocketIO::SIODelegate& delegate);
    /**
     * Destructior of SIOClient class.
     */
    virtual ~SIOClient(void);

    /**
     * Get the delegate for the client
     * @return the delegate object for the client
     */
    SocketIO::SIODelegate* getDelegate() { return _delegate; };

    /**
     * Disconnect from the endpoint, onClose will be called for the delegate when complete
     */
    void disconnect();
    /**
     * Send a message to the socket.io server.
     *
     * @param s message.
     */
    void send(std::string s);
    /**
     *  Emit the eventname and the args to the endpoint that _path point to.
     * @param eventname
     * @param args
     */
    void emit(std::string eventname, std::string args);
    /**
     * Used to register a socket.io event callback.
     * Event argument should be passed using CC_CALLBACK2(&Base::function, this).
     * @param eventName the name of event.
     * @param e the callback function.
     */
    void on(const std::string& eventName, SIOEvent e);
    
    /**
     * Set tag of SIOClient.
     * The tag is used to distinguish the various SIOClient objects.
     * @param tag string object.
     */
    inline void setTag(const char* tag)
    {
        _tag = tag;
    };
    /**
     * Get tag of SIOClient.
     * @return const char* the pointer point to the _tag.
     */
    inline const char* getTag()
    {
        return _tag.c_str();
    };
    
    inline bool isConnected() { return _connected; }
};

class SocketIOPacketV10x;

class CC_DLL SocketIOPacket
{
public:
	typedef enum
	{
		V09x,
		V10x
	}SocketIOVersion;
	
	SocketIOPacket();
	virtual ~SocketIOPacket();
	void initWithType(std::string packetType);
	void initWithTypeIndex(int index);
	
	std::string toString();
	virtual int typeAsNumber();
	std::string typeForIndex(int index);
	
	void setEndpoint(std::string endpoint){ _endpoint = endpoint; };
	std::string getEndpoint(){ return _endpoint; };
	void setEvent(std::string event){ _name = event; };
	std::string getEvent(){ return _name; };
	
	void addData(std::string data);
	std::vector<std::string> getData(){ return _args; };
	virtual std::string stringify();
	
	static SocketIOPacket * createPacketWithType(std::string type, SocketIOPacket::SocketIOVersion version);
	static SocketIOPacket * createPacketWithTypeIndex(int type, SocketIOPacket::SocketIOVersion version);
protected:
	std::string _pId;//id message
	std::string _ack;//
	std::string _name;//event name
	std::vector<std::string> _args;//we will be using a vector of strings to store multiple data
	std::string _endpoint;//
	std::string _endpointseperator;//socket.io 1.x requires a ',' between endpoint and payload
	std::string _type;//message type
	std::string _separator;//for stringify the object
	std::vector<std::string> _types;//types of messages
};

class CC_DLL SocketIOPacketV10x : public SocketIOPacket
{
public:
	SocketIOPacketV10x();
	virtual ~SocketIOPacketV10x();
	int typeAsNumber();
	std::string stringify();
private:
	std::vector<std::string> _typesMessage;
};


/**
 *  @brief The implementation of the socket.io connection
 *		 Clients/endpoints may share the same impl to accomplish multiplexing on the same websocket
 */
class SIOClientImpl :
	public cocos2d::Ref,
	public WebSocket::Delegate
{
private:
	int _port, _heartbeat, _timeout;
	std::string _host, _sid, _uri;
	bool _connected;
	SocketIOPacket::SocketIOVersion _version;

	WebSocket *_ws;

	Map<std::string, SIOClient*> _clients;

public:
	SIOClientImpl(const std::string& host, int port);
	virtual ~SIOClientImpl(void);

	static SIOClientImpl* create(const std::string& host, int port);

	virtual void onOpen(WebSocket* ws);
	virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
	virtual void onClose(WebSocket* ws);
	virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

	void connect();
	void disconnect();
	bool init();
	void handshake();
	void handshakeResponse(HttpClient *sender, HttpResponse *response);
	void openSocket();
	void heartbeat(float dt);

	SIOClient* getClient(const std::string& endpoint);
	void addClient(const std::string& endpoint, SIOClient* client);

	void connectToEndpoint(const std::string& endpoint);
	void disconnectFromEndpoint(const std::string& endpoint);

	void send(std::string endpoint, std::string s);
	void send(SocketIOPacket *packet);
	void emit(std::string endpoint, std::string eventname, std::string args);


};

} // namespace network

NS_CC_END

// end group
/// @}

#endif /* defined(__CC_JSB_SOCKETIO_H__) */

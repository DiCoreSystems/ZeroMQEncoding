#pragma once

#include "capnzero/Common.h"

#include <zmq.h>

#include <string>

#include <assert.h>
#include <functional>
#include <thread>
#include <unistd.h>
#include <vector>
#include "capnzero-base-msgs/string_generated.h"
#include <iostream>

//#define DEBUG_SUBSCRIBER

namespace capnzero
{
class SubscriberFlatbuffers
{
public:
    typedef std::function<void(const MessageFlatbuffers& msg)> callbackFunction;
    static const int WORD_SIZE;

    SubscriberFlatbuffers(void* context, Protocol protocol);
    virtual ~SubscriberFlatbuffers();

    /**
     * Starts the receiving thread, if called for the first time. Changes the callback to the given function and object.
     * @tparam CallbackObjType
     * @param callbackFunction
     * @param callbackObject
     */
    template <class CallbackObjType>
    void subscribe(void (CallbackObjType::*callbackFunction)(const MessageFlatbuffers& msg), CallbackObjType* callbackObject) {
        using std::placeholders::_1;
        this->callbackFunction_ = std::bind(callbackFunction, callbackObject, _1);
        if (!running) {
            this->running = true;
            this->runThread = new std::thread(&SubscriberFlatbuffers::receive, this);
        }
    }

    /**
     * Starts the receiving thread, if called for the first time. Changes the callback to the given function.
     * @param callbackFunction
     */
    void subscribe(void (*callbackFunction)(const MessageFlatbuffers& msg));

    /**
     * Sets the topic to receive from.
     * @param defaultTopic
     */
    void setTopic(std::string topic);

    /**
     * Connects or binds the socket of the subscriber to the given address.
     * @param address The address.
     */
    void addAddress(std::string address);

    /**
     * Sets the receiver high water mark level of the underlying socket of
     * this subscriber.
     * @param queueSize
     */
    void setReceiveQueueSize(int queueSize);

protected:
    void* context;
    void* socket;
    std::string topic;
    Protocol protocol;
    int rcvTimeout; /** < only initialized if needed */

    callbackFunction callbackFunction_;

    std::thread* runThread;
    bool running;

    void receive();
};

} // namespace capnzero

#include "capnproto-base-msgs/string.capnp.h"
#include <capnzero/Publisher.h>
#include <capnzero/Common.h>
#include <capnp/common.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <kj/array.h>
#include <chrono>
#include <thread>
#include <signal.h>
#include <capnzero/Subscriber.h>
#include <string>
#include <iostream>
#include <map>
#include "Statistics.h"
//#define DEBUG_PUB

std::map<int16_t , std::chrono::time_point<std::chrono::high_resolution_clock >> measuringMap;
std::map<long, double >Mymap;
static bool interrupted = false;
static void s_signal_handler(int signal_value)
{
    interrupted = true;
}
void callback(::capnp::FlatArrayMessageReader& reader);

static void s_catch_signals(void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

int main(int argc, char** argv)
{
    int16_t counter=0;
    s_catch_signals();

    if (argc <= 1) {
        std::cerr << "Synopsis: rosrun capnzero pub \"String that should be published!\"" << std::endl;
        return -1;
    }

    for (size_t i = 0; i < argc; i++) {
        std::cout << "Param " << i << ": '" << argv[i] << "'" << std::endl;
    }

    // Cap'n Proto: create proto message
    Statistics<double> *st =new Statistics<double > ;
    // init builder
    ::capnp::MallocMessageBuilder msgBuilder;
    capnproto::Capnprotoperformancetest::Builder beaconMsgBuilder = msgBuilder.initRoot<capnproto::Capnprotoperformancetest>();

    // set content
    beaconMsgBuilder.setMessage(argv[2]);
#ifdef DEBUG_PUB
    std::cout << "pub: Message to send: " << beaconMsgBuilder.toString().flatten().cStr() << std::endl;
#endif
    void* ctx = zmq_ctx_new();

//Publisher  part
    capnzero::Publisher pub = capnzero::Publisher(ctx, argv[1]);
    capnzero::Subscriber* sub = new capnzero::Subscriber(ctx, argv[1]);
    pub.bind(capnzero::CommType::UDP, "224.0.0.2:5500");

//Subscriber part
    sub->connect(capnzero::CommType::UDP, "224.0.0.2:5554");
    sub->subscribe(&callback);

    while (!interrupted)
    {
        {     beaconMsgBuilder.setNumber(counter);
            int numBytesSent = pub.send(msgBuilder);
            measuringMap.emplace(counter, std::chrono::high_resolution_clock::now());
            {
                std::cout << "Publisher is going to publish: "<< numBytesSent << " Bytes sent!" << std::endl;
                std::cout << "Publisher is going to publish: "<< counter <<" Message" << std::endl;}
#ifdef DEBUG_PUB
            std::cout << "I am going to publish the following message : "<< numBytesSent << " Bytes sent!" << std::endl;
#endif
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }// wait until everything is send
        std::this_thread::sleep_for(std::chrono::seconds(2));
        counter++;
    }
    std::cout << "We must have missed: " << measuringMap.size() << " Number of Msgs!" <<std::endl;
    std::cout << "number of rcvd msg: "<<Mymap.size()<<std::endl ;
    for (auto& entry : measuringMap)
    {
        std::cout << "ID: " << entry.first << " StartTime: " << std::chrono::duration_cast<std::chrono::milliseconds>(entry.second.time_since_epoch()).count() << std::endl;
    }
    st->referencemean(Mymap);
    st->referencestd_dev(Mymap);
    st->rmax(Mymap);
    st->rmin(Mymap);
    std::cout << "Cleaning up now. "  << std::endl;
    delete  st;
    Mymap.clear();
    measuringMap.clear();
    delete sub;
    zmq_ctx_term(ctx);
    return 0;
}

void callback(::capnp::FlatArrayMessageReader& reader)

{
    std::cout << "I have received the following from the subscriber: " << std::endl;
    std::string rcvdmsg = reader.getRoot<capnproto::Capnprotoperformancetest>().toString().flatten().cStr() ;
    int16_t rcvmsgnumber=reader.getRoot<capnproto::Capnprotoperformancetest>().getNumber();
    std::string rcvmsgstring=reader.getRoot<capnproto::Capnprotoperformancetest>().getMessage();
    std::cout << "Size of the received str is: " << rcvdmsg.length() << " \n";
    std::cout << "String data inside received msg: " << rcvmsgstring << " \n";
    std::cout << "Integer data inside received msg: "<< rcvmsgnumber << " \n";

    //std::cout <<"The data inside redaer"<<<<std::endl;
    auto mapEntry = measuringMap.find(rcvmsgnumber);
    if (mapEntry != measuringMap.end()) {
        std::cout<<"Received ID: " << rcvmsgnumber << " Time elapsed is: "
                 << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::high_resolution_clock::now() - mapEntry->second).count()<< std::endl;
        double time_passed=double(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - mapEntry->second).count() );
        // here is my Mymap is the Map containner
        Mymap.emplace(rcvmsgnumber,time_passed);

        measuringMap.erase(rcvmsgnumber);
    }
    else
    {
        std::cerr << "That is Strange!" << std::endl;
    }

}
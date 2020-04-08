#include "../../include/benchmark/BenchmarkFlatbuffers.h"

namespace capnzero {

    bool hasReceivedFlatbuffers;
    int messagesReceivedFlatbuffers = 0;
    std::chrono::high_resolution_clock::time_point endFlatbuffers;

    void callback(const capnzero::MessageFlatbuffers& msg)
    {
        auto id = msg.id()->c_str();
        long status = msg.status();
        auto states = msg.states();
        auto messageInfo = msg.messageInfo()->c_str();
        endFlatbuffers = std::chrono::high_resolution_clock::now();
        hasReceivedFlatbuffers = true;
        messagesReceivedFlatbuffers++;
    }

    PublisherFlatbuffers createPublisherFlatbuffers() {
        void *ctx = zmq_ctx_new();
        PublisherFlatbuffers pub = capnzero::PublisherFlatbuffers(ctx, capnzero::Protocol::UDP);
        pub.setDefaultTopic("benchmark");
        pub.addAddress("224.0.0.2:5555");
        return pub;
    }

    SubscriberFlatbuffers* createSubscriberFlatbuffers() {
        void* ctx = zmq_ctx_new();
        capnzero::SubscriberFlatbuffers* sub = new capnzero::SubscriberFlatbuffers(ctx, capnzero::Protocol::UDP);

        sub->setTopic("benchmark");
        sub->addAddress("224.0.0.2:5555");
        sub->subscribe(&callback);

        return sub;
    }

    int BenchmarkFlatbuffers::messageSizeBenchmark(std::string message) {
        //Setup
        int messageSizeInBytes = 0;
        int sendBytes = 0;
        int retries = 0;
        std::chrono::high_resolution_clock::time_point start;
        hasReceivedFlatbuffers = false;

        PublisherFlatbuffers pub = createPublisherFlatbuffers();

        SubscriberFlatbuffers* sub = createSubscriberFlatbuffers();

        //running
        std::cout << "running flatbuffers benchmark with size " << message.size() << std::endl;

        do {
            start = std::chrono::high_resolution_clock::now();

            flatbuffers::FlatBufferBuilder msgBuilder;
            auto id = msgBuilder.CreateString("uuid-1");
            auto status = 1234;
            auto messageInfo = msgBuilder.CreateString(message);

            std::vector<long> vec = {1561,152116,151616,7848};
            auto states = msgBuilder.CreateVector(vec);

            auto msg = CreateMessageFlatbuffers(msgBuilder, id, status, states, messageInfo);
            msgBuilder.Finish(msg);

            messageSizeInBytes = pub.send(msgBuilder);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        } while (!hasReceivedFlatbuffers);

        auto time = std::chrono::duration_cast<std::chrono::microseconds>(endFlatbuffers - start).count();
        std::cout << "time: " << time << "us" << std::endl;
        std::cout << "retries: " << retries << std::endl;

        std::cout << "Message size: " << messageSizeInBytes << " bytes\n" <<  std::endl;
        delete sub;

        return messageSizeInBytes;
    }

    std::string BenchmarkFlatbuffers::maxMessageRateBenchmark(std::string message, int runs, long nsBetweenMessages) {
        int messagesSend = 0;
        int messageSize = 0;

        std::cout << "max message rate flatbuffers benchmark with " << nsBetweenMessages << "ns and " << runs << " runs" << std::endl;
        PublisherFlatbuffers pub = createPublisherFlatbuffers();
        SubscriberFlatbuffers* sub = createSubscriberFlatbuffers();

        messagesReceivedFlatbuffers = 0;

        while (runs > messagesSend) {
            flatbuffers::FlatBufferBuilder msgBuilder;
            auto id = msgBuilder.CreateString("uuid-1234");
            auto status = 1234;
            auto messageInfo = msgBuilder.CreateString(message);

            std::vector<long> vec = {1561,152116,151616,7848};
            auto states = msgBuilder.CreateVector(vec);

            auto msg = CreateMessageFlatbuffers(msgBuilder, id, status, states, messageInfo);
            msgBuilder.Finish(msg);
            messageSize = pub.send(msgBuilder);
            messagesSend++;
            std::this_thread::sleep_for(std::chrono::nanoseconds(nsBetweenMessages));
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "messages send: " << messagesSend << std::endl;
        std::cout << "messages received: " << messagesReceivedFlatbuffers << std::endl;

        std::stringstream ss;
        ss << "\t\tns between messages: " << nsBetweenMessages;
        ss << "\n\t\t\tmessage size in bytes: " << messageSize << "\n";
        ss << "\t\t\tsend: " << messagesSend << "\n";
        ss << "\t\t\treceived: " << messagesReceivedFlatbuffers << "\n";
        delete sub;

        return ss.str();
    }
}

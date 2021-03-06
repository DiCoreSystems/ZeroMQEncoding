#include "../../include/benchmark/BenchmarkProtobuf.h"

namespace capnzero {

    bool hasReceivedProtobuf;
    int messagesReceivedProtobuf = 0;
    std::chrono::high_resolution_clock::time_point endProtobuf;

    void callback(capnzero::MessageProtobuf* msg) {
        std::string id = msg->id();
        long int status = msg->status();
        google::protobuf::RepeatedField<long int> states = msg->states();
        std::string messageInfo = msg->messageinfo();
        endProtobuf = std::chrono::high_resolution_clock::now();
        messagesReceivedProtobuf++;
        hasReceivedProtobuf = true;
    }

    capnzero::PublisherProtobuf createPublisherProtobuf() {
        void *ctx = zmq_ctx_new();
        PublisherProtobuf pub = capnzero::PublisherProtobuf(ctx, capnzero::Protocol::UDP);
        pub.setDefaultTopic("benchmark");
        pub.addAddress("224.0.0.2:5555");
        return pub;
    }

    capnzero::SubscriberProtobuf *createSubscriberProtobuf() {
        void *ctx = zmq_ctx_new();
        capnzero::SubscriberProtobuf *sub = new capnzero::SubscriberProtobuf(ctx, capnzero::Protocol::UDP);

        sub->setTopic("benchmark");
        sub->addAddress("224.0.0.2:5555");
        sub->subscribe(&callback);

        return sub;
    }

    json BenchmarkProtobuf::messageSizeBenchmark(std::string message) {
        //Setup
        int messageSizeInBytes = 0;
        int sendBytes = 0;
        int retries = 0;
        std::chrono::high_resolution_clock::time_point start;
        hasReceivedProtobuf = false;

        capnzero::PublisherProtobuf pub = createPublisherProtobuf();

        capnzero::SubscriberProtobuf *sub = createSubscriberProtobuf();

        //running
        std::cout << "running protobuf benchmark with size " << message.size() << std::endl;

        void *ctx = zmq_ctx_new();

        do {
            start = std::chrono::high_resolution_clock::now();

            capnzero::MessageProtobuf msgBuilder;
            msgBuilder.set_id("uuid-1234");
            msgBuilder.set_status(12151);
            msgBuilder.add_states(245124);
            msgBuilder.add_states(42591865196);
            msgBuilder.add_states(17589169814);
            msgBuilder.set_messageinfo(message);

            messageSizeInBytes = pub.send(msgBuilder);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        } while (!hasReceivedProtobuf);

        auto time = std::chrono::duration_cast<std::chrono::microseconds>(endProtobuf - start).count();

        std::cout << "time: " << time << "us" << std::endl;
        std::cout << "retries: " << retries << std::endl;
        std::cout << "Message size: " << messageSizeInBytes << " bytes\n" << std::endl;

        delete sub;

        json result;
        result["size_in_bytes"] = messageSizeInBytes;
        result["time_in_us"] = time;
        result["retries"] = retries;
        result["time_adjusted"] = (time - (1000000 * retries));

        return result;
    }

    json BenchmarkProtobuf::maxMessageRateBenchmark(std::string message, int runs, long nsBetweenMessages) {
        int messagesSend = 0;
        int messageSize = 0;
        std::cout << "max message rate protobuf benchmark with " << nsBetweenMessages << "ns and " << runs << " runs" << std::endl;
        capnzero::PublisherProtobuf pub = createPublisherProtobuf();
        capnzero::SubscriberProtobuf *sub = createSubscriberProtobuf();

        messagesReceivedProtobuf = 0;
        while (runs > messagesSend) {
            capnzero::MessageProtobuf msgBuilder;
            msgBuilder.set_id("uuid-1234");
            msgBuilder.set_status(12151);
            msgBuilder.add_states(245124);
            msgBuilder.add_states(42591865196);
            msgBuilder.add_states(17589169814);
            msgBuilder.set_messageinfo(message);
            messageSize = pub.send(msgBuilder);
            messagesSend++;
            std::this_thread::sleep_for(std::chrono::nanoseconds(nsBetweenMessages));
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "messages send: " << messagesSend << std::endl;
        std::cout << "messages received: " << messagesReceivedProtobuf << std::endl;

        delete sub;

        json result;
        result["ns_between_messages"] = nsBetweenMessages;
        result["size_in_bytes"] = messageSize;
        result["messages_send"] = messagesSend;
        result["messages_received"] = messagesReceivedProtobuf;

        return result;
    }

    json BenchmarkProtobuf::encodeDecodeBenchmark(std::string message, int runs) {
        std::cout << "encode decode benchmark protobuf with " << runs << " encodes / decodes and size " << message.size() << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < runs; i++) {
            /*
             * Encode message
             */
            capnzero::MessageProtobuf msgBuilder;
            msgBuilder.set_id("uuid-1234");
            msgBuilder.set_status(12151);
            msgBuilder.add_states(245124);
            msgBuilder.add_states(42591865196);
            msgBuilder.add_states(17589169814);
            msgBuilder.set_messageinfo(message);

        }

        auto end = std::chrono::high_resolution_clock::now();
        auto timeEncoding = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "encode " << runs << " messages: " << timeEncoding << "ms" << std::endl;

        /*
         * setup message to decode
         */
        capnzero::MessageProtobuf msgBuilder;
        msgBuilder.set_id("uuid-1234");
        msgBuilder.set_status(12151);
        msgBuilder.add_states(245124);
        msgBuilder.add_states(42591865196);
        msgBuilder.add_states(17589169814);
        msgBuilder.set_messageinfo(message);

        zmq_msg_t msg;
        void* msg_ser = malloc(msgBuilder.ByteSizeLong());
        msgBuilder.SerializeToArray(msg_ser, msgBuilder.ByteSizeLong());
        zmq_msg_init_data(&msg, msg_ser, msgBuilder.ByteSizeLong(), nullptr, NULL);

        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < runs; i++) {
            /*
             * Decode message
             */
            void* data = zmq_msg_data(&msg);
            size_t len = zmq_msg_size(&msg);
            MessageProtobuf messageProtobuf;
            messageProtobuf.ParseFromArray(data, len);

            std::string id = messageProtobuf.id();
            long int status = messageProtobuf.status();
            google::protobuf::RepeatedField<long int> states = messageProtobuf.states();
            std::string messageInfo = messageProtobuf.messageinfo();
        }

        end = std::chrono::high_resolution_clock::now();
        auto timeDecoding = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        free(msg_ser);

        std::cout << "decode " << runs << " messages: " << timeDecoding << "ms\n" << std::endl;

        json result;
        result["size"] = message.size();
        result["runs"] = runs;
        result["time_encoding_ms"] = timeEncoding;
        result["time_decoding_ms"] = timeDecoding;
        return result;
    }
}

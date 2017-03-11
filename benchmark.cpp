#include <string>
#include <set>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <sstream>
#include <sstream>


#include <capnp/message.h>
#include <capnp/serialize.h>

#include "protobuf/record.pb.h"
#include "capnproto/record.capnp.h"
#include "flatbuffers/record_generated.h"
#include "msgpack/record.hpp"

#include "data.hpp"

// http://stackoverflow.com/questions/194465/how-to-parse-a-string-to-an-int-in-c/6154614#6154614
int str2int (const std::string &str) {
  std::stringstream ss(str);
  int num;
  if((ss >> num).fail())
  {
      //ERROR
  }
  return num;
}

void
protobuf_serialization_record(size_t iterations)
{
    using namespace protobuf_record;

    Record r1;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.add_ids(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.add_strings(kStringValue);
    }

    std::string serialized;

    r1.SerializeToString(&serialized);

    // check if we can deserialize back
    Record r2;
    bool ok = r2.ParseFromString(serialized);
    if (!ok /*|| r2 != r1*/) {
        throw std::logic_error("protobuf's case: deserialization failed");
    }

    std::cout << "protobuf: version = " << GOOGLE_PROTOBUF_VERSION << std::endl;
    std::cout << "protobuf: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized.clear();
        r1.SerializeToString(&serialized);
        r2.ParseFromString(serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "protobuf: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
capnproto_serialization_record(size_t iterations)
{
    using namespace capnp_record;

    capnp::MallocMessageBuilder message;
    Record::Builder r1 = message.getRoot<Record>();

    auto ids = r1.initIds(kIntegers.size());
    for (size_t i = 0; i < kIntegers.size(); i++) {
        ids.set(i, kIntegers[i]);
    }

    auto strings = r1.initStrings(kStringsCount);
    for (size_t i = 0; i < kStringsCount; i++) {
        strings.set(i, kStringValue);
    }

    kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> serialized =
        message.getSegmentsForOutput();

    // check if we can deserialize back
    capnp::SegmentArrayMessageReader reader(serialized);
    Record::Reader r2 = reader.getRoot<Record>();
    if (r2.getIds().size() != kIntegers.size()) {
        throw std::logic_error("capnproto's case: deserialization failed");
    }

    size_t size = 0;
    for (auto segment: serialized) {
      size += segment.asBytes().size();
    }

    std::cout << "capnproto: version = " << CAPNP_VERSION << std::endl;
    std::cout << "capnproto: size = " << size << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        capnp::MallocMessageBuilder message;
        Record::Builder r1 = message.getRoot<Record>();

        auto ids = r1.initIds(kIntegers.size());
        for (size_t i = 0; i < kIntegers.size(); i++) {
            ids.set(i, kIntegers[i]);
        }

        auto strings = r1.initStrings(kStringsCount);
        for (size_t i = 0; i < kStringsCount; i++) {
            strings.set(i, kStringValue);
        }

        serialized = message.getSegmentsForOutput();
        capnp::SegmentArrayMessageReader reader(serialized);
        auto r2 = reader.getRoot<Record>();

        (void)r2.getIds().size();
        (void)r2.getStrings().size();
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "capnproto: time = " << duration << " milliseconds" << std::endl << std::endl;
}


void
flatbuffers_serialization_record(size_t iterations)
{
    using namespace flatbuffers_record;

    std::vector<flatbuffers::Offset<flatbuffers::String>> strings;
    strings.reserve(kStringsCount);

    flatbuffers::FlatBufferBuilder builder;
    for (size_t i = 0; i < kStringsCount; i++) {
        strings.push_back(builder.CreateString(kStringValue));
    }

    auto ids_vec = builder.CreateVector(kIntegers);
    auto strings_vec = builder.CreateVector(strings);
    auto r1 = CreateRecord(builder, ids_vec, strings_vec);

    builder.Finish(r1);

    auto p = reinterpret_cast<char*>(builder.GetBufferPointer());
    auto sz = builder.GetSize();
    std::vector<char> buf(p, p + sz);

    auto r2 = GetRecord(buf.data());
    if (r2->strings()->size() != kStringsCount || r2->ids()->size() != kIntegers.size()) {
        throw std::logic_error("flatbuffer's case: deserialization failed");
    }

    std::cout << "flatbuffers: size = " << builder.GetSize() << " bytes" << std::endl;

    builder.ReleaseBufferPointer();

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        builder.Clear();
        strings.clear();
        // buf.clear();

        for (size_t i = 0; i < kStringsCount; i++) {
            strings.push_back(builder.CreateString(kStringValue));
        }

        auto ids_vec = builder.CreateVector(kIntegers);
        auto strings_vec = builder.CreateVector(strings);
        auto r1 = CreateRecord(builder, ids_vec, strings_vec);
        builder.Finish(r1);

        auto p = reinterpret_cast<char*>(builder.GetBufferPointer());
        auto sz = builder.GetSize();
        std::vector<char> buf(p, p + sz);
        auto r2 = GetRecord(buf.data());
        (void)r2->ids()[0];

        builder.ReleaseBufferPointer();
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "flatbuffers: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
msgpack_serialization_record(size_t iterations)
{
    using namespace msgpack_record;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    msgpack::sbuffer sbuf;

    msgpack::pack(sbuf, r1);

    std::string serialized(sbuf.data(), sbuf.size());

    msgpack::object_handle msg = msgpack::unpack(serialized.data(), serialized.size());

    msgpack::object obj = msg.get();

    obj.convert(r2);

    if (r1 != r2) {
        throw std::logic_error("msgpack's case: deserialization failed");
    }

    std::cout << "msgpack: version = " << msgpack_version() << std::endl;
    std::cout << "msgpack: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        sbuf.clear();
        msgpack::pack(sbuf, r1);
        msgpack::object_handle msg = msgpack::unpack(sbuf.data(), sbuf.size());
        msgpack::object obj = msg.get();
        obj.convert(r2);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "msgpack: time = " << duration << " milliseconds" << std::endl << std::endl;
}

int
main(int argc, char **argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " N [thrift-binary thrift-compact protobuf boost msgpack cereal avro capnproto flatbuffers]";
        std::cout << std::endl << std::endl;
        std::cout << "arguments: " << std::endl;
        std::cout << " N  -- number of iterations" << std::endl << std::endl;
        return EXIT_SUCCESS;
    }

    size_t iterations;

    try {
        iterations= str2int(argv[1]);
        iterations = 100000; // boost::lexical_cast<size_t>(argv[1]);
    } catch (std::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        std::cerr << "First positional argument must be an integer." << std::endl;
        return EXIT_FAILURE;
    }

    std::set<std::string> names;

    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            names.insert(argv[i]);
        }
    }

    std::cout << "performing " << iterations << " iterations" << std::endl << std::endl;

    /*std::cout << "total size: " << sizeof(kIntegerValue) * kIntegersCount + kStringValue.size() * kStringsCount << std::endl;*/

    try {

        bool runall = names.empty() ? true : false;

        if (runall || names.find("protobuf") != names.end()) {
            protobuf_serialization_record(iterations);
        }

        if (runall || names.find("capnproto") != names.end()) {
            capnproto_serialization_record(iterations);
        }

        if (runall || names.find("msgpack") != names.end()) {
            msgpack_serialization_record(iterations);
        }

        if (runall || names.find("flatbuffers") != names.end()) {
            flatbuffers_serialization_record(iterations);
        }

    } catch (std::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return EXIT_FAILURE;
    }

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}

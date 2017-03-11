# Serialization Benchmarks

This benchmark is a truncated and modified fork from thekvs's cpp-serializer.

It's only focusing on capnproto, flatbuffers and protobuf now, but not restricted to c++.


## How to build

Thanks thekvs, this project does not have any external library dependencies. They are downloaded
and built automatically.

```
$ git clone https://github.com/piaoger/serializtion-benchmarks.git
$ cd /path/to/serializtion-benchmarks
$ cmake -DCMAKE_BUILD_TYPE=Release
$ make
```

## How to use

run benchmark ..

```
$ #run all cases with iteration == 10000
$ ./benchmark 10000
$ #run some cases with iteration == 10000
$ ./benchmark 100000 protobuf flatbuffeers capnproto
```


## Links

- [capnproto](https://github.com/sandstorm-io/capnproto)

- [flatbuffers](https://github.com/google/flatbuffers)

- [Protobuf](https://code.google.com/p/protobuf/)

- [Msgpack](http://msgpack.org/)

- [rust serialization benchmark](https://github.com/erickt/rust-serialization-benchmarks)

- [thekvs cpp-serializer](https://github.com/thekvs/cpp-serializers)




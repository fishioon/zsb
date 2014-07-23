g++ -g sbclient.cpp sample.pb.cc -lzmq -lczmq -lprotobuf -o ../bin/sbclient
g++ -g sbconfig.cpp sandboxd.cpp sbqueue.cpp sbworker.cpp sample.pb.cc -I../include -L../libs -liniparser -lglog -lzmq -lczmq -lprotobuf -lpthread -o ../bin/sandboxd

PROTOBUF_GENERATE_CPP(PROTO_SRCS
                      PROTO_HDRS
		      ProtocolMessage.proto
                      ModuleMessage.proto
                      ClockSynchronizer.proto
                      LoadBalancing.proto
                      GroupManagement.proto
                      StateCollection.proto
                      VoltVarCtrl.proto
                      DispatchAlgo.proto)

add_library(messages ${PROTO_SRCS} ${PROTO_HDRS})

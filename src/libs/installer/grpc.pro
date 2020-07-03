win32: {
    equals(QMAKE_HOST.arch, x86_64) {
        GRPC_PATH = $$PWD/../extern/grpc-1.12.0-2/x64-windows
    } else {
        GRPC_PATH = $$PWD/../extern/grpc-1.12.0-2/x86-windows
    }

    DEFINES +=_WIN32_WINNT=0X601

    INCLUDEPATH += $$GRPC_PATH/include
    CONFIG(release, debug|release):  LIBS += -L$$GRPC_PATH/lib/ -llibprotobuf -lgrpc -lgrpc++ -lgpr -lzlib -lws2_32 -llibeay32 -lssleay32
    else:CONFIG(debug, debug|release): LIBS += -L$$GRPC_PATH/debug/lib/ -llibprotobufd -lgrpc -lgrpc++ -lgpr -lzlibd -lws2_32 -llibeay32 -lssleay32

    QMAKE_CXXFLAGS_WARN_ON += /wd4251 /wd4100
}

osx: {
    GRPC_PATH = $$PWD/../extern/grpc-1.12.0-2/x64-osx
    INCLUDEPATH += $$GRPC_PATH/include
    CONFIG(release, debug|release):  LIBS += -L$$GRPC_PATH/lib/ -lprotobuf -lgrpc -lgrpc++ -lgpr -lz -lcares -lcrypto -lssl -laddress_sorting
    else:CONFIG(debug, debug|release): LIBS += -L$$GRPC_PATH/debug/lib/ -lprotobufd -lgrpc -lgrpc++ -lgpr -lz -lcares -lcrypto -lssl -laddress_sorting

    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

linux: {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib/ -lprotobuf -lgrpc -lgrpc++ -lgpr -lz -lcares -lcrypto -lssl -laddress_sorting
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}


JAVA_INCLUDE=/usr/lib/jvm/java-1.7.0-openjdk-i386

all:
	gcc -I$(JAVA_INCLUDE)/include/linux/ -I$(JAVA_INCLUDE)/include/ -I./ -fPIC -shared -o libHKPacketJni.so *.c


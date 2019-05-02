2017-4-16
启动脚本：参考serviced
/etc/systemd/system/multi-user.target.wants/phytec-hmiservice.service
---------------------------------------------------------
[Unit]

[Service]
Type=simple
ExecStart=/home/root/hmiservice

# Detaching the framebuffer console from fb0 crashes for am335x ti kernel
# We need to live with the workaround for now
ExecStartPre=-/bin/sh -c "echo 0 > /sys/class/graphics/fbcon/cursor_blink"
ExecStopPost=-/bin/sh -c "systemctl restart getty@tty1"

[Install]
WantedBy=multi-user.target
-----------------------------------------------------------
#/usr/bin/qtLauncher /home/root/hmiclient
cc -g -Os -Wall -fvisibility=hidden -Ibuild -Wno-deprecated-declarations -DOSX -c src/Clients.c src/Heap.c src/LinkedList.c src/Log.c src/MQTTClient.c src/MQTTPacket.c src/MQTTPacketOut.c src/MQTTPersistence.c src/MQTTPersistenceDefault.c src/MQTTProtocolClient.c src/MQTTProtocolOut.c src/Messages.c src/Socket.c src/SocketBuffer.c src/StackTrace.c src/Thread.c src/Tree.c src/utf-8.c
ar rcs libpaho-mqtt3c.a *.o

//sudo svnserve -d -r /erv/svn/

https://sourceforge.net/projects/wqy/files/
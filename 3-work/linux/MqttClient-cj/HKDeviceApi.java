package com.cj.mqtt;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.log4j.PropertyConfigurator;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.cj.mqtt.HKPacket.HKPacketDataHead;
import com.cj.mqtt.HKPacket.HKPacketMsg;
import com.cj.mqtt.HKPacket.HKPacketToDeviceHead;
import com.google.gson.JsonObject;

public class HKDeviceApi {
	static final String MQTT_URL = "tcp://192.168.205.225:1883";
	MqttClient mqttClient = null;
	boolean connectStatus = false;
	boolean isEncodeAes = true;

	private static Logger logger = LoggerFactory.getLogger(HKDeviceApi.class);

	public static class HKDeviceInfo {
		String deviceId;
		String key;
		String iv;

		public HKDeviceInfo() {
			super();
		}

		public HKDeviceInfo(String deviceId, String key, String iv) {
			super();
			this.deviceId = deviceId;
			this.key = key;
			this.iv = iv;
		}
	}

	Map<String, HKDeviceInfo> deviceMap = new ConcurrentHashMap<>();
	HKDeviceApiCallback hkCallback;

	static enum HKAPIStatus {
		MQTTConnectStart, MQTTConnectSuccess, MQTTConnectClose
	}

	interface HKDeviceApiCallback {
		void onStatusChange(HKAPIStatus status);

		/**
		 * 消息接收
		 * 
		 * @param deviceId
		 * @param packet
		 */
		void onDeviceMessageArrived(String deviceId, HKPacket packet);

	}

	public void addDeviceId(String deviceId, String key, String iv) {
		HKDeviceInfo deviceInfo = new HKDeviceInfo(deviceId, key, iv);
		deviceMap.put(deviceId, deviceInfo);
		if (connectStatus) { // 需要重新订阅
			mqttSubTopic(deviceInfo);
		}
	}

	public void setCallback(HKDeviceApiCallback callback) {
		this.hkCallback = callback;
	}

	MqttCallback callback = new MqttCallback() {

		public void messageArrived(String topic, MqttMessage msg) throws Exception {
			String deviceId = null;
			/// System.out.println("收到:" + topic + "," +
			/// HKPacket.bytesToHexString(msg.getPayload(), " "));

			if (topic.startsWith("HK/app/")) {
				String[] split = topic.split("/");
				deviceId = split[2];
				HKDeviceInfo hkDeviceInfo = deviceMap.get(deviceId);
				if (hkDeviceInfo == null) {
					System.out.println("未知设备消息");
					return;
				}
				try {
					HKPacket packet = HKPacket.decodeFromDevice(msg.getPayload(), hkDeviceInfo.key, hkDeviceInfo.iv);
					hkCallback.onDeviceMessageArrived(deviceId, packet);
				} catch (Exception ex) {
					ex.printStackTrace();
				}
			}

		}

		public void deliveryComplete(IMqttDeliveryToken arg0) {
			System.out.println("deliveryComplete:" + arg0.getMessageId());
		}

		public void connectionLost(Throwable arg0) {
			hkCallback.onStatusChange(HKAPIStatus.MQTTConnectClose);

			System.out.println("mqtt restart connect");
			onStart();
		}
	};

	private void mqttSubTopic(HKDeviceInfo deviceInfo) {
		String topic = "HK/app/" + deviceInfo.deviceId;
		try {
			mqttClient.subscribe(topic, 1);
		} catch (MqttException e) {
			e.printStackTrace();
		}
	}

	public void onStart() {
		new Thread(new Runnable() {

			boolean connectMqtt() {
				MemoryPersistence persistence = new MemoryPersistence();
				String clientId = "HK-1234" + System.currentTimeMillis();
				connectStatus = false;

				hkCallback.onStatusChange(HKAPIStatus.MQTTConnectStart);

				try {
					mqttClient = new MqttClient(MQTT_URL, clientId, persistence);
				} catch (MqttException e) {
					e.printStackTrace();
				}
				MqttConnectOptions connOpts = new MqttConnectOptions();
				connOpts.setCleanSession(true);
				connOpts.setKeepAliveInterval(20);
				try {
					mqttClient.connect(connOpts);
					connectStatus = true;
					hkCallback.onStatusChange(HKAPIStatus.MQTTConnectSuccess);
				} catch (MqttSecurityException e) {
					e.printStackTrace();
				} catch (MqttException e) {
					e.printStackTrace();
				}
				return connectStatus;
			}

			@Override
			public void run() {
				while (false == connectMqtt()) {
					System.out.println("reset connect to mqtt");
					try {
						Thread.sleep(1000 * 2);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
				mqttClient.setCallback(callback);
				// 订阅
				try {
					Set<String> keySet = deviceMap.keySet();
					int size = keySet.size();

					if (size > 0) {
						String[] topics = new String[size];
						int[] qos = new int[size];
						int i = 0;
						for (String deviceId : keySet) {
							topics[i] = "HK/app/" + deviceId;
							qos[i] = 1;
							i++;
						}
						mqttClient.subscribe(topics, qos);
						System.out.println("topic sub:" + Arrays.toString(topics));
					}
				} catch (MqttException e) {
					e.printStackTrace();
				}

			}
		}).start();
	}

	public void onStop() {
		if (connectStatus == false)
			return;
		if (mqttClient != null) {
			try {
				mqttClient.disconnect();
				mqttClient.close();
			} catch (MqttException e) {
				e.printStackTrace();
			}
		}
		connectStatus = false;
		hkCallback.onStatusChange(HKAPIStatus.MQTTConnectClose);
	}

	public void onSendData(String deviceId, byte[] data) throws Exception {
		String topic = "HK/" + deviceId;
		System.out.println("send data(" + topic + "):" + HKPacket.bytesToHexString(data, " "));

		if (connectStatus == true && mqttClient != null) {
			mqttClient.publish(topic, data, 1, false);
		} else {
			throw new IOException("not connect to Server");
		}
	}

	static int seq = 0;

	public void onSendJsonMsg(String deviceId, int funId, int fun, String jsonString) throws Exception {
		HKPacketToDeviceHead h1 = new HKPacketToDeviceHead();
		HKPacketDataHead dataHead = new HKPacketDataHead();
		HKPacketMsg msg = new HKPacketMsg();

		h1.setUserid("myisubuntu");

		dataHead.setSeq(seq++);
		dataHead.setFunName(funId);
		dataHead.setMsgDir(0);
		dataHead.setFunType(1);

		msg.setFun(fun);
		msg.setFormat(HKPacketMsg.FORMAT_JSON);
		if (jsonString != null)
			msg.setMessage(jsonString.getBytes());
		byte[] data = null;

		if (isEncodeAes) {
			HKDeviceInfo hkDeviceInfo = deviceMap.get(deviceId);
			data = HKPacket.encodeAes(h1, dataHead, msg, hkDeviceInfo.key, hkDeviceInfo.iv);
		} else
			data = HKPacket.encode(h1, dataHead, msg);

		System.out.println(String.format(" seq:%d,FunID:%02X-%02X:", dataHead.getSeq(), funId, fun) + jsonString);
		onSendData(deviceId, data);
	}

	public static List<String> onLenSearch() {
		int timeOutSeconds = 3;
		DatagramSocket socket = null;
		List<String> snList = new ArrayList<String>();
		Thread.currentThread().setName("SerchThread.");
		try {
			socket = new DatagramSocket();
			String data = "HK/2";
			byte[] bytes = data.getBytes();
			socket.setBroadcast(true);

			DatagramPacket packSend = new DatagramPacket(bytes, bytes.length,
					new InetSocketAddress("255.255.255.255", 10516));
			socket.send(packSend);

			socket.setSoTimeout(timeOutSeconds * 1000);
			long currTime = System.currentTimeMillis();

			while (System.currentTimeMillis() - currTime < (3 * 1000)) {
				byte[] rData = new byte[1024];
				DatagramPacket packRecv = new DatagramPacket(rData, rData.length);
				socket.receive(packRecv);
				String str = new String(packRecv.getData(), 0, packRecv.getLength());
				String[] strings = str.split("/");

				if (strings != null) {
					String sn = strings[strings.length - 1];
					sn += "/" + packRecv.getAddress().getHostAddress();
					snList.add(sn);
				}
			}
		} catch (Exception e) {
			// e.printStackTrace();
		} finally {
			try {
				if (socket != null)
					socket.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return snList;
	}

	public static void addLibraryDir(String libraryPath) throws IOException {
		try {
			Field field = ClassLoader.class.getDeclaredField("usr_paths");
			field.setAccessible(true);
			String[] paths = (String[]) field.get(null);
			for (int i = 0; i < paths.length; i++) {
				if (libraryPath.equals(paths[i])) {
					return;
				}
			}

			String[] tmp = new String[paths.length + 1];
			System.arraycopy(paths, 0, tmp, 0, paths.length);
			tmp[paths.length] = libraryPath;
			field.set(null, tmp);
		} catch (IllegalAccessException e) {
			throw new IOException("Failedto get permissions to set library path");
		} catch (NoSuchFieldException e) {
			throw new IOException("Failedto get field handle to set library path");
		}
	}

	public static void main(String[] args) {
		// List<String> sns = onLenSearch();
		// for (String sn : sns) {
		// System.out.println(sn);
		// }
		System.setProperty("java.library.path", "/opt/cjmqtt-service/src/test/java");

		try {
			addLibraryDir("/opt/cjmqtt-service/src/test/java");
		} catch (IOException e1) {
			e1.printStackTrace();
		}

		System.out.println(System.getenv("LD_LIBRARY_PATH"));
		System.out.println(System.getProperty("java.library.path"));

		String workpath = System.getProperty("user.dir");

		// + # <?>
		// 1 HK/aa/cc/bb
		// 2 HK/aa/#
		// 3 HK/aa/dd/bb
		// 4 HK/+/cc
		// 5 HK/33/cc
		// k v : Map<String:Topic, List<Client:Mqtt>>
		// 主题树搜索算法： # +

		// log4j config init
		try {
			PropertyConfigurator.configure(new File(workpath, "log4j.properties").getCanonicalPath());
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		logger.info("start");
		final HKMessageManagent hkMessageManagent = new HKMessageManagent();
		final Object wait = new Object();
		HKDeviceApi api = new HKDeviceApi();
		api.setCallback(new HKDeviceApiCallback() {
			@Override
			public void onStatusChange(HKAPIStatus status) {
				System.out.println(status);
				if (status == HKAPIStatus.MQTTConnectSuccess) {
					synchronized (wait) {
						wait.notifyAll();
					}
					// 这个时候才能发送命令
				}
			}

			@Override
			public void onDeviceMessageArrived(String deviceId, HKPacket packet) {
				// System.out.println(packet.toString());
				hkMessageManagent.devicePacketDispatcher(deviceId, packet);
			}

		});

		String deviceId = "40539f6c58fdebe7ba5d";

		api.addDeviceId(deviceId, "5a81d32db4bcd731", "4f6676404d1a8c6c");
		api.onStart();

		synchronized (wait) {
			try {
				wait.wait(1000 * 5); // 5s
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

		{
			String jsonString = "";
			JsonObject json = new JsonObject();
			jsonString = json.toString();
			try {
				hkMessageManagent.registerDevicePacketReceiver(deviceId, 0x41, 0x11, new HKDevicePacketReceiver() {

					@Override
					public void onReceive(String deviceId, HKPacket packet) {
						logger.info(">>>> my is echo :" + deviceId + "," + packet.toString());

						hkMessageManagent.unregisterDevicePacketReceiver(this);
					}
				});

				api.onSendJsonMsg(deviceId, 0x41, 0x11, jsonString);

			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		{
			String jsonString = "";
			JsonObject json = new JsonObject();
			json.addProperty("sid", "c2981e815d4441639a9cfdfe79473916");
			jsonString = json.toString();
			try {
				api.onSendJsonMsg(deviceId, 0x41, 0x13, jsonString);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		// api.onStop();
	}

	public interface HKDevicePacketReceiver {
		public void onReceive(String deviceId, HKPacket packet);
	}

	public static class HKMessageManagent {
		private class HKDevicePacketReceiverKey {
			String key;
			HKDevicePacketReceiver receiver;
		}

		private List<HKDevicePacketReceiverKey> devicePacketReceivers = new ArrayList<HKDevicePacketReceiverKey>();

		public HKDevicePacketReceiver registerDevicePacketReceiver(String deviceId, int funId, int fun,
				HKDevicePacketReceiver receiver) {
			HKDevicePacketReceiverKey key = new HKDevicePacketReceiverKey();
			key.key = deviceId + "/" + funId + "/" + fun;
			key.receiver = receiver;
			devicePacketReceivers.add(key);
			return receiver;
		}

		public void unregisterDevicePacketReceiver(String deviceId, int funId, int fun) {
			String key = deviceId + "/" + funId + "/" + fun;
			synchronized (devicePacketReceivers) {
				Iterator<HKDevicePacketReceiverKey> iterator = devicePacketReceivers.iterator();
				while (iterator.hasNext()) {
					HKDevicePacketReceiverKey rkey = iterator.next();
					if (rkey.key.equals(key)) {
						iterator.remove();
					}
				}
			}
		}

		public void unregisterDevicePacketReceiver(HKDevicePacketReceiver receiver) {
			synchronized (devicePacketReceivers) {
				Iterator<HKDevicePacketReceiverKey> iterator = devicePacketReceivers.iterator();
				while (iterator.hasNext()) {
					HKDevicePacketReceiverKey next = iterator.next();
					if (next.receiver == receiver) {
						iterator.remove();
					}
				}
			}
		}

		public void devicePacketDispatcher(String deviceId, HKPacket packet) {
			String key = deviceId + "/" + packet.getDataHead().getFunName() + "/" + packet.getMsg().getFun();
			List<HKDevicePacketReceiver> callList = new ArrayList<>();

			synchronized (devicePacketReceivers) {
				for (HKDevicePacketReceiverKey rkey : devicePacketReceivers) {
					if (rkey.key.equals(key)) {
						// 异步执行
						callList.add(rkey.receiver);
					}
				}
			}

			for (HKDevicePacketReceiver hkDevicePacketReceiver : callList) {
				hkDevicePacketReceiver.onReceive(deviceId, packet);
			}
		}
	}
}

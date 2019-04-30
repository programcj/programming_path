package com.cj.mqtt;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Locale;

public class HKPacket {

	private HKPacketBase base;
	private HKPacketDataHead dataHead;
	private HKPacketMsg msg;

	public HKPacketBase getBase() {
		return base;
	}

	public void setBase(HKPacketBase base) {
		this.base = base;
	}

	public HKPacketDataHead getDataHead() {
		return dataHead;
	}

	public void setDataHead(HKPacketDataHead dataHead) {
		this.dataHead = dataHead;
	}

	public HKPacketMsg getMsg() {
		return msg;
	}

	public void setMsg(HKPacketMsg msg) {
		this.msg = msg;
	}

	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("version:").append(base.getVersion()).append(
				String.format(",Fun(%02X-%02X,%d)", 0x00FF & dataHead.funName, 0x00FF & msg.fun, 0x00FF & msg.format));
		if (msg.format == msg.FORMAT_JSON) {
			sb.append(msg.getMessageString());
		}
		if (msg.format == msg.FORMAT_BIN) {
			sb.append(HKPacket.bytesToHexString(msg.getMessage(), " "));
		}
		return sb.toString();
	}

	public static abstract class HKPacketBase {
		int version;

		public int getVersion() {
			return version;
		}

		public void setVersion(int version) {
			this.version = version;
		}

		abstract byte[] toBytes();
	}

	/**
	 * 设备上报头
	 * 
	 * @author cj
	 *
	 */
	public static class HKPacketFromDeviceHead extends HKPacketBase {
		public final static int MAX_SIZE = 20 + 3;

		int devType;
		String sn; // 20Bytes

		public int getDevType() {
			return devType;
		}

		public void setDevType(int devType) {
			this.devType = devType;
		}

		public String getSn() {
			return sn;
		}

		public void setSn(String sn) {
			this.sn = sn;
		}

		@Override
		byte[] toBytes() {
			ByteArrayOutputStream outArray = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream(outArray);
			try {
				out.writeByte(version);
				out.writeShort(devType);
				out.writeBytes(sn);
				out.flush();
			} catch (IOException e) {
				e.printStackTrace();
			}
			return outArray.toByteArray();
		}
	}

	/**
	 * 到设备的头
	 * 
	 * @author cj
	 *
	 */
	public static class HKPacketToDeviceHead extends HKPacketBase {
		String userid;

		public String getUserid() {
			return userid;
		}

		public void setUserid(String userid) {
			this.userid = userid;
		}

		@Override
		byte[] toBytes() {
			ByteArrayOutputStream outArray = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream(outArray);
			try {
				out.writeByte(version);
				out.writeByte(userid.length());
				out.writeBytes(userid);
				out.flush();
			} catch (IOException e) {
				e.printStackTrace();
			}
			return outArray.toByteArray();
		}
	}

	/**
	 * 消息內容头
	 * 
	 * @author cj
	 *
	 */
	public static class HKPacketDataHead {
		final static int HEAD_LENGTH = 8;
		int head = 0xEEEE;
		int length;
		int seq;
		int funType; // 功能码类型(H) 1：设备控制; 2：设备上报；3:后台->app 4:后台->设备 5：模块-MCU;
						// 6：后台-模块；其他保留
		int msgDir; // 消息方向(L) 0：发送消息；8：消息返回
		int funName;

		public int getHead() {
			return head;
		}

		public void setHead(int head) {
			this.head = head;
		}

		public int getLength() {
			return length;
		}

		public void setLength(int length) {
			this.length = length;
		}

		public int getSeq() {
			return seq;
		}

		public void setSeq(int seq) {
			this.seq = seq;
		}

		public int getFunType() {
			return funType;
		}

		/**
		 * 功能码类型(H) 1：设备控制; 2：设备上报；3:后台->app 4:后台->设备 5：模块-MCU; 6：后台-模块；其他保留
		 * 
		 * @return
		 */
		public void setFunType(int funType) {
			this.funType = funType;
		}

		public int getMsgDir() {
			return msgDir;
		}

		/**
		 * 消息方向(L) 0：发送消息；8：消息返回
		 * 
		 * @param msgDir
		 */
		public void setMsgDir(int msgDir) {
			this.msgDir = msgDir;
		}

		public int getFunName() {
			return funName;
		}

		public void setFunName(int funName) {
			this.funName = funName;
		}

		public byte[] toBytes() {
			ByteArrayOutputStream outArray = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream(outArray);
			try {
				out.writeShort(head); // 引导头 2
				out.writeShort(length); // 串口数据长度 2
				out.writeByte(seq); // 1Byte 包序号
				out.writeByte(funType << 4 | msgDir);
				out.writeByte(funName); // 1Byte 功能码名称
				out.flush();
			} catch (IOException e) {
				e.printStackTrace();
			}
			return outArray.toByteArray();
		}
	}

	/**
	 * 消息内容 需要加密
	 * 
	 * @author cj
	 *
	 */
	public static class HKPacketMsg {
		public static final int FORMAT_BIN = 0x01;
		public static final int FORMAT_JSON = 0x02;
		private int fun;
		private int format;
		private byte[] message;

		public int getFun() {
			return fun;
		}

		public void setFun(int fun) {
			this.fun = fun;
		}

		public int getFormat() {
			return format;
		}

		/**
		 * 1B 内容格式[json/bin] 0x01 =Bin:二进制，数据不可读，需要对表(默认的) 0x02
		 * =Json:数据可读字符串(字段名称统一小写)
		 * 
		 * @param format
		 */
		public void setFormat(int format) {
			this.format = format;
		}

		public byte[] getMessage() {
			return message;
		}

		public String getMessageString() {
			if (message == null)
				return null;
			return new String(message);
		}

		public void setMessage(byte[] message) {
			this.message = message;
		}

		public byte[] toBytes() {
			ByteArrayOutputStream outArray = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream(outArray);
			try {
				out.writeByte(fun);
				out.writeByte(format);
				if (message != null) {
					out.writeShort(message.length);
					out.write(message);
				} else {
					out.writeShort(0);
				}
				out.flush();
			} catch (IOException e) {
				e.printStackTrace();
			}
			return outArray.toByteArray();
		}

	}

	/**
	 * 使用AES加密(HKPacketMsg)消息
	 * 
	 * @param base
	 * @param dataHead
	 * @param msg
	 * @param key
	 * @param iv
	 * @return
	 */
	public static byte[] encodeAes(HKPacketBase base, HKPacketDataHead dataHead, HKPacketMsg msg, String key,
			String iv) {
		ByteArrayOutputStream outArray = new ByteArrayOutputStream();
		DataOutputStream out = new DataOutputStream(outArray);
		base.setVersion(0x04);

		byte[] baseHead = base.toBytes();
		byte[] srcMsgBytes = msg.toBytes();
		byte[] msgBytes = aesEncode(srcMsgBytes, key, iv);

		dataHead.length = HKPacketDataHead.HEAD_LENGTH + msgBytes.length;
		byte[] dataHeadBytes = dataHead.toBytes();
		byte check = 0;

		for (int i = 0; i < dataHeadBytes.length; i++) {
			check += dataHeadBytes[i];
		}
		for (int i = 0; i < msgBytes.length; i++) {
			check += msgBytes[i];
		}
		check = (byte) ~check;
		check += 1;
		try {
			out.write(baseHead);
			out.write(dataHeadBytes);
			out.write(msgBytes);
			out.write(check);
			out.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}
		return outArray.toByteArray();
	}

	/**
	 * 不使用加密消息
	 * 
	 * @param base
	 * @param dataHead
	 * @param msg
	 * @return
	 */
	public static byte[] encode(HKPacketBase base, HKPacketDataHead dataHead, HKPacketMsg msg) {
		ByteArrayOutputStream outArray = new ByteArrayOutputStream();
		DataOutputStream out = new DataOutputStream(outArray);

		base.setVersion(0x01);

		byte[] baseHead = base.toBytes();
		byte[] msgBytes = msg.toBytes();
		dataHead.length = HKPacketDataHead.HEAD_LENGTH + msgBytes.length;
		byte[] dataHeadBytes = dataHead.toBytes();
		byte check = 0;

		for (int i = 0; i < dataHeadBytes.length; i++) {
			check += dataHeadBytes[i];
		}
		for (int i = 0; i < msgBytes.length; i++) {
			check += msgBytes[i];
		}
		check = (byte) ~check;
		check += 1;
		try {
			out.write(baseHead);
			out.write(dataHeadBytes);
			out.write(msgBytes);
			out.write(check);
			out.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}
		return outArray.toByteArray();
	}

	/**
	 * 解码来自设备端的消息
	 * 
	 * @param payload
	 * @param key
	 * @param iv
	 * @return
	 * @throws IOException
	 */
	public static HKPacket decodeFromDevice(byte[] payload, String key, String iv) throws IOException {
		ByteArrayInputStream inbyte = new ByteArrayInputStream(payload);
		DataInputStream in = new DataInputStream(inbyte);

		HKPacketFromDeviceHead baseHead = new HKPacketFromDeviceHead();
		HKPacketDataHead dataHead = new HKPacketDataHead();
		HKPacketMsg dataMsg = new HKPacketMsg();
		byte checkSum = 0;

		byte[] sn = new byte[20];

		// 1Byte 版本号
		// 2Byte 设备类型
		// 20Byte sn序号(device_id)
		baseHead.setVersion(in.readByte());
		baseHead.setDevType(in.readShort());
		in.read(sn, 0, 20);
		baseHead.setSn(new String(sn));

		// 2Byte 引导头 0xEEEE 固定
		// 2Byte 串口数据长度 0x00-0xFFFF(0-65535) 消息长度: 是整个串口数据长度
		// 1Byte 包序号 描述每次回话的序号，回复包的序号保持与发送包的一致
		// 4bit 功能码类型(H) 1：设备控制; 2：设备上报；3:后台->app 4:后台->设备 5：模块-MCU;
		// 6：后台-模块；其他保留
		// 4bit 消息方向(L) 0：发送消息；8：消息返回
		// 1Byte 功能码名称 由厂商配置(功能码-模块类 : )
		// nByte 消息体 可以为空(参考下面的消息体说明)
		// 1Byte 校验码 Byte0 – ByteN求和，取反，加1
		dataHead.setHead(in.readShort());
		dataHead.setLength(in.readShort());
		dataHead.setSeq(in.readByte());

		dataHead.setFunType(in.readByte());
		dataHead.setMsgDir(dataHead.getFunType() & 0x0F);
		dataHead.setFunType((dataHead.getFunType() & 0x0F0) >> 4);

		dataHead.setFunName(in.readByte());

		// 串口数据中的消息体(加密协议时对此数据整体加密)
		if (baseHead.getVersion() >= 3) {
			//
			byte[] enData = new byte[dataHead.getLength() - 8];
			in.read(enData, 0, enData.length);
			// 需要解密
			byte[] decodes = HKPacket.aesDecode(enData, key, iv);
			if (decodes != null) {
				ByteArrayInputStream deinbyte = new ByteArrayInputStream(decodes);
				DataInputStream dein = new DataInputStream(deinbyte);

				dataMsg.setFun(dein.readByte());
				dataMsg.setFormat(dein.readByte());
				int length = dein.readShort();

				byte[] message = new byte[length];
				dein.read(message, 0, message.length);
				dataMsg.setMessage(message);
			}
		} else {
			// 1B 功能
			// 1B 内容格式[json/bin]
			// 2Byte 具体内容长度(最大63k)
			dataMsg.setFun(in.readByte());
			dataMsg.setFormat(in.readByte());
			int length = in.readShort();

			byte[] message = new byte[length];
			in.read(message, 0, message.length);
			dataMsg.setMessage(message);
		}

		checkSum = in.readByte();

		byte check = 0;
		int length = dataHead.getLength() + HKPacketFromDeviceHead.MAX_SIZE - 1;

		for (int i = HKPacketFromDeviceHead.MAX_SIZE; i < length; i++) {
			check += payload[i];
		}
		check = (byte) ~check;
		check += 1;
		if (check != checkSum) {
			System.out.println("check err>>" + String.format("%02X  ->%02X", check, checkSum));
		}

		inbyte.close();

		HKPacket packet = new HKPacket();
		packet.setBase(baseHead);
		packet.setDataHead(dataHead);
		packet.setMsg(dataMsg);
		return packet;
	}

	public static String bytesToHexString(byte[] bs, String spilt) {
		StringBuilder sb = new StringBuilder();
		if (bs != null) {
			for (int i = 0; i < bs.length; i++) {
				sb.append(String.format("%02X", bs[i]));
				if (spilt.length() > 0) {
					sb.append(spilt);
				}
			}
		}
		return sb.toString();
	}

	public static byte[] hexStr2Bytes(String src) {
		src = src.trim().replace(" ", "").toUpperCase(Locale.US);
		// 处理值初始化
		int m = 0, n = 0;
		int iLen = src.length() / 2; // 计算长度
		byte[] ret = new byte[iLen]; // 分配存储空间

		for (int i = 0; i < iLen; i++) {
			m = i * 2 + 1;
			n = m + 1;
			ret[i] = (byte) (Integer.decode("0x" + src.substring(i * 2, m) + src.substring(m, n)) & 0xFF);
		}
		return ret;
	}

	public static native byte[] aesDecode(byte[] data, String key, String iv);

	public static native byte[] aesEncode(byte[] data, String key, String iv);

	static {
		System.loadLibrary("HKPacketJni");
	}
}

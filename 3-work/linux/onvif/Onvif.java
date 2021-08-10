
/**
 * Auth: cc
 * Date: 20210605
 * 
 *  读取局域网所有支持Onvif协议的IP地址
 *  Onvif 的rtsp流读取功能，使用 dom4j 来解析xml
 */
package cc.tool.onvif;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.HttpURLConnection;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.URL;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import org.dom4j.Document;
import org.dom4j.DocumentException;
import org.dom4j.DocumentHelper;
import org.dom4j.Element;

import cc.tool.onvif.Onvif.DeviceInformation;
import cc.tool.onvif.Onvif.MediaProfile;

public class Onvif {
	final static String XML_DISCOVER_Probe_tds_Device = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			+ "<Envelope xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\">"
			+ "<Header><wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:%s</wsa:MessageID>"
			+ "<wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
			+ "<wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
			+ "</Header><Body><Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			+ "<Types>tds:Device</Types><Scopes /></Probe>" + "</Body></Envelope>";

	final static String XML_DISCOVER_Probe_dn_NetworkVideoTransmitter = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			+ "<Envelope xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\">"
			+ "<Header><wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:%s</wsa:MessageID>"
			+ "<wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
			+ "<wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
			+ "</Header><Body>"
			+ "<Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			+ "<Types>dn:NetworkVideoTransmitter</Types><Scopes /></Probe></Body></Envelope>";

	private static void udpBroadMessage(DatagramSocket ds, String message) {
		String host = "255.255.255.255";// 广播地址
		int port = 3702;// 广播的目的端口
		try {
			InetAddress adds = InetAddress.getByName(host);
			DatagramPacket dp = new DatagramPacket(message.getBytes(), message.length(), adds, port);
			ds.send(dp);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public static Map<String, String> discover(int timeoutsec, String myIPAddress) {
		int port = 3702;// 开启监听的端口
		DatagramSocket ds = null;
		DatagramPacket dp = null;
		byte[] buf = new byte[2048];// 存储发来的消息

		Map<String, String> onvifAll = new HashMap<String, String>();
		try {
			// 绑定端口的
			ds = new DatagramSocket(port, InetAddress.getByName(myIPAddress));
			ds.setBroadcast(true);
			ds.setSoTimeout(timeoutsec);
			String uuid = UUID.randomUUID().toString();
			String probe_tds = String.format(XML_DISCOVER_Probe_tds_Device, uuid);
			String probe_dn = String.format(XML_DISCOVER_Probe_dn_NetworkVideoTransmitter, uuid);

			udpBroadMessage(ds, probe_tds);
			udpBroadMessage(ds, probe_dn);

			while (true) {
				dp = new DatagramPacket(buf, buf.length);
				ds.receive(dp);

				if (dp.getAddress().getHostAddress().equals(myIPAddress))
					continue;

				String rmsg = new String(buf, 0, dp.getLength());
				String xaddress = "";

				int xaddr_start = rmsg.indexOf(":XAddrs>");
				if (xaddr_start > 0) {
					xaddr_start += 8;
					int xaddr_end = rmsg.indexOf("</", xaddr_start);
					if (xaddr_end > 0) {
						xaddress = rmsg.substring(xaddr_start, xaddr_end);
						onvifAll.put(dp.getAddress().getHostAddress(), xaddress);
					}
				}
			}
		} catch (Exception e) {
			// e.printStackTrace();
		} finally {
			if (ds != null)
				ds.close();
		}
		return onvifAll;
	}

	static class ThreadDiscover implements Runnable {
		String myipaddress;
		int timeOutSec;
		CallBack callbk;
		boolean exitflag = false;

		static interface CallBack {
			void onDiscoverList(Map<String, String> ips);
		}

		public ThreadDiscover(String myipaddress, int timeOutSec, CallBack callbk) {
			super();
			this.myipaddress = myipaddress;
			this.timeOutSec = timeOutSec;
			this.callbk = callbk;
		}

		@Override
		public void run() {
			Map<String, String> ips = discover(timeOutSec, myipaddress);
			if (this.callbk != null) {
				if (ips.size() > 0)
					this.callbk.onDiscoverList(ips);
			}
			Thread.currentThread().interrupt();
			exitflag = true;
		}

	}

	public static List<String> getHostAllAddress() {
		List<String> list = new ArrayList<String>();

		Enumeration<NetworkInterface> allNetInterfaces;
		try {
			allNetInterfaces = NetworkInterface.getNetworkInterfaces();
			InetAddress ip = null;
			while (allNetInterfaces.hasMoreElements()) {
				NetworkInterface netInterface = (NetworkInterface) allNetInterfaces.nextElement();
				// 排除虚拟接口和没有启动运行的接口
				if (netInterface.isVirtual() || !netInterface.isUp()) {
					continue;
				} else {
					Enumeration<InetAddress> addresses = netInterface.getInetAddresses();
					while (addresses.hasMoreElements()) {
						ip = addresses.nextElement();
						if (ip == null)
							continue;
						if (ip instanceof Inet6Address)
							continue;

						if (ip instanceof Inet4Address) {
							if (ip.isLoopbackAddress())
								continue;
							list.add(ip.getHostAddress());
						}
					}
				}
			}
		} catch (SocketException e) {
			e.printStackTrace();
		}
		return list;
	}

	public static Map<String, String> disconverAllNetwork(int timeoutsec) {
		List<String> hostAllAddress = getHostAllAddress();
		List<ThreadDiscover> threads = new ArrayList<ThreadDiscover>();

		final Map<String, String> map = new HashMap<String, String>();

		for (String ipstr : hostAllAddress) {
			ThreadDiscover threadDiscover = new ThreadDiscover(ipstr, timeoutsec, new ThreadDiscover.CallBack() {

				@Override
				public void onDiscoverList(Map<String, String> ips) {
					Set<String> keySet = ips.keySet();
					for (String ip : keySet) {
						map.put(ip, ips.get(ip));
					}
				}
			});
			Thread thread = new Thread(threadDiscover);
			thread.setName("Onvif-Search:" + ipstr);
			thread.start();
			threads.add(threadDiscover);
		}

		for (ThreadDiscover thread : threads) {
			while (!thread.exitflag) {
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
		return map;
	}

	public static final String DEFAULT_ALGORITHM = "MD5";
	public static final String DEFAULT_SCHEME = "Digest";

	/** to hex converter */
	private static final char[] toHex = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e',
			'f' };

	public static String toHexString(byte b[]) {
		int pos = 0;
		char[] c = new char[b.length * 2];
		for (int i = 0; i < b.length; i++) {
			c[pos++] = toHex[(b[i] >> 4) & 0x0F];
			c[pos++] = toHex[b[i] & 0x0f];
		}
		return new String(c);
	}

	public static Map<String, String> headerValueParse(String msg) {
		Map<String, String> www = new HashMap<>();

		int _index = 0;
		int _next = 0;
		for (int i = 0; i < msg.length(); i++) {
			if (msg.charAt(i) == '=') {
				String name = msg.substring(_index, i).trim();
				i += 1;
				while (msg.charAt(i) == ' ')
					i++;
				_index = i;
				if (msg.charAt(i) == '\"') {
					_index = i + 1;
					_next = msg.indexOf('\"', _index);
				} else {
					_next = msg.indexOf(',', i + 1);
					if (_next == -1)
						_next = msg.length();
				}

				String value = msg.substring(_index, _next);

				_next = msg.indexOf(',', _next);
				if (_next == -1)
					_next = msg.length();
				_index = _next + 1;
				i = _index;

				// System.out.println(name + ":<" + value+">");
				www.put(name.toLowerCase(), value);
			}
		}

		// String[] split = msg.split(", ");

//		for (String c : split) {
//			String name = "";
//			String value = "";
//			int indexOf = c.indexOf("=");
//
//			if (indexOf > 0) {
//				name = c.substring(0, indexOf);
//				value = c.substring(indexOf + 1, c.length());
//				value = value.trim();
//				if (value.startsWith("\"")) {
//					value = value.replaceAll("\"", "");
//				}
//			} else {
//				name = c;
//			}
//			name = name.toLowerCase();
//			www.put(name, value);
//		}
		return www;
	}

	// WWW-Authenticate: Digest realm="Login to
	// 2J01018PAA00813",qop="auth",nonce="b252aWYtZGlnZXN0OjQzMjYxNjc5MTQw",opaque="",
	// stale="false"
	////////////////////////////////////////////////////////////////////
	// Authorization: Digest username="admin", realm="Login to 2J01018PAA00813",
	// qop="auth", algorithm="MD5",
	// uri="/onvif/device_service",
	// nonce="b252aWYtZGlnZXN0OjQzMjYxNjc5MTQw",
	// nc=00000001, cnonce="E715D956496A759F69523156E486B749",
	// opaque="", response="34f53d906234a7a4251de430d0f9907b"
	//////////////////////////////////////////////////////////////////
	// Digest realm="Digest", qop="auth,auth-int", nonce="60baee0cca81c82194e4",
	// opaque="db3b2dea"
	static String digestAuthorizationGet(String www_authorization, String uri, String username, String password) {
		String temp = www_authorization.replaceFirst("Digest", "").trim();
		Map<String, String> www = headerValueParse(temp);
		MessageDigest messageDigest = null;
		try {
			messageDigest = MessageDigest.getInstance(Onvif.DEFAULT_ALGORITHM);
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
		}

		String realm = www.get("realm");
		String nonce = www.get("nonce");
		String scheme = www.get("scheme");
		String qopstr = www.get("qop");
		String nc = www.get("nc");
		String opaque = www.get("opaque");
		String stale = www.get("stale");

		if (nonce == null)
			nonce = "";
		if (qopstr == null)
			qopstr = "auth";

		if (www.get("uri") != null)
			uri = www.get("uri");

		String response = "";
		String cnonce = UUID.randomUUID().toString().replaceAll("-", "");

		// HA1= MD5(username:realm:password
		// HA2=MD5(method:uri)
		String A1 = username + ":" + realm + ":" + password;
		String A2 = "POST:" + uri;

		if (opaque == null)
			opaque = "";

		String HA1 = Onvif.toHexString(messageDigest.digest(A1.getBytes()));
		String HA2 = Onvif.toHexString(messageDigest.digest(A2.getBytes()));

		String A3 = "";

		String[] qops = qopstr.split(",");

		boolean _auth_init_ = false;
		boolean _auth = false;

		for (String qopitem : qops) {
			if (qopitem.equalsIgnoreCase("auth"))
				_auth = true;
			if (qopitem.equalsIgnoreCase("auth-int"))
				_auth = true;
		}

		if (_auth_init_) {
			// A2=MD5(method:digestURI:MD5(entityBody));
			// MD5(HA1:nonce:nonceCount:clientNonce:qop:HA2)
			if (nc == null)
				nc = "00000001";
			A3 = HA1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qopstr + ":" + HA2;
			// 目前不支持
		}

		if (_auth) {
			// A2 = "POST:" + uri;
			// MD5(HA1:nonce:nonceCount:clientNonce:qop:HA2
			if (nc == null)
				nc = "00000001";
			A3 = HA1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qopstr + ":" + HA2;
		}

		if (qopstr == null || qopstr.length() == 0) {
			// MD5(HA1:nonce:HA2)
			A3 = HA1 + ":" + nonce + ":" + HA2;
		}

		String authorization = "";
		response = Onvif.toHexString(messageDigest.digest(A3.getBytes()));

		StringBuilder sb = new StringBuilder();

		sb.append("Digest username=\"").append(username).append("\"").append(", ");
		sb.append("realm=\"").append(realm).append("\"").append(", ");
		sb.append("qop=\"").append(qopstr).append("\"").append(", ");
		sb.append("algorithm=\"").append("MD5").append("\", ");
		sb.append("uri=\"").append(uri).append("\", ");
		sb.append("nonce=\"").append(nonce).append("\", ");
		sb.append("nc=\"").append(nc).append("\", ");
		sb.append("cnonce=\"").append(cnonce).append("\", ");
		sb.append("opaque=\"").append(opaque).append("\", ");
		sb.append("response=\"").append(response).append("\"");
		authorization = sb.toString();
		return authorization;
	}

	public static class HttpResponse {
		public int responseCode;
		public String www_authorization;
		public String body;
	}

	public static HttpResponse httpPost(String urlAddress, String authorization, String text, String username,
			String password) {
		HttpURLConnection connection = null;
		InputStream is = null;
		OutputStream os = null;
		BufferedReader br = null;
		HttpResponse result = new HttpResponse();
		result.responseCode = -1;
		try {
			URL url = new URL(urlAddress);
			// 通过远程url连接对象打开连接
			connection = (HttpURLConnection) url.openConnection();
			// 设置连接请求方式
			connection.setRequestMethod("POST");
			// 设置连接主机服务器超时时间：15000毫秒
			connection.setConnectTimeout(15000);
			// 设置读取主机服务器返回数据超时时间：60000毫秒
			connection.setReadTimeout(60000);

			// 默认值为：false，当向远程服务器传送数据/写数据时，需要设置为true
			connection.setDoOutput(true);
			// 默认值为：true，当前向远程服务读取数据时，设置为true，该参数可有可无
			connection.setDoInput(true);
			// 设置传入参数的格式:请求参数应该是 name1=value1&name2=value2 的形式。
			connection.setRequestProperty("Content-Type", "application/soap+xml; charset=utf-8");

			if (authorization != null && authorization.length() > 0)
				connection.setRequestProperty("Authorization", authorization);

			// 通过连接对象获取一个输出流
			os = connection.getOutputStream();
			// 通过输出流对象将参数写出去/传输出去,它是通过字节数组写出的
			os.write(text.getBytes());
			// 通过连接对象获取一个输入流，向远程读取
			int responseCode = connection.getResponseCode();

			result.responseCode = responseCode;

			if (responseCode == 401) {
				String www_authorization = connection.getHeaderField("WWW-Authenticate");

				if (www_authorization != null || www_authorization.length() > 0) {
					result.www_authorization = www_authorization;
				}
			}

			if (responseCode == 200) {
				is = connection.getInputStream();
				br = new BufferedReader(new InputStreamReader(is, "UTF-8"));

				StringBuffer sbf = new StringBuffer();
				String temp = null;
				while ((temp = br.readLine()) != null) {
					sbf.append(temp);
					sbf.append("\r\n");
				}
				result.body = sbf.toString();
			}
		} catch (MalformedURLException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			// 关闭资源
			if (null != br) {
				try {
					br.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			if (null != os) {
				try {
					os.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			if (null != is) {
				try {
					is.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			// 断开与远程地址url的连接
			connection.disconnect();
		}
		return result;
	}

	public static HttpResponse doPost(String urlAddress, String text, String username, String password) {
		String authorization = null;
		HttpResponse result = httpPost(urlAddress, authorization, text, username, password);
		if (result.responseCode == 401) {
			try {
				URL url = new URL(urlAddress);
				authorization = digestAuthorizationGet(result.www_authorization, url.getPath(), username, password);
				result = httpPost(urlAddress, authorization, text, username, password);
			} catch (MalformedURLException e) {
				e.printStackTrace();
			}
		}
		return result;
	}

	static String XML_GetDeviceInformation = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			+ "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			+ "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			+ "<GetDeviceInformation xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>" + "</s:Body></s:Envelope>";

	public static class DeviceInformation {
		public String Manufacturer;
		public String Model;
		public String FirmwareVersion;
		public String SerialNumber;
		public String HardwareId;

		@Override
		public String toString() {
			return "DeviceInformation [Manufacturer=" + Manufacturer + ", Model=" + Model + ", FirmwareVersion="
					+ FirmwareVersion + ", SerialNumber=" + SerialNumber + ", HardwareId=" + HardwareId + "]";
		}
	}

	// http://192.168.0.150/onvif/device_service
	public static DeviceInformation getDeviceInformation(String url, String username, String password)
			throws Exception {
		// String url = "http://192.168.0.150/onvif/device_service";
		HttpResponse doPost = Onvif.doPost(url, XML_GetDeviceInformation, username, password);
		if (doPost.responseCode == 200) {

			DeviceInformation info = null;
			Document doc = null;
			try {
				doc = DocumentHelper.parseText(doPost.body); // 将字符串转为XML

				Element rootElt = doc.getRootElement(); // 获取根节点
				Element body = rootElt.element("Body");
				// 遍历head节点
				Element GetDeviceInformationResponse = body.element("GetDeviceInformationResponse");
				info = new DeviceInformation();
				info.Manufacturer = GetDeviceInformationResponse.elementTextTrim("Manufacturer"); // 拿到head下的子节点script下的字节点username的值
				info.Model = GetDeviceInformationResponse.elementTextTrim("Model");
				info.FirmwareVersion = GetDeviceInformationResponse.elementTextTrim("FirmwareVersion");
				info.SerialNumber = GetDeviceInformationResponse.elementTextTrim("SerialNumber");
				info.HardwareId = GetDeviceInformationResponse.elementTextTrim("HardwareId");

			} catch (DocumentException e) {
				e.printStackTrace();
			} catch (Exception e) {
				e.printStackTrace();
			}
			return info;
		}
		if (doPost.responseCode == 401) {
			throw new Exception("401");
		}
		return null;
	}

	public static final String XML_GetCapabilities_Media = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			+ "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			+ "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			+ "<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\">" + "<Category>Media</Category>"
			+ "</GetCapabilities>" + "</s:Body></s:Envelope>";

	public static String getCapabilitiesMedia(String url, String username, String password) throws Exception {
		String mediaUrl = null;
		HttpResponse doPost = Onvif.doPost(url, XML_GetCapabilities_Media, username, password);
		if (doPost.responseCode == 401) {
			throw new Exception("401");
		}

		if (doPost.responseCode == 200) {
			Document doc = null;
			try {
				doc = DocumentHelper.parseText(doPost.body); // 将字符串转为XML

				Element rootElt = doc.getRootElement(); // 获取根节点
				Element body = rootElt.element("Body");
				Element GetCapabilitiesResponse = body.element("GetCapabilitiesResponse");
				Element Capabilities = GetCapabilitiesResponse.element("Capabilities");
				Element Media = Capabilities.element("Media");
				mediaUrl = Media.elementText("XAddr");
			} catch (DocumentException e) {
				e.printStackTrace();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return mediaUrl;
	}

	public static final String XML_GetProfiles = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			+ "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			+ "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			+ "<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>" + "</s:Body></s:Envelope>";

	public static class MediaProfile {
		public String token = "";
		public String name = "";
		public String video_Encoding = "";
		public String video_Width = "";
		public String video_Height = "";

		@Override
		public String toString() {
			return "MediaProfile [token=" + token + ", name=" + name + ", video_Encoding=" + video_Encoding
					+ ", video_Width=" + video_Width + ", video_Height=" + video_Height + "]";
		}

	}

	public static List<MediaProfile> GetProfiles(String mediaUrl, String username, String password) throws Exception {
		HttpResponse doPost = Onvif.doPost(mediaUrl, XML_GetProfiles, username, password);
		if (doPost.responseCode == 401) {
			throw new Exception("401," + doPost.www_authorization);
		}

		if (doPost.responseCode == 200) {
			List<MediaProfile> list = new ArrayList<Onvif.MediaProfile>();

			Document doc = null;
			try {
				doc = DocumentHelper.parseText(doPost.body); // 将字符串转为XML

				Element rootElt = doc.getRootElement(); // 获取根节点
				Element body = rootElt.element("Body");
				Element GetProfilesResponse = body.element("GetProfilesResponse");
				Iterator<Element> Profiles = GetProfilesResponse.elementIterator("Profiles");
				while (Profiles.hasNext()) {
					Element profile = Profiles.next();

					MediaProfile mediaProfile = new MediaProfile();
					String token = profile.attributeValue("token");
					String name = profile.elementText("Name");
					String video_Encoding = "";
					String video_Width = "";
					String video_Height = "";

					Element VideoEncoderConfiguration = profile.element("VideoEncoderConfiguration");
					if (VideoEncoderConfiguration != null) {
						video_Encoding = VideoEncoderConfiguration.elementText("Encoding");
						Element Resolution = VideoEncoderConfiguration.element("Resolution");
						if (Resolution != null) {
							video_Width = Resolution.elementText("Width");
							video_Height = Resolution.elementText("Height");
						}
					}
					mediaProfile.name = name;
					mediaProfile.token = token;
					mediaProfile.video_Encoding = video_Encoding;
					mediaProfile.video_Width = video_Width;
					mediaProfile.video_Height = video_Height;
					list.add(mediaProfile);
				}
				return list;
			} catch (DocumentException e) {
				e.printStackTrace();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return null;
	}

	public static final String XML_GetStreamUri = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
			+ "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			+ "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			+ "<GetStreamUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\">" + "<StreamSetup>"
			+ "<Stream xmlns=\"http://www.onvif.org/ver10/schema\">RTP-Unicast</Stream>"
			+ "<Transport xmlns=\"http://www.onvif.org/ver10/schema\">" + "<Protocol>UDP</Protocol>" + "</Transport>"
			+ "</StreamSetup>" + "<ProfileToken>%s</ProfileToken>" + "</GetStreamUri></s:Body>" + "</s:Envelope>";

	public static String GetStreamUri(String mediaUrl, String username, String password, String token)
			throws Exception {

		HttpResponse doPost = Onvif.doPost(mediaUrl, String.format(XML_GetStreamUri, token), username, password);
		if (doPost.responseCode == 401) {
			throw new Exception("401");
		}

		if (doPost.responseCode == 200) {
			Document doc = null;
			try {
				doc = DocumentHelper.parseText(doPost.body); // 将字符串转为XML
				Element rootElt = doc.getRootElement(); // 获取根节点
				Element body = rootElt.element("Body");
				Element GetStreamUriResponse = body.element("GetStreamUriResponse");
				Element MediaUri = GetStreamUriResponse.element("MediaUri");
				String uri = MediaUri.elementText("Uri");
				return uri;
			} catch (DocumentException e) {
				e.printStackTrace();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return null;
	}

	public static void test_main() {
		String username = "admin";
		String password = "admin";
		{
			String www_authorization = "Digest realm=\"Digest\",qop=\"auth,auth-int\", nonce=\"60baef14ca8ad128e454\", opaque=\"c6e2761b\"";
			String uri = "/onvif/device_service";

			System.out.println(Onvif.headerValueParse(www_authorization));

			String digestAuthorizationGet = Onvif.digestAuthorizationGet(www_authorization, uri, username, password);
			System.out.println(digestAuthorizationGet);
		}

		System.out.println("");

		Map<String, String> maps = Onvif.disconverAllNetwork(2000);
		Set<String> keySet = maps.keySet();

		for (String ip : keySet) {
			System.out.println(ip + ", url:" + maps.get(ip));

			String url = maps.get(ip);
			if (url.indexOf(' ') > 0) {
				String[] split = url.split(" ");
				if (split.length > 0)
					url = split[0];
			}

			try {

				DeviceInformation deviceInformation = Onvif.getDeviceInformation(url, username, password);
				String mediaUrl = Onvif.getCapabilitiesMedia(url, username, password);

				System.out.println(deviceInformation);
				System.out.println(mediaUrl);

				List<MediaProfile> profiles = Onvif.GetProfiles(mediaUrl, username, password);
				/// System.out.println(profiles);

				for (MediaProfile mediaProfile : profiles) {
					String uri = Onvif.GetStreamUri(mediaUrl, username, password, mediaProfile.token);
					System.out.println(uri);
				}

			} catch (Exception e) {
				e.printStackTrace();
			}

			System.out.println("");
		}
	}
}

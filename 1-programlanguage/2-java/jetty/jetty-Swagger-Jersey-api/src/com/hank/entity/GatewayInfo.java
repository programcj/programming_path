package com.hank.entity;

public class GatewayInfo {
	private String Sn;
	private String Pin;

	private String Mac1;// varchar(20) utf8_general_ci
	private String Mac2;// varchar(20) utf8_general_ci
	private int FirmwareVer;// int(11)
	private int AppVer;// int(11)
	private String Frequency;// varchar(20) utf8_general_ci
	private String Salesman;// varchar(100) utf8_general_ci
	private String OrderDate;// varchar(20) utf8_general_ci
	private String Customer;// varchar(100) utf8_general_ci
	private String DeliveryDate;// varchar(20) utf8_general_ci
	private int TestFlag;// int(11)
	private String Tester;// varchar(100) utf8_general_ci
	private String TestTime;// varchar(20) utf8_general_ci
	private String CreateTime;// varchar(20) utf8_gene

	public String getSn() {
		return Sn;
	}

	public void setSn(String sn) {
		Sn = sn;
	}

	public String getPin() {
		return Pin;
	}

	public void setPin(String pin) {
		Pin = pin;
	}

	public String getMac1() {
		return Mac1;
	}

	public void setMac1(String mac1) {
		Mac1 = mac1;
	}

	public String getMac2() {
		return Mac2;
	}

	public void setMac2(String mac2) {
		Mac2 = mac2;
	}

	public int getFirmwareVer() {
		return FirmwareVer;
	}

	public void setFirmwareVer(int firmwareVer) {
		FirmwareVer = firmwareVer;
	}

	public int getAppVer() {
		return AppVer;
	}

	public void setAppVer(int appVer) {
		AppVer = appVer;
	}

	public String getFrequency() {
		return Frequency;
	}

	public void setFrequency(String frequency) {
		Frequency = frequency;
	}

	public String getSalesman() {
		return Salesman;
	}

	public void setSalesman(String salesman) {
		Salesman = salesman;
	}

	public String getOrderDate() {
		return OrderDate;
	}

	public void setOrderDate(String orderDate) {
		OrderDate = orderDate;
	}

	public String getCustomer() {
		return Customer;
	}

	public void setCustomer(String customer) {
		Customer = customer;
	}

	public String getDeliveryDate() {
		return DeliveryDate;
	}

	public void setDeliveryDate(String deliveryDate) {
		DeliveryDate = deliveryDate;
	}

	public int getTestFlag() {
		return TestFlag;
	}

	public void setTestFlag(int testFlag) {
		TestFlag = testFlag;
	}

	public String getTester() {
		return Tester;
	}

	public void setTester(String tester) {
		Tester = tester;
	}

	public String getTestTime() {
		return TestTime;
	}

	public void setTestTime(String testTime) {
		TestTime = testTime;
	}

	public String getCreateTime() {
		return CreateTime;
	}

	public void setCreateTime(String createTime) {
		CreateTime = createTime;
	}

	@Override
	public String toString() {
		return "GatewayInfo [Sn=" + Sn + ", Pin=" + Pin + ", Mac1=" + Mac1 + ", Mac2=" + Mac2 + ", FirmwareVer="
				+ FirmwareVer + ", AppVer=" + AppVer + ", Frequency=" + Frequency + ", Salesman=" + Salesman
				+ ", OrderDate=" + OrderDate + ", Customer=" + Customer + ", DeliveryDate=" + DeliveryDate
				+ ", TestFlag=" + TestFlag + ", Tester=" + Tester + ", TestTime=" + TestTime + ", CreateTime="
				+ CreateTime + "]";
	}

}

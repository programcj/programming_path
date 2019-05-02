package com.hank.web;

import java.io.InputStream;

import org.apache.ibatis.session.SqlSession;
import org.apache.ibatis.session.SqlSessionFactory;
import org.apache.ibatis.session.SqlSessionFactoryBuilder;

public class HKSqlSessionFaction {
	static SqlSessionFactory sessionFactory = null;

	// 定义方法
	public static SqlSessionFactory getFactory() {
		String resource = "conf.xml";
		// 读取配置文件，为了解决项目在发布以后的路径带来的问题我们可以去使用反射机制来封装
		InputStream is = HKSqlSessionFaction.class.getClassLoader().getResourceAsStream(resource);
		SqlSessionFactory factory = new SqlSessionFactoryBuilder().build(is);
		return factory;
	}

	public static void init() {
		sessionFactory = getFactory();
	}

	public static SqlSession getSession() {
		return sessionFactory.openSession();
	}
}

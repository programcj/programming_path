package com.hank.web;

import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.nio.SelectChannelConnector;
import org.eclipse.jetty.webapp.WebAppContext;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Main {
	static Logger loggger = LoggerFactory.getLogger(Main.class);

	public static void main(String[] args) {
		// String resource =
		// "C:\\Users\\cj\\workspace-jee\\jetty-webSmartHomeService\\src\\conf.xml";
		// InputStream is = null;
		// try {
		// is = new FileInputStream(new File(resource));
		// } catch (FileNotFoundException e1) {
		// e1.printStackTrace();
		// return;
		// }
		//
		// // 构建sqlSession的工厂
		// SqlSessionFactory sessionFactory = new
		// SqlSessionFactoryBuilder().build(is);
		// // 使用MyBatis提供的Resources类加载mybatis的配置文件（它也加载关联的映射文件）
		// // Reader reader = Resources.getResourceAsReader(resource);
		// // 构建sqlSession的工厂
		// // SqlSessionFactory sessionFactory = new
		// // SqlSessionFactoryBuilder().build(reader);
		// // 创建能执行映射文件中sql的sqlSession
		// SqlSession session = sessionFactory.openSession();
		// /**
		// * 映射sql的标识字符串，
		// * me.gacl.mapping.userMapper是userMapper.xml文件中mapper标签的namespace属性的值，
		// * getUser是select标签的id属性值，通过select标签的id属性值就可以找到要执行的SQL
		// */
		// String statement = "com.hank.entity.gatewayMapper.getGateway";//
		// 映射sql的标识字符串
		// // 执行查询返回一个唯一user对象的sql
		// com.hank.entity.GatewayInfo gateway = session.selectOne(statement,
		// "HK0120162000001");
		// loggger.debug(gateway.toString());
		// session.close();

		///////////////////////////////////////////////////////////
		Server server = new Server();
		Connector connector = new SelectChannelConnector();
		connector.setPort(8080);
		server.addConnector(connector);

		WebAppContext webapp = new WebAppContext();
		webapp.setContextPath("/");
		webapp.setResourceBase("/opt/project-jee/jetty-webSmartHomeService/WebContent");
		webapp.setDescriptor("/opt/project-jee/jetty-webSmartHomeService/WebContent/web.xml");
		webapp.setParentLoaderPriority(true);

//		EnumSet<DispatcherType> set = EnumSet.allOf(DispatcherType.class);
//
//		webapp.addFilter(new FilterHolder(new Filter() {
//
//			@Override
//			public void init(FilterConfig arg0) throws ServletException {
//
//			}
//
//			@Override
//			public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain)
//					throws IOException, ServletException {
//				request.setAttribute("APOLLO_BROKER", "");
//				System.out.println("->................" + request.getParameterMap().toString());
//				chain.doFilter(request, response);
//			}
//
//			@Override
//			public void destroy() {
//
//			}
//		}), "/*", set);

		server.setHandler(webapp);
		try {
			server.start();
			server.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}

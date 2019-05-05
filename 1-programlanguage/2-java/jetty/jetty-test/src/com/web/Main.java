package com.web;

import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.nio.SelectChannelConnector;
import org.eclipse.jetty.webapp.WebAppContext;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Main {
	static Logger loggger = LoggerFactory.getLogger(Main.class);

	public static void main(String[] args) {
		Server server = new Server();
		Connector connector = new SelectChannelConnector();
		connector.setPort(8080);
		server.addConnector(connector);

		WebAppContext webAppContext = new WebAppContext();
		
		webAppContext.setContextPath("/");
		webAppContext.setResourceBase("WebContent/");
		webAppContext.setDescriptor("WebContent/WEB-INF/web.xml");
		webAppContext.setParentLoaderPriority(true);
		
		webAppContext.setClassLoader(Thread.currentThread().getContextClassLoader());
		webAppContext.setConfigurationDiscovered(true);
		webAppContext.setParentLoaderPriority(true);
		server.setHandler(webAppContext);
		
		System.out.println(webAppContext.getContextPath());
		System.out.println(webAppContext.getDescriptor());
		System.out.println(webAppContext.getResourceBase());
		System.out.println(webAppContext.getBaseResource());
		
		System.out.println("start...["+Thread.currentThread().getContextClassLoader()+"]");
		loggger.debug("start....");
		
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

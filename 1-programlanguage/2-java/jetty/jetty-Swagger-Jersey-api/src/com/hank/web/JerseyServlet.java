package com.hank.web;

import javax.servlet.FilterConfig;
import javax.servlet.ServletException;

import org.glassfish.jersey.servlet.ServletContainer;

public class JerseyServlet extends ServletContainer {

	@Override
	public void init() throws ServletException {
		System.out.println("JerseyServlet init");
		super.init();
	}

	@Override
	public void init(FilterConfig arg0) throws ServletException {
		System.out.println("JerseyServlet->" + arg0.toString());
		super.init(arg0);
	}

}

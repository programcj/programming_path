package com.web;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class JerseyServlet extends HttpServlet {
	private static final long serialVersionUID = 1L;

	static Logger loggger = LoggerFactory.getLogger(JerseyServlet.class);

	@Override
	public void init() throws ServletException {
		loggger.debug("init");
		super.init();
	}

	@Override
	public void init(ServletConfig config) throws ServletException {
		loggger.debug("init config");
		super.init(config);
	}
	
	

}

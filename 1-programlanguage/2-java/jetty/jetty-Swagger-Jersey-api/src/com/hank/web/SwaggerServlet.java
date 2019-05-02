package com.hank.web;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;

import org.glassfish.jersey.server.ResourceConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.hank.web.smarthome.api.Test;

import io.swagger.jersey.config.JerseyJaxrsConfig;

public class SwaggerServlet extends JerseyJaxrsConfig {

	@Override
	public void init(ServletConfig servletConfig) throws ServletException {
		System.out.println("SwaggerServlet init " + servletConfig);
		super.init(servletConfig);
	}

}

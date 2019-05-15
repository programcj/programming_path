package com.hank.web.smarthome.api;

import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.ext.ExceptionMapper;
import javax.ws.rs.ext.Provider;

@Provider
public class ExceptionMapperSupport implements ExceptionMapper<Throwable> {

	@Override
	public Response toResponse(Throwable arg0) {
		System.out.println("ExceptionMapperSupport>" + arg0.getMessage());
		ResponseBuilder status = Response.status(401);
		return status.build();
	}
}

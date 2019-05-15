package com.hank.web;

import javax.ws.rs.ApplicationPath;
import javax.ws.rs.ext.Provider;

import org.apache.ibatis.session.SqlSession;
import org.glassfish.jersey.server.ResourceConfig;

import com.hank.web.smarthome.api.Test;

@ApplicationPath(value = "/api")
public class RestJaxRsApplication extends ResourceConfig {
	final public static com.fasterxml.jackson.databind.ObjectMapper mapper = new com.fasterxml.jackson.databind.ObjectMapper();
	static {
		mapper.setSerializationInclusion(com.fasterxml.jackson.annotation.JsonInclude.Include.NON_NULL);
	}

	@Provider
	public static class JacksonJsonProvider extends com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider {
		public JacksonJsonProvider() {
			super(mapper);
		}
	}

	public RestJaxRsApplication() {
		System.out.println("RestJaxRsApplication");

		// packages("com.wordnik.swagger.jaxrs.listing");
		register(io.swagger.jaxrs.listing.ApiListingResource.class);
		register(io.swagger.jaxrs.listing.AcceptHeaderApiListingResource.class);
		register(io.swagger.jaxrs.listing.SwaggerSerializers.class);
		register(Test.class);
		// register(JacksonJsonProvider.class);
		//register(com.hank.web.smarthome.api.ExceptionMapperSupport.class);

		HKSqlSessionFaction.init();
		SqlSession session = HKSqlSessionFaction.getSession();
		String statement = "com.hank.entity.gatewayMapper.getGateway";//
		com.hank.entity.GatewayInfo gateway = session.selectOne(statement, "HK0120162000001");
		System.out.println(gateway);
		session.close();
	}
}

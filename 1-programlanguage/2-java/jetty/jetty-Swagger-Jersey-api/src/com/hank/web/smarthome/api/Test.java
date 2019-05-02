package com.hank.web.smarthome.api;

import java.util.List;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.ws.rs.*;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;

import org.apache.ibatis.session.RowBounds;
import org.apache.ibatis.session.SqlSession;

import com.hank.web.HKSqlSessionFaction;

import io.swagger.annotations.Api;
import io.swagger.annotations.ApiOperation;
import io.swagger.annotations.ApiParam;

@Path("/")
@Api(value = "测试")
public class Test {

	@GET
	@Path("/room")
	@Produces(MediaType.TEXT_PLAIN)
	@ApiOperation(value = "添加房间")
	public String addRoom(@ApiParam(value = "User Name", defaultValue = "admin") @QueryParam("name") String name,
			@Context HttpServletRequest request) {
		System.out.println("whoami " + name + " " + request.getParameterMap().toString());
		return "hello " + name + " " + request.getParameterMap().toString();
	}

	@GET
	@Path("/regedit")
	@Produces(MediaType.APPLICATION_JSON)
	@ApiOperation(value = "User regedit")
	public String regedit(
			@ApiParam(value = "User Name", defaultValue = "admin") @PathParam("userName") String userName) {

		return "user regedit " + userName;
	}

	@GET
	@Path("/gateway")
	@Consumes({ MediaType.APPLICATION_JSON }) // 接收json类型
	@Produces(MediaType.APPLICATION_JSON + ";charset=UTF-8")
	@ApiOperation(value = "get gateway info")
	public com.hank.entity.GatewayInfo getGateway(@ApiParam(value = "sn") @QueryParam("sn") String sn) {
		SqlSession session = HKSqlSessionFaction.getSession();
		String statement = "com.hank.entity.gatewayMapper.getGateway";//
		com.hank.entity.GatewayInfo gateway = session.selectOne(statement, sn);
		System.out.println(gateway);
		session.close();
		return gateway;
	}

	@GET
	@Path("/gateways")
	@Produces(MediaType.APPLICATION_JSON + ";charset=UTF-8")
	@ApiOperation(value = "get gateway info")
	public GatewayInfoList getGateways(@ApiParam(value = "start", defaultValue = "1") @QueryParam("page") int page,
			@ApiParam(value = "start", defaultValue = "10") @QueryParam("rows") int rows) {
		GatewayInfoList result = new GatewayInfoList();
		SqlSession session = HKSqlSessionFaction.getSession();
		int count = (int) session.selectOne("com.hank.entity.gatewayMapper.count");
		String statement = "com.hank.entity.gatewayMapper.getGateways";//
		page--;

		List<com.hank.entity.GatewayInfo> list = session.selectList(statement, "", new RowBounds(page * rows, rows));
		session.close();
		result.total = count;
		result.rows = list;
		return result;
	}

	public static class GatewayInfoList {
		int total;
		List<com.hank.entity.GatewayInfo> rows;

		public int getTotal() {
			return total;
		}

		public void setTotal(int total) {
			this.total = total;
		}

		public List<com.hank.entity.GatewayInfo> getRows() {
			return rows;
		}

		public void setRows(List<com.hank.entity.GatewayInfo> rows) {
			this.rows = rows;
		}
	}
}

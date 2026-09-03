/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm02_inq_m
*  程序描述			: 热轧出厂准发材料查询
*  备注说明			:
*  修改历史			:
*  		wuxin			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"








BM2F_ENTERACE(smsm02_inq_m)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm02_inq_m(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0;

	CModel tsmsm01("TSMSM01");
	CModel tsmsm03("TSMSM03");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_stock_no = " ";
	CString c_ready_bill_no = " ";
	CString c_bill_of_lading_no = " ";
	CString c_order_no_from = " ";
	CString c_order_no_to = " ";
	CString c_mat_no_from = " ";
	CString c_mat_no_to = " ";
	CString c_factory_div = " ";
	CString c_code = " ";
	CString c_plan_status = " ";
	CString table_name = " ";
	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr0("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	/* ***** 应用程序开始处理 ***** */
	try
	{
		tsmsm01["BILL_OF_LADING_NO"] = bcls_rec->Tables[0].Rows[0]["BILL_OF_LADING_NO"].ToString().Trim();//提单号
		//c_plan_status = bcls_rec->Tables[0].Rows[0]["PLAN_STATUS"].ToString().Trim();// 计划状态

		
		
		if (tsmsm01.Query("BILL_OF_LADING_NO"))
		{
			c_plan_status = tsmsm01["PLAN_STATUS"].ToString();
			Log::Info("", __FUNCTION__, "c_plan_status=[{0}]", c_plan_status);
		}
		else
		{
			sprintf(s.msg, "提单号[%s]不存在！", (const char*)tsmsm01["BILL_OF_LADING_NO"]);
			throw CApplicationException(-1, s.msg, log.Location);
		}
		if (c_plan_status == "9")//已完成查历史档
		{
			table_name = "hsmsm03";
		}
		else//执行中查在线档
		{
			table_name = "tsmsm03";
		}
		sqlstr = "select (CASE WHEN A.ORDER_TYPE_CODE='XYC' THEN ' '  ELSE A.ORDER_NO END)ORDER_NO, A.* from " + table_name + " A where 1=1 ";
		if (c_bill_of_lading_no != "") sqlstr += " AND bill_of_lading_no = '"+ tsmsm01["BILL_OF_LADING_NO"].ToString() +"'  ";
		Log::Info("", __FUNCTION__, "sqlstr=[{0}]", sqlstr);

		// SQL语句中的变量赋值
		cmd_inq.SetCommandText(sqlstr);
		
		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
		cmd_inq.Close();
		// -----End IPLAT4C::IPLAT4CServiceCompositeStatementObj()----- //

	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;
	}
	catch (const CApplicationException& ex)
	{
		//	strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应 
		//EDLog(1,1, "error=[%s]", (const char*)s.msg );  
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	catch (const CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399); //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.msg参数对应
		s.flag = ex.GetCode();       //返回前台，与EI.EITuxedo.CallService()方法返回的EI.EIInfo对象的sys_info.flag参数对应
		doFlag = -1;
	}

	return doFlag;
}

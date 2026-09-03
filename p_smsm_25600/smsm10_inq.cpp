/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm10_inq
*  程序描述			: 交库材料查询
*  备注说明			:
*  修改历史			:
*  		wuxin			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"




BM2F_ENTERACE(smsm10_inq)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm10_inq(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0;

	CModel tsmsm03("TSMSM03");


	/* ***** 程序变量 ***** */
	
	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr0("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand cmd_inq(conn);
	CDbCommand execute_sql(conn);

	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */
		sqlstr = " select * from tsmsm03 where 1=1  ";
		if (bcls_rec->Tables[0].Rows[0]["MAT_NO"].ToString().Trim() != "")
		{
			sqlstr += " AND MAT_NO LIKE '" + bcls_rec->Tables[0].Rows[0]["MAT_NO"].ToString() + "%' ";
		}
		if (bcls_rec->Tables[0].Rows[0]["HEAT_NO"].ToString().Trim() != "")
		{
			sqlstr += " AND HEAT_NO LIKE '" + bcls_rec->Tables[0].Rows[0]["HEAT_NO"].ToString() + "%' ";
		}
		Log::Trace("", __FUNCTION__, "sqlst[{0}]", sqlstr);
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.ExecuteQuery(bcls_ret->Tables[0]);
		cmd_inq.Close();
		

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

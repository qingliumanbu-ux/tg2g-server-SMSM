/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm01_inq_m
*  程序描述			: 准发材料查询
*  备注说明			:
*  修改历史			:
*  2011-12-14 吴新			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"



 
 

int f_smsm01_inq_m(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);

BM2F_ENTERACE(smsm01_inq_m)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm01_inq_m(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0;

	CModel tsmpe02("TSMPE02");
	CModel tsmpe00("TSMPE00");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_stock_no = " ";
	CString c_confm_plan_no = " ";
	CString c_ready_bill_no = " ";
	CString c_order_no_from = " ";
	CString c_order_no_to = " ";
	CString c_confm_status_from = " ";
	CString c_confm_status_to = " ";
	CString c_prg_send_time_from = " ";
	CString c_prg_send_time_to = " ";
	CString c_mat_no_from = " ";
	CString c_mat_no_to = " ";
	CString c_heat_no = " ";
	CString c_factory_div = " ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr0("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);

	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */
		c_ready_bill_no = bcls_rec->Tables[0].Rows[0]["ready_bill_no"].ToString().TrimOrBlank();
		c_factory_div = bcls_rec->Tables[0].Rows[0]["factory_div"].ToString().TrimOrBlank();

		/* ***** 获取库区号  ***** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr0 = CString(
				"SELECT mat_kind FROM tsmpe00 WHERE ready_bill_no = @ready_bill_no"
				);

			sqlstr1 = CString(
				" SELECT a.*,b.* FROM tsmpe02  a,tmmsm01 b "
				" WHERE a.mat_no        = b.mat_no "
				"   AND a.ready_bill_no = @ready_bill_no "
				//		         "   AND a.mat_kind      = @factory_div "
				);

			sqlstr2 = CString(
				" SELECT a.*,b.* FROM tsmpe02  a,tmmhr01 b "
				" WHERE a.mat_no      = b.mat_no "
				"   AND a.ready_bill_no = @ready_bill_no "
				//		         "   AND a.mat_kind      = @factory_div "
				);

			//		sqlstr = CString(
			//			     " select * from tsmpe02 " 
			//				 " where a.ready_bill_no = @ready_bill_no "
			//		         "   and a.mat_kind      = @factory_div "
			//				 );
			//
			//		sqlstr1 = CString(
			//			     " select * from tmmsm01 " 
			//				 " where mat_no = @mat_no " 
			//				 );
			//
			//		sqlstr2 = CString(
			//			     " select * from tmmhr01 " 
			//				 " where mat_no = @mat_no " 
			//				 );

			break;
		}
		/* ***** 执行SQL   ***** */
		sqlstr = sqlstr0;
		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Set("ready_bill_no", c_ready_bill_no);
		execute_sql.ExecuteReader();
		while (execute_sql.Read())
		{
			execute_sql.Fetch(tsmpe00);
		}
		execute_sql.Close();

		if (0 == tsmpe00["MAT_KIND"].ToString().Compare("SM"))
		{
			sqlstr = sqlstr1;
		}

		if (0 == tsmpe00["MAT_KIND"].ToString().Compare("HR"))
		{
			sqlstr = sqlstr2;
		}


		execute_sql.SetCommandText(sqlstr);
		execute_sql.Parameters.Set("ready_bill_no", c_ready_bill_no);
		execute_sql.Parameters.Set("factory_div", c_factory_div);
		execute_sql.ExecuteQuery(bcls_ret->Tables[0]);



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

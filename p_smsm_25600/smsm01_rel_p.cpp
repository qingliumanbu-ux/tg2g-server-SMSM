/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm01_rel_p
*  程序描述			: 准发计划释放
*  备注说明			:
*  修改历史			:
*  		2011-12-14 	吴新			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"



 

int f_smsm01_rel_p(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);

BM2F_ENTERACE(smsm01_rel_p)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm01_rel_p(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, ret = 0;

	CModel tsmpe02("TSMPE02");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_stock_no = " ";
	CString c_confm_plan_no = " ";
	CString c_confm_status = " ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);


	/* ***** 应用程序开始处理 ***** */
	try
	{
		/* ***** 获取前台参数  ***** */
		/**************** 接收准发材料 **********************/
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			c_confm_plan_no = bcls_rec->Tables[0].Rows[i]["confm_plan_no"].ToString().TrimOrBlank();

			if (c_confm_plan_no.Compare(" ") == 0)
			{
				break;
			}

			/* ***** 获取库区号  ***** */
			switch (conn->DatabaseKind)
			{
			case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
			case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
			case DB_KIND_MSSQL:	        // MS SQL Server数据库
			case DB_KIND_ORACLE:	        // Oracle 数据库
			default:
				sqlstr1 = CString(
					" SELECT CONFM_STATUS FROM TSMPE01 WHERE CONFM_PLAN_NO = @confm_plan_no "
					);

				sqlstr2 = CString(
					" UPDATE	TSMPE01 "
					"           SET		REC_REVISOR     = @c_user, "
					"                      REC_REVISE_TIME = TO_CHAR(SYSDATE,'yyyymmddhh24miss'), "
					"                      CONFM_STATUS = '3' "
					"            WHERE	CONFM_PLAN_NO = @confm_plan_no "
					"              AND CONFM_STATUS IN ('2', '3') "
					);

				sqlstr3 = CString(
					" UPDATE	TSMPE00 "
					"           SET		REC_REVISOR     = @c_user, "
					"                      REC_REVISE_TIME = TO_CHAR(SYSDATE,'yyyymmddhh24miss'), "
					"                      CONFM_STATUS = '3' "
					"            WHERE	CONFM_PLAN_NO = @confm_plan_no "
					"              AND CONFM_STATUS IN ('2', '3') "
					);

				sqlstr4 = CString(
					" UPDATE	TSMPE02 "
					"           SET		REC_REVISOR     = @c_user, "
					"                      REC_REVISE_TIME = TO_CHAR(SYSDATE,'yyyymmddhh24miss'), "
					"                      CONFM_STATUS = '3' "
					"            WHERE	CONFM_PLAN_NO = @confm_plan_no "
					"              AND CONFM_STATUS IN ('2', '3') "
					);


				break;
			}

			/* ***** 执行SQL   ***** */
			sqlstr = sqlstr1;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteReader();

			if (execute_sql.Read())
			{
				c_confm_status = execute_sql.GetString(1);
			}
			execute_sql.Close();

			/* ***** 判断计划的状态   ***** */
			if (c_confm_status.Compare("2") != 0)
			{
				doFlag = -11;
				{CFormattable arguments[] = { c_confm_plan_no, c_confm_status }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000873")/*计划号[{0}]的准发状态为[{1}]， 不能释放*/, arguments, 2);
				}
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			/* ***** 修改准发计划的状态   ***** */
			sqlstr = sqlstr2;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();

			/* ***** 修改准发单据的状态   ***** */
			sqlstr = sqlstr3;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();

			/* ***** 修改准发材料的状态   ***** */
			sqlstr = sqlstr4;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();



		}


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
		Log::Error("", __FUNCTION__, "error=[{0}]", s.msg);
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

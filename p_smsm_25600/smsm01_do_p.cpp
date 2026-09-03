/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm01_do_p
*  程序描述			: 准发出厂确认
*  备注说明			:
*  修改历史			:
*  		2008-9-2 	BM2IDE			(ADD)程序建立
*		2010-08-23	压块记录数清0
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"



#include "epex.h"
 

 

int f_sm00_record(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_smsm01_do_p(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
int f_sm00_hold_flag_chk(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);	//材料封锁检查

BM2F_ENTERACE(smsm01_do_p)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm01_do_p(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, ret = 0, j = 0;
	CString	record_name = "sm00_record";
	CString hold_flag = "hold_flag";

	CModel tsmpe02("TSMPE02");
	CModel tsmpe00("TSMPE00");
	CModel tsmpe01("TSMPE01");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_del_cause = " ", c_del_type = " ";
	CString c_confm_plan_no = " ", c_confm_status = " ", c_mat_no = " ";
	CString  c_confm_shift = " ", c_confm_group = " ";
	char  c_datetime[15];
	CString CONFM_PLAN_NO, READY_BILL_NO, REC_REVISOR, REC_REVISE_TIME, MAT_NO;

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr0(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5(""), sqlstr6("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	/* *******定义一个EIClass object */
	EIClass ds_ready_bill_no, ds_mat_no;

	/* ***** 创建电文处理对象 ***** */
	EPEX epex(&s);
	CString c_tc_no = "2000S1";

	/* ***** 应用程序开始处理 ***** */
	try
	{
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_mark");
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "userid");
			bcls_rec->Tables[record_name].Rows.Add();
		}
		if (bcls_rec->Tables.IndexOf(hold_flag) < 0)
		{
			bcls_rec->Tables.Add(hold_flag);
			bcls_rec->Tables[hold_flag].Columns.Add(DT_STRING, "mat_no");
			bcls_rec->Tables[hold_flag].Columns.Add(DT_STRING, "table_name");
			bcls_rec->Tables[hold_flag].Rows.Add();
		}
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		strcpy(c_datetime, (const char *)datetime);

		// 调用函数生成班次、班组
		f_epep_get_shift_group("DEFAULT", datetime, c_confm_shift, c_confm_group, conn);					// 调函数 


		/* ***** format sql ****** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr0 = CString(
				" SELECT t.CONFM_STATUS FROM TSMPE01 t WHERE CONFM_PLAN_NO = @confm_plan_no  FOR UPDATE NOWAIT "
				);

			sqlstr1 = CString(
				" SELECT t.* FROM TSMPE00 t WHERE CONFM_PLAN_NO = @confm_plan_no AND CONFM_STATUS = '3' FOR UPDATE NOWAIT "
				);

			sqlstr2 = CString(
				" SELECT t.* FROM TSMPE00 t WHERE READY_BILL_NO = @ready_bill_no AND CONFM_STATUS = '3' FOR UPDATE NOWAIT "
				);

			sqlstr3 = CString(
				" UPDATE	TSMPE00                                  "
				"               SET		REC_REVISOR		= @c_user,   "
				"                       REC_REVISE_TIME = @datetime, "
				"                       CONFM_STATUS	= '4' "
				"                 WHERE CONFM_PLAN_NO = @confm_plan_no AND CONFM_STATUS = '3' "
				);


			sqlstr4 = CString(
				" UPDATE	TSMPE02                        "
				"    SET		REC_REVISOR		= @c_user, "
				"	            REC_REVISE_TIME	= @datetime, "
				"	            CONFM_TIME		= @datetime, "
				"		        BILL_CONFM_DATE	= @bill_confm_date, "
				"		        CONFM_SHIFT		= @confm_shift, "
				"		        CONFM_GROUP 	= @confm_group, "
				"		        CONFM_MAKER 	= @c_user, "
				"		        CONFM_STATUS 	= '4' "
				"  WHERE CONFM_PLAN_NO     = @confm_plan_no AND CONFM_STATUS = '3' "
				);

			sqlstr5 = CString(
				" UPDATE	TSMPE01                                  "
				"               SET		REC_REVISOR		= @c_user,   "
				"                       REC_REVISE_TIME = @datetime, "
				"                       CONFM_STATUS	= '4' "
				"                 WHERE CONFM_PLAN_NO = @confm_plan_no AND CONFM_STATUS = '3' "
				);

			sqlstr6 = CString(

				" SELECT * FROM TSMPE02  WHERE READY_BILL_NO = @ready_bill_no  "
				);

			break;
		}

		/**************** 按计划确认 **********************/
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			c_confm_plan_no = bcls_rec->Tables[0].Rows[i]["confm_plan_no"].ToString().TrimOrBlank();

			if (0 == c_confm_plan_no.Compare(" "))
			{
				break;
			}

			/* ***** 执行SQL   ***** */
			/* ***** 修改单据状态   ***** */
			//	//EDLog(1,1,"[%d],c_confm_plan_no=[%s]", i, (const char*)c_confm_plan_no);  
			/* ***** 判断状态   ***** */
			sqlstr = sqlstr0;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteReader();

			while (execute_sql.Read())
			{
				tsmpe01["CONFM_STATUS"] = execute_sql.GetString(1);

				if (tsmpe01["CONFM_STATUS"].ToString().Compare("4") != 0 && tsmpe01["CONFM_STATUS"].ToString().Compare("3") != 0)
				{
					{CFormattable arguments[] =
					{ tsmpe01["CONFM_PLAN_NO"].ToString(), tsmpe01["CONFM_STATUS"].ToString() }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000851")/*计划号[{0}]的准发状态为[{1}]， 不能确认*/, arguments, 2);
					}
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			execute_sql.Close();

			/* *****  获取单据信息  ***** */
			sqlstr = sqlstr1;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);

			execute_sql.ExecuteReader();
			while (execute_sql.Read())
			{
				execute_sql.Fetch(tsmpe00);
				/* ***** 保存行信息到ds_mat_no.TABLES   ***** */
				tsmpe00.MergeTo(ds_ready_bill_no.Tables[0], false);

			}
			execute_sql.Close();

			/* ***** 修改单据状态   ***** */
			sqlstr = sqlstr3;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();


			/* ***** 修改材料状态   ***** */
			sqlstr = sqlstr4;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.Parameters.Set("bill_confm_date", datetime.Substring(1, 8));
			execute_sql.Parameters.Set("confm_shift", c_confm_shift);
			execute_sql.Parameters.Set("confm_group", c_confm_group);
			execute_sql.ExecuteNonQuery();

			//		/* ***** 修改计划状态   ***** */  
			sqlstr = sqlstr5;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);
			execute_sql.ExecuteNonQuery();

		}

		// 根据配置读取电文号 13801 2015-9-2
		sqlstr = "SELECT CODE_DESC_2_CONTENT FROM TEP0002 "
			" WHERE CODE_CLASS = 'SM00' AND CODE_DESC_5_CONTENT = '1' AND CODE = @tc_no ";
		cmd_inq.Parameters.Set("tc_no", c_tc_no);
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			c_tc_no = cmd_inq.GetString(1);
		}
		cmd_inq.Close();


		/**************** 将确认的单据发送确认电文给L4 **********************/
		for (int i = 0; i < ds_ready_bill_no.Tables[0].Rows.get_Count(); i++)
		{
			CONFM_PLAN_NO = ds_ready_bill_no.Tables[0].Rows[i]["confm_plan_no"].ToString().TrimOrBlank();
			READY_BILL_NO = ds_ready_bill_no.Tables[0].Rows[i]["ready_bill_no"].ToString().TrimOrBlank();
			REC_REVISOR = ds_ready_bill_no.Tables[0].Rows[i]["rec_revisor"].ToString().TrimOrBlank();
			REC_REVISE_TIME = ds_ready_bill_no.Tables[0].Rows[i]["rec_revise_time"].ToString().TrimOrBlank();

			if (READY_BILL_NO.Compare(" ") == 0)
			{
				break;
			}

			/* ***** 电文初始化  ***** */
			ret = 0;
			ret = epex.Initialize(c_tc_no);
			if (ret != 0)
			{
				sprintf(s.msg, epex.GetMsg());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* ***** 电文赋值  ***** */
			ret = 0;
			ret = epex.SetValue("CONFM_PLAN_NO", 0, CONFM_PLAN_NO);
			ret = epex.SetValue("READY_BILL_NO", 0, READY_BILL_NO);
			ret = epex.SetValue("REC_REVISOR", 0, REC_REVISOR);
			ret = epex.SetValue("REC_REVISE_TIME", 0, REC_REVISE_TIME);
			if (ret != 0)
			{
				sprintf(s.msg, epex.GetMsg());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* ***** 执行SQL   ***** */
			sqlstr = sqlstr6;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("ready_bill_no", READY_BILL_NO);

			execute_sql.ExecuteReader();
			j = 0;
			while (execute_sql.Read())
			{
				execute_sql.Fetch(tsmpe02);
				//
				if (f_sm00_mm99(tsmpe02["MAT_NO"].ToString(), 3, 5, s.msg, conn) != 0)
				{
					doFlag = -1;
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				//	调用封锁检查
				bcls_rec->Tables[hold_flag].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
				bcls_rec->Tables[hold_flag].Rows[0]["table_name"] = "TMMSM01";
				ret = 0;
				ret = f_sm00_hold_flag_chk(bcls_rec, bcls_ret, conn);
				if (ret < 0)
				{
					Log::Debug("", __FUNCTION__, "f_sm00_hold_flag_chk函数调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				bcls_rec->Tables[record_name].Rows[0]["mat_no"] = tsmpe02["MAT_NO"];
				bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "4";
				bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

				ret = 0;
				ret = f_sm00_record(bcls_rec, bcls_ret, conn);
				if (ret < 0)
				{
					Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* ***** 组织材料信息  ***** */
				if (epex.SetValue("MAT_NO", j, tsmpe02["MAT_NO"].ToString()) < 0)
				{
					sprintf(s.msg, epex.GetMsg());
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				j++;
				//
			}
			execute_sql.Close();

			//发送电文
			ret = epex.SendTele();					// 调函数
			if (ret != 0)
			{
				sprintf(s.msg, epex.GetMsg());
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			/* 释放 */
			epex.Uninitialize();


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

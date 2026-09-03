/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm01_del
*  程序描述			: 热轧准发吊销
*  备注说明			:
*  修改历史			:
* 2011-12-15 	  吴 新			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include "stdafx.h"



 
#include "epex.h"

int f_sm00_record(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn);
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_smsm01_del(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);

BM2F_ENTERACE(smsm01_del)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm01_del(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, ret = 0;
	CString	record_name = "sm00_record";

	CModel tsmpe02("TSMPE02");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_del_cause = " ", c_del_type = " ", c_del_code = " ";
	CString c_confm_plan_no = " ", c_confm_status = " ", c_mat_no = " ", c_ready_bill_no = " ";
	CString CONFM_PLAN_NO, READY_BILL_NO, MAT_NO, DEL_MAKER, RED_CAUSE_CODE, RED_CAUSE_DESC, DEL_TIME;

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5(""), sqlstr6("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	/* *******定义一个EIClass object */
	EIClass ds_mat_no;


	/* ***** 创建电文处理对象 ***** */
	EPEX epex(&s);
	CString c_tc_no = "2000S2";


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
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		/* ***** 获取前台参数  ***** */
		c_del_type = bcls_rec->Tables[0].Rows[0]["del_type"].ToString().TrimOrBlank();
		c_del_code = bcls_rec->Tables[0].Rows[0]["red_cause_code"].ToString().TrimOrBlank();
		c_del_cause = bcls_rec->Tables[0].Rows[0]["red_cause_content"].ToString().TrimOrBlank();

		/* ***** 判断删除类型  ***** */
		if (c_del_type.Compare("P") != 0 && c_del_type.Compare("B") != 0 && c_del_type.Compare("M") != 0)
		{
			doFlag = -1;
			{
				CFormattable arguments[] = { c_del_type }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0001455")/*吊销类型[{0}]无法识别。*/, arguments, 1);
			}
		}

		/* ***** format sql ****** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:
			sqlstr1 = CString(
				" SELECT t.* FROM tsmpe02 t WHERE CONFM_PLAN_NO = @confm_plan_no FOR UPDATE NOWAIT "
				);

			sqlstr2 = CString(
				" SELECT t.* FROM tsmpe02 t WHERE READY_BILL_NO = @ready_bill_no FOR UPDATE NOWAIT "
				);

			sqlstr3 = CString(
				" SELECT t.* FROM tsmpe02 t WHERE MAT_NO = @mat_no FOR UPDATE NOWAIT "
				);

			sqlstr4 = CString(
				" DELETE FROM tsmpe02 WHERE CONFM_PLAN_NO = @confm_plan_no "
				);

			sqlstr5 = CString(
				" DELETE FROM tsmpe02 WHERE READY_BILL_NO = @ready_bill_no "
				);

			sqlstr6 = CString(
				" DELETE FROM tsmpe02 WHERE MAT_NO = @mat_no "
				);

			break;
		}

		/**************** 按计划删除 **********************/
		if (c_del_type.Compare("P") == 0)
		{
			for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{
				c_confm_plan_no = bcls_rec->Tables[0].Rows[i]["confm_plan_no"].ToString().TrimOrBlank();
				/* ***** 执行SQL   ***** */
				sqlstr = sqlstr1;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("confm_plan_no", c_confm_plan_no);

				execute_sql.ExecuteReader();
				while (execute_sql.Read())
				{
					execute_sql.Fetch(tsmpe02);
					/* ***** 判断材料状态   ***** */
					if (tsmpe02["CONFM_STATUS"].ToString().Compare("2") != 0 && tsmpe02["CONFM_STATUS"].ToString().Compare("3") != 0)
					{
						doFlag = -1;
						{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
						CMessageFormat::Format(s.msg, _RES("SM00S0000835")/*材料[{0}]状态不能吊销.*/, arguments, 1);
						}
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/* ***** 保存行信息到ds_mat_no.TABLES   ***** */
					tsmpe02.MergeTo(ds_mat_no.Tables[0], false);
				}
				execute_sql.Close();
				/* ***** 删除材料   ***** */
				//			sqlstr = sqlstr4;
				//			execute_sql.SetCommandText( sqlstr ); 
				//			execute_sql.Parameters.Set( "confm_plan_no" , c_confm_plan_no );
				//			execute_sql.ExecuteNonQuery(); 

				//更新准发计划表,准发单据表计划重量 材料总重量
				//doFlag = f_sm00_count(tsmpe02["CONFM_PLAN_NO"].ToString(), c_user,conn);
				//if (doFlag < 0)
				//{ 
				// doFlag = -1;
				// sprintf(s.msg ,"f_sm00_count函数调用出错.");
				// throw CApplicationException(-1, s.msg, s.svc_name);
				//} 

			}
		}
		/**************** 按单据删除 **********************/
		if (c_del_type.Compare("B") == 0)
		{
			for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{
				c_ready_bill_no = bcls_rec->Tables[0].Rows[i]["ready_bill_no"].ToString().TrimOrBlank();
				/* ***** 执行SQL   ***** */
				sqlstr = sqlstr2;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("ready_bill_no", c_ready_bill_no);

				execute_sql.ExecuteReader();
				while (execute_sql.Read())
				{
					execute_sql.Fetch(tsmpe02);
					/* ***** 判断材料状态   ***** */
					if (tsmpe02["CONFM_STATUS"].ToString().Compare("2") != 0 && tsmpe02["CONFM_STATUS"].ToString().Compare("3") != 0)
					{
						doFlag = -1;
						{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
						CMessageFormat::Format(s.msg, _RES("SM00S0000835")/*材料[{0}]状态不能吊销.*/, arguments, 1);
						}
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/* ***** 保存行信息到ds_mat_no.TABLES   ***** */
					tsmpe02.MergeTo(ds_mat_no.Tables[0], false);
				}
				execute_sql.Close();
				/* ***** 删除材料   ***** */
				//			sqlstr = sqlstr5;
				//			execute_sql.SetCommandText( sqlstr ); 
				//			execute_sql.Parameters.Set( "ready_bill_no" , c_ready_bill_no );
				//			execute_sql.ExecuteNonQuery(); 

				//更新准发计划表,准发单据表计划重量 材料总重量
				//doFlag = f_sm00_count(tsmpe02["CONFM_PLAN_NO"].ToString(), c_user,conn);
				//if (doFlag < 0)
				//{ 
				// doFlag = -1;
				// sprintf(s.msg ,"f_sm00_count函数调用出错.");
				// throw CApplicationException(-1, s.msg, s.svc_name);
				//} 

			}
		}

		/**************** 按材料删除 **********************/
		if (c_del_type.Compare("M") == 0)
		{
			for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{
				c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();
				/* ***** 执行SQL   ***** */
				sqlstr = sqlstr3;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("mat_no", c_mat_no);

				execute_sql.ExecuteReader();
				while (execute_sql.Read())
				{
					execute_sql.Fetch(tsmpe02);
					/* ***** 判断材料状态   ***** */
					//20130410 HYF 准发确认之前都可以吊销 if (tsmpe02["CONFM_STATUS"].ToString().Compare("2") != 0 && tsmpe02["CONFM_STATUS"].ToString().Compare("3") != 0 )
					if (tsmpe02["CONFM_STATUS"].ToString().Compare("3")>0)
					{
						doFlag = -1;
						{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
						CMessageFormat::Format(s.msg, _RES("SM00S0000835")/*材料[{0}]状态不能吊销.*/, arguments, 1);
						}
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					/* ***** 保存行信息到ds_mat_no.TABLES   ***** */
					tsmpe02.MergeTo(ds_mat_no.Tables[0], false);
				}
				execute_sql.Close();
				/* ***** 删除材料   ***** */
				//			sqlstr = sqlstr6;
				//			execute_sql.SetCommandText( sqlstr ); 
				//			execute_sql.Parameters.Set( "mat_no" , c_mat_no );
				//			execute_sql.ExecuteNonQuery(); 

				//更新准发计划表,准发单据表计划重量 材料总重量
				//doFlag = f_sm00_count(tsmpe02["CONFM_PLAN_NO"].ToString(), c_user,conn);
				//if (doFlag < 0)
				//{ 
				// doFlag = -1;
				// sprintf(s.msg ,"f_sm00_count函数调用出错.");
				// throw CApplicationException(-1, s.msg, s.svc_name);
				//} 

			}
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


		/**************** 将删除的材料发送吊销电文给L4 **********************/
		for (int i = 0; i < ds_mat_no.Tables[0].Rows.get_Count(); i++)
		{
			//获取电文初始值
			CONFM_PLAN_NO = ds_mat_no.Tables[0].Rows[i]["CONFM_PLAN_NO"].ToString().TrimOrBlank();
			READY_BILL_NO = ds_mat_no.Tables[0].Rows[i]["READY_BILL_NO"].ToString().TrimOrBlank();
			MAT_NO = ds_mat_no.Tables[0].Rows[i]["MAT_NO"].ToString().TrimOrBlank();
			DEL_MAKER = c_user;
			RED_CAUSE_CODE = c_del_code;
			RED_CAUSE_DESC = c_del_cause;
			DEL_TIME = datetime;

			if (MAT_NO.Compare(" ") == 0)
			{
				break;
			}


			/* ***** 电文初始化  ***** */
			ret = 0;
			ret = epex.Initialize(c_tc_no);
			if (ret != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* ***** 电文赋值  ***** */
			ret = 0;
			ret = epex.SetValue("CONFM_PLAN_NO", 0, CONFM_PLAN_NO);
			ret = epex.SetValue("READY_BILL_NO", 0, READY_BILL_NO);
			ret = epex.SetValue("MAT_NO", 0, MAT_NO);
			ret = epex.SetValue("DEL_MAKER", 0, DEL_MAKER);
			ret = epex.SetValue("RED_CAUSE_CODE", 0, RED_CAUSE_CODE);
			ret = epex.SetValue("RED_CAUSE_DESC", 0, RED_CAUSE_DESC);
			ret = epex.SetValue("DEL_TIME", 0, DEL_TIME);
			if (ret != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			//发送电文

			ret = epex.SendTele();					// 调函数
			if (ret != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			/* 释放 */
			epex.Uninitialize();

			if (f_sm00_mm99(MAT_NO, 3, -4, s.msg, conn) != 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			bcls_rec->Tables[record_name].Rows[0]["mat_no"] = MAT_NO;
			bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "1";
			bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

			ret = 0;
			ret = f_sm00_record(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			/* ***** 删除材料   ***** */
			sqlstr = sqlstr6;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("mat_no", MAT_NO);
			execute_sql.ExecuteNonQuery();

			//更新准发计划表,准发单据表计划重量 材料总重量
			doFlag = f_sm00_count(CONFM_PLAN_NO, c_user, conn);
			if (doFlag < 0)
			{
				doFlag = -1;
				sprintf(s.msg, "f_sm00_count函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
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

/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm01_red_req
*  程序描述			: 热轧准发红冲请求
*  备注说明			:
*  修改历史			:
*  		2008-8-8 	BM2IDE			(ADD)程序建立
*			... ...
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include <stdio.h>
#include "stdafx.h"



 


#include "epex.h"

int f_sm00_record(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
int f_epep_get_shift_group(CString pszShiftClass, CString pszShiftTime, CString pszShiftNo, CString pszShiftGroup, CDbConnection * conn);	// 生成班次、班组 
int f_sm00_mm99(CString pack_num, int para_type, int event_id, CString msg, CDbConnection * conn);	/* 抛物料跟踪打包函数 */
int f_smsm01_red_req(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);

BM2F_ENTERACE(smsm01_red_req)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm01_red_req(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, ret = 0;
	CString	record_name = "sm00_record";

	CModel tsmpe02("TSMPE02");
	CModel tsmpe00("TSMPE00");
	CModel tsmpe01("TSMPE01");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_del_cause = " ", c_del_type = " ", c_dept_code = " ", c_dept_code_cname = " ", c_red_cause_code = " ", c_red_cause_desc = " ";
	CString c_confm_plan_no = " ", c_ready_bill_no = " ", c_confm_status = " ", c_mat_no = " ", c_shift = " ", c_group = " ";
	char  c_confm_shift[5] = " ", c_confm_group[10] = " ";
	char  c_datetime[15];
	CString ORDER_NO, READY_BILL_NO, MAT_NO, RED_MAKER, RED_CAUSE_CODE, RED_CAUSE_DESC, DEPT_CODE, DEPT_CODE_CNAME, RED_TIME;

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr0(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5(""), sqlstr6("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_inq(conn);

	/* *******定义一个EIClass object */
	EIClass ds_ready_bill_no, ds_mat_no;

	/* ***** 创建电文处理对象 ***** */
	EPEX epex(&s);
	CString c_tc_no = "2000S3";


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

		/* ***** format sql ****** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:

			sqlstr0 = CString(
				" SELECT t.* FROM TSMPE02 t WHERE MAT_NO = @mat_no  FOR UPDATE NOWAIT "
				);

			sqlstr1 = CString(
				"	UPDATE	TSMPE02                                     "
				"               SET		REC_REVISE_TIME = @datetime,    "
				"                   	REC_REVISOR		= @c_user,      "
				"                       RED_FLAG		= '1', "
				"                       RED_CAUSE_CODE	= @red_cause_code, "
				"                       RED_CAUSE_DESC	= @red_cause_desc, "
				"                       DEPT_CODE		= @dept_code, "
				"                       DEPT_CODE_CNAME	= @dept_code_cname, "
				"                       RED_MAKER		= @c_user "
				"                       WHERE	MAT_NO = @mat_no "
				);

			sqlstr2 = CString(
				" SELECT 	CODE_DESC_1_CONTENT "
				"    FROM	TEP0002              "
				"   WHERE	CODE_CLASS = @code_class  "
				"    AND		CODE   = @code     "
				);


			break;
		}

		/**************** 按材料进行红冲请求 **********************/
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();
			c_red_cause_code = bcls_rec->Tables[0].Rows[i]["red_cause_code"].ToString().TrimOrBlank();
			c_red_cause_desc = bcls_rec->Tables[0].Rows[i]["red_cause_desc"].ToString().TrimOrBlank();
			c_dept_code = bcls_rec->Tables[0].Rows[i]["dept_code"].ToString().TrimOrBlank();
			c_dept_code_cname = bcls_rec->Tables[0].Rows[i]["dept_code_cname"].ToString().TrimOrBlank();

			if (0 == c_mat_no.Compare(" "))
			{
				break;
			}

			//EDLog(1,1,"[%d],c_mat_no[%s]", i, (const char*)c_mat_no);

			/* ***** 判断输入条件  ***** */
			if (0 == c_red_cause_code.Compare(" "))
			{
				doFlag = -1;
				{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000853")/*材料号[{0}]没有选择红冲原因*/, arguments, 1);
				}
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			if (0 == c_dept_code.Compare(" "))
			{
				doFlag = -11;
				{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
				CMessageFormat::Format(s.msg, _RES("SM00S0000854")/*材料号[{0}]没有选择红冲请求部门*/, arguments, 1);
				}
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			/* ***** 执行SQL   ***** */
			/* ***** 判断状态   ***** */
			sqlstr = sqlstr0;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("mat_no", c_mat_no);
			execute_sql.ExecuteReader();
			while (execute_sql.Read())
			{
				execute_sql.Fetch(tsmpe02);
				/* ***** 判断材料是否已经红冲   ***** */
				if (0 == tsmpe02["RED_FLAG"].ToString().Compare("1"))
				{
					doFlag = -1;
					{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0001055")/*材料号[{0}]已红冲请求,不能再请求*/, arguments, 1);
					}
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* ***** 判断材料状态   ***** */
				//HYF 20130409 准发状态读取的是SM28代码，其中4是准发计划确认，因此这里判断2和3是不对的 if (tsmpe02["CONFM_STATUS"].ToString().Compare("3") != 0 && tsmpe02["CONFM_STATUS"].ToString().Compare("2") != 0 )

				if (tsmpe02["CONFM_STATUS"].ToString().Compare("4") != 0)
				{
					doFlag = -1;
					{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0001056")/*材料号[{0}]未准发确认,不能红冲*/, arguments, 1);
					}
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				tsmpe02.MergeTo(ds_mat_no.Tables[0], false);
			}
			execute_sql.Close();

			/* ***** 修改材料状态   ***** */
			sqlstr = sqlstr1;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.Parameters.Set("mat_no", c_mat_no);
			execute_sql.Parameters.Set("red_cause_code", c_red_cause_code);
			execute_sql.Parameters.Set("red_cause_desc", c_red_cause_desc);
			execute_sql.Parameters.Set("dept_code", c_dept_code);
			execute_sql.Parameters.Set("dept_code_cname", c_dept_code_cname);
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
		for (int i = 0; i < ds_mat_no.Tables[0].Rows.get_Count(); i++)
		{
			//获取电文初始值
			ORDER_NO = ds_mat_no.Tables[0].Rows[i]["ORDER_NO"].ToString().TrimOrBlank();
			READY_BILL_NO = ds_mat_no.Tables[0].Rows[i]["READY_BILL_NO"].ToString().TrimOrBlank();
			MAT_NO = ds_mat_no.Tables[0].Rows[i]["MAT_NO"].ToString().TrimOrBlank();
			RED_MAKER = c_user;
			RED_CAUSE_CODE = c_red_cause_code;
			RED_CAUSE_DESC = c_red_cause_desc;
			DEPT_CODE = c_dept_code;
			DEPT_CODE_CNAME = c_dept_code_cname;
			RED_TIME = datetime;

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
			ret = epex.SetValue("ORDER_NO", 0, ORDER_NO);
			ret = epex.SetValue("READY_BILL_NO", 0, READY_BILL_NO);
			ret = epex.SetValue("MAT_NO", 0, MAT_NO);
			ret = epex.SetValue("RED_MAKER", 0, RED_MAKER);
			ret = epex.SetValue("RED_CAUSE_CODE", 0, RED_CAUSE_CODE);
			ret = epex.SetValue("RED_CAUSE_DESC", 0, RED_CAUSE_DESC);
			ret = epex.SetValue("DEPT_CODE", 0, DEPT_CODE);
			ret = epex.SetValue("DEPT_CODE_CNAME", 0, DEPT_CODE_CNAME);
			ret = epex.SetValue("RED_TIME", 0, RED_TIME);
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

			//if(f_sm00_mm99(MAT_NO , 3, -4, s.msg,conn) != 0)
			//{ 
			//	throw CApplicationException(-1, s.msg, s.svc_name);
			//} 
			bcls_rec->Tables[record_name].Rows[0]["mat_no"] = MAT_NO;
			bcls_rec->Tables[record_name].Rows[0]["event_mark"] = "2";
			bcls_rec->Tables[record_name].Rows[0]["userid"] = c_user;

			ret = 0;
			ret = f_sm00_record(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				Log::Debug("", __FUNCTION__, "f_sm00_record函数调用出错.");
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

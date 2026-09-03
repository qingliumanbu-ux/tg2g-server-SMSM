/****************************************************************
*	程序功能：	产成品出厂处理……下行车指令					*
*	程序调用：	由画面 smsm02_req 的 F3 功能调用					*
*	编制日期：	2011-12-26  wuxin								*
*	编制人员：	wuxin											*
*	传入参数：	块1,选择的多记录材料信息						*
块2,提单号										*
*	返回参数：	0－成功，1－异常								*
*	出错描述：	s.msg											*
*****************************************************************
*	行车命令生成参数：	mat_no  材料号 comd_div 吊车类型(E)		*
*						块名 YMCMD								*
****************************************************************/
#include <stdio.h>
#include "stdafx.h"






//#include "x3000s2.h"

//int f_ymhrc_cmdmk_chg(EIClass * bcls_rec, EIClass * bcls_ret,CDbConnection * conn);		// 热轧钢卷库行车命令生成
int f_smsm02_req(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);

BM2F_ENTERACE(smsm02_req)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm02_req(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int fetchRowCount = 0;
	int row_count = 0, ret = 0;

	CModel tsmpe02("TSMPE02");
	CModel tsmpe00("TSMPE00");
	CModel tsmpe01("TSMPE01");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	//CString c_stock_no=" ",c_stock_no_end=" ",c_query_type=" ";

	CString c_del_cause = " ", c_del_type = " ", c_dept_code = " ", c_dept_code_cname = " ", c_red_cause_code = " ", c_red_cause_desc = " ";
	CString c_confm_plan_no = " ", c_ready_bill_no = " ", c_confm_status = " ", c_mat_no = " ", c_confm_shift = " ", c_confm_group = " ";
	CString c_car_name = " ", c_fin_pos = " ", c_stock_place_no_to = " ";

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr0(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5(""), sqlstr6("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);

	/* *******定义一个EIClass object */
	EIClass ds_ready_bill_no, ds_mat_no;

	ds_mat_no.Tables[0].Columns.Add(DT_STRING, "mat_no");                   // 材料号
	ds_mat_no.Tables[0].Columns.Add(DT_STRING, "crane_inst_type");          // 吊车命令类别
	ds_mat_no.Tables[0].Columns.Add(DT_STRING, "stock_place_no_to");        // 目标垛位号
	ds_mat_no.Tables[0].Columns.Add(DT_STRING, "fin_pos");                  // 最终位置
	ds_mat_no.Tables[0].Columns.Add(DT_STRING, "car_name");                 // 卡车名

	/* ***** 应用程序开始处理 ***** */
	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		c_stock_place_no_to = bcls_rec->Tables[1].Rows[0]["pile_no"].ToString().TrimOrBlank();
		c_car_name = bcls_rec->Tables[1].Rows[0]["vehicle_no"].ToString().TrimOrBlank();

		/* ***** format sql ****** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:

			sqlstr1 = CString(
				" SELECT t.* from tsmpe02 t where mat_no = @mat_no  for update nowait "
				);

			sqlstr2 = CString(
				"	UPDATE	tsmpe02                                     "
				"               SET		rec_revise_time = @datetime,    "
				"                   	rec_revisor		= @c_user,      "
				"                       out_mark		= '1' "
				"                      WHERE	mat_no = @mat_no "
				);

			break;
		}

		/**************** 按材料进行红冲请求 **********************/
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();


			if (0 == c_mat_no.Compare(" "))
			{
				break;
			}

			//EDLog(1,1,"[%d],c_mat_no[%s]", i, (const char*)c_mat_no); 

			/* ***** 执行SQL   ***** */
			/* ***** 判断状态   ***** */
			sqlstr = sqlstr1;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("mat_no", c_mat_no);
			execute_sql.ExecuteReader();
			while (execute_sql.Read())
			{
				execute_sql.Fetch(tsmpe02);
				/* ***** 判断材料是否已经红冲   ***** */
				if (0 == tsmpe02["OUT_MARK"].ToString().Compare("1"))
				{
					doFlag = -1;
					{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000671")/*此材料[{0}]已经发送行车请求*/, arguments, 1);
					}
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				if (0 == tsmpe02["OUT_MARK"].ToString().Compare("2"))
				{
					doFlag = -1;
					{CFormattable arguments[] = { tsmpe02["MAT_NO"].ToString() }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000672")/*此材料[{0}]已经执行行车指令*/, arguments, 1);
					}
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				//  tsmpe02.MergeTo(ds_mat_no.Tables[0], false);   
			}
			execute_sql.Close();

			/* ***** 修改材料状态   ***** */
			sqlstr = sqlstr2;
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.Parameters.Set("mat_no", c_mat_no);
			execute_sql.ExecuteNonQuery();

			/* ***** 调用吊车命令形成函数   ***** */
			ret = 0;
			ds_mat_no.Tables[0].Rows.Add();
			ds_mat_no.Tables[0].Rows[i]["mat_no"] = c_mat_no;
			ds_mat_no.Tables[0].Rows[i]["crane_inst_type"] = "7";
			ds_mat_no.Tables[0].Rows[i]["stock_place_no_to"] = c_stock_place_no_to;
			//	ds_mat_no.Tables[0].Rows[i]["fin_pos"] = c_fin_pos;
			ds_mat_no.Tables[0].Rows[i]["car_name"] = c_car_name;

			//		ret = f_ymhrc_cmdmk_chg(&ds_mat_no , &bcls_ret);
			//		if	(ret	!=	0)
			//		{ 
			//			{CFormattable arguments[] = { str }; // 定义参数列表的数组
			//			 CMessageFormat::Format (s.msg , _RES("SM00S0000008")/*调用仓库行车命令未成功!,{0}*/ , arguments,1);
			//			}
			//		  throw CApplicationException(-1, s.msg, s.svc_name);
			//		}

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

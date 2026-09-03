/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: smsm02_ins
*  程序描述			: 出厂实绩输入
*  备注说明			:
*  修改历史			:
*  		2012-05-16 	BM2IDE			(ADD)程序建立
*	2014-03-07	增加直接调用仓库出库的接口函数
* **************************************************************************** */
/***** C/C++ 的标准头文件部分 *****/
#include <stdio.h>
#include "stdafx.h"









//int f_sm00_bill_deliver(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);
//int f_ymsm_ym02(EIClass *bcls_rec, EIClass *bcls_ret, CDbConnection * conn);	// 炼钢后备出库处理

BM2F_ENTERACE(smsm02_ins)
/* ***** -EP_SYSTEM_HEAD_END ***** */
int f_smsm02_ins(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);

	/* ***** 静态变量定义 ***** */
	int doFlag = 0;
	int row_count = 0, ret = 0;
	int	fetchRowCount = 0;

	CModel tsmpe02("TSMPE02");
	CModel tsmpe00("TSMPE00");
	CModel tsmpe01("TSMPE01");
	CModel tsmpe10("TSMPE10");
	CModel tsmpe11("TSMPE11");

	/* ***** 程序变量 ***** */
	CString c_user = s.userid, c_mat_kind = " ", datetime = " ";
	CString c_crane_mark = " ";
	CString	block_name = "YM02";

	CString c_out_mark = " ";
	CString c_mat_no = " ";
	CString c_bill_of_lading_no = " ", c_vehicle_no = " ", c_stock_no = " ", c_proc_type = " ", c_act_datetime = " ";

	CString  c_delivy_shift = " ", c_delivy_group = " ";
	char  c_datetime[15];

	/* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""), sqlstr0(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5(""), sqlstr6(""), sqlstr7("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand execute_sql(conn);
	CDbCommand cmd_sql(conn);

	/* *******定义一个EIClass object */
	EIClass bcls_rec_ym02;


	/* ***** 应用程序开始处理 ***** */
	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
		if (!bcls_rec_ym02.Tables.Contains(block_name))
		{
			bcls_rec_ym02.Tables[0].set_TableName(block_name);
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "MAT_NO");					// 材料号
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");		// 库操作指示 2E
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "STOCK_PLACE_NO_FROM");	// 原库位
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "STOCK_PLACE_NO_TO");		// 目标垛位号
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "STOCK_LAYER_TO");			// 最终位置 空
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "CAR_NO");                 // 卡车名
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "STOCK_CHNG_MODE");		// 空
			bcls_rec_ym02.Tables[block_name].Columns.Add(DT_STRING, "LAYERNO");			// 最终位置 空
		}

		//    c_bill_of_lading_no  = bcls_rec->Tables[0].Rows[0]["bill_of_lading_no"].ToString().TrimOrBlank();
		c_vehicle_no = bcls_rec->Tables[0].Rows[0]["vehicle_no"].ToString().TrimOrBlank();
		c_stock_no = bcls_rec->Tables[0].Rows[0]["stock_no"].ToString().TrimOrBlank();
		c_proc_type = bcls_rec->Tables[0].Rows[0]["proc_type"].ToString().TrimOrBlank();
		c_act_datetime = bcls_rec->Tables[0].Rows[0]["ACT_DATETIME"].ToString().TrimOrBlank();

		Log::Info("", __FUNCTION__, "c_vehicle_no =[{0}]", c_vehicle_no);
		Log::Info("", __FUNCTION__, "c_proc_type =[{0}]", c_proc_type);
		Log::Info("", __FUNCTION__, "c_act_datetime =[{0}]", c_act_datetime);
		Log::Info("", __FUNCTION__, "c_stock_no =[{0}]", c_stock_no);

		if (c_vehicle_no == " ")
		{
			strcpy(s.msg, _RES("SM00C0000022")/*请输入车号.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_act_datetime == " ") c_act_datetime = datetime;

		strcpy(c_datetime, (const char *)c_act_datetime);

		// 调用函数生成班次、班组
		f_epep_get_shift_group("DEFAULT", c_act_datetime, c_delivy_shift, c_delivy_group, conn);					// 调函数 

		/* ***** format sql ****** */
		switch (conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default:

			sqlstr1 = CString(
				" SELECT count(1) from tsmpe02 t where bill_of_lading_no = @bill_of_lading_no and confm_status = '6' "
				);

			sqlstr2 = CString(
				"		UPDATE tsmpe02 SET                          "
				"  rec_revise_time = @datetime, "
				"  rec_revisor     = @c_user,       "
				"  delivy_time     = rpad(@delivy_time,14,0), "
				"  delivy_shift    = @delivy_shift, "
				"  delivy_group    = @delivy_group, "
				"  delivy_maker    = @c_user,      "
				"  vehicle_no      = @vehicle_no,    "
				"  confm_status    = '9'             "
				"  WHERE bill_of_lading_no = @bill_of_lading_no and confm_status = '6'"
				);

			sqlstr3 = CString(
				" SELECT count(1) from tsmpe02 t WHERE mat_no = @mat_no and confm_status = '6' "
				);

			sqlstr4 = CString(
				"		UPDATE tsmpe02 SET                          "
				"  rec_revise_time = @datetime, "
				"  rec_revisor     = @c_user,       "
				"  delivy_time     = rpad(@delivy_time,14,0), "
				"  delivy_shift    = @delivy_shift, "
				"  delivy_group    = @delivy_group, "
				"  delivy_maker    = @c_user,      "
				"  vehicle_no      = @vehicle_no,    "
				"  confm_status    = '9'             "
				"  WHERE mat_no = @mat_no and confm_status = '6'"
				);

			sqlstr5 = CString(
				" SELECT count(1) from tsmpe02 "
				" where bill_of_lading_no = @bill_of_lading_no "
				" and	out_mark <> '2' "
				" and	confm_status = '6' "
				);

			sqlstr6 = CString(
				" SELECT out_mark , mat_kind from tsmpe02 where mat_no = @mat_no "
				);

			sqlstr0 = " SELECT CRANE_MARK FROM TSI0021 WHERE stock_no = @c_stock_no ";

			sqlstr7 = " SELECT MAT_NO , MAT_KIND FROM TSMPE02 "
				" WHERE bill_of_lading_no = @bill_of_lading_no "
				" and confm_status = '6' AND out_mark NOT IN ('1' , '2') ";

			break;
		}

		/* 读取行车标记 */
		sqlstr = sqlstr0;
		cmd_sql.SetCommandText(sqlstr);
		cmd_sql.Parameters.Clear();
		cmd_sql.Parameters.Set("c_stock_no", c_stock_no);
		cmd_sql.ExecuteReader();
		if (cmd_sql.Read())
		{
			c_crane_mark = cmd_sql.GetString(1);
		}
		cmd_sql.Close();

		/* ***** 按提单输入实绩   ***** */
		if (c_proc_type == "B")
		{
			for (int i = 0; i < bcls_rec->Tables[1].Rows.get_Count(); i++)
			{
				c_bill_of_lading_no = bcls_rec->Tables[1].Rows[i]["bill_of_lading_no"].ToString().TrimOrBlank();

				if (c_bill_of_lading_no.Trim() == "")
				{
					break;
				}
				/* ***** 检查该提单下的材料是否合法  ***** */
				sqlstr = sqlstr1;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				row_count = execute_sql.ExecuteScalar().ToInt32();


				if (0 == row_count)
				{
					CFormattable arguments[] = { c_bill_of_lading_no }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000015")/*单据号[{0}]提单号[{1}]下没有可以处理的材料.*/, arguments, 2);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* ***** 检查该提单下的材料是否合法  ***** */
				sqlstr = sqlstr5;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				row_count = execute_sql.ExecuteScalar().ToInt32();

				if (c_crane_mark == "1")
				{
					if (row_count > 0)
					{
						CFormattable arguments[] = { c_bill_of_lading_no, row_count }; // 定义参数列表的数组
						CMessageFormat::Format(s.msg, "该提单[{0}]下有材料还没有出库或做行车命令", arguments, 1);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
				}

				/********** 2014-03-07 down **********/
				fetchRowCount = 0;
				bcls_rec_ym02.Tables[block_name].Rows.Clear();
				sqlstr = sqlstr7;
				cmd_sql.SetCommandText(sqlstr);
				cmd_sql.Parameters.Clear();
				cmd_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				cmd_sql.ExecuteReader();
				while (cmd_sql.Read())
				{
					c_mat_no = cmd_sql.GetString(1);
					c_mat_kind = cmd_sql.GetString(2);
					bcls_rec_ym02.Tables[block_name].Rows.Add();
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["MAT_NO"] = c_mat_no;	// 材料号
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["STOCK_OPER_ORDER"] = "2E";		// 库操作指示 2E
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["STOCK_PLACE_NO_FROM"] = " ";	// 原库位
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["STOCK_PLACE_NO_TO"] = " ";		// 目标垛位号
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["STOCK_LAYER_TO"] = " ";			// 最终位置 空
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["CAR_NO"] = c_vehicle_no;	// 卡车名
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["STOCK_CHNG_MODE"] = " ";		// 空
					bcls_rec_ym02.Tables[block_name].Rows[fetchRowCount]["LAYERNO"] = " ";		// 空

					fetchRowCount++;
				}
				cmd_sql.Close();

				//if	(c_mat_kind == "SM")	ret = f_ymsm_ym02(&bcls_rec_ym02 , bcls_ret , conn);
				if (ret != 0)
				{
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				row_count = 0;	// 当增加了这段就不用判下面的

				/********** 2014-03-07 up **********/

				if (row_count > 0)
				{
					CFormattable arguments[] = { c_bill_of_lading_no, row_count }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0001493")/*该提单下有未出库的材料.*/, arguments, 2);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* ***** 修改发货材料的提单号、卡车号、  ***** */
				sqlstr = sqlstr2;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("datetime", datetime);
				execute_sql.Parameters.Set("c_user", c_user);
				execute_sql.Parameters.Set("delivy_time", c_act_datetime);
				execute_sql.Parameters.Set("delivy_shift", c_delivy_shift);
				execute_sql.Parameters.Set("delivy_group", c_delivy_group);
				execute_sql.Parameters.Set("c_user", c_user);
				execute_sql.Parameters.Set("vehicle_no", c_vehicle_no);
				execute_sql.Parameters.Set("bill_of_lading_no", c_bill_of_lading_no);
				execute_sql.ExecuteNonQuery();

			}
			/* ***** 调用发货处理函数  ***** */
			ret = 0;
			//ret = f_sm00_bill_deliver(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

		}

		/* ***** 按材料输入实绩   ***** */
		if (c_proc_type == "M")
		{
			for (int i = 0; i < bcls_rec->Tables[1].Rows.get_Count(); i++)
			{
				c_mat_no = bcls_rec->Tables[1].Rows[i]["mat_no"].ToString().TrimOrBlank();
				c_bill_of_lading_no = bcls_rec->Tables[1].Rows[i]["bill_of_lading_no"].ToString().TrimOrBlank();

				Log::Info("", __FUNCTION__, "[{0}],c_mat_no=[{1}]", i, c_mat_no);

				if (c_mat_no == " ")
				{
					break;
				}

				/* ***** 检查发货材料是否合法  ***** */
				sqlstr = sqlstr3;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				row_count = execute_sql.ExecuteScalar().ToInt32();

				if (0 == row_count)
				{
					CFormattable arguments[] = { c_mat_no }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0000015")/*单据号[{0}]提单号[{1}]下没有可以处理的材料.*/, arguments, 2);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* ***** 检查发货材料是否完成出库  ***** */
				sqlstr = sqlstr6;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				execute_sql.ExecuteReader();

				if (execute_sql.Read())
				{
					c_out_mark = execute_sql.GetString(1);
					c_mat_kind = execute_sql.GetString(2);
				}
				c_out_mark = c_out_mark.TrimOrBlank();
				execute_sql.Close();
				Log::Debug("", __FUNCTION__, "c_out_mark=[{0}]", c_out_mark);

				// 判此仓库是否需要发送行车指令,且是否已经发送了行车指令
				if (c_crane_mark == "1" && c_out_mark == "1")
				{
					CFormattable arguments[] = { c_mat_no };
					CMessageFormat::Format(s.msg, "此仓库需要发送行车命令，此材料[{0}]已经发送行车命令", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				if (c_crane_mark == "1" && c_out_mark != "2")
				{
					CFormattable arguments[] = { c_mat_no }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, _RES("SM00S0001405")/*此仓库需要发送行车命令，此材料{0}还没有到发货口.*/, arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
				/* 2014-03-07 down */
				if (c_out_mark != "2")
				{
					bcls_rec_ym02.Tables[block_name].Rows.Clear();
					bcls_rec_ym02.Tables[block_name].Rows.Add();
					bcls_rec_ym02.Tables[block_name].Rows[0]["MAT_NO"] = c_mat_no;	// 材料号
					bcls_rec_ym02.Tables[block_name].Rows[0]["STOCK_OPER_ORDER"] = "2E";		// 库操作指示 2E
					bcls_rec_ym02.Tables[block_name].Rows[0]["STOCK_PLACE_NO_FROM"] = " ";	// 原库位
					bcls_rec_ym02.Tables[block_name].Rows[0]["STOCK_PLACE_NO_TO"] = " ";		// 目标垛位号
					bcls_rec_ym02.Tables[block_name].Rows[0]["STOCK_LAYER_TO"] = " ";			// 最终位置 空
					bcls_rec_ym02.Tables[block_name].Rows[0]["CAR_NO"] = c_vehicle_no;	// 卡车名
					bcls_rec_ym02.Tables[block_name].Rows[0]["STOCK_CHNG_MODE"] = " ";		// 空

					//if	(c_mat_kind == "SM")	ret = f_ymsm_ym02(&bcls_rec_ym02 , bcls_ret , conn);
					if (ret != 0)
					{
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
					c_out_mark = "2";
				}
				/* 2014-03-07 up */
				if (c_out_mark != "2")
				{
					CFormattable arguments[] = { c_mat_no }; // 定义参数列表的数组
					CMessageFormat::Format(s.msg, "此材料{0}还没有到发货口", arguments, 1);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* ***** 修改发货材料的提单号、卡车号、  ***** */
				sqlstr = sqlstr4;
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("datetime", datetime);
				execute_sql.Parameters.Set("c_user", c_user);
				execute_sql.Parameters.Set("delivy_time", c_act_datetime);
				execute_sql.Parameters.Set("delivy_shift", c_delivy_shift);
				execute_sql.Parameters.Set("delivy_group", c_delivy_group);
				execute_sql.Parameters.Set("c_user", c_user);
				execute_sql.Parameters.Set("vehicle_no", c_vehicle_no);
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				execute_sql.ExecuteNonQuery();

			}
			/* ***** 调用发货处理函数  ***** */
			ret = 0;
			//ret = f_sm00_bill_deliver(bcls_rec, bcls_ret, conn);
			if (ret < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

		/* ***** 按材料输入实绩   ***** */
		if (0 != c_proc_type.Compare("M") && 0 != c_proc_type.Compare("B"))
		{
			sprintf(s.msg, "选择类型不符");
			throw CApplicationException(-1, s.msg, s.svc_name);
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

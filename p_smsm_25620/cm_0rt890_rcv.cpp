/*************************************************
Copyright: Baosight Software LTD.co Copyright (c) 2019
Author:      lizhen
Version:     1.0
Date:        2024-03-01
Description:
**************************************************/

//框架头文件
#include "stdafx.h"
#include "epex.h"

/*<remark>=========================================================
/// <summary>
/// 调拨应答
/// </summary>
/// <returns></returns>
===========================================================</remark>*/

//业务头文件
//记录发货履历
int f_smsm_trace(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
//物料跟踪
int f_mmsm99(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);




BM2F_ENTERACE_TELE(cm_0rt890_rcv)

int f_cm_0rt890_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__); 	//系统日志类定义

	/* 程序内部变量 */
	int doFlag = 0;
	int row_count = 0, i = 0, ret = 0;
	int blkNum = 0;
	int blkNum_pmol02 = 0;
	/* 业务变量 */
	CString	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	/* 业务变量 */
	CString c_user = " ", c_tc_no = " ", c_mat_kind = " ",  c_factory_div = " ", c_event_name = " ";
	CString plan_type = " ";//计划类型
	CString c_order_no, c_fin_user_name, c_goods_flag = "";
	CString    c_proc_type;
	CString    balance_user_name = "";

	int        i_plan_num = 0;
	int        i_mm99 = 0;
	CString    c_mat_no = "", c_bill_no = "";
	CDecimal out_stock_num = 0;
	CDecimal out_stock_wt = 0;
	CString prod_code = " ";
	/* 实体类定义 */
	CModel tmmsm01("TMMSM01");
	CModel tmmsm96("TMMSM96");
	CModel tsmsm01("TSMSM01");
	CModel tsmsm03("TSMSM03");
	CModel tsmsm09("TSMSM09");
	/* 数据库SQL操作字符串 */
	CString sqlstr;
	CDbCommand cmd_sql(conn);
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_code(conn);
	CDbCommand execute_sql(conn);




	try
	{
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		if (bcls_rec->Tables.IndexOf("SMSM09") < 0)
		{
			bcls_rec->Tables.Add("SMSM09");
			bcls_rec->Tables["SMSM09"].Columns.Add(tsmsm03);
			bcls_rec->Tables["SMSM09"].Columns.Add(DT_STRING, "event_id");
		}
		bcls_rec->Tables["SMSM09"].Rows.Clear();

		//虚拟转库发送离库电文
		if (!bcls_rec->Tables.Contains("SEND_MAT"))
		{
			bcls_rec->Tables.Add("SEND_MAT");
		}
		bcls_rec->Tables["SEND_MAT"].Rows.Clear();

		if (!bcls_rec->Tables.Contains("SEND"))
		{
			bcls_rec->Tables.Add("SEND");
			bcls_rec->Tables["SEND"].Columns.Add(DT_STRING, "MY_FLAG");
			bcls_rec->Tables["SEND"].Columns.Add(DT_STRING, "OUTPASS_NO");
			bcls_rec->Tables["SEND"].Columns.Add(DT_DECIMAL, "TOTAL_NUM");
			bcls_rec->Tables["SEND"].Columns.Add(DT_DECIMAL, "TOTAL_WT");
		}
		bcls_rec->Tables["SEND"].Rows.Clear();
		bcls_rec->Tables["SEND"].Rows.Add();

		if (!bcls_rec->Tables.Contains("MM0099"))//判断是否有块，如果没有指定块的话，则加上
		{
			bcls_rec->Tables.Add("MM0099");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "EVENT_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "FUNC_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "SYSTEM_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "MAT_NO");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "DELIVY_PLAN_NO");
		}
		bcls_rec->Tables["MM0099"].Rows.Clear();


		/* ***** 获取电文号 ***** */
		c_tc_no = s.username;
		c_user = c_tc_no;

		/* ***** 解析电文 ***** */
		c_proc_type = bcls_rec->Tables[0].Rows[0]["ARCHIVE_FLAG"].ToString().TrimOrBlank();
		prod_code = bcls_rec->Tables[0].Rows[0]["PROD_CODE"].ToString().TrimOrBlank();
		tsmsm01.MergeFrom(bcls_rec->Tables[0].Rows[0]);

		Log::Trace("", __FUNCTION__, "IN:c_proc_type=[{0}],BILL_OF_LADING_NO =[{1}] prod_code_hp[{2}]", c_proc_type, tsmsm01["BILL_OF_LADING_NO"].ToString());
		if (tsmsm01["BILL_OF_LADING_NO"].ToString().Compare(" ") == 0)
		{
			doFlag = -1;
			sprintf(s.msg, _RES("SM00S0000769")/*接收提单号为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		if (c_proc_type.Compare(" ") == 0)
		{
			doFlag = -1;
			sprintf(s.msg, _RES("SM00S0000770")/*接收下发吊销标记为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		//*********发货计划接收*********
		if (c_proc_type.Compare("I") == 0)
		{
			if (tsmsm01["TRNP_MODE_CODE"].ToString().Trim() == "") {
				strcpy(s.msg, "运输方式不能为空");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			tsmsm01["ARCHIVE_FLAG"].ToString() = "0";

			//*********发货材料进行循环处理*********
			for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{
				c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();
				if (0 == c_mat_no.Compare(" "))
				{
					break;
				}
				Log::Trace("", __FUNCTION__, "tsmsm03[""].ToString()MAT_NO = [{0}] ", c_mat_no);
				Log::Trace("", __FUNCTION__, "tsmsm03[""].ToString()BILL_NO = [{0}] ", c_bill_no);

				/**************** 判断材料号是否合法 **********************/
				Log::Trace("", __FUNCTION__, "判断材料号是否合法 ");
				sqlstr = CString(
					" select count(1) from tsmsm03 where mat_no = @mat_no "
				);
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				row_count = execute_sql.ExecuteScalar().ToInt32();

				if (0 == row_count)
				{
					//如果在历史表存在，说明已经离库，当前材料不报错，跳过（处理异常数据需要）
					//sqlstr = CString(
					//	" select count(1) from hdehp03 where mat_no = @mat_no "
					//	);
					//execute_sql.SetCommandText(sqlstr);
					//execute_sql.Parameters.Clear();
					//execute_sql.Parameters.Set("mat_no", c_mat_no);
					//row_count = execute_sql.ExecuteScalar().ToInt32();

					//if (row_count == 1)
					//{
					//	Log::Trace("", __FUNCTION__, "材料已经离库，跳过处理 ");

					//	continue;
					//}
					//else
					//{
					//	doFlag = -1;
					//	sprintf(s.msg, _RES("SM00S0000781")/*接收出厂计划电文发现材料号在准发材料表中不存在.*/);
					//	throw CApplicationException(-1, s.msg, s.svc_name);
					//}
					doFlag = -1;
					sprintf(s.msg, "材料号[%s]在准发材料在线档不存在！", (const char*)c_mat_no);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				//if (1 < row_count)
				//{
				//	doFlag = -1;
				//	sprintf(s.msg, "材料号[%s]在计划中存在多条记录！", (const char*)c_mat_no);
				//	throw CApplicationException(-1, s.msg, s.svc_name);
				//}

				sqlstr = CString(
					" SELECT * FROM tsmsm03 WHERE mat_no = @mat_no "
				);
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				execute_sql.ExecuteReader();
				while (execute_sql.Read())
				{
					execute_sql.Fetch(tsmsm03);
				}
				execute_sql.Close();

				Log::Trace("", __FUNCTION__, "tsmsm03[""].ToString()MAT_STATUS = [{0}] ", tsmsm03["MAT_STATUS"].ToString());

				if (tsmsm03["MAT_STATUS"].ToString().Compare("4") != 0)
				{
					doFlag = -1;
					sprintf(s.msg, "材料号：" + tsmsm03["MAT_NO"].ToString() + " 状态不是准发确认，不能接收！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}


				if (tsmsm03["BILL_OF_LADING_NO"].ToString().TrimOrBlank().Compare(" ") != 0 && tsmsm03["PRE_OUT_FLAG"].ToString() != "1")//已接收了出厂计划且非预出
				{
					doFlag = -1;
					sprintf(s.msg, "材料号：" + tsmsm03["MAT_NO"].ToString() + " 已经接受了出厂计划，不能再次接收！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				//提单号第一位：1 汽  2铁  3水   9或6转库
				//计算转库计划目标库区
				if (tsmsm01["BILL_OF_LADING_NO"].ToString().Substring(0, 1) == "9" || tsmsm01["BILL_OF_LADING_NO"].ToString().Substring(0, 1) == "6")
				{
					tsmsm03["MOVE_IN_STOCK_NO"].ToString() = tsmsm01["MOVE_IN_STOCK_NO"].ToString();
				}
				else
				{
					tsmsm03["MOVE_IN_STOCK_NO"].ToString() = " ";
				}

				Log::Trace("", __FUNCTION__, "修改准发材料表发货状态及提单号 ");
				/* *****	修改准发材料表发货状态及提单号 *********************************************** */
				sqlstr = CString(
					"  UPDATE	tsmsm03 "
					"              SET	  BILL_OF_LADING_NO = @BILL_OF_LADING_NO, "
					"					  DELIVY_PLAN_NO = @DELIVY_PLAN_NO, "
					"                     mat_status = CASE mat_status WHEN '7' THEN '7' ELSE '6' END, "//红冲请求状态不可抹掉
					"                     MOVE_IN_STOCK_NO= @move_in_stock_no, "
					"                     rec_revise_time   = @datetime, "
					"                     rec_revisor       = @c_user  "
					"                     WHERE	mat_no = @mat_no "
				);

				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Set("BILL_OF_LADING_NO", tsmsm01["BILL_OF_LADING_NO"].ToString());
				execute_sql.Parameters.Set("DELIVY_PLAN_NO", tsmsm01["DELIVY_PLAN_NO"].ToString());
				execute_sql.Parameters.Set("move_in_stock_no", tsmsm03["MOVE_IN_STOCK_NO"].ToString());
				execute_sql.Parameters.Set("c_user", c_user);
				execute_sql.Parameters.Set("datetime", datetime);
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				execute_sql.ExecuteNonQuery();

				tsmsm03["BILL_OF_LADING_NO"].ToString() = tsmsm01["BILL_OF_LADING_NO"].ToString();

				Log::Trace("", __FUNCTION__, "调用物料跟踪");
				/**************** 调用物料跟踪*********************/
				bcls_rec->Tables["MM0099"].Rows.Add();
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["EVENT_ID"] = "SM04"; //发货计划编入
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["EVENT_LINE_TYPE"] = "00";
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["FUNC_ID"] = s.svc_name;
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["SYSTEM_ID"] = "SMSM";
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["MAT_NO"] = tsmsm03["MAT_NO"].ToString();
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["DELIVY_PLAN_NO"] = tsmsm01["DELIVY_PLAN_NO"].ToString();
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["BILL_OF_LADING_NO"] = tsmsm01["BILL_OF_LADING_NO"].ToString();


				/**************** 记录发货履历 **********************/
				bcls_rec->Tables["SMSM09"].Rows.Add();
				bcls_rec->Tables["SMSM09"].Rows[i_mm99].Merge(tsmsm03);
				bcls_rec->Tables["SMSM09"].Rows[i_mm99]["event_id"] = "20";//出厂计划接收

				i_mm99++;

			}//材料循环结束

			/* 新增出厂计划记录 */
			tsmsm01["REC_CREATE_TIME"] = datetime;	/* 创建日期 */
			tsmsm01["REC_CREATOR"] = c_user;	/* 创建人 */
			tsmsm01["REC_REVISE_TIME"] = datetime;	/* 修改日期 */
			tsmsm01["REC_REVISOR"] = c_user;	/* 修改人 */
			tsmsm01["PLAN_STATUS"] = "1";//已接收状态
			tsmsm01["ARCHIVE_FLAG"] = "0";
			
			Log::Trace("", __FUNCTION__, "判断是否需要新增出厂计划记录  ");
			//如果同一批下发的计划已经存在，不用再新增一次了
			sqlstr = "SELECT COUNT(*) FROM TSMSM01 WHERE BILL_OF_LADING_NO = @BILL_OF_LADING_NO AND REC_CREATE_TIME like @rec_revise_time||'%' ";
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("BILL_OF_LADING_NO", tsmsm01["BILL_OF_LADING_NO"].ToString());
			execute_sql.Parameters.Set("rec_revise_time", datetime.Substring(0, 13));//精确到10秒
			row_count = execute_sql.ExecuteScalar().ToInt16();

			if (row_count == 0)
			{
				Log::Trace("", __FUNCTION__, "新增出厂计划记录  ");

				///* ***** 删除原来的出厂计划 *********************************************** */
				sqlstr = CString(
					" delete from tsmsm01 where BILL_OF_LADING_NO = @BILL_OF_LADING_NO  "
				);
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("BILL_OF_LADING_NO", tsmsm01["BILL_OF_LADING_NO"].ToString());
				execute_sql.ExecuteNonQuery();


				///* 插入计划表 */
				if (!tsmsm01.Insert())
				{
					doFlag = -1;
					sprintf(s.msg, "提单号[%s] insert TABLE tsmsm01 失败.", (const char*)tsmsm01["BILL_OF_LADING_NO"].ToString());
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}

			Log::Trace("", __FUNCTION__, "计算提单计划下的计划量，发货量 ");
			sqlstr = CString( /* 计算某计划下的计划量，发货量 */
				"  update  tsmsm01 a set rec_revise_time        =  @datetime, "
				"     	                   rec_revisor		  =  @c_user, "
				"                        (plan_num,plan_wt) = (select sum(aa),sum(bb) from "
				"  ((select  count(1) aa, COALESCE(SUM(mat_wt), 0) bb from tsmsm03 b where b.BILL_OF_LADING_NO = @BILL_OF_LADING_NO  ) union all  "
				"   (select  count(1) aa,COALESCE(SUM(mat_wt), 0) bb from hsmsm03 c where c.BILL_OF_LADING_NO = @BILL_OF_LADING_NO  ))), "
				"    (DELIVY_NUM,DELIVY_WT ) = (select  count(1), COALESCE(SUM(mat_wt), 0) from hsmsm03 b where b.BILL_OF_LADING_NO = @BILL_OF_LADING_NO ) "
				"                    where BILL_OF_LADING_NO = @BILL_OF_LADING_NO "
			);
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("BILL_OF_LADING_NO", tsmsm01["BILL_OF_LADING_NO"].ToString());
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.ExecuteNonQuery();


			/********** 抛物料跟踪 **************/
			if (i_mm99 == 0)
			{
				return 0;
			}

			if (tsmsm03["MAT_KIND"].ToString() == "SM")
			{
				doFlag = f_mmsm99(bcls_rec, bcls_ret, conn);
				if (doFlag < 0)
				{
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}
			

			/********** 抛发货履历 **************/
			doFlag = f_smsm_trace(bcls_rec, bcls_ret, conn);
			if (doFlag < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}
		//*********出厂计划吊销*********
		if (c_proc_type.Compare("D") == 0)
		{
			//*********发货材料进行循环处理*********
			for (i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
			{

				c_mat_no = bcls_rec->Tables[0].Rows[i]["mat_no"].ToString().TrimOrBlank();

				if (0 == c_mat_no.Compare(" "))
				{
					break;

				}
				Log::Trace("", __FUNCTION__, "tsmsm03[""].ToString()MAT_NO = [{0}] ", c_mat_no);

				/**************** 判断材料号是否合法 **********************/
				Log::Trace("", __FUNCTION__, "判断材料号是否合法 ");
				sqlstr = CString(
					" select count(1) from tsmsm03 where mat_no = @mat_no "
				);
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("mat_no", c_mat_no);

				row_count = execute_sql.ExecuteScalar().ToInt32();

				if (0 == row_count)
				{
					//如果在历史表存在，说明已经离库，当前材料不报错，跳过（处理异常数据需要）
					sqlstr = CString(
						" select count(1) from hsmsm03 where mat_no = @mat_no "
					);
					execute_sql.SetCommandText(sqlstr);
					execute_sql.Parameters.Clear();
					execute_sql.Parameters.Set("mat_no", c_mat_no);

					row_count = execute_sql.ExecuteScalar().ToInt32();

					if (row_count == 1)
					{
						continue;
					}
					else
					{
						doFlag = -1;
						sprintf(s.msg, _RES("SM00S0000781")/*接收出厂计划电文发现材料号在准发材料表中不存在.*/);
						throw CApplicationException(-1, s.msg, s.svc_name);
					}

				}

				if (1 < row_count)
				{
					doFlag = -1;
					sprintf(s.msg, "材料号[%s]在计划中存在多条记录！", (const char*)c_mat_no);
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				sqlstr = CString(
					" SELECT * FROM tsmsm03 WHERE mat_no = @mat_no "
				);
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				execute_sql.ExecuteReader();
				if (execute_sql.Read())
				{
					execute_sql.Fetch(tsmsm03);
				}
				execute_sql.Close();

				if (tsmsm03["BILL_OF_LADING_NO"].ToString() != tsmsm01["BILL_OF_LADING_NO"].ToString())
				{
					doFlag = -1;
					sprintf(s.msg, tsmsm03["MAT_NO"].ToString() + "提单号不一致，不可撤销！");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}

				/* *****	修改准发材料表发货状态 *********************************************** */
				sqlstr = CString(
					"  UPDATE	tsmsm03 "
					"              SET	  BILL_OF_LADING_NO = ' ', "
					"                     work_pick_num = ' ', "
					"                     DELIVY_PLAN_NO = ' ', "
					"                     mat_status      = CASE mat_status WHEN '7' THEN '7' ELSE '4' END, "
					"                     rec_revise_time   = @datetime, "
					"                     rec_revisor       = @c_user  "
					"                     WHERE	mat_no = @mat_no "
				);
				execute_sql.SetCommandText(sqlstr);
				execute_sql.Parameters.Clear();
				execute_sql.Parameters.Set("c_user", c_user);
				execute_sql.Parameters.Set("datetime", datetime);
				execute_sql.Parameters.Set("mat_no", c_mat_no);
				execute_sql.ExecuteNonQuery();

				Log::Trace("", __FUNCTION__, "调用物料跟踪");
				/**************** 调用物料跟踪*********************/
				bcls_rec->Tables["MM0099"].Rows.Add();
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["EVENT_ID"] = "SM05"; //发货计划撤销
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["EVENT_LINE_TYPE"] = tsmsm03["MAT_KIND"].ToString();
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["FUNC_ID"] = s.svc_name;
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["SYSTEM_ID"] = "SMSM";
				bcls_rec->Tables["MM0099"].Rows[i_mm99]["MAT_NO"] = tsmsm03["MAT_NO"].ToString();

				/**************** 记录发货履历 **********************/
				bcls_rec->Tables["SMSM09"].Rows.Add();
				bcls_rec->Tables["SMSM09"].Rows[i_mm99].Merge(tsmsm03);
				bcls_rec->Tables["SMSM09"].Rows[i_mm99]["event_id"] = "21";//撤销

				i_mm99++;



			}//材料循环结束


			Log::Info("", __FUNCTION__, "计算某计划下的计划量，发货量");
			sqlstr = CString( /* 计算某计划下的计划量，发货量 */
				"  update  tsmsm01 a set rec_revise_time        =  @datetime, "
				"     	                   rec_revisor		  =  @c_user, "
				"                        (plan_num,plan_wt) = (select sum(aa),sum(bb) from "
				"  ((select  count(1) aa, COALESCE(SUM(mat_wt), 0) bb from tsmsm03 b where b.BILL_OF_LADING_NO = @BILL_OF_LADING_NO  ) union all  "
				"   (select  count(1) aa,COALESCE(SUM(mat_wt), 0) bb from hsmsm03 c where c.BILL_OF_LADING_NO = @BILL_OF_LADING_NO  ))), "
				"                        (DELIVY_NUM,DELIVY_WT ) = (select  count(1), COALESCE(SUM(mat_wt), 0) from hsmsm03 b where b.BILL_OF_LADING_NO = @BILL_OF_LADING_NO ) "
				"                    where BILL_OF_LADING_NO = @BILL_OF_LADING_NO "
			);
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("BILL_OF_LADING_NO", tsmsm01["BILL_OF_LADING_NO"].ToString());
			execute_sql.Parameters.Set("c_user", c_user);
			execute_sql.Parameters.Set("datetime", datetime);
			execute_sql.ExecuteNonQuery();
			execute_sql.Close();

			sqlstr = CString( /* 更新提单状态为完成  */
				" update tsmsm01  a  "
				" set  plan_status =  '9'  "
				" where BILL_OF_LADING_NO = @BILL_OF_LADING_NO"
				"  and  not exists ( select 1 "
				"                   from  tsmsm03 b "
				"                   where a.BILL_OF_LADING_NO = b.BILL_OF_LADING_NO) "
			);
			execute_sql.SetCommandText(sqlstr);
			execute_sql.Parameters.Clear();
			execute_sql.Parameters.Set("BILL_OF_LADING_NO", tsmsm01["BILL_OF_LADING_NO"].ToString());
			execute_sql.ExecuteNonQuery();
			execute_sql.Close();


			if (i_mm99 == 0)
			{
				return 0;
			}

			/********** 抛物料跟踪 **************/
			if (tsmsm03["MAT_KIND"].ToString() == "SM")
			
			{
				doFlag = f_mmsm99(bcls_rec, bcls_ret, conn);
				if (doFlag < 0)
				{
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}

			/********** 抛发货履历 **************/
			doFlag = f_smsm_trace(bcls_rec, bcls_ret, conn);
			if (doFlag < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

		}

	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		//返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);
		s.flag = -1;
		//数据库异常时返回-1，事务将被回滚
		doFlag = -1;
	}
	//捕获应用错误
	catch (CApplicationException& ex)
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	cmd_inq.Close();
	//返回-1时事务将回滚，返回为0是事务将提交	return doFlag;
}



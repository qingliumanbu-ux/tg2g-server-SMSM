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



BM2F_ENTERACE_TELE(cm_0rt807_rcv)

int f_cm_0rt807_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__); 	//系统日志类定义

	/* 程序内部变量 */
	int doFlag = 0;
	int blkNum = 0;
	int blkNum_pmol02 = 0;
	int row_count = 0, i = 0, ret = 0;
	/* 业务变量 */
	CString	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	/* 业务变量 */

	/* 实体类定义 */
	CModel tmmsm01("TMMSM01");
	CModel tmmsm96("TMMSM96");
	CModel tsmsm03("TSMSM03");
	CModel tsmsm09("TSMSM09");
	/* 数据库SQL操作字符串 */
	CString sqlstr, sqlstr1, sqlstr2;
	CString    c_mat_no, c_wt_method_code, c_reject_maker, c_mat_kind, c_reject_time, c_reject_cause;
	CString    c_rec_revisor;
	CString    c_rec_revise_time;
	CDecimal   d_mat_theory_wt, d_mat_act_wt;
	CDbCommand cmd_sql(conn);
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_code(conn);




	try
	{
		if (bcls_rec->Tables.IndexOf("SMSM09") < 0)
		{
			bcls_rec->Tables.Add("SMSM09");
			bcls_rec->Tables["SMSM09"].Columns.Add(tsmsm09);
			bcls_rec->Tables["SMSM09"].Rows.Add();
		}

		//抛物料跟踪
		if (!bcls_rec->Tables.Contains("MM0099"))
		{
			bcls_rec->Tables.Add("MM0099");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "EVENT_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "FUNC_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "SYSTEM_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "MAT_NO");
			//20230424准发红冲增加质量封锁功能
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "HOLD_CAUSE_CODE");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "HOLD_REMARK");
			bcls_rec->Tables["MM0099"].Rows.Add();
		}





		/* ***** 解析电文 ***** */
		c_mat_no = bcls_rec->Tables[0].Rows[0]["MAT_NO"].ToString().TrimOrBlank();
		c_mat_kind = bcls_rec->Tables[0].Rows[0]["MAT_KIND"].ToString().TrimOrBlank();
		c_wt_method_code = bcls_rec->Tables[0].Rows[0]["WT_METHOD_CODE"].ToString().TrimOrBlank();
		d_mat_theory_wt = bcls_rec->Tables[0].Rows[0]["MAT_THEORY_WT"].ToDecimal();
		d_mat_act_wt = bcls_rec->Tables[0].Rows[0]["MAT_ACT_WT"].ToDecimal();
		c_reject_maker = bcls_rec->Tables[0].Rows[0]["REJECT_MAKER"].ToString().TrimOrBlank();
		c_reject_time = bcls_rec->Tables[0].Rows[0]["REJECT_TIME"].ToString().TrimOrBlank();
		c_reject_cause = bcls_rec->Tables[0].Rows[0]["REJECT_CAUSE"].ToString().TrimOrBlank();
		Log::Trace("", __FUNCTION__, "MAT_NO[{0}]MAT_KIND[{1}]WT_METHOD_CODE [{2}] mat_theory_wt[{3}]mat_act_wt [{4}]", c_mat_no, c_mat_kind, c_wt_method_code, d_mat_theory_wt, d_mat_act_wt);
		Log::Trace("", __FUNCTION__, "c_reject_maker[{0}]c_reject_time[{1}]c_reject_cause [{2}] ", c_reject_maker, c_reject_time, c_reject_cause);
		if (c_mat_no.Compare(" ") == 0)
		{
			doFlag = -1;
			strcpy(s.msg, _RES("SM00S0000760")/*接收材料号为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}



		/**************** 判断材料号是否合法 **********************/
		sqlstr = CString(
			" select count(1) from tsmsm03 where mat_no = @mat_no "
		);
		sqlstr = sqlstr;
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Clear();
		cmd_inq.Parameters.Set("mat_no", c_mat_no);
		row_count = cmd_inq.ExecuteScalar().ToInt32();

		if (0 == row_count)
		{
			doFlag = -1;
			strcpy(s.msg, "该材料号【" + tsmsm03["MAT_NO"].ToString() + "】在准发材料档无记录，不能红冲！");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		sqlstr1 = CString(
			" select * from tsmsm03 where mat_no = @mat_no "
		);
		sqlstr = sqlstr1;
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Clear();
		cmd_inq.Parameters.Set("mat_no", c_mat_no);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			cmd_inq.Fetch(tsmsm03);
		}
		cmd_inq.Close();

		/**************** 红冲逻辑开始编写  **********************/

		if (tsmsm03["MAT_STATUS"].ToString().Compare("4") != 0)
		{
			doFlag = -1;
			strcpy(s.msg, "该材料号【" + tsmsm03["MAT_NO"].ToString() + "】不是准发确认状态，不能红冲！");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		/**************** 调用物料封装  函数*********************/
		bcls_rec->Tables["MM0099"].Rows[0]["EVENT_ID"] = "PM13"; //准发红冲
		bcls_rec->Tables["MM0099"].Rows[0]["EVENT_LINE_TYPE"] = tsmsm03["MAT_KIND"];
		bcls_rec->Tables["MM0099"].Rows[0]["FUNC_ID"] = "cm_0rt807_rcv";
		bcls_rec->Tables["MM0099"].Rows[0]["SYSTEM_ID"] = "SM";
		bcls_rec->Tables["MM0099"].Rows[0]["MAT_NO"] = tsmsm03["MAT_NO"];
		bcls_rec->Tables["MM0099"].Rows[0]["HOLD_CAUSE_CODE"] = "PM06";
		doFlag = f_mmsm99(bcls_rec, bcls_ret, conn);
		if (doFlag < 0)
		{
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		/**************** 调用物料封装  函数结束  **********************/
		/* ***** 记录log ***** */
		//记录发货履历
		bcls_rec->Tables["SMSM09"].Rows[0].Merge(tsmsm03);
		bcls_rec->Tables["SMSM09"].Rows[0]["event_id"] = "11";//准发红冲
		bcls_rec->Tables["SMSM09"].Rows[0]["RED_CAUSE_CODE"] = " ";
		bcls_rec->Tables["SMSM09"].Rows[0]["RED_CAUSE_DESC"] = c_reject_cause;
		bcls_rec->Tables["SMSM09"].Rows[0]["RED_CONFM_MAKER"] = c_reject_maker;
		bcls_rec->Tables["SMSM09"].Rows[0]["RED_CONFM_TIME"] = c_reject_time;
		doFlag = f_smsm_trace(bcls_rec, bcls_ret, conn);
		if (ret < 0)
		{
			Log::Debug("", __FUNCTION__, "f_dehp_trace函数调用出错.");
			throw CApplicationException(-1, s.msg, s.svc_name);
		}

		/* *****	删除准发材料信息*********************************************** */
		Log::Debug("", __FUNCTION__, "删除准发计划材料信息.");
		sqlstr2 = CString(
			" delete from tsmsm03 where mat_no = @mat_no  "
		);
		sqlstr = sqlstr2;
		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("mat_no", c_mat_no);
		Log::Trace("", __FUNCTION__, "sqlstr = [{0}],c_mat_no = [{1}] ", sqlstr, c_mat_no);
		cmd_inq.ExecuteNonQuery();
		cmd_inq.Close();
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



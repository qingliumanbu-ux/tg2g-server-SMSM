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
//码单红冲处理函数
int f_smsm_md_red(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
//发送电文21a009
int f_wmsm_21a009_snd(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
//发货履历
int f_smsm_trace(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);



BM2F_ENTERACE_TELE(cm_0rt891_rcv)

int f_cm_0rt891_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__); 	//系统日志类定义

	/* 程序内部变量 */
	int doFlag = 0;
	int blkNum = 0;
	int blkNum_pmol02 = 0;
	/* 业务变量 */
	CString	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	/* 业务变量 */
	CString c_delivy_plan_no, c_mat_no, c_mat_kind, truck_seq_no, c_user, c_tc_no, archive_flag;
	CString	sqlstr(""), sqlstr1(""), sqlstr2(""), sqlstr3(""), sqlstr4(""), sqlstr5("");
	int row_count = 0;
	int row_count2 = 0;
	/* 实体类定义 */
	CModel tmmsm01("TMMSM01");
	CModel tmmsm96("TMMSM96");
	CModel twmsm61 = CModel("TWMSM61");
	CModel tsmsm01("TSMSM01");
	CModel tsmsm03("TSMSM03");
	CModel tsmsm09("TSMSM09");
	/* 数据库SQL操作字符串 */
	
	CDbCommand cmd_sql(conn);
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_code(conn);




	try
	{

		if (bcls_rec->Tables.IndexOf("21A009") < 0)
		{
			bcls_rec->Tables.Add("21A009");
			bcls_rec->Tables["21A009"].Columns.Add(twmsm61);
		}
		bcls_rec->Tables["21A009"].Rows.Clear();
		/* ***** 解析电文 ***** */
		c_mat_kind = bcls_rec->Tables[0].Rows[0]["MAT_KIND"].ToString().TrimOrBlank();
		c_mat_no = bcls_rec->Tables[0].Rows[0]["MAT_NO"].ToString().TrimOrBlank();
		archive_flag = bcls_rec->Tables[0].Rows[0]["ARCHIVE_FLAG"].ToString().TrimOrBlank();
		Log::Trace("", __FUNCTION__, "c_mat_kind[{0}]c_mat_no[{1}] archive_flag[{2}]", c_mat_kind, c_mat_no, archive_flag);
		if (c_mat_no.Compare(" ") == 0)
		{
			doFlag = -1;
			strcpy(s.msg, _RES("SM00S0000760")/*接收材料号为空.*/);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		if (archive_flag == "I")//码单红冲业务
		{
			//查询当前准发材料的信息	
			tsmsm03["MAT_NO"].ToString() = c_mat_no;
			if (tsmsm03.Query("MAT_NO"))
			{
				c_delivy_plan_no = tsmsm03["DELIVY_PLAN_NO"].ToString();
				truck_seq_no = tsmsm03["TRUCK_SEQ_NO"].ToString();
			}

			Log::Trace("", __FUNCTION__, "c_delivy_plan_no[{0}]truck_seq_no[{1}] ", c_delivy_plan_no, truck_seq_no);
			//调用码单红冲处理函数
			doFlag = f_smsm_md_red(bcls_rec, bcls_ret, conn);
			if (doFlag < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}

			//获取电文号的计划号DELIVY_PLAN_NO信息
			//c_delivy_plan_no = bcls_rec->Tables[0].Rows[0]["DELIVY_PLAN_NO"].ToString().TrimOrBlank();


			//判断该车（计划）是否全部码单红冲  在HDEHP03 档中 DELIVY_PLAN_NO 计划号下的材料数 0 全部码单红冲 大于0则 没完成码单红冲

			sqlstr = "SELECT COUNT(MAT_NO) FROM HSMSM03 WHERE DELIVY_PLAN_NO = @delivy_plan_no";

			cmd_inq.SetCommandText(sqlstr);
			cmd_inq.Parameters.Clear();
			cmd_inq.Parameters.Set("delivy_plan_no", c_delivy_plan_no);
			row_count = cmd_inq.ExecuteScalar().ToInt32();
			cmd_inq.Close();
			Log::Trace("", __FUNCTION__, "row_count[{0}] ", row_count);

			//全部红冲则需发物流系统装车取消电文  电文号 21a009  电文字段DEAL_FLAG为D
			if (row_count == 0)
			{


			



				//Log::Trace("", __FUNCTION__, "11111 ");

				////组装车取消电文头信息
				//bcls_rec->Tables["21A009"].Rows.Add();
				//
				//bcls_rec->Tables["243105"].Rows[0].Merge(x243105);

				//Log::Trace("", __FUNCTION__, " DEAL_FLAG[{0}]", x243105.DEAL_FLAG);


				


				

				//③发送243105电文 
				//发送物流信息电文

				doFlag = f_wmsm_21a009_snd(bcls_rec, bcls_ret, conn);
				if (doFlag < 0)
				{
					Log::Debug("", __FUNCTION__, "向物流系统发送21A009电文调用出错.");
					throw CApplicationException(-1, s.msg, s.svc_name);
				}
			}
		}
		else if (archive_flag == "U")//余材挂上现货合同业务，只记录发货履历
		{
			//记录发货履历
			CString	record_name = "SMSM09";
			if (bcls_rec->Tables.IndexOf("SMSM09") < 0)
			{
				bcls_rec->Tables.Add("SMSM09");
				bcls_rec->Tables["SMSM09"].Columns.Add(tsmsm03);
			}
			bcls_rec->Tables["SMSM09"].Rows.Clear();
			tsmsm03["MAT_NO"] = c_mat_no;
			if (tsmsm03.Query("MAT_NO"))
			{
				/* ***** 记录发货履历 ***** */
				bcls_rec->Tables["SMSM09"].Rows.Add();
				bcls_rec->Tables["SMSM09"].Rows[0].Merge(tsmsm03);
				bcls_rec->Tables["SMSM09"].Rows[0].Merge(bcls_rec->Tables[0].Rows[0]);
				bcls_rec->Tables["SMSM09"].Rows[0]["event_id"] = "25";  //材料现货挂单
				/********** 抛发货履历 **************/
				if (bcls_rec->Tables["SMSM09"].Rows.get_Count() > 0)
				{
					doFlag = f_smsm_trace(bcls_rec, bcls_ret, conn);
					if (doFlag < 0)
					{
						throw CApplicationException(-1, s.msg, s.svc_name);
					}
				}
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



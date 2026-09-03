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



BM2F_ENTERACE_TELE(cm_0rt806_rcv)

int f_cm_0rt806_rcv(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__); 	//系统日志类定义

	/* 程序内部变量 */
	int doFlag = 0;
	int blkNum = 0;
	int blkNum_pmol02 = 0;
	/* 业务变量 */
	CString	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	CString wt_method_code = " ";//计重方式
	CString confm_maker, confm_time, order_type_code, order_no, mat_kind = " ";
	CDecimal mat_theory_wt, mat_act_wt = 0;
	/* 业务变量 */

	/* 实体类定义 */
	CModel tmmsm01("TMMSM01");
	CModel tmmsm96("TMMSM96");
	CModel tsmsm03("TSMSM03");
	/* 数据库SQL操作字符串 */
	CString sqlstr;
	CDbCommand cmd_sql(conn);
	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);
	CDbCommand cmd(conn);




	try
	{
		if (!bcls_rec->Tables.Contains("MM0099"))//判断是否有块，如果没有指定块的话，则加上
		{
			bcls_rec->Tables.Add("MM0099");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "EVENT_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");	// 事件产线类型，全产线/00,其他产线/SM,HR,CR等
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "SYSTEM_ID");			// 系统标识4位，到二级模块
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "FUNC_ID");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "MAT_KIND");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "MAT_NO");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "CONFM_FLAG");//准发标记	
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "ORDER_NO");//合同号
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "WHOLE_BACKLOG_CODE");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "WHOLE_BACKLOG_SEQ");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "NEXT_WHOLE_BACKLOG_CODE");
			bcls_rec->Tables["MM0099"].Columns.Add(DT_STRING, "NEXT_WHOLE_BACKLOG_SEQ");
		}
		bcls_rec->Tables["MM0099"].Rows.Clear();


		/* *******定义一个发货履历块 */
		if (bcls_rec->Tables.IndexOf("SMSM09") < 0)
		{
			bcls_rec->Tables.Add("SMSM09");
			bcls_rec->Tables["SMSM09"].Columns.Add(tsmsm03);
			bcls_rec->Tables["SMSM09"].Columns.Add(DT_STRING, "event_id");
			bcls_rec->Tables["SMSM09"].Columns.Add(DT_STRING, "event_name");
		}
		bcls_rec->Tables["MM0099"].Rows.Clear();
		datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	

		/****** 获取首行电文数据 ******/
		tsmsm03.MergeFrom(bcls_rec->Tables[0].Rows[0]);
		wt_method_code = bcls_rec->Tables[0].Rows[0]["WT_METHOD_CODE"].ToString();
		confm_maker = bcls_rec->Tables[0].Rows[0]["CONFM_MAKER"].ToString();
		confm_time = bcls_rec->Tables[0].Rows[0]["CONFM_TIME"].ToString();
		order_type_code = bcls_rec->Tables[0].Rows[0]["ORDER_TYPE_CODE"].ToString();
		order_no = bcls_rec->Tables[0].Rows[0]["ORDER_NO"].ToString().Trim();
		mat_kind = bcls_rec->Tables[0].Rows[0]["MAT_KIND"].ToString();
		mat_theory_wt = bcls_rec->Tables[0].Rows[0]["mat_theory_wt"].ToDecimal();
		mat_act_wt = bcls_rec->Tables[0].Rows[0]["mat_act_wt"].ToDecimal();
		Log::Info("", __FUNCTION__, "wt_method_code[{0}]confm_maker[{1}]confm_time[{2}]order_type_code[{3}]order_no[{4}]", wt_method_code, confm_maker, confm_time, order_type_code, order_no);
		Log::Info("", __FUNCTION__, "mat_kind[{0}]mat_theory_wt[{1}]mat_act_wt[{2}]", mat_kind, mat_theory_wt, mat_act_wt);
		///*基本校验*/

		if (tsmsm03.Query("MAT_NO"))
		{
			sprintf(s.msg, "材料号[%s]在准发材料档中已存在.", (const char*)tsmsm03["MAT_NO"].ToString());
			throw CApplicationException(-1, s.msg, log.Location);
		}
		tmmsm01["MAT_NO"] = tsmsm03["MAT_NO"];
		if (!tmmsm01.Query("MAT_NO"))
		{
			sprintf(s.msg, "材料号[%s]在物料主档中不存在.", (const char*)tsmsm03["MAT_NO"]);
			throw CApplicationException(-1, s.msg, log.Location);
		}
		

		if (tmmsm01["MAT_STATUS"].ToString() != "33")
		{
			sprintf(s.msg, "材料号[%s]在物料主档状态不是33待交库.", (const char*)tmmsm01["MAT_NO"]);
			throw CApplicationException(-1, s.msg, log.Location);
		}
		if (order_type_code != "XYC" && tmmsm01["ORDER_NO"].ToString().Trim() != order_no)//
		{
			sprintf(s.msg, "非预合同下，材料号[%s]在物料主档的L3/L4合同号不一致.", (const char*)tmmsm01["MAT_NO"]);
			throw CApplicationException(-1, s.msg, log.Location);
		}


		if (tmmsm01["MAT_ACT_WT"].ToDecimal() != mat_act_wt)
		{
			sprintf(s.msg, "材料号[" + tmmsm01["MAT_NO"].ToString() + "]的L4[" + mat_act_wt.ToString() + "]实重与L3[" + tmmsm01["MAT_ACT_WT"].ToString() + "]不一致，失败！");
			throw CApplicationException(-1, s.msg, log.Location);
		}
		
		tsmsm03.CopyFrom(tmmsm01);
		//tsmsm03["STOCK_NO"] = bcls_rec->Tables[0].Rows[0]["STOCK_NO"];
		tsmsm03["MAT_STATUS"] = "4";//材料准发状态默认已确定
		//0实重，1理重
		if (wt_method_code == "0" && order_type_code != "XYC")//计重方式实重 非现货合同
		{
			if (mat_act_wt == 0)//实重为0报错
			{
				sprintf(s.msg, "材料号[" + tmmsm01["MAT_NO"].ToString() + "]计重方式为实重，实重不能为0，失败！");
				throw CApplicationException(-1, s.msg, log.Location);
			}
			tsmsm03["MAT_WT"] = tmmsm01["MAT_ACT_WT"];
		}
		else if (wt_method_code == "1" && order_type_code != "XYC")//计重方式理重 非现货合同
		{
			if (mat_theory_wt == 0)//理重为0报错
			{
				sprintf(s.msg, "材料号[" + tmmsm01["MAT_NO"].ToString() + "]计重方式为理重，理重不能为0，失败！");
				throw CApplicationException(-1, s.msg, log.Location);
			}
			tsmsm03["MAT_WT"] = tmmsm01["MAT_ACT_WT"];
		}
		else if (order_type_code == "XYC")//现货预合同 优先取实重，实重没有，取目标重量
		{
			if (mat_act_wt != 0)//实重不为0
			{
				tsmsm03["MAT_WT"] = tmmsm01["MAT_ACT_WT"];
			}
			else
			{
				tsmsm03["MAT_WT"] = tmmsm01["MAT_ACT_WT"];
			}
		}
		else
		{
			strcpy(s.msg, "计重方式代码错误！");
			Log::Error("", __FUNCTION__, "wt_method_code[{0}]计重方式错误", wt_method_code);
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
		tsmsm03["CONFM_MAKER"] = confm_maker; //准发责任者
		tsmsm03["CONFM_TIME"] = confm_time;
		if (confm_time.GetLength() > 8)
		{
			tsmsm03["CONFM_DATE"] = confm_time.SubstringNE(8);//准发确认日期
			//调用函数生成班次、班组
			CString confm_shift("");
			CString confm_group("");
			CString confm_date("");
			f_epep_get_shift_group_day("SM", confm_time, confm_shift, confm_group, confm_date, conn);
			tsmsm03["CONFM_SHIFT"] = confm_shift;
				tsmsm03["CONFM_GROUP"] = confm_group;
				tsmsm03["CONFM_DATE"] = confm_date;
		}
		tsmsm03["PRE_OUT_FLAG"] = "0";//默认正常出库

		tsmsm03["REC_CREATOR"] = s.userid;
		tsmsm03["REC_CREATE_TIME"] = datetime;
		//设置材料交库的名义规格
		tsmsm03["MAT_THICK"] = tmmsm01["MAT_THICK"];
		tsmsm03["MAT_WIDTH"] = tmmsm01["MAT_WIDTH"];
		tsmsm03["MAT_LEN"] = tmmsm01["MAT_LEN"];
		//设置材料交库的合同

		tsmsm03["ORDER_NO"] = order_no;
		//设置包装信息 
	
		//写入准发材料表
		tsmsm03.Insert();


		//记录发货履历
		bcls_rec->Tables["SMSM09"].Rows.Add();
		bcls_rec->Tables["SMSM09"].Rows[bcls_rec->Tables["SMSM09"].Rows.get_Count() - 1].Merge(tsmsm03);
		bcls_rec->Tables["SMSM09"].Rows[bcls_rec->Tables["SMSM09"].Rows.get_Count() - 1]["event_id"] = "10";//准发确认


		//抛物料跟踪
		bcls_rec->Tables["MM0099"].Rows.Add();
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["EVENT_ID"] = "PM05";
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["EVENT_LINE_TYPE"] = "00";
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["SYSTEM_ID"] = tsmsm03["MAT_KIND"];
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["FUNC_ID"] = "cm_0rT806_rcv";
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["MAT_KIND"] = tsmsm03["MAT_KIND"];
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["MAT_NO"] = tsmsm03["MAT_NO"];
		bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["CONFM_FLAG"] = "2";
		//bcls_rec->Tables["MM0099"].Rows[bcls_rec->Tables["MM0099"].Rows.get_Count() - 1]["ORDER_NO"] = order_no;

		//调用发货履历
		if (bcls_rec->Tables["SMSM09"].Rows.get_Count() > 0)
		{
			doFlag = f_smsm_trace(bcls_rec, bcls_ret, conn);
			if (doFlag < 0)
			{
				Log::Debug("", __FUNCTION__, "f_dehp_trace函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}


		//抛物料跟踪
		if (bcls_rec->Tables["MM0099"].Rows.get_Count() > 0)
		{
			doFlag = f_mmsm99(bcls_rec, bcls_ret, conn);
			if (doFlag < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}
		///*处理成功。*/
		strcpy(s.msg, _RES("GCRSS0000002"));
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



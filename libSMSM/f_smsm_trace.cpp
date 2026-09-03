/****************************************************
*	程序功能：写履历记录							*
*	编制日期： 				                		*
*	编制人员： 								        *
*	传入参数：材料号、履历类型						*
*	返回参数：0	成功	-1	失败					*
*	出错描述：s.msg									*
****************************************************/
#include "stdafx.h"		// 框架头，不可删除



BM2_FUNCTION_EXPORT
int  f_smsm_trace(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/* 程序内部变量 */
	int	doFlag = 0;
	int	zlfh = 1;		//重量符号

	int	i = 0;
	int	rows = 0;

	/* 业务变量 */
	CString	sqlstr(""), sqlstr1("");              // 数据库SQL操作字符串
	CString	dateTime("");
	CString	str("");
	CString	mat_no("");
	CString	mark("");


	/* 实体类定义 */
	CModel 	tsmsm03("TSMSM03");
	CModel	tsmsm01("TSMSM01");
	CModel	tsmsm09("TSMSM09");

	CString v_remark = "";
	CString v_record_flag = "";

	/* 数据库操作类定义 */
	CDbCommand cmd_inq(conn);

	try
	{
		dateTime = CDateTime::Now().ToString("yyyyMMddHHmmss");

		/* 判块是否存在 */
		if (bcls_rec->Tables.IndexOf("SMSM09") < 0)
		{
			strcpy(s.msg, "传入参数不包含块[SMSM09]");
		}

		mark = bcls_rec->Tables["SMSM09"].Rows[0]["event_id"].ToString().TrimOrBlank();

		if (bcls_rec->Tables["SMSM09"].Columns.Contains("EVENT_NAME"))
		{
			v_remark = bcls_rec->Tables["SMSM09"].Rows[0]["EVENT_NAME"].ToString().TrimOrBlank();
		}

		Log::Info("", __FUNCTION__, "履历类型=[{0}]", mark);

		/*if (mark != "3" && mark != "4" && mark != "5" && mark != "6")
		{
			Log::Info("", __FUNCTION__, "当前事件=[{0}]不记录履历，跳出返回", mark);
			return 0;
		}*/

		/* 读取事件名称 */
		tsmsm09["EVENT_NAME"] = " ";
		sqlstr = " SELECT code_desc_1_content,code_desc_2_content FROM TEP0002"
			" WHERE	CODE_CLASS = 'DE03' "
			" AND	CODE = @mark ";

		cmd_inq.SetCommandText(sqlstr);
		cmd_inq.Parameters.Set("mark", mark);
		cmd_inq.ExecuteReader();
		if (cmd_inq.Read())
		{
			tsmsm09["EVENT_NAME"] = cmd_inq.GetString(1).Trim();
			v_record_flag = cmd_inq.GetString(2).Trim();
		}
		cmd_inq.Close();

		if (v_remark.Trim() != "")
		{
			tsmsm09["EVENT_NAME"] = tsmsm09["EVENT_NAME"].ToString() + ":" + v_remark;
		}

		if (v_record_flag == "0")
		{
			//配置不记录履历
			Log::Info("", __FUNCTION__, "当前事件=[{0}][{1}]不记录履历，跳出返回", mark, tsmsm09["EVENT_NAME"].ToString());
			return 0;
		}


		if (mark == "6" || mark == "3")
		{
			//红冲事件重量取负数
			zlfh = -1;
		}

		rows = bcls_rec->Tables["SMSM09"].Rows.get_Count();
		for (i = 0; i < rows; i++)
		{
			tsmsm09.MergeFrom(bcls_rec->Tables["SMSM09"].Rows[i]);

			//传入的参数
			Log::Info("", __FUNCTION__, "材料号=[{0}]", tsmsm09["MAT_NO"].ToString());


			//数据
			//tsmsm09[""]CopyFrom(tdehp03);
			tsmsm09["REC_CREATOR"] = s.userid;	/* 记录创建责任者 */
			tsmsm09["REC_CREATE_TIME"] = dateTime;	/* 记录创建时刻 */
			tsmsm09["REC_REVISOR"] = s.userid;	/* 记录修改责任者 */
			tsmsm09["REC_REVISE_TIME"] = dateTime;	/* 记录修改时刻 */
			tsmsm09["EVENT_ID"] = mark;	/* 事件标识 */
			tsmsm09["EVENT_MAKER"] = s.userid;	/* 事件责任者 */

			//调用函数生成班次、班组
			CString shift_no = "";
			CString shift_group = "";
			CString event_date = "";
			f_epep_get_shift_group_day("SM", dateTime, shift_no, shift_group, event_date, conn);
			tsmsm09["SHIFT_NO"] = shift_no;
			tsmsm09["SHIFT_GROUP"] = shift_group;
			tsmsm09["EVENT_DATE"] = event_date;

			/* 事件跟踪序列号 */
			CDateTime currTime = CDateTime::Now();
			tsmsm09["TRACK_SEQ_NO"] = dateTime + CString::Format("%04ld", currTime.Millisecond()).SubstringNE(0, 4);

			if (tsmsm09["BILL_OF_LADING_NO"].ToString().Trim() != "")
			{
				sqlstr = " SELECT * FROM TSMSM01 "
					" WHERE	BILL_OF_LADING_NO = @BILL_OF_LADING_NO ";

				cmd_inq.SetCommandText(sqlstr);
				cmd_inq.Parameters.Set("BILL_OF_LADING_NO", tsmsm09["BILL_OF_LADING_NO"].ToString());
				cmd_inq.ExecuteReader();
				if (cmd_inq.Read())
				{
					cmd_inq.Fetch(tsmsm01);
				}
				cmd_inq.Close();
				tsmsm09["FIN_CUST_CODE"] = tsmsm01["FIN_CUST_CODE"];
				tsmsm09["FIN_USER_NAME"] = tsmsm01["FIN_USER_NAME"];
				tsmsm09["ORDER_CUST_CODE"] = tsmsm01["ORDER_CUST_CODE"];
				tsmsm09["ORDER_CUST_CNAME"] = tsmsm01["ORDER_CUST_CNAME"];
				tsmsm09["ORDER_CUST_ENAME"] = tsmsm01["ORDER_CUST_ENAME"];
				tsmsm09["CONSIGN_USER_CODE"] = tsmsm01["CONSIGN_USER_CODE"];
				tsmsm09["CONSIGN_CUST_CNAME"] = tsmsm01["CONSIGN_CUST_CNAME"];
				tsmsm09["CONSIGN_CUST_ENAME"] = tsmsm01["CONSIGN_CUST_ENAME"];
				tsmsm09["DELIVY_PLACE_CODE"] = tsmsm01["DELIVY_PLACE_CODE"];
				tsmsm09["DELIVY_PLACE_NAME"] = tsmsm01["DELIVY_PLACE_NAME"];
				tsmsm09["DELIVY_PLACE_NAME_ENGLISH"] = tsmsm01["DELIVY_PLACE_NAME_ENGLISH"];
				tsmsm09["TERMINAL_CODE"] = tsmsm01["TERMINAL_CODE"];
				tsmsm09["TERMINAL_NAME"] = tsmsm01["TERMINAL_NAME"];
				tsmsm09["PRIVATE_ROUTE_CODE"] = tsmsm01["PRIVATE_ROUTE_CODE"];
				tsmsm09["PRIVATE_ROUTE_NAME"] = tsmsm01["PRIVATE_ROUTE_NAME"];
				tsmsm09["ORDER_NO_SALE"] = tsmsm01["ORDER_NO"];//ADD BY SUNHAO 20220810 记录发货的合同号
			}






			//部分字段名不一致的字段单独赋值


			tsmsm09["MAT_WT"] = tsmsm09["MAT_WT"].ToDecimal() * zlfh;	/* 材料重量 */
			tsmsm09["LOG_IP_ADDRESS"] = s.fore_ip;


			if (tsmsm09["CANCEL_CAUSE_CODE"].ToString().GetLength() > 2)
			{
				tsmsm09["CANCEL_CAUSE_CODE"] = " ";
			}

			tsmsm09.TrimOrBlank();

			//写履历档信息
			Log::Info("", __FUNCTION__, "写履历档信息[{0}]", tsmsm09["EVENT_NAME"].ToString());
			//tsmsm09[""]Print();
			tsmsm09.Insert();

		}

		sprintf(s.msg, _RES("GCRSS0000002")/*处理成功。*/);
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		Log::Error("", __FUNCTION__, "error=[{0}]", str);

		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), sizeof(s.msg) - 1);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	return doFlag;
}

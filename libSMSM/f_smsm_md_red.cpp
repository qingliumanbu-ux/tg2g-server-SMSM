/****************************************************
*	程序功能：码单红冲处理函数						*
*	编制日期：2024-3-3 				                		*
*	编制人员：李振						        *
*	传入参数：材料号						*
*****************************************************/
//码单生成函数
#include "stdafx.h"

//入库函数
int f_wmsmsm_stock_in(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
//发货履历
int f_smsm_trace(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
//物料跟踪
int f_mmsm99(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);

BM2_FUNCTION_EXPORT
int f_smsm_md_red(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn)
{
	CTracer log(__FUNCTION__);
	int doFlag = 0;
	int blkNum = 0;
	int ret = 0;
	CModel tmmsm01("TMMSM01");
	CModel twma0 = CModel("TWMA0");
	CModel tsmsm01("TSMSM01");
	CModel tsmsm03("TSMSM03");
	CModel hsmsm03("HSMSM03");
	CModel tsmsm05("TSMSM05");
	CString	sqlstr("");

	/* ***** 数据库操作类定义 ***** */
	CDbCommand cmd_inq(conn);
	CString datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");

	CString  v_mat_kind = "";
	CString  mat_no = "";
	CString  stacking_no = "", pre_out_flag = "0";//pre_out_flag 0为正常出 1为预出库码单
	CString bill_of_lading_no = "";
	EIClass	bcls_mm_sm02;
	try
	{
		//记录发货履历
		CString	record_name = "SMSM09";
		if (bcls_rec->Tables.IndexOf(record_name) < 0)
		{
			bcls_rec->Tables.Add(record_name);
			bcls_rec->Tables[record_name].Columns.Add(tsmsm03);
			bcls_rec->Tables[record_name].Columns.Add(DT_STRING, "event_id");
		}
		bcls_rec->Tables[record_name].Rows.Clear();


		CString blkname = "MM0099";

		//码单红冲的物料跟踪
		if (!bcls_mm_sm02.Tables.Contains(blkname))//判断是否有块，如果没有指定块的话，则加上
		{
			bcls_mm_sm02.Tables.Add(blkname);
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "EVENT_ID");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "EVENT_LINE_TYPE");	// 事件产线类型
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "SYSTEM_ID");			// 系统标识4位，到二级模块
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "FUNC_ID");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "MAT_NO");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "BILL_OF_LADING_NO");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "ORDER_NO");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "STOCK_NO");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "MAT_KIND");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "CONFM_FLAG");
			bcls_mm_sm02.Tables[blkname].Columns.Add(DT_STRING, "CONFM_PLAN_NO");
		}
		bcls_mm_sm02.Tables[blkname].Rows.Clear();
		//仓库入库函数块
		blkNum = bcls_rec->Tables.IndexOf("WM_STOCK");
		if (blkNum < 0)
		{
			bcls_rec->Tables.Add("WM_STOCK");
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "MAT_NO");
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "STOCK_OPER_ORDER");        //库业务类型
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_DECIMAL, "STOCK_OPER_ORDER_DIV");   //业务类型内区分
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "STOCK_NO");				//库号
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "STOCK_PLACE_NO");			//材料库位号
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "ROWNO");					//行号
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "COLUMN_NO");				//列号
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "LAYERNO");					//层号
			bcls_rec->Tables["WM_STOCK"].Columns.Add(DT_STRING, "STOCK_PLACE_POSITION");	//库位内位置
		}

		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			v_mat_kind = "SM";
			if (bcls_rec->Tables[0].Columns.Contains("MAT_KIND"))
			{
				v_mat_kind = bcls_rec->Tables[0].Rows[0]["MAT_KIND"].ToString();
			}
			mat_no = bcls_rec->Tables[0].Rows[0]["MAT_NO"].ToString();
			Log::Info("", __FUNCTION__, "mat_no[{0}] MAT_KIND[{1}]", mat_no, v_mat_kind);



			Log::Info("", __FUNCTION__, " xxxxxxxxxxxx2222xxxx");
			//准发材料档信息拉回
			hsmsm03["MAT_NO"].ToString() = mat_no;
			tsmsm03["MAT_NO"].ToString() = mat_no;
			tsmsm03["PRE_OUT_FLAG"].ToString() = "1";
			if (hsmsm03.Query("MAT_NO"))
			{
				bill_of_lading_no = hsmsm03["BILL_OF_LADING_NO"].ToString();
				Log::Info("", __FUNCTION__, " BILL_OF_LADING_NO[{0}]", bill_of_lading_no);
				tsmsm01["BILL_OF_LADING_NO"].ToString() = bill_of_lading_no;
				if (bill_of_lading_no.Trim() != "")
				{
					//删除该材料所属提单信息
					int count = tsmsm01.Delete("BILL_OF_LADING_NO");
					Log::Info("", __FUNCTION__, " BILL_OF_LADING_NO[{0}]删除count[{1}]", bill_of_lading_no, count);
				}
				//清空提单信息
				hsmsm03["DELIVY_PLAN_NO"].ToString() = " ";
				hsmsm03["BILL_OF_LADING_NO"].ToString() = " ";
				//清空部分已出厂信息
				hsmsm03["REC_REVISE_TIME"] = datetime;
				hsmsm03["REC_REVISOR"] = s.svc_name;
				hsmsm03["STACKING_NO"] = " ";
				hsmsm03["OUT_STOCK_DATE"] = " ";
				hsmsm03["OUT_MAKER"] = " ";
				hsmsm03["PD_SEQ_NO"] = " ";
				hsmsm03["OUT_YARD_TIME"] = " ";
				hsmsm03["TRUCK_NO"] = " ";
				hsmsm03["TRUCK_LAYER"] = 0;
				hsmsm03["SOURCE_POS"] = " ";
				hsmsm03["SHIFT_GROUP"] = " ";
				hsmsm03["SHIFT_NO"] = " ";
				hsmsm03["OUTPASS_NO"] = " ";
				//设置材料状态为已接收提单
				hsmsm03["MAT_STATUS"] = "4";

				//准发材料档拉回
				tsmsm03.Reset();
				tsmsm03.CopyFrom(hsmsm03);//复制历史档记录
				tsmsm03["PRE_OUT_FLAG"].ToString() = "0";//冲回来默认正常发货
				tsmsm03.Insert();
				hsmsm03.Delete();
			}
			else if (tsmsm03.Query("MAT_NO,PRE_OUT_FLAG"))//在线未归档，进一步查看是否是预出第一段码单
			{
				pre_out_flag = "1";
				tsmsm03["DELIVY_PLAN_NO"] = " ";
				tsmsm03["BILL_OF_LADING_NO"] = " ";
				//清空部分已出厂信息
				tsmsm03["REC_REVISE_TIME"] = datetime;
				tsmsm03["REC_REVISOR"] = s.svc_name;
				//设置材料状态为准发确认
				tsmsm03["MAT_STATUS"] = "4";
				//红冲后设置预出标记为0
				tsmsm03["PRE_OUT_FLAG"] = "0";
				//清空码单号
				tsmsm03["STACKING_NO"] = " ";
				tsmsm03.Update("DELIVY_PLAN_NO,BILL_OF_LADING_NO,REC_REVISE_TIME,REC_REVISOR,MAT_STATUS,PRE_OUT_FLAG,STACKING_NO", "MAT_NO");
			}
			else
			{
				sprintf(s.msg, mat_no + "没有唯一TDEHP03记录！！");
				Log::Error("", __FUNCTION__, mat_no + "没有唯一TDEHP03记录！！");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
			if (pre_out_flag == "0")
			{
				//物料档拉回
				bcls_mm_sm02.Tables[blkname].Rows.Add();
				bcls_mm_sm02.Tables[blkname].Rows[i]["EVENT_ID"] = "SM02"; //SM02:码单红冲 
				bcls_mm_sm02.Tables[blkname].Rows[i]["FUNC_ID"] = s.svc_name;
				bcls_mm_sm02.Tables[blkname].Rows[i]["SYSTEM_ID"] = tsmsm03["MAT_KIND"].ToString();
				bcls_mm_sm02.Tables[blkname].Rows[i]["EVENT_LINE_TYPE"] = tsmsm03["MAT_KIND"].ToString();
				bcls_mm_sm02.Tables[blkname].Rows[i]["MAT_NO"] = tsmsm03["MAT_NO"].ToString();
				bcls_mm_sm02.Tables[blkname].Rows[i]["MAT_KIND"] = tsmsm03["MAT_KIND"].ToString();
				bcls_mm_sm02.Tables[blkname].Rows[i]["STOCK_NO"] = tsmsm03["STOCK_NO"].ToString();
				bcls_mm_sm02.Tables[blkname].Rows[i]["CONFM_FLAG"] = "2";
				bcls_mm_sm02.Tables[blkname].Rows[i]["CONFM_PLAN_NO"] = " ";

				//装载入库函数块
				twma0["MAT_NO"] = tmmsm01["MAT_NO"];
				twma0["STOCK_OPER_ORDER"] = "1B";
				twma0.Query("MAT_NO,STOCK_OPER_ORDER");
				bcls_rec->Tables["WM_STOCK"].Rows.Add();
				bcls_rec->Tables["WM_STOCK"].Rows[0]["MAT_NO"] = tmmsm01["MAT_NO"];
				bcls_rec->Tables["WM_STOCK"].Rows[0]["STOCK_OPER_ORDER"] = "1B";					//库业务类型
				bcls_rec->Tables["WM_STOCK"].Rows[0]["STOCK_OPER_ORDER_DIV"] = "1";					//业务类型内区分
				bcls_rec->Tables["WM_STOCK"].Rows[0]["STOCK_NO"] = "SYA";			//库号
				bcls_rec->Tables["WM_STOCK"].Rows[0]["STOCK_PLACE_NO"] = "SYA";						//材料库位号
				bcls_rec->Tables["WM_STOCK"].Rows[0]["ROWNO"] = " ";								//行号
				bcls_rec->Tables["WM_STOCK"].Rows[0]["COLUMN_NO"] = " ";							//列号
				bcls_rec->Tables["WM_STOCK"].Rows[0]["LAYERNO"] = 0;								//层号
				bcls_rec->Tables["WM_STOCK"].Rows[0]["STOCK_PLACE_POSITION"] = "1";				//库位内位置
			}

			/* ***** 记录发货履历 ***** */
			bcls_rec->Tables["SMSM09"].Rows.Add();
			bcls_rec->Tables["SMSM09"].Rows[i].Merge(tsmsm03);
			bcls_rec->Tables["SMSM09"].Rows[i]["event_id"] = "43";  //码单红冲


		}

		if (bcls_mm_sm02.Tables[blkname].Rows.get_Count() > 0)
		{
			
			doFlag = f_mmsm99(&bcls_mm_sm02, bcls_ret, conn);
			

			if (doFlag < 0)
			{

				Log::Debug("", __FUNCTION__, "物料跟踪码单红冲函数调用出错.");
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}

		/********** 抛发货履历 **************/
		if (bcls_rec->Tables["SMSM09"].Rows.get_Count() > 0)
		{
			doFlag = f_smsm_trace(bcls_rec, bcls_ret, conn);
			if (doFlag < 0)
			{
				throw CApplicationException(-1, s.msg, s.svc_name);
			}
		}
		//调用入库函数
		if (bcls_rec->Tables["WM_STOCK"].Rows.get_Count() > 0)
		{
			doFlag = f_wmsmsm_stock_in(bcls_rec, bcls_ret, conn);
			if (doFlag != 0)
			{
				throw CApplicationException(-1, s.msg, log.Location);
			}
		}

	}
	catch (CDbException& ex)
	{
		CFormattable arguments[] = { ex.GetCode(), ex.GetMsg() };
		CMessageFormat::Format(s.msg, "Database Error,sqlcode=[{0}],sqlmsg=[{1}]", arguments, 2);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);
		s.flag = -1;
		doFlag = -1;
	}
	catch (CApplicationException& ex)
	{
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch (CException& ex)
	{
		strcpy(s.msg, ex.GetMsg());
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	return doFlag;
}



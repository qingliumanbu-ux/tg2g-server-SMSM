/************************************************
*	程序名称：产成品发货处理——码单生成		*
*	编制日期：2024-4-1   	                    *
*	编 制 人：ke2111					            *
*************************************************/
#include "stdafx.h"




//程序用头文件




//#include "tom01.h"


/* ***** 外部函数申明 ***** */
int f_sm00_md_no(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);
int f_mmsm99(EIClass* bcls_rec, EIClass* bcls_ret, CDbConnection* conn);

// service入口
BM2F_ENTERACE(sm0008_pro)
/* -EP_SYSTEM_HEAD_END */
int f_sm0008_pro(EIClass * bcls_rec, EIClass * bcls_ret, CDbConnection * conn)
{
	CTracer log(__FUNCTION__);
	/*程序用变量*/
	int blkNum = 0;
	int	ret = 0;
	int	doFlag = 0;
	int	fetchRowCount = 0;
	int	fetchRowCount1 = 0;
	int	blkSeq = 0;
	int	i = 0;
	
	CString	stacking_no = "";			/* 码单号 */

	CModel tsmsm01("TSMSM01");
	CModel tsmsm03("TSMSM03");
	CModel tsmsm05("TSMSM05");
	
	

	CString	datetime = "";
	CString	date = "";
	CString	time = "";
	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
	date = datetime.Substring(0, 8);
	time = datetime.Substring(8, 6);

	

	CDbCommand cmd_inq(conn);
	CDbCommand cmd_inq_loop(conn);
	CDbCommand cmd_inq_loop2(conn);
	CDbCommand cmd_upd(conn);
	CString sqlstr;

	try
	{
		EIClass in_rec;
		EIClass out_ret;
		in_rec.Tables[0].Columns.Add(DT_STRING, "STOCK_NO");
		in_rec.Tables[0].Rows.Add();

		in_rec.Tables[0].Rows[0]["STOCK_NO"] = bcls_rec->Tables[2].Rows[0]["STOCK_NO"].ToString();
		doFlag = f_sm00_md_no(&in_rec, &out_ret, conn);
		if (doFlag < 0)
		{
			sprintf(s.msg, "生成码单号失败！");
			throw	CApplicationException(-1, s.msg, log.Location);
		}
		stacking_no = out_ret.Tables[0].Rows[0]["STACKING_NO"].ToString();

		int total_count = 0;
		double total_weight = 0;
		for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++)
		{
			tsmsm03.MergeFrom(bcls_rec->Tables[0].Rows[i]);
			tsmsm03["MAT_STATUS"] = "9";
			tsmsm03["STACKING_NO"] = stacking_no;
			tsmsm03.Update("MAT_STATUS,STACKING_NO", "MAT_NO");

			total_count += 1;
			total_weight = total_weight + tsmsm03["MAT_WT"].ToDouble();
		}
		tsmsm05.MergeFrom(bcls_rec->Tables[2].Rows[0]);
		tsmsm05["REC_CREATOR"] = s.userid;
		tsmsm05["REC_CREATE_TIME"] = datetime;
		tsmsm05["STACKING_NO"] = stacking_no;
		tsmsm05["STACKING_NUM"] = total_count;
		tsmsm05["STACKING_WT"] = total_weight;
		tsmsm05["VEHICLE_NO"] = bcls_rec->Tables[1].Rows[0]["VEHICLE_NO"];
		tsmsm05.Insert();
		
	}
	catch (CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { ex.GetCode() };
		CMessageFormat::Format(s.msg, _RES("GCRSS0000006")/*数据库处理出错，sqlcode=[{0}]。请联系系统维护人员。*/, arguments, 1);
		CString str = sqlstr + "\r\n" + ex.GetMsg();
		strncpy(s.sysmsg, (const char*)str, sizeof(s.sysmsg) - 1);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch (CApplicationException& ex)  //捕获应用错误
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
	if (doFlag < 0)
	{
		CFormattable arguments[] = { s.svc_name, s.msg };	// 主程序用
		//CFormattable arguments[] = { __FUNCTION__, s.msg };	// 函数用
		CMessageFormat::Format(s.msg, "{0} :{1}", arguments, 2);
	}

	return doFlag;

}

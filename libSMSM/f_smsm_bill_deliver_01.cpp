/* ****************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_cm_3000s4_snd
*  程序描述			: 热轧发货实绩电文发送
*  备注说明			:
*  修改历史			:
*  		wuxin 2011-12-29			(ADD)程序建立
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除
#include "epex.h"
#include "tsmpe11.h"
#include "tsmpe02.h"
#include "x3000s4.h"


//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明 
BM2_FUNCTION_EXPORT
 int f_smsm_bill_deliver_01(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn) 
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0,i,blkNum,fetchRowCount,row_count=0,ret=0,j=0;

	/* 业务变量 */
	CString	datetime("");
	CString	blkname("");	/* 块名 */

	 /* ***** 程序变量 ***** */
   CString c_user=s.userid,c_stock_no=" ",c_stacking_no=" ",c_rowid=" ",c_mat_no= "";

   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5(""),sqlstr6(""),sqlstr7(""),sqlstr8("");                    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);
    CTSMPE11 tsmpe11(conn);
  	CTSMPE02 tsmpe02(conn);
	C3000S4  x3000s4(conn);
   /* ***** 创建电文处理对象 ***** */
	EPEX epex(&s);
	CString c_tc_no="3000S4";


/* ***** 应用程序开始处理 ***** */
	try
	{
	
	    datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");  

		switch(conn->DatabaseKind)
		{
		case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
		case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
		case DB_KIND_MSSQL:	        // MS SQL Server数据库
		case DB_KIND_ORACLE:	        // Oracle 数据库
		default: 
		/* ***** tsmpea7 该表不需要建立索引，一个主键已经足够 ***** */
		sqlstr1 = CString(    
				   " SELECT * FROM tsmpe11 where stacking_no = @stacking_no "
										);
										
		sqlstr2 = CString(    
				   " SELECT * FROM tsmpe02 where stacking_no = @stacking_no "  
										);	 

		break; 
		}  

		/* ***** 电文初始化  ***** */
		 //EDLog(1,1,"anqi1");
		ret = 0;
		ret = epex.Initialize(c_tc_no);			 
		if	(ret != 0)
		{
			throw CApplicationException(-1, s.msg, s.svc_name);
		}
         //EDLog(1,1,"anqi2");
        for (int i = 0; i < bcls_rec->Tables[0].Rows.get_Count(); i++) 
		{ 
			c_stacking_no = bcls_rec->Tables[0].Rows[0]["stacking_no"].ToString().TrimOrBlank();
 
             //EDLog(1,1,c_stacking_no);

			if ( 0 == c_stacking_no.Compare(" ") )
			{
			  break;
			}

			/* *****获取码单信息   ***** */
			sqlstr = sqlstr1;
			execute_sql.SetCommandText( sqlstr ); 
			execute_sql.ExecuteReader(); 

			while(execute_sql.Read())
			{   
			 execute_sql.Fetch(tsmpe11);   
			}
			execute_sql.Close();

			/* ***** 组织码单信息  ***** */
            //x3000s4 = tsmpe11;
			x3000s4.CopyFrom(tsmpe11);

	        /* ***** 电文赋值  ***** */
			 ret = 0;
			 ret = epex.SetValue(0, x3000s4);					 
			 if	(ret != 0)
			 {
				throw CApplicationException(-1, s.msg, s.svc_name);
			 }


			/* *****获取材料信息   ***** */
			sqlstr = sqlstr2;
			execute_sql.SetCommandText( sqlstr ); 
			execute_sql.ExecuteReader(); 
             
			j = 0;
			while(execute_sql.Read())
			{   
			 execute_sql.Fetch(tsmpe02);  
             /* ***** 组织材料信息  ***** */ 
			 if(epex.SetValue("MAT_NO",j,tsmpe02.MAT_NO)<0)  
			 {
			   throw CApplicationException(-1, s.msg, s.svc_name);
			 }

			 if(epex.SetValue("MAT_WT",j,tsmpe02.MAT_NO)<0)   
			 {
			   throw CApplicationException(-1, s.msg, s.svc_name);
			 }

             j++; 

			}
			execute_sql.Close(); 
	   
	   /* ***** 电文发送  ***** */
			ret = 0;
			ret = epex.SendTele();					 
			if	(ret != 0)
			 {
				throw CApplicationException(-1, s.msg, s.svc_name);
			 }
		
		}



	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TSMPE02"), ex.GetCode() };
		CMessageFormat::Format(s.msg,  _RES("GCRSS0000017")/*读取数据失败,表[{0}],sqlcode=[{1}]。请联系系统维护人员。*/, arguments, 2);

		CString str = sqlstr + "\r\n" + ex.GetMsg();
		//EDLog(1,1, "error=[%s]", (const char*)str );

		strncpy(s.sysmsg, (const char*)str, 399);  //返回前台，与EI.EIInfo对象的sys_info.sysmsg参数对应
		s.flag = -1;
		doFlag = -1;                 //数据库异常时返回-1，事务将被回滚
	}
	catch(CApplicationException& ex)  //捕获应用错误
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}
	catch(CException& ex)
	{
		strncpy(s.msg, (const char*)ex.GetMsg(), 399);
		s.flag = ex.GetCode();
		doFlag = -1;
	}

	return doFlag;
}

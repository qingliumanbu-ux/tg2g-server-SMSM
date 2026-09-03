/* **************************************************************************
*	Copyright (c) Baosight Corporation 2008 . All Rights Reserved.
*  	BM2PES 宝信生产执行系统
*****************************************************************************
*  程序名称			: f_smsm_bill_deliver
*  程序描述			: 发货实绩输入处理
*  备注说明			:
*  修改历史			:
*  		wuxin 2011-12-27			(ADD)程序建立
*			... ...
* **************************************************************************** */
/* C/C++ 的标准头文件部分 */
#include "stdafx.h"		// 框架头，不可删除 
#include "tsmpe02.h" 
#include "tsmpe00.h"
#include "tsmpe01.h"
#include "tsmpe10.h"
#include "tsmpe11.h"

//名称空间引用
using namespace BM2;
using namespace BM2::Data;
using namespace BM2::Data::DbClient;

//外部函数声明

BM2_FUNCTION_IMPORT
 int f_smsm_bill_deliver_01(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_smsm_bill_deliver_02(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn);
int f_sm00_count(CString p_confm_plan_no, CString p_userid, CDbConnection * conn);

BM2_FUNCTION_IMPORT
 int f_sm00_md_no(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn);

BM2_FUNCTION_EXPORT
 int f_smsm_bill_deliver(EIClass *bcls_rec,EIClass *bcls_ret, CDbConnection * conn) 
{
	CTracer log(__FUNCTION__);  // 系统日志，必须在代码段开始处定义

	/*程序用变量*/
	int doFlag = 0,i,blkNum,fetchRowCount,row_count=0,ret=0;
	int v_ii = 0;	// 读取计数器
	CString LOADING_NO = "";

	/* 业务变量 */
	CString	datetime("");
	CString	blkname("");	/* 块名 */

	 /* ***** 程序变量 ***** */
   CString c_user=s.userid,c_stock_no=" ",c_stacking_no=" ",c_rowid=" ";

   /* ***** 数据库SQL操作字符串 ***** */
	CString	sqlstr(""),sqlstr1(""),sqlstr2(""),sqlstr3(""),sqlstr4(""),sqlstr5(""),sqlstr6(""),sqlstr7(""),sqlstr8(""),sqlstr9(""),sqlstr10(""),sqlstr11(""),sqlstr12("");                    
	
   /* ***** 数据库操作类定义 ***** */ 
	CDbCommand execute_sql(conn);
    CDbCommand execute_sql_01(conn);

    
	CTSMPE02 tsmpe02(conn);
	CTSMPE00 tsmpe00(conn);
	CTSMPE01 tsmpe01(conn);
	CTSMPE10 tsmpe10(conn);
	CTSMPE11 tsmpe11(conn);
	CModel ted21 = CModel("TED21");

   /* *******定义一个获取码单号 EIClass object */
    EIClass  ds_stacking_no_rec;
	         ds_stacking_no_rec.Tables[0].Columns.Add( DT_STRING, "stock_no"); 
			 ds_stacking_no_rec.Tables[0].Columns.Add( DT_STRING, "stacking_no"); 
			 ds_stacking_no_rec.Tables[0].Rows.Add();
	EIClass  ds_stacking_no_ret; 
 
			 


/* ***** 应用程序开始处理 ***** */
	try
	{
	
	datetime = CDateTime::Now().ToString("yyyyMMddHHmmss");
   
	/* ***** 查找没有材料的准发计划  ***** */ 
	switch(conn->DatabaseKind)
	{
	case DB_KIND_DB2:	        // DB2 数据库（未开Oracle兼容）
	case DB_KIND_DB2_ORACLE:	    // DB2 数据库（开Oracle兼容）
	case DB_KIND_MSSQL:	        // MS SQL Server数据库
	case DB_KIND_ORACLE:	        // Oracle 数据库
	default: 
	 
	sqlstr1 = CString(    
			"		select stock_no,bill_of_lading_no,order_no,vehicle_no,delivy_time     "
			"		 from tsmpe02                                                "
			"		 where confm_status = '9'                                    "
			"		 group by stock_no,bill_of_lading_no,order_no,vehicle_no,delivy_time  "  
									);	
    
	sqlstr2 = CString(    
		       " UPDATE TSMPE02 SET  stacking_no = @stacking_no " 
			   "                     WHERE stock_no = @stock_no "
	           "                       and bill_of_lading_no = @bill_of_lading_no "
			   "                       and order_no = @order_no "
			   "                       and vehicle_no = @vehicle_no "
			   "                       and delivy_time = @delivy_time "
									);	

	sqlstr3 = CString(    
		        " SELECT * FROM tsmpe10 WHERE bill_of_lading_no = @bill_of_lading_no "
									);	

    sqlstr4 = CString(    
		        " SELECT * FROM tsmpe00 WHERE order_no = @order_no and rownum = 1 "
									);	

	sqlstr5 = CString(    
		        " SELECT * FROM tsmpe02 WHERE stacking_no = @stacking_no and rownum = 1 "
									);	

	sqlstr6 = CString(    
		        " SELECT sum(1) stacking_num,sum(mat_wt) stacking_wt FROM tsmpe02 WHERE stacking_no = @stacking_no "
									);	
	
	sqlstr7 = CString(    
		        " INSERT INTO tsmpe12 SELECT * FROM tsmpe02 WHERE stacking_no = @stacking_no and confm_status = '9' "
									);
	
	sqlstr8 = CString(    
		        " DELETE FROM tsmpe02 WHERE stacking_no = @stacking_no and confm_status = '9' "
									);
    
    sqlstr11 = CString(/* 计算某提单下的计划量及发货量 */
		              "  update  tsmpe10 a set  "
					  "                        rec_revise_time    = @rec_revise_time,    "
					  "                        rec_revisor        = @rec_revisor,        "
					  "                        delivy_plan_status = '4',    "
	                  "                        (plan_num,plan_wt) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe02 b where a.bill_of_lading_no = b.bill_of_lading_no ), "
	                  "                        (delivy_num,delivy_wt ) = (select  NVL(SUM(1), 0), NVL(SUM(mat_wt), 0) from tsmpe12 b where a.bill_of_lading_no = b.bill_of_lading_no ) "
					  "                    where bill_of_lading_no = @bill_of_lading_no "
		 
									); 

	sqlstr12 = CString(/* 计算某提单的状态 */
		              "  update  tsmpe10 a set  "
					  "                        delivy_plan_status = '5'   " 
					  "                    where bill_of_lading_no = @bill_of_lading_no and plan_num = 0 "
		 
									); 

	        break; 
	  }  

	 
	 /* ***** 同一提单、合同、车号、离库时间的材料为一个码单  ***** */ 
	sqlstr = sqlstr1;
	execute_sql.SetCommandText( sqlstr ); 
	execute_sql.ExecuteReader();  
	while(execute_sql.Read())
	{   
		 tsmpe02.STOCK_NO          = execute_sql.GetString(1);
		 tsmpe02.BILL_OF_LADING_NO = execute_sql.GetString(2);
		 tsmpe02.ORDER_NO          = execute_sql.GetString(3);
		 tsmpe02.VEHICLE_NO        = execute_sql.GetString(4);
		 tsmpe02.DELIVY_TIME       = execute_sql.GetString(5);


		 if (v_ii == 0)
		 {
			 /* 生成装车清单号 */
			 ted21["SEQ_NAME"] = "LOADING_NO_" + tsmpe02.STOCK_NO;
			 //到流水号表按关键字读取记录
			 int count1 = ted21.QueryCount("SEQ_NAME");
			 if (count1 == 0)
			 {
				 //新增记录
				 ted21["SEQ_DESC"] = "仓库" + tsmpe02.STOCK_NO + "装车单流水号";
				 ted21["SEQ_BEGIN"] = 1;
				 ted21["SEQ_NOW"] = 0;
				 ted21["SEQ_END"] = 99999;
				 ted21["SEQ_PRE"] = "";	// 流水号前缀
				 ted21["SEQ_LEN"] = 5;
				 ted21["SEQ_RECYCLE_FLAG"] = "2";	// 0--最大值,1--按年复位,2--按月复位,3--按日,4--按周一,5--按周日
				 ted21["REC_CREATE_TIME"] = datetime;
				 ted21["REC_CREATOR"] = s.userid;
				 ted21.TrimOrBlank();
				 if (ted21.Insert() == false)
				 {
					 sprintf(s.msg, "新增流水号记录失败");
					 throw CApplicationException(-1, s.msg, log.Location);
				 }
			 }
			 LOADING_NO = tsmpe02.STOCK_NO + datetime.Substring(2, 4) + EPGetNextSeq(ted21["SEQ_NAME"], conn);
			 v_ii++;
		 }


		 /* ***** 根据库区生成码单号  ***** */ 
		 ret = 0;
		 ds_stacking_no_rec.Tables[0].Rows[0]["stock_no"] = tsmpe02.STOCK_NO;
		 ret = f_sm00_md_no(&ds_stacking_no_rec,&ds_stacking_no_ret, conn);
		 if (ret < 0)
		 {
			 throw CApplicationException(-1, s.msg, s.svc_name);
		 }

		 tsmpe02.STACKING_NO = ds_stacking_no_ret.Tables[0].Rows[0]["stacking_no"].ToString().TrimOrBlank();  

		 /* ***** 将码单号回写到材料表  ***** */  
			sqlstr = sqlstr2; 
			execute_sql_01.SetCommandText( sqlstr ); 
			execute_sql_01.Parameters.Set( "stock_no" ,tsmpe02.STOCK_NO );
			execute_sql_01.Parameters.Set( "stacking_no" ,tsmpe02.STACKING_NO  );
			execute_sql_01.Parameters.Set( "bill_of_lading_no" , tsmpe02.BILL_OF_LADING_NO );
			execute_sql_01.Parameters.Set( "order_no" ,    tsmpe02.ORDER_NO);
			execute_sql_01.Parameters.Set( "vehicle_no" ,  tsmpe02.VEHICLE_NO );
			execute_sql_01.Parameters.Set( "delivy_time" , tsmpe02.DELIVY_TIME );
			execute_sql_01.ExecuteNonQuery();

		/*  ***** 获取提单信息 ***** */ 
			sqlstr = sqlstr3; 
			execute_sql_01.SetCommandText( sqlstr ); 
			execute_sql_01.Parameters.Set( "bill_of_lading_no" , tsmpe02.BILL_OF_LADING_NO );

			execute_sql_01.ExecuteReader();
			while(execute_sql_01.Read())
			{   
			   execute_sql_01.Fetch(tsmpe10);  
			 } 
			 execute_sql_01.Close(); 
 
		/*  ***** 获取准发单据信息 ***** */ 
			sqlstr = sqlstr4; 
			execute_sql_01.SetCommandText( sqlstr ); 
			execute_sql_01.Parameters.Set( "order_no" , tsmpe02.ORDER_NO );

			execute_sql_01.ExecuteReader();
			while(execute_sql_01.Read())
			{   
			   execute_sql_01.Fetch(tsmpe00);  
			 } 
			 execute_sql_01.Close();  

		/*  ***** 获取准发材料信息 ***** */ 
			sqlstr = sqlstr5; 
			execute_sql_01.SetCommandText( sqlstr ); 
			execute_sql_01.Parameters.Set( "stacking_no" , tsmpe02.STACKING_NO );

			execute_sql_01.ExecuteReader();
			while(execute_sql_01.Read())
			{   
			   execute_sql_01.Fetch(tsmpe02);  
			 } 
			 execute_sql_01.Close();

		/*  ***** 计算码单重量 ***** */ 
			sqlstr = sqlstr6; 
			execute_sql_01.SetCommandText( sqlstr ); 
			execute_sql_01.Parameters.Set( "stacking_no" , tsmpe02.STACKING_NO );

			execute_sql_01.ExecuteReader();
			while(execute_sql_01.Read())
			{   
			    tsmpe11.STACKING_NUM           = execute_sql_01.GetDecimal(1);
				tsmpe11.STACKING_WT            = execute_sql_01.GetDecimal(2);
			 } 
			 execute_sql_01.Close();

			
         /* ***** 形成码单表  ***** */ 
			tsmpe11.REC_CREATE_TIME =  datetime;
			tsmpe11.REC_CREATOR =  c_user;
			tsmpe11.REC_REVISE_TIME =  " ";
			tsmpe11.REC_REVISOR =  " "; 
			tsmpe11.ARCHIVE_FLAG =  " ";
			tsmpe11.STACKING_NO =  tsmpe02.STACKING_NO;
			tsmpe11.FACTORY_DIV =  tsmpe02.FACTORY_DIV;
			tsmpe11.STOCK_NO =  tsmpe02.STOCK_NO;
			tsmpe11.BILL_OF_LADING_NO =  tsmpe02.BILL_OF_LADING_NO;

			tsmpe11.CONFM_PLAN_NO =  tsmpe00.CONFM_PLAN_NO;
			tsmpe11.READY_BILL_NO =  tsmpe00.READY_BILL_NO;
			 
			tsmpe11.VEHICLE_NO =  tsmpe02.VEHICLE_NO;
			tsmpe11.ORDER_NO =  tsmpe02.ORDER_NO;
			tsmpe11.EXPORT_FLAG =  tsmpe00.EXPORT_FLAG;
			tsmpe11.SG_SIGN =  tsmpe00.SG_SIGN;
			tsmpe11.SG_STD =  tsmpe00.SG_STD;
			tsmpe11.PROD_CODE =  tsmpe00.PROD_CODE;
			tsmpe11.PROD_CNAME =  tsmpe00.PROD_CNAME;
			tsmpe11.PROD_ENAME =  tsmpe00.PROD_ENAME;
			tsmpe11.ORDER_THICK = tsmpe00.ORDER_THICK;
			tsmpe11.ORDER_WIDTH = tsmpe00.ORDER_WIDTH;
			tsmpe11.ORDER_LEN = tsmpe00.ORDER_LEN;
			tsmpe11.ORDER_LEN_MIN = tsmpe00.ORDER_MIN_LEN;
			tsmpe11.ORDER_LEN_MAX = tsmpe00.ORDER_MAX_LEN;
			tsmpe11.DELIVY_TIME =  tsmpe02.DELIVY_TIME;	 
			tsmpe11.DELIVY_SHIFT =  tsmpe02.DELIVY_SHIFT;
			tsmpe11.DELIVY_GROUP =  tsmpe02.DELIVY_GROUP;
			tsmpe11.DELIVY_MAKER =  tsmpe02.DELIVY_MAKER;
			tsmpe11.TRNP_MODE_CODE =  tsmpe10.TRNP_MODE_CODE;
			tsmpe11.CONSIGNE_NAME =  tsmpe00.CONSIGN_CUST_CNAME;
			tsmpe11.ORDER_CUST_CNAME =  tsmpe00.ORDER_CUST_CNAME;
			tsmpe11.BALANCE_USER_NAME =  tsmpe10.BALANCE_USER_NAME;
			tsmpe11.CONVEY_UNIT_NAME =  tsmpe10.CONVEY_UNIT_NAME;
			tsmpe11.DELIVY_PLACE_NAME =  tsmpe10.DELIVY_PLACE_NAME;
			tsmpe11.PRIVATE_ROUTE_CODE =  tsmpe00.PRIVATE_ROUTE_CODE;
			tsmpe11.PRIVATE_ROUTE_NAME =  tsmpe00.PRIVATE_ROUTE_NAME;
			tsmpe11.STACKING_PRINTS = 0; 
			tsmpe11.DELIVY_REMARK =  tsmpe10.DELIVY_REMARK;

			tsmpe11.LOADING_NO = LOADING_NO;

			/* 插入码单表 */
			if (!tsmpe11.Insert())
			{ 
			throw CApplicationException(-1, s.msg, s.svc_name);
			}

//			/* *****以码单为单位发送电文给 L4  ***** */ 
//			 ret = 0;
//		     ds_stacking_no_rec.Tables[0].Rows[0]["stacking_no"] = tsmpe02.STACKING_NO;
//		     ret = f_smsm_bill_deliver_01(&ds_stacking_no_rec,&ds_stacking_no_ret, conn);
//			 if (ret < 0)
//			 {
//				 throw CApplicationException(-1, s.msg, s.svc_name);
//			 } 
//            /* *****以码单下的材料为单位调用物料程序  ***** */ 
//			 ret = 0;
//		     ds_stacking_no_rec.Tables[0].Rows[0]["stacking_no"] = tsmpe02.STACKING_NO;
//		     ret = f_smsm_bill_deliver_02(&ds_stacking_no_rec,&ds_stacking_no_ret, conn);
//			 if (ret < 0)
//			 {
//				 throw CApplicationException(-1, s.msg, s.svc_name);
//			 } 
			/* *****将已经发货的材料写到发货历史表  ***** */  
			 /* ***** 将码单号回写到材料表  ***** */  
			sqlstr = sqlstr7;  
			execute_sql_01.SetCommandText( sqlstr );  
			execute_sql_01.Parameters.Set( "stacking_no" ,tsmpe02.STACKING_NO  ); 
			execute_sql_01.ExecuteNonQuery();

			/* ***** 删除已经发货的材料  ***** */  
			sqlstr = sqlstr8; 
			execute_sql_01.SetCommandText( sqlstr );  
			execute_sql_01.Parameters.Set( "stacking_no" ,tsmpe02.STACKING_NO  ); 
			execute_sql_01.ExecuteNonQuery();

            
			/* ***** 重新计算该提单的发货量，计划量  ***** */  
			sqlstr = sqlstr11; 
			execute_sql_01.SetCommandText( sqlstr );  
			execute_sql_01.Parameters.Set( "bill_of_lading_no" ,tsmpe02.BILL_OF_LADING_NO  ); 
			execute_sql_01.Parameters.Set( "rec_revise_time" ,datetime  ); 
			execute_sql_01.Parameters.Set( "rec_revisor" ,c_user  ); 
			execute_sql_01.ExecuteNonQuery();
			/* ***** 重新计算该准发计划下的发货量，计划量  ***** */  
			doFlag = f_sm00_count(tsmpe02.CONFM_PLAN_NO, tsmpe02.REC_REVISOR,conn);
			if (doFlag < 0)
			 { 
			   doFlag = -1;
			   sprintf(s.msg ,"f_sm00_count函数调用出错.");
			   throw CApplicationException(-1, s.msg, s.svc_name);
			 } 
 
		  
	 } 
	 execute_sql.Close();  
	  

	}
	catch(CDbException& ex)  //捕获数据库操作异常
	{
		CFormattable arguments[] = { _S("TSMPEA7"), ex.GetCode() };
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

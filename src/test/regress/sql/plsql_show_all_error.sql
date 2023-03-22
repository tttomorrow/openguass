set plsql_show_all_error to on;
truncate DBE_PLDEVELOPER.gs_source;
truncate DBE_PLDEVELOPER.gs_errors;
create table vector_to_number_tab_002(COL_TONUM varchar) ;
insert into vector_to_number_tab_002 values(to_number(+123.456, '9.9EEEE')); 
CREATE OR REPLACE PACKAGE AA AS

  type ref_cursor IS ref CURSOR;

  PROCEDURE get_info(appinfo OUT ref_cursor);

  PROCEDURE get_info(appname IN varchar2, servinfo OUT ref_cursor);

  PROCEDURE get_switch(appname    IN varchar2,
                                    switchinfo OUT ref_cursor);

  PROCEDURE get_use(checkers OUT ref_cursor);

  PROCEDURE get_define(bb OUT ref_cursor);

  PROCEDURE get_resource_define(bbOUT ref_cursor);

  PROCEDURE get_bb_info(bbRef OUT ref_cursor);

  PROCEDURE get_resource_info(bbRef OUT ref_cursor);
END AA;
/
select id from dbe_pldeveloper.gs_source;
CREATE OR REPLACE PACKAGE AA AS

  type ref_cursor IS ref CURSOR;

  PROCEDURE get_info(appinfo OUT ref_cursor);

  PROCEDURE get_info(appname IN varchar2, servinfo OUT ref_cursor);

  PROCEDURE get_switch(appname    IN varchar2,
                                    switchinfo OUT ref_cursor);

  PROCEDURE get_use(checkers OUT ref_cursor);

  PROCEDURE get_define(bb OUT ref_cursor);

  PROCEDURE get_resource_define(bbOUT ref_cursor);

  PROCEDURE get_bb_info(bbRef OUT ref_cursor);

  PROCEDURE get_resource_info(bbRef OUT ref_cursor);
END AA;
/
select id from dbe_pldeveloper.gs_source;

CREATE OR REPLACE PROCEDURE exce_pro2()
AS
a int;
b int;
BEGIN
 a:=2/0;
 EXCEPTION
  WHEN division_by_zeros THEN
   insert into t1 value(1);
END;
/

create or replace procedure pro1
as
begin
drop t1; --缺失table关键字
drop tables t1; --错写
crop table t1; --错写
create table t1 t2(c1 int,c2 int);--表名不正确
create table t1(c1 ,c2 int);--漏写数据类型
create tables t1(c1 int,c2 int); --行号不对
creat table t1(c1 int,c2 int); --未标行号
creat table (c1 int,c2 int) t1; --顺序错误
end;
/

select line,src from dbe_pldeveloper.gs_errors where name='pro1';

CREATE OR REPLACE PACKAGE AA is


  TYPE r_list IS RECORD(
    indextype aa_mx_stat_info.indextype%TYPE,
    value number);

  TYPE cur_list IS REF CURSOR RETURN r_list;

  TYPE r_avgop_time IS RECORD(
    opname  aa_mx_stat_opavgtime_info.opname%TYPE,
    avgtime number);

  TYPE cur_avgop_time IS REF CURSOR RETURN r_avgop_time;

  aaedure aa_update_value(i_hostname  in varchar2,
                                   i_indextype in varchar2,
                                   i_value number);

  aaedure aa_value(o_list OUT cur_list);

  aaedure aa_del_value;

  aaedure aa_avgop_time(o_avgop_time OUT cur_avgop_time);

  aaedure aa_del_avgop_time;

  aaedure aa_update_monitor_flag(i_switchType  in varchar2,
                                     i_channelType in varchar2,
                                     i_action      in varchar2,
                                     i_modifyuser  in varchar2,
                                     i_hostname    in varchar2);
end AA;

/

CREATE OR REPLACE PACKAGE AA as

  procedure aa_remove(p_self in out nocopy json, pair_name varchar2);
  procedure aa_bb(p_self in out nocopy json, pair_name varchar2, pair_value json_value, position pls_integer default null);
  procedure aa_bb(p_self in out nocopy json, pair_name varchar2, pair_value varchar2, position pls_integer default null);
  procedure aa_bb(p_self in out nocopy json, pair_name varchar2, pair_value number, position pls_integer default null);
  procedure aa_bb(p_self in out nocopy json, pair_name varchar2, pair_value boolean, position pls_integer default null);
  procedure aa_check_duplicate(p_self in out nocopy json, v_set boolean);
  procedure aa_remove_duplicates(p_self in out nocopy json);

  procedure aa_bb(p_self in out nocopy json, pair_name varchar2, pair_value json, position pls_integer default null);
  procedure aa_bb(p_self in out nocopy json, pair_name varchar2, pair_value json_list, position pls_integer default null);

  function aa_count(p_self in json) return number;
  function aa_get(p_self in json, pair_name varchar2) return json_value;
  function aa_get(p_self in json, position pls_integer) return json_value;
  function aa_index_of(p_self in json, pair_name varchar2) return number;
  function aa_exist(p_self in json, pair_name varchar2) return boolean;

  function aa_to_char(p_self in json, spaces boolean default true, chars_per_line number default 0) return varchar2;
  procedure aa_to_clob(p_self in json, buf in out nocopy clob, spaces boolean default false, chars_per_line number default 0, erase_clob boolean default true);
  procedure aa_print(p_self in json, spaces boolean default true, chars_per_line number default 8192, jsonp varchar2 default null);
  procedure aa_htp(p_self in json, spaces boolean default false, chars_per_line number default 0, jsonp varchar2 default null);

  function aa_to_json_value(p_self in json) return json_value;
  function aa_path(p_self in json, json_path varchar2, base number default 1) return json_value;

  procedure aa_path_bb(p_self in out nocopy json, json_path varchar2, elem json_value, base number default 1);
  procedure aa_path_bb(p_self in out nocopy json, json_path varchar2, elem varchar2  , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json, json_path varchar2, elem number    , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json, json_path varchar2, elem boolean   , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json, json_path varchar2, elem json_list , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json, json_path varchar2, elem json      , base number default 1);

  procedure aa_path_remove(p_self in out nocopy json, json_path varchar2, base number default 1);

  function aa_get_values(p_self in json) return json_list;
  function aa_get_keys(p_self in json) return json_list;

  --json_list type methods
  procedure aa_append(p_self in out nocopy json_list, elem json_value, position pls_integer default null);
  procedure aa_append(p_self in out nocopy json_list, elem varchar2, position pls_integer default null);
  procedure aa_append(p_self in out nocopy json_list, elem number, position pls_integer default null);
  procedure aa_append(p_self in out nocopy json_list, elem boolean, position pls_integer default null);
  procedure aa_append(p_self in out nocopy json_list, elem json_list, position pls_integer default null);

  procedure aa_replace(p_self in out nocopy json_list, position pls_integer, elem json_value);
  procedure aa_replace(p_self in out nocopy json_list, position pls_integer, elem varchar2);
  procedure aa_replace(p_self in out nocopy json_list, position pls_integer, elem number);
  procedure aa_replace(p_self in out nocopy json_list, position pls_integer, elem boolean);
  procedure aa_replace(p_self in out nocopy json_list, position pls_integer, elem json_list);

  function aa_count(p_self in json_list) return number;
  procedure aa_remove(p_self in out nocopy json_list, position pls_integer);
  procedure aa_remove_first(p_self in out nocopy json_list);
  procedure aa_remove_last(p_self in out nocopy json_list);
  function aa_get(p_self in json_list, position pls_integer) return json_value;
  function aa_head(p_self in json_list) return json_value;
  function aa_last(p_self in json_list) return json_value;
  function aa_tail(p_self in json_list) return json_list;

  function aa_to_char(p_self in json_list, spaces boolean default true, chars_per_line number default 0) return varchar2;
  procedure aa_to_clob(p_self in json_list, buf in out nocopy clob, spaces boolean default false, chars_per_line number default 0, erase_clob boolean default true);
  procedure aa_print(p_self in json_list, spaces boolean default true, chars_per_line number default 8192, jsonp varchar2 default null);
  procedure aa_htp(p_self in json_list, spaces boolean default false, chars_per_line number default 0, jsonp varchar2 default null);

  function aa_path(p_self in json_list, json_path varchar2, base number default 1) return json_value;
  procedure aa_path_bb(p_self in out nocopy json_list, json_path varchar2, elem json_value, base number default 1);
  procedure aa_path_bb(p_self in out nocopy json_list, json_path varchar2, elem varchar2  , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json_list, json_path varchar2, elem number    , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json_list, json_path varchar2, elem boolean   , base number default 1);
  procedure aa_path_bb(p_self in out nocopy json_list, json_path varchar2, elem json_list , base number default 1);

  procedure aa_path_remove(p_self in out nocopy json_list, json_path varchar2, base number default 1);

  function aa_to_json_value(p_self in json_list) return json_value;

  --json_value


  function aa_get_type(p_self in json_value) return varchar2;
  function aa_get_string(p_self in json_value, max_byte_size number default null, max_char_size number default null) return varchar2;
  procedure aa_get_string(p_self in json_value, buf in out nocopy clob);
  function aa_get_number(p_self in json_value) return number;
  function aa_get_bool(p_self in json_value) return boolean;
  function aa_get_null(p_self in json_value) return varchar2;

  function aa_is_object(p_self in json_value) return boolean;
  function aa_is_array(p_self in json_value) return boolean;
  function aa_is_string(p_self in json_value) return boolean;
  function aa_is_number(p_self in json_value) return boolean;
  function aa_is_bool(p_self in json_value) return boolean;
  function aa_is_null(p_self in json_value) return boolean;

  function aa_to_char(p_self in json_value, spaces boolean default true, chars_per_line number default 0) return varchar2;
  procedure aa_to_clob(p_self in json_value, buf in out nocopy clob, spaces boolean default false, chars_per_line number default 0, erase_clob boolean default true);
  procedure aa_print(p_self in json_value, spaces boolean default true, chars_per_line number default 8192, jsonp varchar2 default null);
  procedure aa_htp(p_self in json_value, spaces boolean default false, chars_per_line number default 0, jsonp varchar2 default null);

  function aa_value_of(p_self in json_value, max_byte_size number default null, max_char_size number default null) return varchar2;


end AA;
/

CREATE OR REPLACE PACKAGE aa  AS

  null_as_empty_string   boolean not null := true;  
  include_dates          boolean not null := true;  
  include_clobs          boolean not null := true;
  include_blobs          boolean not null := false;

  function executeList(stmt varchar2, bindvar json default null, cur_num number default null) return json_list;

  function executeObject(stmt varchar2, bindvar json default null, cur_num number default null) return json;

  function executeList2(stmt varchar2, bindvar json default null, cur_num NUMBER default null) return json_list;


end aa;
/

CREATE OR REPLACE PACKAGE AA IS
  TYPE r_menu_list IS RECORD(
      MENU_FLAG          VARCHAR2(2),
      MENU_num             CTP_MENU.num%TYPE,
      MENU_MINGZI             CTP_MENU_NLS.MINGZI%TYPE,
      MENU_STATUS             CTP_MENU.STATUS%TYPE,
      MENU_PARENT             CTP_MENU.Parent_num%TYPE,
      MENU_PRIVILAGE             VARCHAR2(2),
      MENU_SERIALNO             CTP_MENU.Serialno%TYPE,
      menu_LONG         varchar2(2)
    );
  TYPE r_AA IS RECORD(
      AA_num             CTP_AA.num%TYPE,
      AA_MINGZI             CTP_AA_NLS.MINGZI%TYPE,
      AA_DESCRIPTION             CTP_AA_NLS.DESCRIPTION%TYPE,
      AA_LST_MODI_TIME             CTP_AA.LST_MODI_TIME%TYPE,
      AA_AA_LONG             CTP_AA.AA_LONG%TYPE,
      AA_BRANCH_num             CTP_AA.BRANCH_num%TYPE,
      AA_AA_LONG_ADMIN             CTP_AA.AA_LONG_ADMIN%TYPE,
      AA_BRANCH_num_ADMIN             CTP_AA.BRANCH_num_ADMIN%TYPE,
      AA_AA_CATEGORY             CTP_AA.AA_CATEGORY%TYPE,
      AA_LST_MODI_USER_num             CTP_AA.LST_MODI_USER_num%TYPE,
      AA_PRIVILEGE_ALL             CTP_AA.PRIVILEGE_ALL%TYPE,
      AA_PRIVILEGE_SELF             CTP_AA.PRIVILEGE_SELF%TYPE,
      AA_PRIVILEGE_OTHER             CTP_AA.PRIVILEGE_OTHER%TYPE,
      AA_PRIVILEGE             CTP_AA.PRIVILEGE%TYPE,
      AA_BRANCH_MINGZI             CTP_BRANCH_NLS.MINGZI%TYPE,
      AA_BRANCH_LONG             CTP_BRANCH.BRANCH_LONG%TYPE
 );

  TYPE r_AA_list IS RECORD(
      AA_num             CTP_AA.num%TYPE,
      AA_MINGZI             CTP_AA_NLS.MINGZI%TYPE,
      AA_DESCRIPTION             CTP_AA_NLS.DESCRIPTION%TYPE,
      AA_LST_MODI_TIME             CTP_AA.LST_MODI_TIME%TYPE,
      AA_AA_LONG             CTP_AA.AA_LONG%TYPE,
      AA_BRANCH_num             CTP_AA.BRANCH_num%TYPE,
      AA_AA_LONG_ADMIN             CTP_AA.AA_LONG_ADMIN%TYPE,
      AA_BRANCH_num_ADMIN             CTP_AA.BRANCH_num_ADMIN%TYPE,
      AA_AA_CATEGORY             CTP_AA.AA_CATEGORY%TYPE,
      AA_LST_MODI_USER_num             CTP_AA.LST_MODI_USER_num%TYPE,
      AA_PRIVILEGE_ALL             CTP_AA.PRIVILEGE_ALL%TYPE,
      AA_PRIVILEGE_SELF             CTP_AA.PRIVILEGE_SELF%TYPE,
      AA_PRIVILEGE_OTHER             CTP_AA.PRIVILEGE_OTHER%TYPE,
      AA_PRIVILEGE             CTP_AA.PRIVILEGE%TYPE,
      AA_BRANCH_MINGZI             CTP_BRANCH_NLS.MINGZI%TYPE,
      AA_BRANCH_LONG             CTP_BRANCH.BRANCH_LONG%TYPE,
      AA_USERNO                   VARCHAR2(5)

 );
   TYPE r_pos_AA_list IS RECORD(
       AA_LST_MODI_USER_num            CTP_AA.LST_MODI_USER_num%TYPE,
             AA_LST_MODI_TIME               CTP_AA.LST_MODI_TIME%TYPE,
             AA_PRIVILEGE_ALL               CTP_AA.PRIVILEGE_ALL%TYPE,
       AA_PRIVILEGE_SELF              CTP_AA.PRIVILEGE_SELF%TYPE,
       AA_PRIVILEGE_OTHER             CTP_AA.PRIVILEGE_OTHER%TYPE,
             AA_num                          CTP_AA.num%TYPE,
       AA_MINGZI                        CTP_AA_NLS.MINGZI%TYPE,
       AA_DESCRIPTION                 CTP_AA_NLS.DESCRIPTION%TYPE,
             AA_AA_LONG                  CTP_AA.AA_LONG%TYPE,
       AA_BRANCH_num                   CTP_AA.BRANCH_num%TYPE,
             AA_BRANCH_MINGZI                 CTP_BRANCH_NLS.MINGZI%TYPE
 );

  TYPE ref_AA IS REF CURSOR RETURN r_AA;
  TYPE ref_AA_list IS REF CURSOR RETURN r_AA_list;
  TYPE ret_pos_AA_list IS REF CURSOR RETURN r_pos_AA_list;
  TYPE c_menu_list IS REF CURSOR RETURN r_menu_list;

  FUNCTION aa_LG_AA_GETUSERNUM(
                        AAnum IN VARCHAR2
                        )
  RETURN INTEGER;

  PROCEDURE aa_LG_AA_GETBRANCHDEP( branchnum  IN VARCHAR2,   --机构编码
                                        out_flag OUT VARCHAR2,   --存储过程返回标志
                                        branchDep OUT number     --当前机构层级深度
                                       ) ;

    PROCEDURE aa_LG_AA_QUERYUSERNUM(
                        AAnum IN VARCHAR2,
                        out_flag  OUT VARCHAR2,
                        usr_num  OUT number);

  PROCEDURE aa_LG_AA_MODAAMENU(
                        privilege IN VARCHAR2,
                        menunum IN VARCHAR2,
                        AAnum IN VARCHAR2,
                        out_flag  OUT VARCHAR2);

  PROCEDURE aa_LG_AA_GETAAMENU(
                        AALONG IN VARCHAR2,
                        branchnum IN VARCHAR2,
                        AAnum IN VARCHAR2,
                        Language IN VARCHAR2,
                        out_flag  OUT VARCHAR2,
                        o_menu_list OUT c_menu_list);

  PROCEDURE aa_LG_AA_DELETEAA(AAnum  IN VARCHAR2,
                       out_flag OUT VARCHAR2
                       );



  PROCEDURE aa_LG_AA_UPDATEAA(AALONG IN VARCHAR2,
                         usernum IN VARCHAR2,
                         priAll IN VARCHAR2,
                         priSelf IN VARCHAR2,
                         priOther IN VARCHAR2,
                         AAnum IN VARCHAR2,
                         AAMINGZI IN VARCHAR2,
                         AADes IN VARCHAR2,
                         Language IN VARCHAR2,
                         out_flag OUT VARCHAR2
                      ) ;

  PROCEDURE aa_LG_AA_ADDAA(  AAnum IN VARCHAR2,
                         branchnum IN VARCHAR2,
                         AALONG IN VARCHAR2,
                         usernum IN VARCHAR2,
                         priSelf IN VARCHAR2,
                         priAll IN VARCHAR2,
                         priOther IN VARCHAR2,
                         AAMINGZI IN VARCHAR2,
                         AADes IN VARCHAR2,
                         Language IN VARCHAR2,
                         out_flag OUT VARCHAR2
                      ) ;

  PROCEDURE aa_LG_AA_GETAALIST(branchnum IN VARCHAR2,
                           branchLONG IN VARCHAR2,
                           languageCode IN VARCHAR2,
                           begNum IN VARCHAR2,
                           fetchNum IN VARCHAR2,
                           out_flag OUT VARCHAR2,
                           totalNum OUT VARCHAR2,
                           ret_AA_list OUT ret_pos_AA_list
                           );

  PROCEDURE aa_LG_AA_SEAAABYMINGZI(branchnum IN VARCHAR2,
                                           branchLONG IN VARCHAR2,
                                           languageCode IN VARCHAR2,
                                           begNum IN VARCHAR2,
                                           fetchNum IN VARCHAR2,
                                           keyword IN VARCHAR2,
                                           out_flag OUT VARCHAR2,
                                           o_totalNum OUT VARCHAR2,
                                           ret_AA_list OUT ret_pos_AA_list
                                          );

  PROCEDURE LOG(proc_MINGZI IN VARCHAR2,
               info      IN VARCHAR2);
END AA;
/

CREATE OR REPLACE PACKAGE AA
IS
a int;
end AA;
/

CREATE OR REPLACE PACKAGE BODY AA IS
  FUNCTION AA_BB_GETCCNUM(
                        BBId IN VARCHAR2
                        )
  RETURN INTEGER IS
  temp INTEGER;
  BEGIN
     SELECT count(*) INTO temp FROM DD_BB_CC_REL WHERE BB_ID=BBId;
     RETURN temp;
  EXCEPTION
    WHEN OTHERS THEN
      log('AA_BB_getCCnum()',SQLERRM(SQLCODE));
      RETURN -1;
  END;

  PROCEDURE AA_BB_GETBRANCHDEP( branchId  IN VARCHAR2,   --机构编码
                                        out_flag OUT VARCHAR2,   --存储过程返回标志
                                        branchDep OUT number     --当前机构层级深度
                                       ) IS
  maxLevel number;
  curLevel number;
  BEGIN
         out_flag := '0';
         SELECT MAX(branch_Level) INTO maxLevel FROM DD_BRANCH;
         SELECT branch_Level INTO curLevel FROM DD_BRANCH a where a.id=branchId;
         branchDep := maxLevel-curLevel;
  EXCEPTION
    WHEN OTHERS THEN
         log('AA_BB_deleteBB()',SQLERRM(SQLCODE));
         out_flag := '-1';
         rollback;
      RETURN;
  END;


  PROCEDURE AA_BB_QUERYCCNUM(BBId          IN VARCHAR2, --角色编码
                                       out_flag        OUT VARCHAR2,--存储过程返回标志
                                       usr_num         OUT number   --关联用户数
                                       ) IS
  BEGIN
  out_flag := '0';
  usr_num:=AA_BB_getCCnum(BBId);
    RETURN;
  EXCEPTION
    WHEN OTHERS THEN
      log('AA_BB_queryCCnum()',SQLERRM(SQLCODE));
      out_flag := '-1';
      rollback;
      RETURN;
  END;


  PROCEDURE AA_BB_MODBBMENU(privilege IN VARCHAR2, --菜单权限
                                         menuId IN VARCHAR2,    --菜单编码
                                         BBId IN VARCHAR2,    --角色编码
                                         out_flag  OUT VARCHAR2 --存储过程返回标志
                                         ) IS
  v_id VARCHAR2 (50);
  v_flag VARCHAR2 (8);

  BEGIN
  out_flag := '0';
    SELECT nvl(SUBSTR(menuId,1,INSTR(menuId,'M')-1),menuId) INTO v_id FROM dual;
    SELECT SUBSTR(menuId,1,1) INTO v_flag FROM dual;
    delete from DD_BB_MENU_REL where BB_ID=BBId and MENU_ID=v_id;
    delete from DD_BB_ITEM_REL where BB_ID=BBId and ITEM_ID=v_id;
    update DD_BB_CC_REL set MENUCHG_FLAG='1' where BB_ID=BBId;


    IF privilege!='-1' THEN
        IF v_flag ='M' THEN
           insert into DD_BB_MENU_REL(BB_ID,MENU_ID,PRIVILEGE) VALUES(BBId,v_id,privilege);
        END IF;
        IF v_flag='I'THEN
           insert into DD_BB_ITEM_REL(BB_ID,ITEM_ID,PRIVILEGE) VALUES(BBId,v_id,privilege);
        END IF;
    END IF;
    commit;
    RETURN;
  EXCEPTION
    WHEN OTHERS THEN
      log('AA_BB_modifyBBmenu()',SQLERRM(SQLCODE));
      out_flag := '-1';
      rollback;
      RETURN;
  END;

  PROCEDURE AA_BB_GETBBMENU(BBLevel IN VARCHAR2,       --角色级别
                                      branchId IN VARCHAR2,        --机构编码
                                      BBId IN VARCHAR2,          --角色编码
                                      Language IN VARCHAR2,        --语言编码
                                      out_flag  OUT VARCHAR2,      --存储过程返回标志
                                      o_menu_list OUT c_menu_list  --菜单列表
                                      ) IS
  BEGIN
  out_flag := '0';
  OPEN o_menu_list FOR
select 'M' flag,
       menu.menuId menuId,
       menu_nls.default_label menuName,
       menu.menuState menuState,
       menu.menuParentId menuParentId,
       menu.menuPrivilege menuPrivilege,
       menu.menCCialNo menCCialNo,
       level blevel
  from (select menuId, menuState, menuParentId, menuPrivilege, menCCialNo
          from (select distinct a.id menuId,
                                F.name menuName,
                                a.status menuState,
                                a.parent_id menuParentId,
                                '-1' menuPrivilege,
                                a.serialNo menCCialNo
                  from DD_MENU a, DD_MENU_NLS F
                 start with a.id in
                            (SELECT distinct b.menu_id
                               FROM DD_MENU_ITEM_REL b
                              WHERE b.item_id in
                                    (SELECT c.ID
                                       FROM DD_ITEM c
                                      WHERE c.status = '1'
                                        and func_DD_lg_canBeUsedByBB(branchId,
                                                                        BBLevel,
                                                                        c.item_level,
                                                                        c.item_branch_id) = '1'))
                connect by a.id = prior a.parent_id
                       and f.locale = Language
                       and a.id = f.id)
         where menuId not in (select d.menu_id
                                from DD_BB_menu_REL d
                               WHERE d.BB_ID = BBId)
        union
        select menuId, menuState, menuParentId, e.Privilege, menCCialNo
          from (select distinct a.id menuId,
                                F.name menuName,
                                a.status menuState,
                                a.parent_id menuParentId,
                                '-1' menuPrivilege,
                                a.serialNo menCCialNo
                  from DD_MENU a, DD_MENU_NLS F
                 start with a.id in
                            (SELECT distinct b.menu_id
                               FROM DD_MENU_ITEM_REL b
                              WHERE b.item_id in
                                    (SELECT c.ID
                                       FROM DD_ITEM c
                                      WHERE (c.status = '1' and
                                            func_DD_lg_canBeUsedByBB(branchId,
                                                                         BBLevel,
                                                                         c.item_level,
                                                                         c.item_branch_id) = '1')))
                connect by a.id = prior a.parent_id
                       and f.locale = Language
                       and a.id = f.id) d,
               DD_BB_MENU_rel e
         where e.BB_id = BBId
           and e.menu_id = d.menuId) menu,
       DD_menu_nls menu_nls
 where menu_nls.locale = Language
   and menu.menuId = menu_nls.id
CONNECT BY PRIOR MENUID = MENUPARENTID
 START WITH MENUPARENTID IS NULL
UNION
select 'I' flag,
       itemId,
       itemName,
       itemState,
       menuId,
       itemPrivilege,
       itemSerialNo,
       menu.blevel + 1 blevel
  from (select distinct a.item_id itemId,
                        d.DEFAULT_LABEL itemName,
                        b.STATUS itemState,
                        a.menu_id menuId,
                        '-1' itemPrivilege,
                        a.serialno itemSerialNo
          from DD_menu_item_rel a, DD_item b, DD_item_nls d
         where b.status = '1'
           and d.locale = Language
           and b.id = d.id
           and func_DD_lg_canBeUsedByBB(branchId,
                                           BBLevel,
                                           b.item_Level,
                                           b.item_Branch_Id) = '1'
           and b.id not in (select c.item_id
                              from DD_BB_ITEM_REL c
                             WHERE c.BB_ID = BBId)
           and b.id = a.item_id
        union
        select distinct a.item_id   itemId,
                        d.DEFAULT_LABEL      itemName,
                        b.STATUS    itemState,
                        a.menu_id   menuId,
                        c.privilege itemPrivilege,
                        a.serialno  itemSerialNo
          from DD_menu_item_rel a,
               DD_item          b,
               DD_item_nls      d,
               DD_BB_item_rel c
         where b.status = '1'
           and d.locale = Language
           and b.id = d.id
           and func_DD_lg_canBeUsedByBB(branchId,
                                           BBLevel,
                                           b.item_Level,
                                           b.item_Branch_Id) = '1'
           and c.BB_id = BBId
           and c.item_id = b.id
           and b.id = a.item_id) item,
       (select t.id, level blevel
          from DD_menu t
         start with t.parent_id is null
        connect by t.parent_id = prior t.id) menu
 where item.menuid = menu.id;

    RETURN;
  EXCEPTION
    WHEN OTHERS THEN
      if o_menu_list%isopen
      then
         close o_menu_list;
      end if;
      log('AA_BB_getBBmenu()',SQLERRM(SQLCODE));
      out_flag := '-1';
      RETURN;
  END;


  PROCEDURE AA_BB_DELETEBB(BBId  IN VARCHAR2,     --角色编码
                                     out_flag OUT VARCHAR2    --存储过程返回标志
                                     ) IS
  BEGIN
         out_flag := '0';
         DELETE FROM DD_BB_CC_REL WHERE BB_ID=BBId;
         DELETE FROM DD_BB_ITEM_REL WHERE BB_ID=BBId;
         DELETE FROM DD_BB_MENU_REL WHERE BB_ID=BBId;
         DELETE FROM DD_BB_NLS WHERE ID=BBId;
         DELETE FROM DD_BB WHERE ID=BBId;
         --commit;
  EXCEPTION
    WHEN OTHERS THEN
         log('AA_BB_deleteBB()',SQLERRM(SQLCODE));
         out_flag := '-1';
         rollback;
      RETURN;
  END;


  PROCEDURE AA_BB_UPDATEBB(BBLevel IN VARCHAR2,   --角色级别
                                     CCId IN VARCHAR2,      --用户编码
                                     priAll IN VARCHAR2,      --所有机构标识
                                     priSelf IN VARCHAR2,     --本机构标识
                                     priOther IN VARCHAR2,    --可维护下属机构级别
                                     BBId IN VARCHAR2,      --角色编码
                                     BBName IN VARCHAR2,    --角色名称
                                     BBDes IN VARCHAR2,     --角色描述
                                     Language IN VARCHAR2,    --语言编码
                                     out_flag OUT VARCHAR2    --存储过程返回标志
                                     ) IS
                                     tmp_num number;

  BEGIN
       out_flag := '0';

       UPDATE DD_BB SET  BB_LEVEL=BBLevel,LST_MODI_TIME=TO_CHAR(SYSDATE, 'YYYYMMDD'),LST_MODI_CC_ID=CCId,PRIVILEGE_ALL=priAll,PRIVILEGE_SELF=priSelf,PRIVILEGE_OTHER=priOther WHERE ID=BBId;

      -- UPDATE DD_BB_NLS SET NAME=BBName,DESCRIPTION=BBDes WHERE ID=BBId and LOCALE=Language;
     select count(*)
      into tmp_num
      from DD_BB_NLS r
     where r.id = BBId
       AND r.locale = Language;

    if (tmp_num = 0) then
      INSERT INTO DD_BB_NLS
        (ID,NAME,DESCRIPTION,LOCALE)
      VALUES
        (BBId,
        BBName,
        BBDes,
        Language);
    else
      UPDATE DD_BB_NLS SET NAME=BBName,DESCRIPTION=BBDes WHERE ID=BBId and LOCALE=Language;
    end if;
       --COMMIT;
  EXCEPTION
    WHEN OTHERS THEN
         log('AA_BB_updateBB()',SQLERRM(SQLCODE));
         out_flag := '-1';
         ROLLBACK;
      RETURN;
  END;


  PROCEDURE AA_BB_ADDBB(  BBId IN VARCHAR2,       --角色编码
                                    branchId IN VARCHAR2,     --机构编码
                                    BBLevel IN VARCHAR2,    --角色级别
                                    CCId IN VARCHAR2,       --用户编码
                                    priSelf IN VARCHAR2,      --本机构标识
                                    priAll IN VARCHAR2,       --所有机构标识
                                    priOther IN VARCHAR2,     --可维护下属机构级别
                                    BBName IN VARCHAR2,     --角色名称
                                    BBDes IN VARCHAR2,      --角色描述
                                    Language IN VARCHAR2,     --语言编码
                                    out_flag OUT VARCHAR2     --存储过程返回标志
                                    ) IS
BBCount NUMBER;
  BEGIN
   out_flag := '0';

   SELECT count(*)
      INTO BBCount
      FROM DD_BB a
     WHERE a.id = BBId;

    IF BBCount != 0 THEN  --该角色编号已经存在
      out_flag := '-1';
      RETURN;
    END IF;

   INSERT INTO DD_BB(ID,BRANCH_ID,BB_LEVEL,LST_MODI_TIME,LST_MODI_CC_ID,PRIVILEGE_SELF,PRIVILEGE_ALL,PRIVILEGE_OTHER)  VALUES(BBId,branchId,BBLevel,TO_CHAR(SYSDATE, 'YYYYMMDD'),CCId,priSelf,priAll,priOther);

   INSERT INTO DD_BB_NLS(ID,NAME,DESCRIPTION,LOCALE) VALUES(BBId,BBName,BBDes,Language);

       --COMMIT;
  EXCEPTION
    WHEN OTHERS THEN
         PCKG_DD_LG_PUBLIC.log('AA_BB_addBB()',SQLERRM(SQLCODE));
         out_flag := '-2';
         ROLLBACK;
      RETURN;
  END;

  PROCEDURE AA_BB_GETBBLIST(branchId IN VARCHAR2,              --机构编码
                                      branchLevel IN VARCHAR2,           --机构级别
                                      languageCode IN VARCHAR2,          --语言编码
                                      begNum IN VARCHAR2,                --开始取数
                                      fetchNum IN VARCHAR2,              --获取数
                                      out_flag OUT VARCHAR2,             --存储过程返回标志
                                      totalNum OUT VARCHAR2,             --最终获取数
                                      ret_BB_list OUT ret_pos_BB_list--角色列表
                                      )IS

  BEGIN
       out_flag := '0';

       SELECT COUNT(*)
       INTO totalNum
       FROM DD_BB A
       WHERE(a.BRANCH_ID=branchId
       or (substr(a.BB_level,branchLevel,1) ='1'
       and branchId in
       (select d.id
       from DD_branch d start with d.id=a.BRANCH_ID
       connect by prior d.id=d.parent_id)));

  OPEN ret_BB_list FOR
  SELECT lastModifyCC,
             lastModifyDate,
             priAll,
             priSelf,
             priOther,
             BBId,
             BBName,
             BBDes,
             BBLevel,
             BBBranchId,
             BBBranchName
             FROM
             (SELECT                 lastModifyCC,
                             lastModifyDate,
                             priAll,
                             priSelf,
                             priOther,
                             BBId,
                             BBName,
                             BBDes,
                             BBLevel,
                             BBBranchId,
                             BBBranchName,
                             ROWNUM row_id
                             FROM
                             (SELECT DISTINCT A.LST_MODI_CC_ID lastModifyCC,
                                              A.LST_MODI_TIME    lastModifyDate,
                                              A.PRIVILEGE_ALL    priAll,
                                              A.PRIVILEGE_SELF   priSelf,
                                              A.PRIVILEGE_OTHER  priOther,
                                              a.ID               BBId,
                                              a.NAME             BBName,
                                              a.DESCRIPTION      BBDes,
                                              a.BB_LEVEL       BBLevel,
                                              a.BRANCH_ID        BBBranchId,
                                              c.name             BBBranchName,
                                              ROWNUM             ROW_ID
                                              FROM (select e.LST_MODI_CC_ID,
                                                           e.LST_MODI_TIME,
                                                           e.PRIVILEGE_ALL,
                                                           e.PRIVILEGE_SELF,
                                                           e.PRIVILEGE_OTHER,
                                                           e.ID,
                                                           f.NAME,
                                                           f.DESCRIPTION,
                                                           e.BB_LEVEL,
                                                           e.BRANCH_ID
                                                           from DD_BB e
                                            left outer join DD_BB_nls f on e.id = f.id
                                      and f.locale = languageCode) A,DD_BRANCH_NLS c,                                                                                               (select b.id
                                              from DD_branch b
                                              start with b.id = branchId
                                              connect by b.id = prior b.parent_id) tempb
                                              WHERE (a.BRANCH_ID = branchId
                                              or
                                              substr(a.BB_level, branchLevel, 1) = '1')
                                              AND C.ID = A.BRANCH_ID
                                              AND tempb.id = a.branch_Id
                                              AND C.LOCALE = languageCode
                                              order by BBId asc)
                                              WHERE ROW_ID <to_number(begNum) + to_number(fetchNum))
                                              WHERE ROW_ID >=to_number(begNum);


  EXCEPTION
    WHEN OTHERS THEN
         if ret_BB_list%isopen
         then
            close ret_BB_list;
         end if;
         log('AA_BB_getBBlist()',SQLERRM(SQLCODE));
        -- out_flag := SQLERRM(SQLCODE);
        out_flag := '-1';
      RETURN;
  END;


  PROCEDURE AA_BB_SEABBBYNAME(branchId IN VARCHAR2,               --机构编码
                                           branchLevel IN VARCHAR2,            --机构级别
                                           languageCode IN VARCHAR2,           --语言编码
                                           begNum IN VARCHAR2,                 --开始取数
                                           fetchNum IN VARCHAR2,               --获取数
                                           keyword IN VARCHAR2,                --角色名称关键字
                                           out_flag OUT VARCHAR2,              --存储过程返回标志
                                           o_totalNum OUT VARCHAR2,            --最终获取数
                                           ret_BB_list OUT ret_pos_BB_list --角色列表
                                          ) IS
  v_BBName varchar2(20);
  BEGIN
  out_flag := '0';

  v_BBName := keyword;
  if(v_BBName is null) then
        v_BBName := '%';
  end if;

  SELECT COUNT(*) INTO o_totalNum
    FROM DD_BB A,DD_BB_NLS B
    where B.NAME like '%'||v_BBName||'%'
    AND A.ID = B.ID
    AND B.LOCALE=languageCode;

  OPEN ret_BB_list FOR
  SELECT lastModifyCC,
               lastModifyDate,
               priAll,
               priSelf,
               priOther,
               BBId,
               BBName,
               BBDes,
               BBLevel,
               BBBranchId,
               BBBranchName
  FROM  (SELECT lastModifyCC,
                      lastModifyDate,
                      priAll,
                      priSelf,
                      priOther,
                      BBId,
                      BBName,
                      BBDes,
                      BBLevel,
                      BBBranchId,
                      BBBranchName
                      FROM (SELECT lastModifyCC,
                                   lastModifyDate,
                                   priAll,
                                   priSelf,
                                   priOther,
                                   BBId,
                                   BBName,
                                   BBDes,
                                   BBLevel,
                                   BBBranchId,
                                   BBBranchName,
                                   ROWNUM row_id
                                   FROM (SELECT DISTINCT A.LST_MODI_CC_ID lastModifyCC,
                                              A.LST_MODI_TIME    lastModifyDate,
                                              A.PRIVILEGE_ALL    priAll,
                                              A.PRIVILEGE_SELF   priSelf,
                                              A.PRIVILEGE_OTHER  priOther,
                                              a.ID               BBId,
                                              a.NAME             BBName,
                                              a.DESCRIPTION      BBDes,
                                              a.BB_LEVEL       BBLevel,
                                              a.BRANCH_ID        BBBranchId,
                                              c.name             BBBranchName,
                                              ROWNUM             ROW_ID
                                              FROM (select e.LST_MODI_CC_ID,
                                                           e.LST_MODI_TIME,
                                                           e.PRIVILEGE_ALL,
                                                           e.PRIVILEGE_SELF,
                                                           e.PRIVILEGE_OTHER,
                                                           e.ID,
                                                           f.NAME,
                                                           f.DESCRIPTION,
                                                           e.BB_LEVEL,
                                                           e.BRANCH_ID
                                                           from DD_BB e
                                            left outer join DD_BB_nls f on e.id = f.id
                                      and f.locale = languageCode) A,DD_BRANCH_NLS c,                                                                                               (select b.id
                                              from DD_branch b
                                              start with b.id = branchId
                                              connect by b.id = prior b.parent_id) tempb
                                              WHERE (a.BRANCH_ID = branchId
                                              or
                                              substr(a.BB_level, branchLevel, 1) = '1')
                                              AND C.ID = A.BRANCH_ID
                                              AND tempb.id = a.branch_Id
                                              AND C.LOCALE = languageCode
                                              order by BBId asc)
                                              WHERE ROW_ID <to_number(begNum) + to_number(fetchNum))
                                              WHERE ROW_ID >=to_number(begNum)) BBSet
                                        WHERE BBSet.BBName LIKE '%'||v_BBName||'%';

  EXCEPTION
    WHEN OTHERS THEN
         if ret_BB_list%isopen
         then
            close ret_BB_list;
         end if;
         log('DD_proc_BB_getBBlist()',SQLERRM(SQLCODE));
        out_flag := '-1';
      RETURN;
  END;


  PROCEDURE log(proc_name IN VARCHAR2,
            info      IN VARCHAR2) IS
    PRAGMA AUTONOMOUS_TRANSACTION;
    time_str VARCHAR2(100);
  BEGIN
    --IF check_log_on THEN
    SELECT to_char(SYSDATE,'mm - dd - yyyy hh24 :mi :ss')
      INTO time_str
      FROM dual;
    INSERT INTO DD_proc_log
    VALUES
      (proc_name, time_str, info);
    COMMIT;
    --END IF;
    RETURN;
  END;

END AA ;
/

create or replace procedure test_pro1
as
type tpc1 is ref cursor;
--v_cur tpc1;
begin
open v_cur for select c1,c2 from tab1;
end;
/
create table if not exists tb(eno int);
create or replace procedure test_pro2
as
type t1 is table of tb.eno%type;
v1 t1;
begin
forall i in 1 .. v1.count save exceptions
    insert into tb values v1(i);
end;
/
drop table if exists tb;
drop procedure if exists test_pro1;

create table t1(c1 int, c2 int);
create or replace package pack2 is
procedure pro1();
procedure pro2();
end pack2;
/
create or replace package body pack2 is
procedure pro1
as
begin
update table t1 set c1=1 and c2=1;
end;
procedure pro2
as
begin
update table t1 set c1=1 and c2=1;
end;
end pack2;
/
drop table t1;
drop package pack2;

create or replace function f(v int[]) return int
as
n int;
begin
n := v();
return n;
end;
/

create or replace PACKAGE z_pk2
AS
 PROCEDURE pro(p1 int);
END z_pk2;
/

create or replace PACKAGE BODY z_pk2
AS
 p1 int := 1;
 p2 int := 1 ;
 PROCEDURE pro()
 AS
    p2 int;
 BEGIN
    select 1 into p2;
 END;
END z_pk2;
/


create or replace PACKAGE package_020
AS
  PROCEDURE pro1(p1 int,p2 int ,p3 VARCHAR2(5) ,p4 out int);
  PROCEDURE pro2(p1 int,p2 out int,p3 inout varchar(20));
END package_020;
/
create or replace PACKAGE body package_020
AS
   PROCEDURE pro1(p1 int,p2 int ,p3 ,p4 out int )
  as
  BEGIN
         p4 := 0;
      if p3 = '+' then
          p4 := p1 + p2;
       end if;
       
       if p3 = '-' then
          p4 := p1 - p2;
       end if;
       
       if p3 = '*' then
          p4 := p1 * p2;
       end if;
       
       if p3 = '/' then
          p4 := p1 / p2;
       end if;
  END;
  PROCEDURE pro2(p1 int,p2 out int,p3 inout varchar(20))
  AS
  BEGIN
      p2 := p1;
      p3 := p1 ||'___a';
  END;
END package_020;
/

drop procedure pro1;
select line,src from dbe_pldeveloper.gs_errors order by line,src;

create table pro_tblof_tbl_013(c1 number(3,2),c2 varchar2(20),c3 clob,c4 blob);
create type pro_tblof_013 is table of pro_tblof_tbl_013.c1%type;
create or replace procedure pro_tblof_pro_013_1()
as
tblof001 pro_tblof_013;
i int :=1;
cursor cor1 is select c1,c2,c3,c4 from pro_tblof_tbl_013 order by 1,2,3,4;
begin
open cor1;
loop
fetch cor1 into tblof001(i).c1,tblof001(i).c2,tblof001(i).c3,tblof001(i).c4;
EXIT WHEN cor1%NOTFOUND;
DBE_OUTPUT.PRINT_LINE('tblof001('||i||') is '||tblof001(i));
i=i+1;
end loop;
close cor1;
raise info 'tblof001 is %',tblof001;
raise info 'i is%',i;
end;
/
drop procedure pro_tblof_pro_013_1();
drop type pro_tblof_013;
drop table pro_tblof_tbl_013;

set behavior_compat_options = 'allow_procedure_compile_check';
create or replace procedure pro19
as
a int;
begin
 select c1 from t1 into ;             
 select c1 from t1  limit  1  a;       
 select c1 from t1 where c2=1 intoo a; 
 select c1 from t1 into b;                          
end;
/

create or replace procedure pro20
as
a int;
begin
 select  into a from t1.c1;
 select 1  into ab;
end;
/

create or replace procedure pro21
as
a int;
begin
 select 1  into ab;
 select 1  intoo a;
 dbe_output.print_line('a  is:'||a);
end;
/

create or replace procedure pro22
as
a int;
b char(1);
begin
 b=:1;
 select 1  into a;
 dbe_output.print_line('a  is:'||a);
end;
/
set behavior_compat_options = '';

drop package if exists package_020;
drop package if exists z_pk2;
drop package if exists aa;
truncate DBE_PLDEVELOPER.gs_source;
truncate DBE_PLDEVELOPER.gs_errors;

DO
$$
BEGIN
  CREATE TABLE t(c float(0));
END;
$$;

DO
$$
BEGIN
  CREATE TABLE t(c float(55));
END;
$$;

create or replace package pck1 as
type t1 is ref cursor;
type t2 is record(c1 t1,c2 int);
end pck1;
/

create or replace function testInsertRecordError1() RETURNS int as $$
declare
i int;
begin
i = 1;
insert into insert_table values i;
return 1;
end;
$$ language plpgsql;

set plsql_show_all_error to off;

--------------------------------------------------------------------
-------------------test rownum pseudocolumn ------------------------
--------------------------------------------------------------------

------------------------------------
--test the basic function of rownum
------------------------------------

--create test table
create table rownum_table (name varchar(20), age int, address varchar(20));

--insert data to test table
insert into rownum_table values ('leon', 23, 'xian');
insert into rownum_table values ('james', 24, 'bejing');
insert into rownum_table values ('jack', 35, 'xian');
insert into rownum_table values ('mary', 42, 'chengdu');
insert into rownum_table values ('perl', 35, 'shengzhen');
insert into rownum_table values ('rose', 64, 'xian');
insert into rownum_table values ('under', 57, 'xianyang');
insert into rownum_table values ('taker', 81, 'shanghai');
insert into rownum_table values ('frank', 19, 'luoyang');
insert into rownum_table values ('angel', 100, 'xian');

--the query to test rownum
select * from rownum_table where rownum < 5;
select rownum, * from rownum_table where rownum < 1;
select rownum, * from rownum_table where rownum <= 1;
select rownum, * from rownum_table where rownum <= 10;
select rownum, * from rownum_table where address = 'xian';
select rownum, * from rownum_table where address = 'xian' and rownum < 4;
select rownum, name, address, age from rownum_table where address = 'xian' or rownum < 8;

------------------
--avoid optimize
------------------

--test order by
--create test table
create table test_table
(
    id       integer       primary key ,
    name     varchar2(20)  ,
    age      integer       check(age > 0),
    address  varchar2(20)   not null,
    tele     varchar2(20)   default '101'
);
--insert data
insert into test_table values(1,'charlie', 40, 'shanghai');
insert into test_table values(2,'lincon', 10, 'xianyang');
insert into test_table values(3,'charlie', 40, 'chengdu');
insert into test_table values(4,'lincon', 10, 'xian', '');
insert into test_table values(5,'charlie', 40, 'chengdu');
insert into test_table values(6,'lincon', 10, 'xian', '12345657');
--test order by
select * from (select * from test_table order by id) as result where rownum < 4;
select * from (select * from test_table order by id desc) as result where rownum < 2;
select * from (select * from test_table order by id asc) as result where rownum <= 5;

--test union and intersect
--create test table
create table distributors (id int, name varchar(20));
create table actors (id int, name varchar(20));
--insert data
insert into distributors values (1, 'westward');
insert into distributors values (1, 'walt disney');
insert into distributors values (1, 'warner bros');
insert into distributors values (1, 'warren beatty');

insert into actors values (1, 'woody allen');
insert into actors values (1, 'warren beatty');
insert into actors values (1, 'walter matthau');
insert into actors values (1, 'westward');
--test union
select rownum, name from (select name from distributors union all select name from actors order by 1) as result where rownum <= 1;
select rownum, name from (select name from distributors union all select name from actors order by 1) as result where rownum < 3;
select rownum, name from (select name from distributors union all select name from actors order by 1) as result where rownum < 6;
select rownum, name from (select name from distributors where rownum < 3 union all select name from actors where rownum < 3 order by 1) as result;
--test intersect
select rownum, name from (select name from distributors intersect all select name from actors order by 1) as result where rownum <= 1;
select rownum, name from (select name from distributors intersect all select name from actors order by 1) as result where rownum < 3;
select rownum, name from (select name from distributors intersect all select name from actors order by 1) as result where rownum < 6;
select rownum, name from (select name from distributors where rownum <= 4 intersect all select name from actors where rownum <= 4 order by 1) as result;
--test group by
select rownum from distributors group by rownum;
select rownum rn from distributors group by rn;
select rownum + 1 from dual group by rownum;
select rownum + 1 rn from dual group by rn;
--test alias name after where
select rownum rn, name from distributors where rn<3;
select rownum rowno2, * from (select rownum rowno1, * from distributors order by id desc) where rowno2 < 2;
--test default rownum when creating table
create table student(id int default rownum, stuname varchar(5));
create table student(id int default rownum+1, stuname varchar(5));
--test insert when values include rownum
insert into distributors values (rownum, 'qwer');
insert into distributors(id, name) values (2, 'abcd'), (rownum+1, 'qwer');
--test VALUES clause that's being used as a standalone SELECT
select * from (values(rownum, 1)) x(a, b);
select * from (values(rownum+1, 1)) x(a, b);

--test except and minus
--create test table
create table except_table (a int, b int);
create table except_table1 (a int, b int);
--insert data
insert into except_table values (3, 4);
insert into except_table values (5, 4);
insert into except_table values (3, 4);
insert into except_table values (4, 4);
insert into except_table values (6, 4);
insert into except_table values (3, 4);
insert into except_table values (3, 4); 
insert into except_table1 values (3, 4);
--test except and minus
select rownum, * from (select * from except_table except select * from except_table1 order by 1) as result where rownum <= 2;
select rownum, * from (select * from except_table minus select * from except_table1 order by 1) as result where rownum <= 3;
select rownum, * from (select * from except_table where rownum <= 3 except select * from except_table1 where rownum <=2 order by 1) as result;
select rownum, * from (select * from except_table where rownum <= 3 minus select * from except_table1 where rownum <=2 order by 1) as result;

--drop the test table
drop table rownum_table;
drop table test_table;
drop table distributors;
drop table actors;
drop table except_table;
drop table except_table1;

create table tbl_a(v1 integer);
insert into tbl_a values(1001);
insert into tbl_a values(1002);
insert into tbl_a values(1003);
insert into tbl_a values(1004);
insert into tbl_a values(1005);
insert into tbl_a values(1002);
create table tbl_b(v1 integer, v2 integer);
insert into tbl_b values (1001,214);
insert into tbl_b values (1003,216);
insert into tbl_b values (1002,213);
insert into tbl_b values (1002,212);
insert into tbl_b values (1002,211);
insert into tbl_b values (1003,217);
insert into tbl_b values (1005,218);
update tbl_a a set a.v1 = (select v2 from tbl_b b where a.v1 = b.v1 and rownum <= 1);
select * from tbl_a order by 1;

update tbl_b set v2 = rownum where v1 = 1002;
select * from tbl_b where v1 = 1002 and rownum < 4 order by 1, 2;

delete tbl_b where rownum > 3 and v1 = 1002;
delete tbl_b where rownum < 100 and v1 = 1002;

select * from tbl_b order by 1, 2;

drop table tbl_a;
drop table tbl_b;

--adapt pseudocolumn "rowid" of oracle, using "ctid" of postgresql
create table test_tbl(myint integer);
insert into test_tbl values(1);
insert into test_tbl values(2);
insert into test_tbl values(3);
select rowid,* from test_tbl;
select max(rowid) from test_tbl;
delete from test_tbl a where a.rowid != (select max(b.rowid) from test_tbl b);
select rowid,* from test_tbl;
drop table test_tbl;

create table aaaa (
    smgwname character varying(255),
    seid character varying(33),
    igmgwidx integer,
    imsflag smallint
);
insert into aaaa values ('mrp', 'mrp', 0, 1);

create table bbbb (
    imgwindex integer,
    imsflag smallint
);
insert into bbbb values (0, 1);
insert into bbbb values (0, 1);
select (select a1.smgwname from aaaa a1 where a1.seid = ( select a2.seid from aaaa a2 where a2.igmgwidx = b.imgwindex and a2.imsflag = b.imsflag and rownum <=1)) from bbbb b;
drop table aaaa;
drop table bbbb;

--test query plan after optimizing
create table student(id int, stuname varchar(10) );
insert into student values(1, 'stu1');
insert into student values(2, 'stu2');
insert into student values(3, 'stu3');
insert into student values(4, 'stu4');
insert into student values(5, 'stu5');
insert into student values(6, 'stu6');
insert into student values(7, 'stu7');
insert into student values(8, 'stu8');
insert into student values(9, 'stu9');
insert into student values(10, 'stu10');

create table test(id int, testchar varchar(10));
insert into test values(1, 'test1');
insert into test values(2, 'test2');
insert into test values(3, 'test3');
insert into test values(4, 'test4');
insert into test values(5, 'test5');
insert into test values(6, 'test6');
insert into test values(7, 'test7');
insert into test values(8, 'test8');
insert into test values(9, 'test9');
insert into test values(10, 'test10');

-- operator '<' (with 'and')
-- n > 1 
explain select * from student where rownum < 5;
explain select * from student where rownum < 5 and id > 5;
explain select * from student where rownum < 5 and id > 5 and id < 9;
explain select * from student where rownum < 5 and rownum < 6;
explain select * from student where rownum < 5 and rownum < 6 and rownum < 9;
explain select * from student where rownum < 5 and rownum < 6 and rownum < 9 and rownum < 12;

-- n <= 1 
explain select * from student where rownum < 1;
explain select * from student where rownum < -5;
explain select * from student where rownum < -5 and id > 5;
explain select * from student where rownum < -5 and id > 5 and id < 9;
explain select * from student where rownum < -5 and rownum < 6;
explain select * from student where rownum < -5 and rownum < 6 and rownum < 9;
explain select * from student where rownum < -5 and rownum < 6 and rownum < 9 and rownum < 12;

-- operator '<=' (with 'and')
-- n >= 1
explain select * from student where rownum <= 1;
explain select * from student where rownum <= 5;
explain select * from student where rownum <= 5 and id > 5;
explain select * from student where rownum <= 5 and id > 5 and id < 9;
explain select * from student where rownum <= 5 and rownum < 6;
explain select * from student where rownum <= 5 and rownum < 6 and rownum < 9;
explain select * from student where rownum <= 5 and rownum < 6 and rownum < 9 and rownum < 12;

-- n < 1
explain select * from student where rownum <= -5;
explain select * from student where rownum <= -5 and id > 5;
explain select * from student where rownum <= -5 and id > 5 and id < 9;
explain select * from student where rownum <= -5 and rownum < 6;
explain select * from student where rownum <= -5 and rownum < 6 and rownum < 9;
explain select * from student where rownum <= -5 and rownum < 6 and rownum < 9 and rownum < 12;


-- operator '=' (with 'and')
-- n = 1
explain select * from student where rownum = 1;
explain select * from student where rownum = 1 and id > 5;
explain select * from student where rownum = 1 and rownum = 2 and id > 5;

-- n != 1
explain select * from student where rownum = 2;
explain select * from student where rownum = 2 and id > 5;

-- operator '!=' (with 'and')
-- n = 1
explain select * from student where rownum != 1;
explain select * from student where rownum != 1 and id > 5;
explain select * from student where rownum != 1 and rownum != 2 and id > 5;

-- n > 1
explain select * from student where rownum != 5;
explain select * from student where rownum != 5 and id > 5;
explain select * from student where rownum != 5 and rownum != 8 and id > 5;

-- n < 1
explain select * from student where rownum != -5;
explain select * from student where rownum != -5 and id > 5;
explain select * from student where rownum != -5 and rownum != -8 and id > 5;

-- operator '>' (with 'and')
-- n >= 1
explain select * from student where rownum > 1;
explain select * from student where rownum > 5;
explain select * from student where rownum > 5 and id > 5;
explain select * from student where rownum > 5 and id > 5 and id < 9;
explain select * from student where rownum > 5 and rownum > 6;
explain select * from student where rownum > 5 and rownum > 6 and rownum > 9;
explain select * from student where rownum > 5 and rownum < 6 and rownum < 9 and rownum < 12;

--n < 1
explain select * from student where rownum > -5;
explain select * from student where rownum > -5 and id > 5;
explain select * from student where rownum > -5 and id > 5 and id < 9;
explain select * from student where rownum > -5 and rownum > 6;
explain select * from student where rownum > -5 and rownum > 6 and rownum < 9;
explain select * from student where rownum > -5 and rownum > 6 and rownum < 9 and rownum < 12;

-- operator '>=' (with 'and')
-- n > 1
explain select * from student where rownum >= 5;
explain select * from student where rownum >= 5 and id > 5;
explain select * from student where rownum >= 5 and id > 5 and id < 9;
explain select * from student where rownum >= 5 and rownum > 6;
explain select * from student where rownum >= 5 and rownum > 6 and rownum > 9;
explain select * from student where rownum >= 5 and rownum < 6 and rownum < 9 and rownum < 12;

-- n <= 1
explain select * from student where rownum >= 1;
explain select * from student where rownum >= -5;
explain select * from student where rownum >= -5 and id > 5;
explain select * from student where rownum >= -5 and id > 5 and id < 9;
explain select * from student where rownum >= -5 and rownum > 6;
explain select * from student where rownum >= -5 and rownum > 6 and rownum < 9;
explain select * from student where rownum >= -5 and rownum > 6 and rownum < 9 and rownum < 12;

-- operator '<' with 'or'
-- n > 1
-- can not be optimized
explain select * from student where rownum < 5 or id > 5;

-- n <= 1
explain select * from student where rownum < -5;
explain select * from student where rownum < -5 or id > 5;
explain select * from student where rownum < -5 or id > 5 or id < 9;

-- operator '<=' with 'or'
-- n >= 1
-- can not be optimized
explain select * from student where rownum <= 5 or id > 5;

-- n < 1
explain select * from student where rownum <= -5;
explain select * from student where rownum <= -5 or id > 5;
explain select * from student where rownum <= -5 or id > 5 or id < 9;

-- operator '=' with 'or'
-- n > 0
-- can not be optimized
explain select * from student where rownum = 5 or id > 5;

-- n <= 0
explain select * from student where rownum = 0 or id > 5;
explain select * from student where rownum = -1 or id > 5;

-- operator '!=' with 'or'
-- n >= 1
-- can not be optimized
explain select * from student where rownum != 6 or id > 5;

-- n<1
explain select * from student where rownum != 0 or id > 5;

-- operator '>' with 'or'
-- n >= 1  
-- can not be optimized
explain select * from student where rownum > 5 or id > 5;

-- n < 1
explain select * from student where rownum > -5;
explain select * from student where rownum > -5 or id > 5;
explain select * from student where rownum > -5 or id > 5 or id < 9;
-- operator '>=' with 'or'
-- n > 1  
-- can not be optimized
explain select * from student where rownum >= 5 or id > 5;

-- n <= 1
explain select * from student where rownum >= -5;
explain select * from student where rownum >= -5 or id > 5;
explain select * from student where rownum >= -5 or id > 5 or id < 9;

-- limit
explain select * from student where rownum < 5 limit 3;
explain select * from student where rownum < 3 limit 5;
explain select * from student where rownum <= 5 limit 3;
explain select * from student where rownum <= 3 limit 5;

-- subqueries
explain select * from (select * from student where rownum < 5);
explain select * from (select * from student where rownum < 5) where rownum < 9;
explain select * from (select * from student where rownum < 5 and id < 7);
explain select * from (select * from student where rownum < 3 and id < 10) where rownum < 5;
explain select * from (select * from student where rownum < 3 and id < 10) where rownum < 2 and stuname = 'stu1';

--sublink
explain select * from student where id in (select id from test where rownum < 4);
explain select * from student where id in (select id from test where rownum < 4) and rownum < 6;
explain select * from student where id in (select id from test where rownum < 4) and stuname in (select stuname from student where rownum < 6);
explain select * from student where id in (select id from test where rownum < 4 and id < 7);
explain select * from student where id in (select id from test where rownum < 4) and rownum < 6 and id > 3;

-- insert 
explain insert into test select * from student where rownum < 5;
explain insert into test select * from student where rownum < 5 and id > 3;

-- between
explain select * from student where rownum between 1 and 5;
explain select * from student where rownum between 2 and 8;
explain select * from student where rownum between -5 and 8;
explain select * from student where rownum between -5 and -2;

--update
explain update student set id = 5 where rownum < 3;
explain update student set id = 5 where rownum < 3 and rownum < 5;
explain update student set id = 5 where rownum > 3;

--delete
explain delete from student where rownum < 3;
explain delete from student where rownum < 3 and rownum < 5;
explain delete from student where rownum > 3;

-- have not been optimized yet
explain select * from student where rownum < 6.5;
explain select * from student where rownum <= 6.5;
explain select * from student where rownum = 6.5;
explain select * from student where rownum != 6.5;
explain select * from student where rownum > 6.5;
explain select * from student where rownum >= 6.5;

explain delete from student where 3 > rownum;
explain delete from student where 3 < rownum;

explain delete from student where rownum < 5 or rownum < 6;
explain delete from student where rownum > 5 or rownum > 6;

drop table student;
drop table test;
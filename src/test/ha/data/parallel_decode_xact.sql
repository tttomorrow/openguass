drop table if exists t1_decode;
drop table if exists t2_decode;
drop table if exists t3_decode;
drop table if exists t4_decode;
drop table if exists t5_decode;

create table t1_decode(a int, b text);
alter table t1_decode replica identity full;
insert into t1_decode values(1,'abc');
update t1_decode set b = 'cde' where a = 1;
delete from t1_decode;
insert into t1_decode select 1, string_agg(g.i::text,'') from generate_series(1,2000) g(i);
insert into t1_decode values(generate_series(1,5000),'ab');
begin;
insert into t1_decode values(10,'abc');
savepoint a;
insert into t1_decode values(20,'abc');
rollback to savepoint a;
insert into t1_decode values(30,'abc');
commit;
begin;
insert into t1_decode values(11,'a');
savepoint a;
insert into t1_decode values(22,'a');
rollback to savepoint a;
insert into t1_decode values(33,'a');
rollback;
begin;
insert into t1_decode values(generate_series(1,5000),'a');
savepoint a;
insert into t1_decode values(generate_series(1,5000),'b');
rollback to savepoint a;
insert into t1_decode values(generate_series(1,5000),'c');
commit;

create table t2_decode(a int, b text) with(storage_type = ustore);
insert into t2_decode values(1,'abc');
update t2_decode set b = 'cde' where a = 1;
delete from t2_decode;
insert into t2_decode select 1, string_agg(g.i::text,'') from generate_series(1,2000) g(i);
insert into t2_decode values(generate_series(1,5000),'ab');
begin;
insert into t2_decode values(10,'abc');
savepoint a;
insert into t2_decode values(20,'abc');
rollback to savepoint a;
insert into t2_decode values(30,'abc');
commit;
begin;
insert into t2_decode values(11,'a');
savepoint a;
insert into t2_decode values(22,'a');
rollback to savepoint a;
insert into t2_decode values(33,'a');
rollback;
begin;
insert into t2_decode values(generate_series(1,5000),'a');
savepoint a;
insert into t2_decode values(generate_series(1,5000),'b');
rollback to savepoint a;
insert into t2_decode values(generate_series(1,5000),'c');
commit;


create table t3_decode(a int, b text);
alter table t3_decode replica identity full;
insert into t3_decode values(1,'abc');
update t3_decode set b = 'cde' where a = 1;
delete from t3_decode;
insert into t3_decode select 1, string_agg(g.i::text,'') from generate_series(1,2000) g(i);
insert into t3_decode values(generate_series(1,5000),'ab');
begin;
insert into t3_decode values(10,'abc');
savepoint a;
insert into t3_decode values(20,'abc');
rollback to savepoint a;
insert into t3_decode values(30,'abc');
commit;
begin;
insert into t3_decode values(11,'a');
savepoint a;
insert into t3_decode values(22,'a');
rollback to savepoint a;
insert into t3_decode values(33,'a');
rollback;
begin;
insert into t3_decode values(generate_series(1,5000),'a');
savepoint a;
insert into t3_decode values(generate_series(1,5000),'b');
rollback to savepoint a;
insert into t3_decode values(generate_series(1,5000),'c');
commit;


create table t4_decode(a int, b text) with(storage_type = ustore);
insert into t4_decode values(1,'abc');
update t4_decode set b = 'cde' where a = 1;
delete from t4_decode;
insert into t4_decode select 1, string_agg(g.i::text,'') from generate_series(1,2000) g(i);
insert into t4_decode values(generate_series(1,5000),'ab');
begin;
insert into t4_decode values(10,'abc');
savepoint a;
insert into t4_decode values(20,'abc');
rollback to savepoint a;
insert into t4_decode values(30,'abc');
commit;
begin;
insert into t4_decode values(11,'a');
savepoint a;
insert into t4_decode values(22,'a');
rollback to savepoint a;
insert into t4_decode values(33,'a');
rollback;
begin;
insert into t4_decode values(generate_series(1,5000),'a');
savepoint a;
insert into t4_decode values(generate_series(1,5000),'b');
rollback to savepoint a;
insert into t4_decode values(generate_series(1,5000),'c');
commit;

create table t5_decode(a int, b bool, c bit, d text);
insert into t5_decode values(1, true, B'1', '你好');
insert into t5_decode values(2);
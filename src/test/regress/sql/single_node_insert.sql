--
-- insert with DEFAULT in the target_list
--
create table inserttest (col1 int4, col2 int4 NOT NULL, col3 text default 'testing');
insert into inserttest (col1, col2, col3) values (DEFAULT, DEFAULT, DEFAULT);
insert into inserttest (col2, col3) values (3, DEFAULT);
insert into inserttest (col1, col2, col3) values (DEFAULT, 5, DEFAULT);
insert into inserttest values (DEFAULT, 5, 'test');
insert into inserttest values (DEFAULT, 7);

select * from inserttest;

--
-- insert with similar expression / target_list values (all fail)
--
insert into inserttest (col1, col2, col3) values (DEFAULT, DEFAULT);
insert into inserttest (col1, col2, col3) values (1, 2);
insert into inserttest (col1) values (1, 2);
insert into inserttest (col1) values (DEFAULT, DEFAULT);

select * from inserttest;

--
-- VALUES test
--
insert into inserttest values(10, 20, '40'), (-1, 2, DEFAULT),
    ((select 2), (select i from (values(3)) as foo (i)), 'values are fun!');

select * from inserttest;

--
-- TOASTed value test
--
insert into inserttest values(30, 50, repeat('x', 10000));

select col1, col2, char_length(col3) from inserttest;

drop table inserttest;

create table s1_TESTTABLE(id int, num int);
create sequence ss1_TESTTABLE;
select setval('ss1_TESTTABLE', 10);
select * from ss1_TESTTABLE;
alter table s1_TESTTABLE alter column id set default nextval('ss1_TESTTABLE');
insert into s1_TESTTABLE (num) values (11);
insert into s1_TESTTABLE (num) values (12);
select * from s1_TESTTABLE order by id;
select * from ss1_TESTTABLE;
drop table s1_TESTTABLE;
drop sequence ss1_TESTTABLE;
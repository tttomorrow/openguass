CREATE SCHEMA synonym_test_schema;
SET current_schema to 'synonym_test_schema';
-- create relation test
CREATE TABLE t1(c1 int);
INSERT INTO t1 VALUES(1);
CREATE TABLE t2(c1 varchar(20));
INSERT INTO t2 VALUES('test');
CREATE VIEW test as select * from t1;
CREATE SYNONYM test for t2;  --expected: CREATE SYNONYM ERROR
DROP VIEW test;
CREATE TABLE test as select * from t1;
CREATE SYNONYM test for t2;  --expected: CREATE SYNONYM ERROR
DROP TABLE test;
CREATE SYNONYM test for t2;
CREATE VIEW test as select * from t1;  --expected: CREATE relation ERROR
CREATE TABLE test as select * from t1;
DROP SYNONYM test;

CREATE OR REPLACE FUNCTION test RETURNS INT AS
$$ 
BEGIN
RETURN 0;
END;
$$
LANGUAGE 'plpgsql';
CREATE SYNONYM test for t2;  --expected: CREATE SYNONYM ERROR
DROP FUNCTION test;
CREATE OR REPLACE PROCEDURE test()
AS
BEGIN
    SELECT 1;
END;
/
CREATE SYNONYM test for t2;  --expected: CREATE SYNONYM ERROR
DROP PROCEDURE test;
CREATE SYNONYM test for t2;
CREATE OR REPLACE FUNCTION test RETURNS INT AS  --expected: CREATE FUNCTION ERROR
$$ 
BEGIN
RETURN 0;
END;
$$
LANGUAGE 'plpgsql';
CREATE OR REPLACE PROCEDURE test()  --expected: CREATE PROCEDURE ERROR
AS
BEGIN
    SELECT 1;
END;
/
-- rename object test
CREATE TABLE rename_test(a int);
ALTER TABLE rename_test RENAME TO test;  --expected: RENAME TABLE ERROR
DROP TABLE rename_test;
CREATE VIEW rename_test AS SELECT 1;
ALTER VIEW rename_test RENAME TO test;  --expected: RENAME VIEW ERROR
DROP VIEW rename_test;
CREATE OR REPLACE FUNCTION rename_test RETURNS INT AS
$$ 
BEGIN
RETURN 0;
END;
$$
LANGUAGE 'plpgsql';
ALTER FUNCTION rename_test() RENAME TO test;S  --expected: RENAME FUNCTION ERROR
DROP FUNCTION rename_test();
CREATE OR REPLACE PROCEDURE rename_test()
AS
BEGIN
    SELECT 1;
END;
/
ALTER PROCEDURE rename_test() RENAME TO test;  --expected: RENAME PROCEDURE ERROR
DROP PROCEDURE rename_test;
DROP SYNONYM test;

-- move to other namespace test
CREATE SCHEMA target_schema;
CREATE SCHEMA source_schema;
CREATE TABLE target_schema.test_table (a int);
CREATE SYNONYM target_schema.test FOR target_schema.test_table;
CREATE TABLE source_schema.test (a int);
CREATE OR REPLACE FUNCTION source_schema.test RETURNS INT AS
$$ 
BEGIN
RETURN 0;
END;
$$
LANGUAGE 'plpgsql';
ALTER TABLE source_schema.test SET SCHEMA target_schema;  --expected: SET SCHEMA ERROR
ALTER FUNCTION source_schema.test() SET SCHEMA target_schema;  --expected: SET SCHEMA ERROR
DROP TABLE source_schema.test;
DROP FUNCTION source_schema.test();
CREATE VIEW source_schema.test AS SELECT 1;
CREATE OR REPLACE PROCEDURE source_schema.test()
AS
BEGIN
    SELECT 1;
END;
/
ALTER VIEW source_schema.test SET SCHEMA target_schema;  --expected: SET SCHEMA ERROR
ALTER PROCEDURE source_schema.test() SET SCHEMA target_schema;  --expected: SET SCHEMA ERROR

-- clean up
RESET current_schema;
DROP SCHEMA synonym_test_schema CASCADE;
DROP SCHEMA target_schema CASCADE;
DROP SCHEMA source_schema CASCADE;
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'test_webserver') THEN
        CREATE USER test_webserver WITH PASSWORD '123456';
        RAISE NOTICE '用户 test_webserver 创建成功';
    ELSE
        RAISE NOTICE '用户 test_webserver 已存在';
    END IF;
    
    ALTER USER test_webserver WITH PASSWORD '123456';
    RAISE NOTICE '用户密码已重置';
END
$$;


SELECT 'CREATE DATABASE webserver_db OWNER test_webserver'
WHERE NOT EXISTS (SELECT 1 FROM pg_database WHERE datname = 'webserver_db')\gexec

SELECT CASE 
    WHEN EXISTS (SELECT 1 FROM pg_database WHERE datname = 'webserver_db') 
    THEN '数据库 webserver_db 已就绪' 
    ELSE '数据库创建失败' 
END AS status;

GRANT ALL PRIVILEGES ON DATABASE webserver_db TO test_webserver;

\c webserver_db

GRANT ALL ON SCHEMA public TO test_webserver;

ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON TABLES TO test_webserver;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON SEQUENCES TO test_webserver;
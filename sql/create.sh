#!/bin/bash

echo "正在创建 PostgreSQL 用户和数据库..."

if ! systemctl is-active --quiet postgresql 2>/dev/null && ! pgrep -x "postmaster" > /dev/null; then
    echo "错误: PostgreSQL 服务未运行"
    echo "请先启动 PostgreSQL:"
    echo "sudo systemctl start postgresql   # Ubuntu/Debian"
    echo "brew services start postgresql    # macOS"
    exit 1
fi

# 使用 postgres 系统用户执行
if command -v sudo &> /dev/null; then
    echo "使用 sudo 以 postgres 用户执行..."
    sudo -u postgres psql -f create_base.sql
    sudo -u postgres psql -d webserver_db -f schema.sql
    EXIT_CODE=$?
else
    echo "尝试切换到 postgres 用户..."
    su -c "psql -f create_base.sql" postgres
    su -c "psql -d webserver_db -f schema.sql" postgres
    EXIT_CODE=$?
fi

# 检查执行结果
if [ $EXIT_CODE -eq 0 ]; then
    echo ""
    echo "创建成功！"
    echo ""
    echo "数据库连接信息："
    echo "用户名: test_webserver"
    echo "密码: 123456"
    echo "数据库: webserver_db"
    echo "主机: localhost"
    echo ""
    echo "连接命令:"
    echo "psql -U test_webserver -d webserver_db -h localhost"
else
    echo "创建失败，退出代码: $EXIT_CODE"
    exit $EXIT_CODE
fi
-- init.sql - SQLite数据库初始化脚本
-- 基于RK3588的嵌入式多终端通信系统

-- 终端信息表
CREATE TABLE IF NOT EXISTS terminals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    terminal_id INTEGER UNIQUE NOT NULL,
    name TEXT NOT NULL,
    ip_address TEXT,
    status INTEGER DEFAULT 0,          -- 0=离线, 1=在线, 2=异常
    registered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_heartbeat DATETIME
);

-- 消息日志表
CREATE TABLE IF NOT EXISTS message_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    msg_type INTEGER NOT NULL,
    src_terminal INTEGER,
    dst_terminal INTEGER,
    payload_size INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 系统配置表
CREATE TABLE IF NOT EXISTS config (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 插入默认配置
INSERT OR IGNORE INTO config (key, value) VALUES
    ('system.name', 'RK3588多终端通信系统'),
    ('system.version', '1.0.0'),
    ('router.max_connections', '32'),
    ('router.heartbeat_interval', '5'),
    ('storage.max_log_days', '30'),
    ('protocol.version', '1');
